/*
  fitparameters.cc: implementation of the FitParameters class
  Copyright 2011, 2012 by Vincent Fourmond

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

/// A storage class for parameters as read from a file
///
/// @todo Maybe this class could handle the writing of the parameters
/// file as well.
class FitParametersFile {
public:

  /// The names of the datasets, as guessed from the comments
  QStringList datasetsName;

  /// The original name of the fit
  QString fitName;

  /// Comments (all of them)
  QStringList comments;
  
  class Parameter {
  public:
    /// Name of the parameter
    QString name;

    /// Index of the dataset (-1 if unspecified)
    int datasetIndex;

    /// String value of the parameter.
    QString value;

    Parameter(const QString & n, int ds, const QString & v) :
      name(n), datasetIndex(ds), value(v) {;};

    double toDouble(bool * ok = NULL) const {
      return value.toDouble(ok);
    };

    void replaceParameter(FitParameter * & parameter, double * tg, 
                          int idx, int ds) {
      FitParameter * npm = 
        FitParameter::loadFromString(value, tg, idx, ds);
      delete parameter;
      parameter = npm;
    };
  };

  /// All the parameters read from the file.
  QList<Parameter> parameters;

  /// Reads a stream and parses the contents into
  void readFromStream(QTextStream & in) {

    QRegExp paramRE("^([^\t []+)\\s*(?:\\[#(\\d+)\\])?\t(.*)");
    QRegExp commentRE("^\\s*#\\s*(.*)");
    QRegExp blankLineRE("^\\s*$");

    QString line;

    int nb = 0;
    
    while(true) {
      line = in.readLine();
      if(line.isNull())
        break;                    // EOF
      nb++;
      if(paramRE.indexIn(line) == 0) {
        int ds = -1;
        if(! paramRE.cap(2).isEmpty())
          ds = paramRE.cap(2).toInt();
        parameters << Parameter(paramRE.cap(1), ds, paramRE.cap(3));
      }
      else if(commentRE.indexIn(line) == 0) {
        comments << commentRE.cap(1);

        // Parse comments, if possible.
      }
      else if(blankLineRE.indexIn(line) == 0) {
        continue;
      }
      else {
        Terminal::out << "Line #" << nb << "not understood: '" 
                      << line.trimmed() << "'" << endl;
      }
    }
  };

  
};


//////////////////////////////////////////////////////////////////////

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


QString FitParameters::parameterName(int idx) const
{
  return fitData->parameterDefinitions[idx].name;
}

void FitParameters::prepareExport(QStringList & lst, QString & lines, 
                                  bool exportErrors) const
{
  double conf = fitData->confidenceLimitFactor(0.975);
  lst.clear();
  lst << "Buffer";
  for(int i = 0; i < nbParameters; i++) {
    QString name = fitData->parameterDefinitions[i].name;
    lst << name;
    if(exportErrors)
      lst << QString("%1_err").arg(name);
  }
  lst << "xstart" << "xend" << "residuals" << "rel_residuals" 
      << "buffer_weight";

  double res = fitData->residuals();
  double rel_res = fitData->relativeResiduals();
  
  const gsl_matrix * cov = (exportErrors ? fitData->covarianceMatrix() : NULL);

  QStringList ls2;
  lines.clear();
  for(int i = 0; i < datasets; i++) {
    ls2.clear();
    ls2 << fitData->datasets[i]->name;
    for(int j = 0; j < nbParameters; j++) {
      ls2 << QString::number(getValue(j, i));
      if(exportErrors) {
        int idx = (isGlobal(j) ? j : j + i * nbParameters);
        ls2 << QString::number(conf*sqrt(gsl_matrix_get(cov, idx, idx)));
      }
    }
    ls2 << QString::number(fitData->datasets[i]->x().min());
    ls2 << QString::number(fitData->datasets[i]->x().max());
    ls2 << QString::number(res) << QString::number(rel_res);
    ls2 << QString::number(fitData->weightsPerBuffer[i]);
    lines += ls2.join("\t") + "\n";
  }
}

void FitParameters::exportToOutFile(bool exportErrors, OutFile * out) const
{
  if( ! out)
    out = &OutFile::out;

  QStringList lst;
  QString lines;
  prepareExport(lst, lines, exportErrors);

  out->setHeader(QString("Fit: %1\n%2").
                 arg(fitData->fit->fitName()).
                 arg(lst.join("\t")));
  (*out) << lines << flush;
}

void FitParameters::exportParameters(QIODevice * stream, 
                                     bool exportErrors) const
{
  QTextStream out(stream);
  QStringList lst;
  out << "# Fit used: " << fitData->fit->fitName() 
      << ", residuals: " << fitData->residuals() << endl;

  QString lines;
  prepareExport(lst, lines, exportErrors);
  out << "## " << lst.join("\t") << endl;
  out << lines << flush;
}

void FitParameters::writeToTerminal(bool /*writeMatrix*/) const
{
  const gsl_matrix * mat = fitData->covarianceMatrix();
  double conf = fitData->confidenceLimitFactor(0.975);
  // First, write out global parameters.
  bool hasGlobal = false;
  for(int i = 0; i < nbParameters; i++) {
    if(isGlobal(i)) {
      if(! hasGlobal) {
        hasGlobal = true;
        Terminal::out << "Global parameters: \n" << endl;
      }
      double value = getValue(i, 0);
      double error = sqrt(gsl_matrix_get(mat, i, i)); // correct ?
      Terminal::out << parameterName(i) << "\t=\t" 
                    << QString::number(value) << "\t"
                    << (isFixed(i, 0) ? "(fixed)" :
                        QString("+- %1\t+-%2%").
                        arg(conf*error, 0, 'g', 2).
                        arg(conf*fabs(error/value)*100, 0, 'g', 2))
                    << endl;
    }
  }

  if(hasGlobal)
    Terminal::out << "\n\n";
  Terminal::out << "Buffer-local parameters: \n" << endl;


  for(int j = 0; j < datasets; j++) {
    Terminal::out << "Buffer: " << fitData->datasets[j]->name 
                  << " (weight: " << fitData->weightsPerBuffer[j] 
                  << ")" << endl;
    for(int i = 0; i < nbParameters; i++) {
      if(isGlobal(i))
        continue;
      double value = getValue(i, j);
      double error = sqrt(gsl_matrix_get(mat, i + j*nbParameters, 
                                         i + j*nbParameters)); // correct ?
      Terminal::out << parameterName(i) << "\t=\t" 
                    << QString::number(value) << "\t"
                    << (isFixed(i, j) ? "(fixed)" :
                        QString("+- %1\t+- %2%").
                        arg(conf*error, 0, 'g', 2).
                        arg(conf*fabs(error/value)*100, 0, 'g', 2))
                    << endl;
    }
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
      const FitParameter * param = parameter(i, 0);
      out << fitData->parameterDefinitions[i].name 
          << "\t" << param->saveAsString(getValue(i, 0))
          << endl;
    }
  }
  if(datasets > 1) {
    for(int j = 0; j < datasets; j++) {
      out << "buffer_weight[#" << j << "]\t" << fitData->weightsPerBuffer[j] 
          << endl;
      for(int i = 0; i < nbParameters; i++) {
        if(isGlobal(i))
          continue;
        const FitParameter * param = parameter(i, j);
        out << fitData->parameterDefinitions[i].name 
            << "[#" << j << "]\t"
            << "\t" << param->saveAsString(getValue(i, j))
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

void FitParameters::setValue(const QString & name, double value)
{
  QRegExp specRE("([^[]+)\\[#(\\d+)\\]");
  QString p = name;
  int dsi = -1;
  if(specRE.indexIn(name) >= 0) {
    p = specRE.cap(1);
    dsi = specRE.cap(2).toInt();
  }
  int idx = parameterIndices.value(p, -1);
  if(idx < 0) {
    Terminal::out << "No such parameter: '" << name << "'" << endl;
    return;
  }
  if(dsi >= datasets)
    return;
  if(dsi >= 0)
    setValue(idx, dsi, value);
  else
    for(int i = 0; i < datasets; i++)
      setValue(idx, i, value);
}

void FitParameters::loadParameters(QIODevice * source, 
                                   int targetDS, int sourceDS)
{
  QTextStream in(source);
  FitParametersFile params;

  params.readFromStream(in);

  loadParameters(params, targetDS, sourceDS);
}


void FitParameters::loadParameters(FitParametersFile & params, 
                                   int targetDS, int sourceDS)
{
  for(int k = 0; k < params.parameters.size(); k++) {
    FitParametersFile::Parameter & param = params.parameters[k];
    int ds = param.datasetIndex; // That's cheating ;-)...
    if(targetDS >= 0) {
      if(ds >= 0 && ds != sourceDS)
        continue;               // We only care about the parameters
                                // which are global or from sourceDS
      ds = targetDS;
    }
    
    if(param.name == "buffer_weight") {
      if(ds < 0) {
        Terminal::out << "Found a global 'buffer_weight' specification. "
          "That doesn't make sense !" << endl;
        continue;
      }
      if(ds >= fitData->datasets.size()) {
        Terminal::out << "Ignoring extra 'buffer_weight' specification." 
                      << endl;
        continue;
      }
      bool ok = false;
      double w = param.toDouble(&ok);
      if(ok)
        fitData->weightsPerBuffer[ds] = w;
      else
        Terminal::out << "Weight not understood: '" << param.value 
                      << "'" << endl;
    }
    else {
      int idx = parameterIndices.value(param.name, -1);
      if(idx < 0) {
        Terminal::out << "Found unkown parameter: '" << param.name
                      << "', ignoring" << endl;
        continue;
      }
      if(ds >= 0) {
        if(ds >= datasets) {
          Terminal::out << "Ignoring parameter '" << param.name
                        << "' for extra dataset #" << ds << endl;
          continue;
        }

        /// @todo Avoid setting a global parameter this way ?
        param.replaceParameter(parameter(idx, ds), &valueFor(idx, ds),
                               idx, ds);
      }
      else {
        param.replaceParameter(parameter(idx, 0), &valueFor(idx, 0),
                               idx, -1);
        for(int i = 1; i < datasets; i++) {
          values[idx + i * nbParameters] = values[idx];
          delete parameter(idx, i);
          parameter(idx, i) = NULL;
        }
      }
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

void FitParameters::setupWithCovarianceMatrix(QTableWidget * widget)
{
  const gsl_matrix * mat = fitData->covarianceMatrix();
  
  widget->setColumnCount(mat->size1);
  widget->setRowCount(mat->size1);

  QStringList heads;
  for(int i = 0; i < datasets; i++)
    for(int j = 0; j < nbParameters; j++)
      heads << QString("%1[%2]").arg(parameterName(j)).arg(i);
  widget->setHorizontalHeaderLabels(heads);
  widget->setVerticalHeaderLabels(heads);

  for(int i = 0; i < mat->size1; i++)
    for(int j = 0; j < mat->size1; j++)
      widget->
        setItem(i,j, new QTableWidgetItem(QString::
                                          number(gsl_matrix_get(mat, i, j))));
  
}
