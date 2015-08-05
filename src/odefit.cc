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

void ODEFit::processOptions(const CommandOptions & opts)
{
  // Options get processed when everything else is done.

  QStringList lst;
  updateFromOptions(opts, "with", lst);

  voltammogram = false;

  hasOrigTime = false;
  updateFromOptions(opts, "choose-t0", hasOrigTime);

  processSoftOptions(opts);

  // Get rid of all time-dependent parameters first.
  timeDependentParameters.clear();
  timeDependentParameters.parseFromStrings(lst, [this](const QString & n) {
      return getParameterIndex(n);
    });

  // Recompute the parameters and all the indices ne
  updateParameters();
}

bool ODEFit::hasReporters() const
{
  return false;
}

double ODEFit::reporterValue() const
{
  throw InternalError("Reporters unimplemented but requested");
  return 0.0;
}

CommandOptions ODEFit::currentSoftOptions() const
{
  return solver()->getStepperOptions().asOptions();
}

void ODEFit::processSoftOptions(const CommandOptions & opts)
{
  // Parse ODEStepperOptions
  ODESolver * slv = solver();
  ODEStepperOptions op = slv->getStepperOptions();
  op.fixed = false;           // Drop non-adaptative steps !
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

void ODEFit::updateParameters()
{
  parametersCache.clear();
  QList<ParameterDefinition> & defs = parametersCache;
  
  
  QStringList parameters = systemParameters();
  timeIndex= -1;

  if(! hasReporters()) {
    QStringList names = variableNames();
    for(int i = 0; i < names.size(); i++)
      defs << ParameterDefinition(QString("y_%1").
                                  arg(names[i]), i != 0);
  }
  // If there are reporters, then the underlying parameters are
  // already taken care of by the systemParameters()
  if(hasOrigTime)
    defs << ParameterDefinition("t0", true);
    
  parametersBase = defs.size();
  
  skippedIndices.clear();
  for(int i = 0; i < parameters.size(); i++) {
    const QString & name = parameters[i];
    if(name == "t") {
      timeIndex = i;
      skippedIndices.insert(i);
      continue;
    }
      
    if(timeDependentParameters.contains(name)) {
      skippedIndices.insert(i);
      continue;
    }

    defs << ParameterDefinition(name, isFixed(name)); 
  }
  tdBase = defs.size();
  defs += timeDependentParameters.fitParameters();
  parametersNumber = defs.size();
}

bool ODEFit::isFixed(const QString & ) const
{
  return false;
}

QList<ParameterDefinition> ODEFit::parameters() const
{
  return parametersCache;
}

ODEFit::~ODEFit()
{
  // Anything to do here ?
}

void ODEFit::function(const double * a, FitData * data, 
                      const DataSet * ds , gsl_vector * target)
{
  ODESolver * slv = solver();
  slv->resetStepper();

  double sr = 0;

  setupCallback([this, a](double t, double * params) {
      if(timeIndex >= 0)
        params[timeIndex] = t;
      timeDependentParameters.computeValues(t, params, a + tdBase);
    });

  const Vector & xv = ds->x();

  double ini = voltammogram ? 0 : xv[0];
  initialize(hasOrigTime ? *(a + parametersBase - 1) : ini, a);

  
  // if(data->debug)
  //   dumpAllParameters();

  Vector discontinuities = timeDependentParameters.discontinuities(a + tdBase);

  direction = xv[1] > xv[0] ? sr : -sr;
  lastTime = 0;
  lastPot = xv[0];

  bool reporters = hasReporters();
  for(int i = 0; i < xv.size(); i++) {
    // Here, we must be wary of the discontinuity points (ie those
    // of the time-evolving stuff)


    double tg = (voltammogram ? lastTime : xv[i]);
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
    if(reporters)
      val = reporterValue();
    else {
      int sz = slv->dimension();
      for(int j = 0; j < sz; j++)
        val += cv[j] * a[j + (voltammogram ? 2 : 0)];
    }
    gsl_vector_set(target, i, val);
  }

  if(data->debug) {
    QTextStream o(stdout);
    o << "Number of evaluations: " << slv->evaluations << endl;
  }
      
}

//   virtual void initialGuess(FitData * /*params*/, 
//                             const DataSet *ds,
//                             double * a)
//   {
//     const Vector & x = ds->x();
//     const Vector & y = ds->y();


//     int nb = system->speciesNumber();
//     if(! system->reporterExpression) {
//       // All currents to 0 but the first

//       for(int i = 0; i < nb; i++)
//         a[i + (voltammogram ? 2 : 0)] = (i == 0 ? y.max() : 0);
//     }

//     double * b = a + parametersBase;
//     if(hasOrigTime)
//       b[-1] = x[0] - 20;

//     // All initial concentrations to 0 but the first
//     for(int i = 0; i < nb; i++)
//       b[i] = (i == 0 ? 1 : 0);
    
//     // All rate constants and other to 1 ?

//     // We can't use params->parameterDefinitions.size(), as this will
//     // fail miserably in combined fits
//     for(int i = nb + parametersBase; i < parametersNumber;
//         i++)
//       a[i] = 1;                 // Simple, heh ?

//     // And have the parameters handle themselves:
//     timeDependentParameters.setInitialGuesses(a + tdBase, ds);

//   };

//   virtual ArgumentList * fitArguments() const {
//     if(system)
//       return NULL;
//     return new 
//       ArgumentList(QList<Argument *>()
//                    << new FileArgument("system", 
//                                        "System",
//                                        "Path to the file describing the "
//                                        "system"));
//   };

ArgumentList * ODEFit::fitSoftOptions() const
{
  return new ArgumentList(ODEStepperOptions::commandOptions());
}

ArgumentList * ODEFit::fitHardOptions() const
{
  return new 
    ArgumentList(QList<Argument *>()
                 << new SeveralStringsArgument(QRegExp(";"),
                                               "with", 
                                               "Time dependent parameters",
                                               "Dependency upon time of "
                                               "various parameters")
                 << new BoolArgument("choose-t0", 
                                     "If on, one can choose the initial time")
                 );
}


//   KineticSystemFit() :
//     PerDatasetFit("kinetic-system", 
//                   "Full kinetic system",
//                   "", 1, -1, false), system(NULL), evolver(NULL)
//   { 
//     makeCommands(NULL, 
//                  effector(this, &KineticSystemFit::runFitCurrentDataSet, true),
//                  effector(this, &KineticSystemFit::runFit, true),
//                  NULL,
//                  effector(this, &KineticSystemFit::computeFit)
//                  );
//   };

//   KineticSystemFit(const QString & name, 
//                    KineticSystem * sys,
//                    const QString & file
//                    ) : 
//     PerDatasetFit(name.toLocal8Bit(), 
//                   QString("Kinetic system of %1").arg(file).toLocal8Bit(),
//                   "", 1, -1, false), system(sys), evolver(NULL)
//   {
//     system->prepareForTimeEvolution();
//     evolver = new KineticSystemEvolver(system);
//     makeCommands();
//   }
// };
