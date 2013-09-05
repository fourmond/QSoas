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
#include <fitengine.hh>
#include <fit.hh>
#include <fitdata.hh>
#include <fitparameter.hh>

#include <terminal.hh>
#include <soas.hh>
#include <dataset.hh>
#include <outfile.hh>

#include <utils.hh>

/// A storage class for parameters as read from a file
///
/// @todo Maybe this class could handle the writing of the parameters
/// file as well.
class FitParametersFile {
public:

  /// The names of the datasets, as guessed from the comments
  QHash<int, QString> datasetNames;

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

    // The \\S really shouldn't be necessary, but it looks like a Qt
    // regexp bug ?
    QRegExp paramRE("^([^\t []+)\\s*(?:\\[#(\\d+)\\])?\t\\s*(\\S.*)");
    QRegExp commentRE("^\\s*#\\s*(.*)$");
    QRegExp bufferCommentRE("^\\s*Buffer\\s*#(\\d+)\\s*:\\s(.*)");
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
        QString cmt = commentRE.cap(1);
        comments << cmt;
        if(bufferCommentRE.indexIn(cmt) == 0) {
          // We found a comment describing the buffers
          int idx = bufferCommentRE.cap(1).toInt();
          datasetNames[idx] = bufferCommentRE.cap(2);
        }

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

    // Now we output the list of files we detected:
    // QList<int> lst = datasetNames.keys();
    // QTextStream o(stdout);
    // for(int i = 0; i < lst.size(); i++) {
    //   int idx = lst[i];
    //   o << "Buffer #" << idx << " was " << datasetNames[idx] << endl;
    // }
  };

  
};


//////////////////////////////////////////////////////////////////////

FitParameters::FitParameters(FitData * d) :
  fitData(d), parameters(d->datasets.size() * 
                         d->parametersPerDataset()),
  rawCVMatrix(NULL), cookedCVMatrix(NULL)
{
  datasets = d->datasets.size();
  nbParameters = d->parametersPerDataset();
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

void FitParameters::freeMatrices()
{
  if(rawCVMatrix) {
    gsl_matrix_free(rawCVMatrix);
    rawCVMatrix = NULL;
    gsl_matrix_free(cookedCVMatrix);
    cookedCVMatrix = NULL;
  }
}

FitParameters::~FitParameters()
{
  delete[] values;
  freeMatrices();
}

bool FitParameters::isGlobal(int index) const
{
  return parameter(index, 0)->global();
}

bool FitParameters::isFixed(int index, int ds) const
{
  // Make sure the correct dataset is used.
  if(ds < 0 || isGlobal(index))
    ds = 0;
  return parameter(index, ds)->fixed();
}

void FitParameters::updateParameterValues()
{
  // These steps are necessary to ensure the correct initialization of
  // formula-based stuff.
  sendDataParameters();
  fitData->initializeParameters();

  QVarLengthArray<double, 1000> params(fitData->freeParameters());

  int sz = fitData->freeParameters();
  if(sz == 0)               /// @hack Work around GSL limitation on
                            /// empty vectors
    sz = 1;                     
  gsl_vector_view v = gsl_vector_view_array(params.data(), sz);

  // We need a pack/unpack cycle to ensure the dependent parameters
  // are computed correctly.

  /// @todo maybe the pack/unpack cycle should be implemented at the
  /// FitData level ? 
  fitData->packParameters(values, &v.vector);
  fitData->unpackParameters(&v.vector, values);
}

void FitParameters::recompute()
{
  updateParameterValues();
  fitData->fit->function(values, fitData, fitData->storage);
  computeResiduals();
}

void FitParameters::computeResiduals()
{
  double tw = 0;                // weights
  double tr = 0;                // residuals
  double td = 0;                // data

  pointResiduals.clear();
  relativeResiduals.clear();
  for(int i = 0; i < fitData->datasets.size(); i++) {
    gsl_vector_view v = fitData->viewForDataset(i, fitData->storage);
    double w = fitData->weightedSquareSumForDataset(i, NULL, false);
    double d = fitData->weightedSquareSumForDataset(i, NULL, true);
    double r = fitData->weightedSquareSumForDataset(i, &v.vector, true);

    tw += w;
    tr += r;
    td += d;

    pointResiduals << sqrt(r/w);
    relativeResiduals << sqrt(r/d);
  }
  overallPointResiduals = sqrt(tr/tw);
  overallRelativeResiduals = sqrt(tr/td);
}

QList<Vector> FitParameters::computeSubFunctions()
{
  QList<Vector> ret;
  if(! fitData->fit->hasSubFunctions())
    return ret;
  updateParameterValues();
  QStringList str;
  fitData->fit->computeSubFunctions(values, fitData, 
                                    &ret, &str);
  return ret;
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

void FitParameters::prepareFit(FitEngineFactoryItem * it)
{
  sendDataParameters();
  fitData->initializeSolver(values, it);
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
                                  bool exportErrors)
{
  updateParameterValues();
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
      << "overall_res" << "overall_rel_res"
      << "buffer_weight";

  const gsl_matrix * cov = (exportErrors ? fitData->covarianceMatrix() : NULL);

  QStringList ls2;
  lines.clear();
  for(int i = 0; i < datasets; i++) {
    ls2.clear();
    ls2 << fitData->datasets[i]->name;
    for(int j = 0; j < nbParameters; j++) {
      // We don't use getValue here because it can be misleading ? 
      ls2 << QString::number(values[i * nbParameters + j]);
      if(exportErrors) {
        int idx = (isGlobal(j) ? j : j + i * nbParameters);
        ls2 << QString::number(conf*sqrt(gsl_matrix_get(cov, idx, idx)));
      }
    }
    ls2 << QString::number(fitData->datasets[i]->x().min());
    ls2 << QString::number(fitData->datasets[i]->x().max());
    ls2 << QString::number(pointResiduals[i]) 
        << QString::number(relativeResiduals[i]);
    ls2 << QString::number(overallPointResiduals) 
        << QString::number(overallRelativeResiduals);
    ls2 << QString::number(fitData->weightsPerBuffer[i]);
    lines += ls2.join("\t") + "\n";
  }
}

void FitParameters::exportToOutFile(bool exportErrors, OutFile * out)
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
                                     bool exportErrors)
{
  QTextStream out(stream);
  QStringList lst;
  out << "# Fit used: " << fitData->fit->fitName() 
      << ", residuals: " << overallPointResiduals << endl;

  QString lines;
  prepareExport(lst, lines, exportErrors);
  out << "## " << lst.join("\t") << endl;
  out << lines << flush;
}

template <typename T> void FitParameters::writeText(T & target, 
                                                    bool /*writeMatrix*/,
                                                    const QString & prefix) const
{
  const gsl_matrix * mat = fitData->covarianceMatrix();
  double conf = fitData->confidenceLimitFactor(0.975);
  // First, write out global parameters.
  bool hasGlobal = false;
  for(int i = 0; i < nbParameters; i++) {
    if(isGlobal(i)) {
      if(! hasGlobal) {
        hasGlobal = true;
        target << prefix << "Global parameters: \n" << endl;
      }
      double value = getValue(i, 0);
      double error = sqrt(gsl_matrix_get(mat, i, i)); // correct ?
      target << prefix << parameterName(i) << "\t=\t" 
             << QString::number(value) << "\t"
             << (isFixed(i, 0) ? "(fixed)" :
                 QString("+- %1\t+-%2%").
                 arg(conf*error, 0, 'g', 2).
                 arg(conf*fabs(error/value)*100, 0, 'g', 2))
             << endl;
    }
  }

  if(hasGlobal)
    target << prefix << "\n" << prefix << "\n";
  target << prefix << "Buffer-local parameters: \n" << prefix << endl;
  

  for(int j = 0; j < datasets; j++) {
    target << prefix << "Buffer: " << fitData->datasets[j]->name 
           << " (weight: " << fitData->weightsPerBuffer[j] 
           << ")" << endl;
    for(int i = 0; i < nbParameters; i++) {
      if(isGlobal(i))
        continue;
      double value = getValue(i, j);
      double error = sqrt(gsl_matrix_get(mat, i + j*nbParameters, 
                                         i + j*nbParameters)); // correct ?
      target << prefix << parameterName(i) << "\t=\t" 
             << QString::number(value) << "\t"
             << (isFixed(i, j) ? "(fixed)" :
                 QString("+- %1\t+- %2%").
                 arg(conf*error, 0, 'g', 2).
                 arg(conf*fabs(error/value)*100, 0, 'g', 2))
             << endl;
    }
  }
}

void FitParameters::writeToTerminal(bool writeMatrix)
{
  writeText(Terminal::out, writeMatrix);
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
  out << "# The following contains a more human-readable listing of the "
    "parameters that is NEVER READ by QSoas" << endl;
  writeText(out, false, "# ");
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

void FitParameters::computeMatrices()
{
  const gsl_matrix * mat = fitData->covarianceMatrix();
  freeMatrices();
  rawCVMatrix = gsl_matrix_alloc(mat->size1, mat->size2);
  gsl_matrix_memcpy(rawCVMatrix, mat);
  cookedCVMatrix = gsl_matrix_alloc(mat->size1, mat->size2);
  for(size_t i = 0; i < mat->size1; i++)
    for(size_t j = 0; j < mat->size2; j++) {
      double rawValue = gsl_matrix_get(mat, i, j);
      double cooked = rawValue;
      if(j == i)
        cooked = sqrt(rawValue);
      else {
        double norm = gsl_matrix_get(mat, i, i) * gsl_matrix_get(mat, j, j);
        norm = sqrt(norm);
        if(norm > 0)
          cooked /= norm;
      }
      gsl_matrix_set(cookedCVMatrix, i, j, cooked);
    }
}

void FitParameters::setupWithCovarianceMatrix(QTableWidget * widget, 
                                              bool raw)
{
  computeMatrices();

  widget->setColumnCount(rawCVMatrix->size1);
  widget->setRowCount(rawCVMatrix->size1);

  QStringList heads;
  for(int i = 0; i < datasets; i++)
    for(int j = 0; j < nbParameters; j++)
      heads << QString("%1[%2]").arg(parameterName(j)).arg(i);
  widget->setHorizontalHeaderLabels(heads);
  widget->setVerticalHeaderLabels(heads);

  for(size_t i = 0; i < rawCVMatrix->size1; i++)
    for(size_t j = 0; j < rawCVMatrix->size1; j++) {
      double rawValue = gsl_matrix_get(rawCVMatrix, i, j);
      double cooked = gsl_matrix_get(cookedCVMatrix, i, j);
      QTableWidgetItem * it = 
        new QTableWidgetItem(QString::number(raw ? rawValue : cooked));

      // Whatever mode, we set the color of non-diagonal coefficients
      // according to the cooked value.
      QColor col("white");
      if(i != j) {
        // We assume that the value is somewhat between -1 and 1
        if(cooked < 0) {
          col.setGreenF(1 + cooked*0.3);
          col.setBlueF(1 + cooked*0.3);
        }
        else {
          col.setRedF(1 - cooked*0.3);
          col.setBlueF(1 - cooked*0.3);
        }
      }
      it->setBackground(col);
      it->setTextAlignment(Qt::AlignCenter);
      widget->setItem(i, j, it);
    }
}

void FitParameters::writeCovarianceMatrix(QTextStream & out,  bool raw)
{
  computeMatrices();            // Makes double work, but that isn't a
                                // problem ?
  const gsl_matrix  * mat = (raw ? rawCVMatrix : cookedCVMatrix);
  out << Utils::matrixString(mat);
}

Vector FitParameters::saveParameterValues()
{
  updateParameterValues();
  Vector ret;
  int size = nbParameters * datasets;
  for(int i = 0; i < size; i++)
    ret << values[i];
  return ret;
}

void FitParameters::restoreParameterValues(const Vector & vect)
{
  int size = nbParameters * datasets;
  for(int i = 0; (i < size && i < vect.size()); i++)
    values[i] = vect[i];
  updateParameterValues();
}


Vector FitParameters::saveParameterErrors(double th)
{
  Vector ret;
  for(int i = 0; i < datasets; i++)
    for(int j = 0; j < nbParameters; j++)
      ret << getParameterError(j, i, th);
  return ret;
}

void FitParameters::writeCovarianceMatrixLatex(QTextStream & out,  bool raw)
{
  computeMatrices();            // Makes double work, but that isn't a
                                // problem ?
  const gsl_matrix  * mat = (raw ? rawCVMatrix : cookedCVMatrix);

  /// @todo Probably the code here should to to Utils later on.
  QRegExp conv("(.*)e([+-])0*(\\d+)");
  for(size_t i = 0; i < mat->size1; i++) {
    for(size_t j = 0; j < mat->size2; j++) {
      if(j)
        out << " & ";
      QString number = QString::number(gsl_matrix_get(mat, i, j));
      if(conv.indexIn(number) == 0) {
        number = conv.cap(1) + "\\times 10^{" + conv.cap(2) + 
          conv.cap(3) + "}";
      }
      out << number;
    }
    out << "\\\\\n";
  }
}

double FitParameters::getParameterError(int i, int ds, double th) const
{
  const gsl_matrix * mat = fitData->covarianceMatrix();
  double conf = fitData->confidenceLimitFactor(th);
  double value;
  double error;
  if(isFixed(i, ds))
    return 0;                   // Always !
  if(isGlobal(i)) {
    value = getValue(i, 0);
    error = sqrt(gsl_matrix_get(mat, i, i));
  }
  else {
    value = getValue(i, ds);
    error = sqrt(gsl_matrix_get(mat, ds*nbParameters + i, 
                                ds*nbParameters + i));
  }
  return conf*fabs(error/value);
}

void FitParameters::recomputeJacobian()
{
  fitData->recomputeJacobian();
}


//////////////////////////////////////////////////////////////////////

CovarianceMatrixDisplay::CovarianceMatrixDisplay(FitParameters * params, 
                                                 QWidget * parent) :
  QDialog(parent), parameters(params)
{
  setupFrame();
  parameters->setupWithCovarianceMatrix(widget);
  widget->resizeColumnsToContents();
  widget->resizeRowsToContents();

  resize(sizeHint());           // ??
}

void CovarianceMatrixDisplay::setupFrame()
{
  QVBoxLayout * layout = new QVBoxLayout(this);
  widget = new QTableWidget();
  layout->addWidget(widget);
  QHBoxLayout * horiz = new QHBoxLayout();
  
  QPushButton * bt = new QPushButton("Export");
  connect(bt, SIGNAL(clicked()), SLOT(exportToFile()));
  horiz->addWidget(bt);

  bt = new QPushButton("LaTeX");
  connect(bt, SIGNAL(clicked()), SLOT(exportAsLatex()));
  horiz->addWidget(bt);

  bt = new QPushButton("Close");
  connect(bt, SIGNAL(clicked()), SLOT(accept()));
  horiz->addWidget(bt);

  layout->addLayout(horiz);
  /// @todo Add the possibility to switch to raw display + export the
  /// matrix.
}

void CovarianceMatrixDisplay::exportToFile()
{
  // Exports the current contents to file
  QString file = QFileDialog::getSaveFileName(this, tr("Save matrix"));
  if(file.isEmpty())
    return;
  QFile f(file);
  Utils::open(&f, QIODevice::WriteOnly|QIODevice::Text);
  QTextStream o(&f);
  parameters->writeCovarianceMatrix(o);
}

void CovarianceMatrixDisplay::exportAsLatex()
{
  QString data;
  QTextStream o(&data);
  parameters->writeCovarianceMatrixLatex(o);
  QClipboard * clipboard = QApplication::clipboard();
  clipboard->setText(data);
}

