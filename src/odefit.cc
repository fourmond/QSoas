/*
  odefit.cc: implementation of ODEFit
  Copyright 2015 by CNRS/AMU

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <headers.hh>
#include <odefit.hh>
#include <odesolver.hh>
#include <terminal.hh>
#include <dataset.hh>
#include <fitdata.hh>

#include <general-arguments.hh>
#include <debug.hh>

void ODEFit::processOptions(const CommandOptions & opts, FitData * data) const
{
  // Options get processed when everything else is done.
  Storage * s = storage<Storage>(data);

  QStringList lst;
  updateFromOptions(opts, "with", lst);

  s->voltammogram = false;
  updateFromOptions(opts, "voltammogram", s->voltammogram);

  s->hasOrigTime = false;
  updateFromOptions(opts, "choose-t0", s->hasOrigTime);

  processSoftOptions(opts, data);

  // Get rid of all time-dependent parameters first.
  s->timeDependentParameters.clear();
  s->timeDependentParameters.parseFromStrings(lst, [this, data](const QString & n) {
      return getParameterIndex(n, data);
    });

  // Recompute the parameters and all the indices ne
  updateParameters(data);
}

bool ODEFit::hasReporters(FitData * ) const
{
  return false;
}

double ODEFit::reporterValue(double t, FitData * ) const
{
  throw InternalError("Reporters unimplemented but requested");
  return 0.0;
}

CommandOptions ODEFit::currentSoftOptions(FitData * data) const
{
  return solver(data)->getStepperOptions().asOptions();
}

void ODEFit::processSoftOptions(const CommandOptions & opts, FitData * data) const
{
  // Parse ODEStepperOptions
  ODESolver * slv = solver(data);
  ODEStepperOptions op = slv->getStepperOptions();
  op.fixed = false;           // Drop non-adaptive steps !
  op.parseOptions(opts);

  if(op.fixed) {
    // Decrease drastically the precision !
    op.epsAbs = 1e-2;
    op.epsRel = 1e-2;

    // By default, set the step size to 0 in that case 
    // (meaning make one step by data point)
    op.hStart = 0;
    op.parseOptions(opts);    // Parse again in case the step was
    // set in the options
  }

  slv->setStepperOptions(op);
  slv->resetStepper();

  // Dump the options on the terminal ?
  Terminal::out << "Using integrator parameters: " 
                << op.description() << endl;
}

void ODEFit::updateParameters(FitData * data) const
{
  Storage * s = storage<Storage>(data);

  s->parametersCache.clear();
  QList<ParameterDefinition> & defs = s->parametersCache;

  if(s->voltammogram) {
    // add temperature and scan rate at the beginning
    defs << ParameterDefinition("temperature", true)
         << ParameterDefinition("v", true);
    
    // reset the indices
    s->temperatureIndex = -1;
    s->potentialIndex = -1;
    s->faraIndex = -1;
  }

  QStringList parameters = systemParameters(data);
  s->timeIndex= -1;

  if(! hasReporters(data)) {
    QStringList names = variableNames(data);
    for(int i = 0; i < names.size(); i++)
      defs << ParameterDefinition(QString("y_%1").
                                  arg(names[i]), i != 0);
  }
  // If there are reporters, then the underlying parameters are
  // already taken care of by the systemParameters()
  if(s->hasOrigTime)
    defs << ParameterDefinition("t0", true);
    
  s->parametersBase = defs.size();
  
  s->skippedIndices.clear();
  for(int i = 0; i < parameters.size(); i++) {
    const QString & name = parameters[i];
    if(name == "t") {
      s->timeIndex = i;
      s->skippedIndices.insert(i);
      continue;
    }
      
    if(s->timeDependentParameters.contains(name)) {
      s->skippedIndices.insert(i);
      continue;
    }

    if(s->voltammogram && name == "temperature") {
      s->temperatureIndex = i;
      s->skippedIndices.insert(i);
      continue;
    }
    if(s->voltammogram && name == "e") {
      s->potentialIndex = i;
      s->skippedIndices.insert(i);
      continue;
    }
    if(s->voltammogram && name == "fara") {
      s->faraIndex = i;
      s->skippedIndices.insert(i);
      continue;
    }

    defs << ParameterDefinition(name, isFixed(name, data)); 
  }
  s->tdBase = defs.size();
  defs += s->timeDependentParameters.fitParameters();
  s->parametersNumber = defs.size();
}

bool ODEFit::isFixed(const QString &, FitData * ) const
{
  return false;
}

QList<ParameterDefinition> ODEFit::parameters(FitData * data) const
{
  Storage * s = storage<Storage>(data);
  return s->parametersCache;
}

ODEFit::~ODEFit()
{
  // Anything to do here ?
}

void ODEFit::function(const double * a, FitData * data, 
                      const DataSet * ds , gsl_vector * target) const
{
  compute(a, data, ds, target, NULL);
}

void ODEFit::computeSubFunctions(const double * a, FitData * data,
                                 const DataSet * ds,
                                 QList<Vector> * targetData,
                                 QStringList * targetAnnotations) const
{
  compute(a, data, ds, NULL, targetData);
  if(targetAnnotations)
    *targetAnnotations = variableNames(data);
}

void ODEFit::compute(const double * a, FitData * data,
                     const DataSet * ds,
                     gsl_vector * target,
                     QList<Vector> * targetData) const
{
  Storage * s = storage<Storage>(data);
  ODESolver * slv = solver(data);
  slv->resetStepper();

  s->timeDependentParameters.initialize(a + s->tdBase);

  double sr = 0;

  if(s->voltammogram) {
    sr = a[1];
  }

  setupCallback([this, a, s](double t, double * params) {
      if(s->timeIndex >= 0)
        params[s->timeIndex] = t;
      s->timeDependentParameters.computeValues(t, params, a + s->tdBase);

      if(s->voltammogram && s->potentialIndex >= 0) {
        params[s->potentialIndex] = s->lastPot + 
          s->direction * (t - s->lastTime);
      }
      
    }, data);

  const Vector & xv = ds->x();
  if(xv.size() < 2)
    throw RuntimeError("Not enough data points");

  double ini = s->voltammogram ? 0 : xv[0];
  initialize(s->hasOrigTime ? *(a + s->parametersBase - 1) : ini, a, data);

  
  // if(data->debug)
  //   dumpAllParameters();

  Vector discontinuities = s->timeDependentParameters.
    discontinuities(a + s->tdBase);

  s->direction = xv[1] > xv[0] ? sr : -sr;
  s->lastTime = 0;
  s->lastPot = xv[0];
  
  bool reporters = hasReporters(data);

  if(targetData) {
    targetData->clear();
    int sz = slv->dimension();
    for(int j = 0; j < sz; j++)
      *targetData << Vector();
  }

  for(int i = 0; i < xv.size(); i++) {
    // Here, we must be wary of the discontinuity points (ie those
    // of the time-evolving stuff)

    if(s->voltammogram && i > 0) {
      double pot = xv[i];
      if((pot - s->lastPot) * s->direction < 0) // at change of direction
        s->direction = -s->direction;
      s->lastTime += (pot - s->lastPot) / s->direction;
      s->lastPot = pot;
    }

    double tg = (s->voltammogram ? s->lastTime : xv[i]);
    while(discontinuities.size() > 0 && 
          discontinuities[0] < slv->currentTime())
      discontinuities.remove(0);

    if(discontinuities.size() > 0) {
      double td = discontinuities[0];
      if(slv->currentTime() < td && tg > td) {
        // Make a first step to the discontinuity
        slv->stepTo(td);
        discontinuities.remove(0);
      }
    }

    slv->stepTo(tg);
    double val = 0;
    const double * cv = slv->currentValues();
    if(reporters) {
      val = reporterValue(slv->currentTime(), data);
    }
      
    else {
      int sz = slv->dimension();
      for(int j = 0; j < sz; j++)
        val += cv[j] * a[j + (s->voltammogram ? 2 : 0)];
    }
    if(targetData) {
      int sz = slv->dimension();
      for(int j = 0; j < sz; j++)
        (*targetData)[j] << cv[j];
    }
    if(target)
      gsl_vector_set(target, i, val);
  }

  if(data->debug) {
    Debug::debug() << "Number of evaluations: " << slv->evaluations << endl;
  }
      
}

bool ODEFit::hasSubFunctions(FitData * data) const
{
  return true;
}

bool ODEFit::displaySubFunctions(FitData * data) const
{
  return false;
}


ArgumentList ODEFit::fitSoftOptions() const
{
  return ODEStepperOptions::commandOptions();
}

ArgumentList ODEFit::fitHardOptions() const
{
  return
    ArgumentList(QList<Argument *>()
                 << new TDPArgument("with", 
                                    "Time dependent parameters",
                                    "Make certain parameters depend "
                                    "upon time")
                 << new BoolArgument("voltammogram", 
                                     "If on, x is not taken to be time, but "
                                     "potential in a voltammetric experiment")
                 << new BoolArgument("choose-t0",
                                     "Choose initial time",
                                     "If on, one can choose the initial time")
                 );
}
