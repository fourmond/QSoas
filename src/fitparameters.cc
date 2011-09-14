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
#include <fitdata.hh>

#include <terminal.hh>
#include <soas.hh>
#include <dataset.hh>
#include <outfile.hh>

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

void FitParameters::prepareFit()
{
  setDataParameters();
  fitData->initializeSolver(values);
}

void FitParameters::retrieveParameters()
{
  fitData->unpackCurrentParameters(values);
}


void FitParameters::exportParameters(QIODevice * stream) const
{
  QTextStream out(stream);
  QStringList lst;
  out << "# Fit used: " << fitData->fit->fitName() << endl;
  lst << "Buffer";
  for(int i = 0; i < nbParameters; i++)
    lst << fitData->parameterDefinitions[i].name;
  // We add xstart and xend:
  lst << "xstart" << "xend";

  out << "## " << lst.join("\t") << endl;
  
  for(int i = 0; i < datasets; i++) {
    lst.clear();
    lst << fitData->datasets[i]->name;
    for(int j = 0; j < nbParameters; j++)
      lst << QString::number(getValue(j, i));
    lst << QString::number(fitData->datasets[i]->x().min());
    lst << QString::number(fitData->datasets[i]->x().max());
    out << lst.join("\t") << endl;
  }
}

void FitParameters::exportToOutFile(OutFile * out) const
{
  if( ! out)
    out = &OutFile::out;

  QStringList lst;
  lst << "Buffer";
  for(int i = 0; i < nbParameters; i++)
    lst << fitData->parameterDefinitions[i].name;
  // We add xstart and xend:
  lst << "xstart" << "xend";

  out->setHeader(QString("Fit: %1\n%2").
                 arg(fitData->fit->fitName()).
                 arg(lst.join("\t")));
  
  for(int i = 0; i < datasets; i++) {
    lst.clear();
    lst << fitData->datasets[i]->name;
    for(int j = 0; j < nbParameters; j++)
      lst << QString::number(getValue(j, i));
    lst << QString::number(fitData->datasets[i]->x().min());
    lst << QString::number(fitData->datasets[i]->x().max());
    (*out) << lst.join("\t") << "\n" << flush;
  }
}

void FitParameters::saveParameters(QIODevice * stream) const
{
  QTextStream out(stream);
  
  out << "# The following information are comments, " 
    "but Soas may make use of those if they are present" << endl;

  out << "# Fit used: " << fitData->fit->fitName() << endl;
  for(int i = 0; i < datasets; i++)
    out << "# Buffer #" << i << " : " 
        << fitData->datasets[i]->name << endl;

  // A first pass to print out the global parameters
  for(int i = 0; i < nbParameters; i++) {
    if(isGlobal(i) || datasets == 1)
      out << fitData->parameterDefinitions[i].name 
          << "\t" << getValue(i, 0) << "\t!\t"
          << (isFixed(i, 0) ? "0" : "1")
          << endl;
  }
  if(datasets > 1) {
    for(int j = 0; j < datasets; j++) {
      for(int i = 0; i < nbParameters; i++) {
        if(isGlobal(i))
          continue;
        out << fitData->parameterDefinitions[i].name 
            << "[#" << j << "]\t"
            << getValue(i, j) << "\t!\t"
            << (isFixed(i, j) ? "0" : "1")
            << endl;
      }
    }
  }
}
