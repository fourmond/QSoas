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
  fitData(d), parameters(d->datasets.size() * 
                         d->parameterDefinitions.size())
{
  datasets = d->datasets.size();
  nbParameters = d->parameterDefinitions.size();
  values = new double[nbParameters * datasets];

  // Now populate default values and fill the cache
  d->fit->initialGuess(d, values);
  for(int i = 0; i < nbParameters; i++) {
    const ParameterDefinition * def = &d->parameterDefinitions[i];
    parameterIndices[def->name] = i;

    if(def->canBeBufferSpecific) {
      for(int j = 0; j < datasets; j++) {
        if(def->defaultsToFixed)
          parameter(i,j) = new FixedParameter(i, j, valueFor(i, j));
        else 
          parameter(i,j) = new FreeParameter(i, j);
      }
    }
    else {
      // Keep in mind that all the rest has been initialized to 0
      if(def->defaultsToFixed)
        parameter(i,0) = new FixedParameter(i, -1, getValue(i, 0));
      else
        parameter(i,0) = new FreeParameter(i, -1);
    }
  }
}

FitParameters::~FitParameters()
{
  delete[] values;
}

bool FitParameters::isGlobal(int index) const
{
  return parameter(index, 0)->global();
}

bool FitParameters::isFixed(int index, int ds) const
{
  return parameter(index, (ds >= 0 ? ds : 0))->fixed();
}

void FitParameters::recompute()
{
  fitData->fit->function(values, fitData, fitData->storage);
}

void FitParameters::sendDataParameters()
{
  fitData->parameters.clear();
  
  for(int i = 0; i < parameters.size(); i++) {
    FitParameter * param = parameters[i];
    if(! param)
      continue;
    fitData->parameters << param->dup();
  }
}

void FitParameters::prepareFit()
{
  sendDataParameters();
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
  out << "# Fit used: " << fitData->fit->fitName() 
      << ", residuals: " << fitData->residuals() << endl;
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
    if(isGlobal(i) || datasets == 1) {
      const FitParameter * param = parameters[i];
      out << fitData->parameterDefinitions[i].name 
          << "\t" << param->textValue(getValue(i, 0)) << "\t!\t"
          << (param->fixed() ? "0" : "1")
          << endl;
    }
  }
  if(datasets > 1) {
    for(int j = 0; j < datasets; j++) {
      for(int i = 0; i < nbParameters; i++) {
        if(isGlobal(i))
          continue;
        const FitParameter * param = parameters[i];
        out << fitData->parameterDefinitions[i].name 
            << "[#" << j << "]\t"
            << param->textValue(getValue(i, 0)) << "\t!\t"
            << (param->fixed() ? "0" : "1")
            << endl;
      }
    }
  }
}

void FitParameters::clear()
{
  for(int i = 0; i < parameters.size(); i++) {
    delete parameters[i];
    parameters[i] = NULL;
  }
}

void FitParameters::loadParameters(QIODevice * source)
{
  QString line;
  QRegExp paramRE("^([^\t []+)\\s*(?:\\[#(\\d+)\\])?\t(\\S+)\\s*!\\s*([01])");
  QTextStream in(source);
  
  // clear();                      // Do we want that ?

  while(true) {
    line = in.readLine();
    if(line.isNull())
      break;                    // EOF
    if(paramRE.indexIn(line) == 0) {
      // We found a parameter
      QString paramName = paramRE.cap(1);
      int ds = -1;
      if(! paramRE.cap(2).isEmpty())
        ds = paramRE.cap(2).toInt();
      
      QString value = paramRE.cap(3);
      bool fxd = (paramRE.cap(4).toInt() == 0);

      int idx = parameterIndices.value(paramName, -1);
      if(idx < 0) {
        Terminal::out << "Found unkown parameter: '" << paramName 
                      << "', ignoring" << endl;
        continue;
      }

      if(ds >= 0) {
        if(ds >= datasets) {
          Terminal::out << "Ignoring parameter '" << paramName 
                        << "' for extra dataset #" << ds << endl;
          continue;
        }
        
        FitParameter * & pm = parameter(idx, ds);
        delete pm;
        pm = FitParameter::fromString(value, & valueFor(idx, ds),
                                      fxd, idx, ds);
      }
      else {
        FitParameter * & pm = parameter(idx, 0);
        delete pm;
        pm = FitParameter::fromString(value, & valueFor(idx, 0),
                                      fxd, idx, -1);
        for(int i = 1; i < datasets; i++) {
          values[idx + i * nbParameters] = values[idx];
          delete parameter(idx, i);
          parameter(idx, i) = NULL;
        }
      }
    }
    else {
      if(! line.startsWith("#"))
        Terminal::out << "Line not understood: '" 
                      << line << "'" << endl;
    }
  }
}

void FitParameters::resetAllToInitialGuess()
{
  fitData->fit->initialGuess(fitData, values);
}

void FitParameters::resetToInitialGuess(int ds)
{
  QVarLengthArray<double, 1024> params(nbParameters * datasets);
  fitData->fit->initialGuess(fitData, params.data());
  for(int i = 0; i < nbParameters; i++)
    values[i + ds * nbParameters] = params[i + ds * nbParameters];
}

void FitParameters::dump() const 
{
  QTextStream o(stdout);
  for(int i = 0; i < parameters.size(); i++) {
    const FitParameter * pm = parameters[i];
    o << "Param #" << i << "\t" << pm << " = " << values[i] << "\t";
    if(pm)
      o << "fixed: " << pm->fixed() << "\tglobal:" << pm->global()
        << "\t" << pm->paramIndex << "," << pm->dsIndex;
    o << endl;
  }
  o << endl;
}
