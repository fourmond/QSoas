/*
  fitparameters.cc: implementation of the FitParameters class
  Copyright 2011 by Vincent Fourmond

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
#include <fitparameters.hh>
#include <fit.hh>

#include <terminal.hh>
#include <soas.hh>
#include <dataset.hh>

FitParameters::FitParameters(FitData * d) :
  fitData(d)
{
  datasets = d->datasets.size();
  nbParameters = d->parameterDefinitions.size();
  global = new bool[nbParameters];
  fixed = new bool[nbParameters * datasets];
  values = new double[nbParameters * datasets];

  // Now populate default values:
  d->fit->initialGuess(d, values);
  for(int i = 0; i < nbParameters; i++)
    global[i] = ! d->parameterDefinitions[i].canBeBufferSpecific;
  for(int i = 0; i != datasets * nbParameters; i++)
    fixed[i] = d->parameterDefinitions[i % nbParameters].defaultsToFixed;
}

FitParameters::~FitParameters()
{
  delete[] values;
  delete[] global;
  delete[] fixed;
}

void FitParameters::recompute()
{
  fitData->fit->function(values, fitData, fitData->storage);

}

void FitParameters::setDataParameters()
{
  fitData->parameters.clear();
  fitData->fixedParameters.clear();

  for(int i = 0; i < datasets; i++) {
    for(int j = 0; j < nbParameters; j++) {
      int ds = (isGlobal(j) ? -1 : i);
      if(isGlobal(j)) {
        if(i)
          continue;
        if(fixed[j])
          fitData->fixedParameters 
            << FixedParameter(j, ds, values[j]);
        else 
          fitData->parameters
            << ActualParameter(j, ds);
      }
      else {
        if(isFixed(j, i))
          fitData->fixedParameters 
            << FixedParameter(j, ds, getValue(j, i));
        else 
          fitData->parameters
            << ActualParameter(j, ds);
      }
    }
  }
}

void FitParameters::doFit()
{
  setDataParameters();
  fitData->initializeSolver(values);
  Terminal::out << "Starting fit '" << fitData->fit->fitName() << "' with "
                << fitData->parameters.size() << " free parameters"
                << endl;

  /// @todo customize the number of iterations
  while(fitData->iterate() == GSL_CONTINUE && fitData->nbIterations < 100) {
    soas().showMessage(QObject::tr("Fit iteration %1").
                       arg(fitData->nbIterations));
    Terminal::out << "Iteration " << fitData->nbIterations 
                  << ", residuals :" << fitData->residuals() << endl;
  }
  fitData->unpackCurrentParameters(values);
}


void FitParameters::exportParameters(QIODevice * stream)
{
  QTextStream out(stream);
  QStringList lst;
  lst << "Buffer";
  for(int i = 0; i < nbParameters; i++)
    lst << fitData->parameterDefinitions[i].name;
  out << "## " << lst.join("\t") << endl;
  
  for(int i = 0; i < datasets; i++) {
    lst.clear();
    lst << fitData->datasets[i]->name;
    for(int j = 0; j < nbParameters; j++)
      lst << QString::number(getValue(j, i));
    out << lst.join("\t") << endl;
  }
}
