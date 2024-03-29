/*
  fitworkspace.cc: implementation of the FitWorkspace class
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014, 2015 by CNRS/AMU

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
#include <fitengine.hh>
#include <fitworkspace.hh>
#include <fit.hh>
#include <fitdata.hh>
#include <fitparameter.hh>

#include <terminal.hh>
#include <soas.hh>
#include <dataset.hh>
#include <outfile.hh>

#include <utils.hh>
#include <debug.hh>

#include <file.hh>

#include <sparsejacobian.hh>
#include <datastackhelper.hh>

#include <fitparametersfile.hh>
#include <fittrajectory.hh>

#include <parameterspaceexplorer.hh>
#include <command.hh>

#include <mruby.hh>

#include <gsl/gsl_sf.h>
#include <gsl/gsl_multifit.h>


#include <onetimewarnings.hh>

#include <idioms.hh>

#include <exceptions.hh>

#include <file.hh>

// smart(er) memory management for GSL types
#include <gsl-types.hh>

/// @todo Implement a command-line interface to handle fits.
/// 
/// @li this means implementing @b contexts for commands, most
/// probably a class name (probably through a class hierarchy
/// ?). Then, one would need to track the current context, and enable
/// only the appropriate commands.
///
/// @li commands with context would get another extra parameter
/// (i.e. the context, in our case the fit workspace ?).
///
/// @li in some case, one would want to only CREATE the commands at
/// the moment one enters the context ? (and this would make it far
/// easier to pass around the pointer to FitWorkspace) Possibly using
/// some lambda tricks ?
///
/// @li Write out everything to the Terminal, and display a copy of
/// the terminal on the fit window (although smaller)
///
/// @li Current actions drop-down would be replace by a decent menu bar.
///
/// @li one-by-one iteration should be possible, with fine control on
/// the convergence conditions
///
/// @li Loading of parameters with fine control on the parameter names
/// (define replacements, either by hand or using regexps)
///
/// @li pushing buffers, saving/loading parameters, exporting
/// parameters, and so on, everything gets simpler this way...
///
/// @li the delicate thing will be to find a way to have keyboard
/// shortcuts ? Although in principle, just loading them onto the menu
/// should be enough
///
/// @li Adding keyboard shortcuts to normal commands too...

FitWorkspace * FitWorkspace::currentWS = NULL;

FitWorkspace * FitWorkspace::currentWorkspace()
{
  return currentWS;
}

FitWorkspace::FitWorkspace(FitData * d) :
  fitData(d), currentDS(0), parameters(d->datasets.size() * 
                                       d->parametersPerDataset()),
  warnings(NULL),
  rawCVMatrix(NULL), cookedCVMatrix(NULL),
  currentExplorer(NULL),
  shouldCancelFit(false),
  trajectories(this),
  fitEnding(NotStarted),
  parametersStatus(FitWorkspace::ParametersUnknown),
  covarianceMatrixOK(false),
  tracingStream(NULL)
{
  if(currentWS)
    throw InternalError("Trying to recurse into a fit workspace, this should not happen");
  currentWS = this;
  if(! d->parametersStorage)
    throw InternalError("Trying to use an uninitialized FitData");
  
  datasets = d->datasets.size();
  nbParameters = d->parametersPerDataset();
  values = new double[nbParameters * datasets];
  errors = new double[nbParameters * datasets];

  // First, initialize the values, to avoid having unitialized stuff
  // showing up (in particular extra parameters, than don't get
  // initialized by the fits).
  for(int i = 0; i < nbParameters * datasets; i++)
    values[i] = 0;

  // We next initialize the vector view
  for(int i = 0; i < nbParameters; i++) {
    parameterView << gsl_vector_view_array_with_stride(values + i,
                                                       nbParameters, datasets);
    errorView << gsl_vector_view_array_with_stride(errors + i,
                                                   nbParameters, datasets);
  }

  // Now populate default values and fill the cache
  d->fit->initialGuess(d, values);
  // Save the initial guess
  initialGuess = saveParameterValues();
  for(int i = 0; i < nbParameters; i++) {
    const ParameterDefinition * def = &d->parameterDefinitions[i];
    parameterIndices[def->name] = i;

    if(def->canBeBufferSpecific) {
      for(int j = 0; j < datasets; j++) {
        if(def->defaultsToFixed)
          parameterRef(i,j) = new FixedParameter(i, j, valueFor(i, j));
        else 
          parameterRef(i,j) = new FreeParameter(i, j);
      }
    }
    else {
      // Keep in mind that all the rest has been initialized to 0
      if(def->defaultsToFixed)
        parameterRef(i,0) = new FixedParameter(i, -1, getValue(i, 0));
      else
        parameterRef(i,0) = new FreeParameter(i, -1);
    }
  }

  // generatedCommands += FitEngine::createCommands(this);
  connect(this, SIGNAL(parameterChanged(int, int)),
          SLOT(invalidateStatus()));
  connect(this, SIGNAL(parametersChanged()),
          SLOT(invalidateStatus()));
}

FitWorkspace::~FitWorkspace()
{
  delete[] values;
  delete[] errors;
  freeMatrices();
  currentWS = NULL;

  for(Command * cmd : generatedCommands)
    delete cmd;
  generatedCommands.clear();
}

void FitWorkspace::invalidateStatus()
{
  parametersStatus = ParametersUnknown;
  covarianceMatrixOK = false;
}

bool FitWorkspace::hasSubFunctions() const
{
  return fitData->fit->hasSubFunctions(fitData);
}

bool FitWorkspace::displaySubFunctions() const
{
  return fitData->fit->displaySubFunctions(fitData);
}

QString FitWorkspace::annotateDataSet(int idx) const
{
  return fitData->fit->annotateDataSet(idx, fitData);
}

CommandOptions FitWorkspace::currentSoftOptions() const
{
  return fitData->fit->currentSoftOptions(fitData);
}

void FitWorkspace::processSoftOptions(const CommandOptions & opts) const
{
  return fitData->fit->processSoftOptions(opts, fitData);
}

QString FitWorkspace::fitName(bool includeOptions) const
{
  return fitData->fit->fitName(includeOptions, fitData);
}


QString FitWorkspace::formatResiduals(double res)
{
  return QString::number(res, 'e', 7);
}

void FitWorkspace::computePerpendicularCoordinates(const QString & perpendicularMeta)
{
  // Here, setup the perpendicular coordinates
  if(fitData->datasets.size() > 1) {
    if(!perpendicularMeta.isEmpty()) {
      for(int i = 0; i < fitData->datasets.size(); i++) {
        const DataSet * ds = fitData->datasets[i];
        if(ds->getMetaData().contains(perpendicularMeta)) {
          perpendicularCoordinates << ds->getMetaData(perpendicularMeta).toDouble();
        }
      }
    }
    else {
      // Gather from datasets
      for(int i = 0; i < fitData->datasets.size(); i++) {
        const DataSet * ds = fitData->datasets[i];
        if(ds->perpendicularCoordinates().size() > 0)
          perpendicularCoordinates << ds->perpendicularCoordinates()[0];
      }
    }
    if(perpendicularCoordinates.size() > 0 && 
       perpendicularCoordinates.size() != fitData->datasets.size()) {
      Terminal::out << "Did get only " << perpendicularCoordinates.size() 
                    << " perpendicular coordinates, but I need " 
                    <<  fitData->datasets.size() << ", ignoring them" << endl;
      perpendicularCoordinates.clear();
    }
  }
}

bool FitWorkspace::hasPerpendicularCoordinates() const
{
  return perpendicularCoordinates.size() == datasets;
}

void FitWorkspace::freeMatrices()
{
  if(rawCVMatrix) {
    gsl_matrix_free(rawCVMatrix);
    rawCVMatrix = NULL;
    gsl_matrix_free(cookedCVMatrix);
    cookedCVMatrix = NULL;
  }
}



void FitWorkspace::setExplorer(ParameterSpaceExplorer * expl)
{
  if(currentExplorer)
    delete currentExplorer;
  currentExplorer = expl;
  if(expl)
    Terminal::out << "Selected parameter space explorer: '"
                  << expl->createdFrom->name << "'" << endl;
}


bool FitWorkspace::isGlobal(int index) const
{
  return parameter(index, 0)->global();
}

bool FitWorkspace::isFixed(int index, int ds) const
{
  // Make sure the correct dataset is used.
  if(ds < 0 || isGlobal(index))
    ds = 0;
  return parameter(index, ds)->fixed();
}

void FitWorkspace::updateParameterValues(bool dontSend)
{
  // These steps are necessary to ensure the correct initialization of
  // formula-based stuff.

  if(! dontSend) {
    sendDataParameters();
    fitData->initializeParameters();
  }

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

bool FitWorkspace::recompute(bool dontSend, bool silent)
{
  updateParameterValues(dontSend);

  try {
    fitData->computeFunction(values, fitData->storage);
  }
  catch (::Exception & e) {
    parametersStatus = ParametersFailed;
    if(silent)
      return false;
    else
      throw;
  }
  parametersStatus = ParametersOK;
  computeResiduals();
  return true;
}

QHash<QString, double> FitWorkspace::parametersForDataset(int ds) const
{
  if(ds < 0 || ds >= datasets)
    throw InternalError("Asking for invalid dataset %1").arg(ds);
  
  QHash<QString, double> ret;
  for(int i = 0; i < nbParameters; i++)
    ret[fitData->parameterDefinitions[i].name] = 
      values[ds * nbParameters + i];
  return ret;
}

void FitWorkspace::computeResiduals(bool setRuby)
{
  double tw = 0;                // weights
  double tr = 0;                // residuals
  double td = 0;                // data

  // Store the weights in fitData->storage2
  gsl_vector_set_all(fitData->storage2, 1);
  // True because we want the errors to be applied for all fits 
  fitData->weightVector(fitData->storage2, true);

  pointResiduals.clear();
  relativeResiduals.clear();

  for(int i = 0; i < fitData->datasets.size(); i++) {
    gsl_vector_view fv = fitData->viewForDataset(i, fitData->storage);
    gsl_vector_view wv = fitData->viewForDataset(i, fitData->storage2);

    double w = 0;               // Sum of the square of the weights
    double r = 0;               // Sum of the weighted square of the residuals
    double d = 0;               // Sum of the weighted square of data

    const DataSet * ds = fitData->datasets[i];
    int sz = ds->nbRows();
    for(int j = 0; j < sz; j++) {
      double cw = gsl_vector_get(&wv.vector, j);
      w += gsl_pow_2(cw);
      double v = gsl_vector_get(&fv.vector, j);
      d += gsl_pow_2(cw*v);
      v -= ds->y()[j];
      r += gsl_pow_2(cw*v);
    }

    tw += w;
    tr += r;
    td += d;

    pointResiduals << sqrt(r/w);
    relativeResiduals << sqrt(r/d);
  }
  overallPointResiduals = sqrt(tr/tw);
  overallRelativeResiduals = sqrt(tr/td);

  overallChiSquared = tr;

  if(setRuby)
    setRubyResiduals();
}


void FitWorkspace::setRubyResiduals()
{
  MRuby * mr = MRuby::ruby();
  mrb_value v = mr->newFloat(overallPointResiduals);
  mr->setGlobal("$residuals", v);
}

double FitWorkspace::goodnessOfFit()
{
  if(! fitData->standardYErrors)
    return -1;                  // No goodness-of-fit if no errors

  computeResiduals();
  // -> sets overallChiSquared
  return gsl_sf_gamma_inc_Q(0.5 * fitData->doF(), 0.5 * overallChiSquared);
}

QList<Vector> FitWorkspace::computeSubFunctions(bool dontSend,
                                                QStringList * ann)
{
  QList<Vector> ret;
  if(! fitData->fit->hasSubFunctions(fitData))
    return ret;
  updateParameterValues(dontSend);
  QStringList str;
  fitData->fit->computeSubFunctions(values, fitData, 
                                    &ret, ann ? ann : &str);

  return ret;
}

DataSet * FitWorkspace::computedData(int i, bool residuals)
{
  const DataSet * base = fitData->datasets[i];
  gsl_vector_view v =  fitData->viewForDataset(i, fitData->storage);
  Vector ny = Vector::fromGSLVector(&v.vector);
  if(residuals)
    ny = base->y() - ny;
  DataSet * ds = base->derivedDataSet(ny, (residuals ? "_delta_" : "_fit_")
                                      + fitName(false) + ".dat");
  
  ds->setMetaData("fit", fitName());
  ds->setMetaData("base", base->name);
  QHash<QString, double> params = parametersForDataset(i);
  for(QHash<QString, double>::iterator i = params.begin(); 
      i != params.end(); ++i)
    ds->setMetaData(i.key(), i.value());

  return ds;
}

void FitWorkspace::pushComputedData(bool residuals, bool subfunctions,
                                    DataStackHelper * help)
{
  QList<Vector> sf;
  QStringList names;
  if(subfunctions && fitData->fit->hasSubFunctions(fitData)) {
    sf = computeSubFunctions(true, &names);
    names.insert(0, "sim");
  }
  
  int base = fitData->dataPoints();

  for(int i = datasets-1; i >= 0; i--) {
    DataSet * ds = computedData(i, residuals);
    int sz = ds->x().size();
    base -= sz;
    for(int j = 0; j < sf.size(); j++)
      ds->appendColumn(sf[j].mid(base, sz));
    
    if(sf.size() > 0)
      ds->setMetaData("computed", names);
        
    if(help)
      *help << ds;
    else
      soas().pushDataSet(ds);
  }
}

void FitWorkspace::pushAnnotatedData(DataStackHelper * help)
{
  for(int i = 0; i < datasets; i++) {
    const DataSet * base = fitData->datasets[i];
    DataSet * ds =
      base->derivedDataSet(base->y(), "_fit_" + fitName(false) + ".dat");

    ds->setMetaData("fit", fitName());
    QHash<QString, double> params = parametersForDataset(i);
    for(QHash<QString, double>::iterator i = params.begin(); 
        i != params.end(); ++i)
      ds->setMetaData(i.key(), i.value());
    
    
    if(help)
      *help << ds;
    else
      soas().pushDataSet(ds);
  }
}

void FitWorkspace::sendDataParameters()
{
  fitData->clearParameters();
  
  for(int i = 0; i < parameters.size(); i++) {
    FitParameter * param = parameters[i];
    if(! param)
      continue;
    fitData->pushParameter(param->dup());
  }
}

void FitWorkspace::prepareFit(CommandOptions * opts)
{
  sendDataParameters();
  fitData->initializeSolver(values, opts);
  for(int i = 0; i < datasets * nbParameters; i++)
    errors[i] = 0;
}

void FitWorkspace::retrieveParameters()
{
  fitData->unpackCurrentParameters(values);
}


QString FitWorkspace::parameterName(int idx) const
{
  return fitData->parameterDefinitions[idx].name;
}

QString FitWorkspace::fullParameterName(int idx) const
{
  int ds = idx / nbParameters;
  idx = idx % nbParameters;
  if(isGlobal(idx))
    return parameterName(idx);
  else
    return QString("%1[#%2]").
      arg(parameterName(idx)).arg(ds);
}

QStringList FitWorkspace::parameterNames() const
{
  QStringList rv;
  for(const ParameterDefinition & def : fitData->parameterDefinitions)
    rv << def.name;
  return rv;
}

int FitWorkspace::datasetNumber() const
{
  return datasets;
}

int FitWorkspace::parametersPerDataset() const
{
  return nbParameters;
}

int FitWorkspace::totalParameterNumber() const
{
  return datasets * nbParameters;
}

DataSet * FitWorkspace::exportAsDataSet(bool errors, bool meta,
                                        const double * pV,
                                        const double * eV)
{
  retrieveParameters();
  double conf = fitData->confidenceLimitFactor(0.975);
  QStringList colNames;
  QList<Vector> vects;
  
 
  const gsl_matrix * cov = ((errors && ! eV) ? fitData->covarianceMatrix() : NULL);

  if(! pV)
    pV = values;


  for(int j = 0; j < nbParameters; j++) {
    QString name = fitData->parameterDefinitions[j].name;
    Vector v, err;
    for(int i = 0; i < datasets; i++) {
      v << pV[i * nbParameters + j];
      if(errors) {
        if(eV)
          err << eV[i * nbParameters + j];
        else {
          int idx = (isGlobal(j) ? j : j + i * nbParameters);
          err << conf*sqrt(gsl_matrix_get(cov, idx, idx));
        }
      }
    }
    colNames << name;
    vects << v;
    if(errors) {
      colNames << name + "_err";
      vects << err;
    }
  }

  // Last, the per-dataset stuff:
  QList<Vector> extra_vals;
  class ExtraColumn {
  public:
    QString name;
    std::function<double (int index)> func;
    // ExtraColumn(const QString & n, std::function<double (int index)> f) :
    //   name(n), func(f) {
    // };
  };

  ExtraColumn extras[] =
    {
     {"xstart", [this](int i) -> double {
                  return fitData->datasets[i]->x().min();
                }
     },
     {"xend", [this](int i) -> double {
                return fitData->datasets[i]->x().max();
              }
     },
     {"residuals", [this](int i) -> double {
                     return pointResiduals[i];
                   }
     },
     {"rel_residuals", [this](int i) -> double {
                         return relativeResiduals[i];
                       }
     },
     {"buffer_weight", [this](int i) -> double {
                         return fitData->weightsPerBuffer[i];
                       }
     }
    };

  for(const ExtraColumn & c : extras) {
    colNames << c.name;
    extra_vals << Vector();
  }

  for(int i = 0; i < datasets; i++) {
    int j = 0;
    for(const ExtraColumn & c : extras) {
      extra_vals[j] << c.func(i);
      ++j;
    }
  }

  vects += extra_vals;

  QStringList rowNames;
  QStringList metaNames;

  Vector xV;
  QString xN;

  prepareInfo(&rowNames, &xV, &xN, (meta ? &metaNames : NULL), &extra_vals);
  vects.insert(0, xV);
  colNames.insert(0, xN);


  if(meta) {
    colNames += metaNames;
    vects += extra_vals;
  }


  DataSet * ds = new DataSet(vects);
  ds->name = "Parameters";
  ds->columnNames << colNames;
  ds->rowNames << rowNames;

  return ds;
  
}

void FitWorkspace::prepareInfo(QStringList * rowNames,
                               Vector * xCoords,
                               QString * xName,
                               QStringList * metaNames,
                               QList<Vector> * metaValues) const
{
  rowNames->clear();
  for(int i = 0; i < datasets; i++)
    *rowNames << fitData->datasets[i]->name;


  if(hasPerpendicularCoordinates()) {
    *xName = "Z";
    *xCoords = perpendicularCoordinates;
  }
  else {
    *xName = "index";
    Vector v;
    for(int i = 0; i < datasets; i++)
      v << i;
    * xCoords = v;
  }


  if(metaNames && metaValues) {
    QSet<QString> names = fitData->datasets[0]->getMetaData().
      extractDoubles().keys().toSet();
    for(int i = 1; i < datasets; i++)
      names.intersect(fitData->datasets[i]->getMetaData().extractDoubles().keys().toSet());
    *metaNames = names.toList();
    std::sort(metaNames->begin(), metaNames->end());
    metaValues->clear();
    for(const QString & n : *metaNames)
      *metaValues << Vector();

    for(int i = 0; i < datasets; i++) {
      QHash<QString, double> vls =
        fitData->datasets[i]->getMetaData().extractDoubles();
      for(int j = 0; j < metaNames->size(); j++)
        (*metaValues)[j] << vls[(*metaNames)[j]];
    }
  }
}



void FitWorkspace::prepareExport(QStringList & lst, QString & lines, 
                                 bool exportErrors, bool exportMeta)
{
  retrieveParameters();
  double conf = fitData->confidenceLimitFactor(0.975);
  lst.clear();
  lst << "Buffer";
  if(hasPerpendicularCoordinates())
    lst << "Z";
  for(int i = 0; i < nbParameters; i++) {
    QString name = fitData->parameterDefinitions[i].name;
    lst << name;
    if(exportErrors)
      lst << QString("%1_err").arg(name);
  }
  lst << "xstart" << "xend" << "residuals" << "rel_residuals" 
      << "overall_res" << "overall_rel_res"
      << "buffer_weight";

  QStringList meta;
  if(exportMeta) {
    QSet<QString> names = fitData->datasets[0]->getMetaData().extractDoubles().keys().toSet();
    for(int i = 1; i < datasets; i++)
      names.intersect(fitData->datasets[i]->getMetaData().extractDoubles().keys().toSet());
    meta = names.toList();
    std::sort(meta.begin(), meta.end());
    lst << meta;
  }

  const gsl_matrix * cov = (exportErrors ? fitData->covarianceMatrix() : NULL);

  QStringList ls2;
  lines.clear();
  for(int i = 0; i < datasets; i++) {
    ls2.clear();
    ls2 << fitData->datasets[i]->name;
    if(hasPerpendicularCoordinates())
      ls2 << QString::number(perpendicularCoordinates[i]);

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
    ls2 << formatResiduals(pointResiduals[i])
        << formatResiduals(relativeResiduals[i]);
    ls2 << formatResiduals(overallPointResiduals) 
        << formatResiduals(overallRelativeResiduals);
    ls2 << QString::number(fitData->weightsPerBuffer[i]);

    if(meta.size() > 0) {
      QHash<QString, double> vls = fitData->datasets[i]->getMetaData().extractDoubles();
      for(int j = 0; j < meta.size(); j++)
        ls2 << QString::number(vls[meta[j]]);
    }
    lines += ls2.join("\t") + "\n";
  }
}

void FitWorkspace::exportToOutFile(bool exportErrors, OutFile * out)
{
  if( ! out)
    out = &OutFile::out;

  QStringList lst;
  QString lines;
  prepareExport(lst, lines, exportErrors);

  out->setHeader(QString("# Fit: %1\n## %2").
                 arg(fitName()).
                 arg(lst.join("\t")));
  (*out) << lines << flush;
}

void FitWorkspace::exportParameters(QIODevice * stream, 
                                     bool exportErrors)
{
  QTextStream out(stream);
  QStringList lst;
  out << "# Fit used: " << fitName() 
      << ", residuals: " << formatResiduals(overallPointResiduals) << endl;

  QString lines;
  prepareExport(lst, lines, exportErrors);
  out << "## " << lst.join("\t") << endl;
  out << lines << flush;
}

template <typename T> void FitWorkspace::writeText(T & target, 
                                                    bool /*writeMatrix*/,
                                                    const QString & prefix) const
{
  const gsl_matrix * mat = fitData->covarianceMatrix();
  double conf = fitData->confidenceLimitFactor(0.975);
  // First, write out global parameters.
  bool hasGlobal = false;

  // Writing down the goodness of fit

  target << prefix << "Final residuals: "
         << formatResiduals(overallPointResiduals) << endl;
  if(fitData->standardYErrors)
    target << prefix
           << "Final chi-squared: " << overallChiSquared << endl;

  
  double gof = const_cast<FitWorkspace *>(this)->goodnessOfFit();
  if(gof >= 0)
    target << prefix << "Chi-squared goodness of fit: " << gof << endl;

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

void FitWorkspace::writeToTerminal(bool writeMatrix)
{
  writeText(Terminal::out, writeMatrix);
}

void FitWorkspace::saveParameters(QIODevice * stream,
                                  const QStringList & comments) const
{
  QTextStream out(stream);
  
  out << "# QSoas saved parameters" << endl;
  out << "# Fit used: " << fitName() << endl;
  out << "# Command-line: " << soas().currentCommandLine() << endl;

  if(comments.size() > 0) {
    out << "# Comments:" << endl;
    for(const QString & s: comments)
      out << "#   " << s << endl;
  }

  // A first pass to print out the global parameters
  for(int i = 0; i < nbParameters; i++) {
    if(isGlobal(i) || datasets == 1) {
      const FitParameter * param = parameter(i, 0);
      out << fitData->parameterDefinitions[i].name 
          << "\t" << param->saveAsString(getValue(i, 0))
          << endl;
    }
  }
  for(int j = 0; j < datasets; j++) {
    out << "buffer_weight[#" << j << "]\t" << fitData->weightsPerBuffer[j] 
        << endl;
    out << "buffer_name[#" << j << "]\t" << fitData->datasets[j]->name
        << endl;
    if(hasPerpendicularCoordinates())
      out << "Z[#" << j << "]\t" << perpendicularCoordinates[j] 
          << endl;
        
    for(int i = 0; i < nbParameters; i++) {
      if(isGlobal(i) || datasets == 1)
        continue;
      const FitParameter * param = parameter(i, j);
      out << fitData->parameterDefinitions[i].name 
          << "[#" << j << "]\t"
          << "\t" << param->saveAsString(getValue(i, j))
          << endl;
    }
  }
  writeText(out, false, "# ");
}

void FitWorkspace::saveParameters(const QString & fileName,
                                  bool overwrite,
                                  const QStringList & comments,
                                  const CommandOptions & opts) const
{
  File f(fileName, (overwrite ? File::TextOverwrite : File::TextWrite), opts);
  saveParameters(f, comments);
  Terminal::out << "Saved fit parameters to file " << fileName << endl;
}


void FitWorkspace::clear()
{
  for(int i = 0; i < parameters.size(); i++) {
    delete parameters[i];
    parameters[i] = NULL;
  }
}

void FitWorkspace::checkIndex(int index, int dataset) const
{
  if(index < 0 || index >= nbParameters)
    throw InternalError("Incorrect index: %1/%2").
      arg(index).arg(nbParameters);
  if(dataset >= datasets)
    throw InternalError("Incorrect dataset: %1/%2").
      arg(dataset).arg(datasets);
}


void FitWorkspace::setFixed(int index, int ds, bool fixed)
{
  checkIndex(index, ds);
  if(ds >= 0 || isGlobal(index)) {
    bool cur = isFixed(index, ds);
    if((fixed && cur) || (! fixed && !cur)) {
      return;                     // not touching anything.
    }
  }
  
  if(ds < 0) {
    // Run for all
    for(int i = 0; i < datasets; i++)
      setFixed(index, i, fixed);
    return;
  }
  // Global parameters
  if(isGlobal(index))
    ds = 0;

  FitParameter *& target = parameterRef(index, ds);

  // Get the real dataset in case we are redirecting to other
  // parameters.
  ds = target->dsIndex;

  delete target;
  if(fixed) {
    FixedParameter * param = new FixedParameter(index, ds,  0);
    target = param;
  }
  else {
    target = new FreeParameter(index, ds);
  }
  emit(parameterChanged(index, ds));
}


int FitWorkspace::parameterIndex(const QString & name) const
{
  return parameterIndices.value(name, -1);
}


QList<QPair<int, int> > FitWorkspace::parseParameterList(const QString & spec,
                                                         QStringList * unknowns) const
{
  QRegExp specRE("([^[]+)\\[([0-9#,-]+)\\]");
  QList<QPair<int, int> > rv;

  auto fnd = [this, unknowns](const QString & n) -> int {
    int idx = parameterIndices.value(n, -1);
    if(idx < 0) {
      if(unknowns)
        *unknowns << n;
      throw RuntimeError("No such parameter: '%1'").arg(n);
    }
    return idx;
  };

  try {
    if(specRE.indexIn(spec) >= 0) {
      QString p = specRE.cap(1);
      int idx = fnd(p);
      QStringList lst = specRE.cap(2).split(",");
      QRegExp sRE("^#?(\\d+)$");
      QRegExp rRE("^#?(\\d*)-(\\d*)$");
      for(const QString & spc : lst) {
        if(sRE.indexIn(spc) >= 0) {
          int ds = sRE.cap(1).toInt();
          rv << QPair<int, int>(idx, ds);
        }
        else {
          if(rRE.indexIn(spc) >= 0) {
            int l = 0;
            if(! rRE.cap(1).isEmpty())
              l = rRE.cap(1).toInt();
            int r = datasets-1;
            if(! rRE.cap(2).isEmpty())
              r = rRE.cap(2).toInt();
            while(l <= r)
              rv << QPair<int, int>(idx, l++);
          }
          else
            throw RuntimeError("Could not understand parameter number spec '%1'").arg(spc);
        }
      }
    }
    else {
      int idx = fnd(spec);
      if(isGlobal(idx))
        rv << QPair<int,int>(idx, -1);
      else {
        // returning the list of all the parameters, one by one
        // Makes a huge difference for the case when one uses a formula.
        for(int i = 0; i < datasets; i++)
          rv << QPair<int,int>(idx, i);
      }
    }
  }
  catch(const RuntimeError & re) {
    if(! unknowns)
      throw;
  }

  return rv;
}


void FitWorkspace::setValue(int index, int dataset, double val)
{
  checkIndex(index, dataset);
  if(dataset < 0 || isGlobal(index)) {
    for(int i = 0; i < datasets; i++) {
      values[index % nbParameters + i*nbParameters] = val;
      emit(parameterChanged(index, i));
    }
  }
  else {
    values[dataset * nbParameters + (index % nbParameters)] = val;
    emit(parameterChanged(index, dataset));
  }
}

void FitWorkspace::setValue(const QString & name, double value, int dsi)
{
  QRegExp specRE("([^[]+)\\[#(\\d+)\\]");
  QString p = name;
  if(dsi < 0 && specRE.indexIn(name) >= 0) {
    p = specRE.cap(1);
    dsi = specRE.cap(2).toInt();
  }
  if(dsi >= datasets) {
    Terminal::out << "Attempting to set parameter: '" << name
                  << "' to a non-existent buffer:" << dsi << endl;
    return;
  }
  if(p == "buffer_weight")
    setBufferWeight(dsi, value);
  else {
    int idx = parameterIndices.value(p, -1);
    if(idx < 0) {
      Terminal::out << "No such parameter: '" << name << "'" << endl;
      return;
    }
    if(dsi >= 0)
      setValue(idx, dsi, value);
    else
      for(int i = 0; i < datasets; i++)
        setValue(idx, i, value);
  }
}

void FitWorkspace::loadParameters(const QString & file, 
                                   int targetDS, int sourceDS)
{
  File f(file, File::TextRead);
  loadParameters(f, targetDS, sourceDS);
}

void FitWorkspace::loadParameters(QIODevice * source, 
                                   int targetDS, int sourceDS)
{
  QTextStream in(source);
  FitParametersFile params;

  params.readFromStream(in);

  loadParameters(params, targetDS, sourceDS);
}

void FitWorkspace::loadParameters(const FitParametersFile & params, 
                                  const QHash<int, int> & splice)
{
  for(int dst : splice.keys())
    loadParameters(params, dst, splice[dst]);
}


void FitWorkspace::loadParameters(const FitParametersFile & params, 
                                   int targetDS, int sourceDS)
{
  for(int k = 0; k < params.parameters.size(); k++) {
    const FitParametersFile::Parameter & param = params.parameters[k];
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
        if(! (param.name == "Z" || param.name == "buffer_name"))   // Just silently ignore Z parameters and buffer names
                                // (if there aren't any)
          Terminal::out << "Found unknown parameter: '" << param.name
                        << "', ignoring" << endl;
        continue;
      }
      if(ds >= 0) {
        if(ds >= datasets) {
          Terminal::out << "Ignoring parameter '" << param.name
                        << "' for extra dataset #" << ds << endl;
          continue;
        }

        if(isGlobal(idx))
          setGlobal(idx, false); // Making it local.
        param.replaceParameter(parameterRef(idx, ds), &valueFor(idx, ds),
                               idx, ds);
      }
      else {
        param.replaceParameter(parameterRef(idx, 0), &valueFor(idx, 0),
                               idx, -1);
        for(int i = 1; i < datasets; i++) {
          values[idx + i * nbParameters] = values[idx];
          delete parameter(idx, i);
          parameterRef(idx, i) = NULL;
        }
      }
    }
  }
  emit(parametersChanged());
}

void FitWorkspace::loadParametersValues(FitParametersFile & params)
{
  if(! hasPerpendicularCoordinates())
    throw InternalError("Cannot load parameter values without perpendicular coordinates");

  QHash<QString, DataSet> vals = params.parameterValuesAsfZ();
  for(auto i = vals.begin(); i != vals.end(); i++) {
    int idx = parameterIndices.value(i.key(), -1);
    if(idx == -1)
      Terminal::out << "Ignoring parameters '" << i.key() << "'" << endl;
    const DataSet & ds = i.value();
    for(int j = 0; j < datasets; j++) {
      double val = ds.yValueAt(perpendicularCoordinates[j], true);
      setValue(idx, j, val);
    }
  }
  // emit(parametersChanged());
}

void FitWorkspace::loadParametersValues(QIODevice * source)
{
  QTextStream in(source);
  FitParametersFile params;

  params.readFromStream(in);
  if(! params.hasPerpendicularCoordinates())
    throw RuntimeError("File does not have perpendicular coordinates information");
    

  loadParametersValues(params);
}

void FitWorkspace::loadParametersValues(const QString & file)
{
  File f(file, File::TextRead);
  loadParametersValues(f);
}



void FitWorkspace::dump() const 
{
  for(int i = 0; i < parameters.size(); i++) {
    const FitParameter * pm = parameters[i];
    Debug::debug()
      << "Param #" << i << "\t" << pm << " = " << values[i] << "\t";
    if(pm)
      Debug::debug()
        << "fixed: " << pm->fixed() << "\tglobal:" << pm->global()
        << "\t" << pm->paramIndex << "," << pm->dsIndex;
    Debug::debug()
      << endl;
  }
  Debug::debug()
    << endl;
}

void FitWorkspace::computeMatrices()
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

void FitWorkspace::setupWithCovarianceMatrix(QTableWidget * widget, 
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

void FitWorkspace::writeCovarianceMatrix(QTextStream & out,  bool raw)
{
  computeMatrices();            // Makes double work, but that isn't a
                                // problem ?
  const gsl_matrix  * mat = (raw ? rawCVMatrix : cookedCVMatrix);
  out << Utils::matrixString(mat);
}

Vector FitWorkspace::saveParameterValues()
{
  if(fitData->hasEngine())
    retrieveParameters();
  
  Vector ret;
  int size = nbParameters * datasets;
  for(int i = 0; i < size; i++)
    ret << values[i];
  return ret;
}

void FitWorkspace::restoreParameterValues(const Vector & vect, int ds)
{
  int size = nbParameters * datasets;
  int beg = 0;
  int end = size;
  if(ds >= 0) {
    beg = nbParameters * ds;
    end = nbParameters * (ds+1);
  }
  for(int i = beg; (i < end && i < vect.size()); i++) {
    values[i] = vect[i];
  }
  emit(parametersChanged());
  updateParameterValues();
}

void FitWorkspace::restoreParameterValues(const Vector & vect, const QList<QPair<int, int> > & resetOnly)
{
  for(QPair<int, int> p : resetOnly) {
    values[p.first + p.second * nbParameters] =
      vect[p.first + p.second * nbParameters];
    emit(parameterChanged(p.first, p.second));
  }
  updateParameterValues();
}

void FitWorkspace::resetToBackup()
{
  restoreParameterValues(parametersBackup);
}

void FitWorkspace::resetToBackup(const QList<QPair<int, int> > & resetOnly)
{
  restoreParameterValues(parametersBackup, resetOnly);
}

void FitWorkspace::resetAllToInitialGuess()
{
  restoreParameterValues(initialGuess);
}

void FitWorkspace::resetToInitialGuess(const QList<QPair<int, int> > & resetOnly)
{
  restoreParameterValues(initialGuess, resetOnly);
}

void FitWorkspace::resetToInitialGuess(int ds)
{
  for(int i = 0; i < nbParameters; i++) {
    values[i + ds * nbParameters] = initialGuess[i + ds * nbParameters];
    emit(parameterChanged(i, ds));
  }
}

Vector FitWorkspace::saveParameterErrors(double th)
{
  Vector ret;
  for(int i = 0; i < datasets; i++)
    for(int j = 0; j < nbParameters; j++)
      ret << getParameterError(j, i, th);
  return ret;
}

void FitWorkspace::writeCovarianceMatrixLatex(QTextStream & out,  bool raw)
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

double FitWorkspace::getParameterError(int i, int ds, double th) const
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

void FitWorkspace::recomputeErrors(double threshold)
{
  for(int i = 0; i < datasets; i++)
    for(int j = 0; j < nbParameters; j++)
      errors[i*nbParameters + j] = getParameterError(j, i, threshold);
}

void FitWorkspace::recomputeJacobian()
{
  fitData->recomputeJacobian();
}

void FitWorkspace::setGlobal(int index, bool global)
{
  bool cur = isGlobal(index);
  if((global && cur) || (!global && !cur))
    return;                     // Nothing to do !

  FitParameter * target = parameter(index, 0);
  if(global) {
    target->dsIndex = -1;
    for(int i = 1; i < datasets; i++) {
      delete parameterRef(index, i);
      parameterRef(index, i) = NULL;
    }
    emit(parameterChanged(index, -1));
  }
  else {
    target->dsIndex = 0;
    for(int i = 1; i < datasets; i++) {
      FitParameter * p = target->dup();
      p->dsIndex = i;
      parameterRef(index, i) = p;
    }
    emit(parameterChanged(index, -1));
  }
}


void FitWorkspace::setValue(int index, int dataset, const QString & str)
{
  if(isGlobal(index) || dataset < 0)
    dataset = 0;
  if(isFixed(index,dataset) && str.size() > 0) {
    // We need to detect if we are changing the category (ie normal
    // fixed vs other) of the parameter.
    FitParameter *& target = parameterRef(index, dataset);
    int ds = target->dsIndex;
    if(dynamic_cast<FormulaParameter *>(target) != NULL && str[0] != '=') {
      // Replace
      FixedParameter * param = new FixedParameter(index, ds,  0);
      delete target;
      target = param;
      emit(parameterChanged(index, ds));
    }
    else if(dynamic_cast<FormulaParameter *>(target) == NULL 
            && str[0] == '=') {
      FormulaParameter * param = new FormulaParameter(index, ds, "0");
      delete target;
      target = param;
      emit(parameterChanged(index, ds));
    }
  }

  // We ask the parameters to set themselves...
  if(isGlobal(index)) {
    for(int i = 0; i < datasets; i++) {
      parameter(index, 0)->setValue(&valueFor(index, i), str);
      emit(parameterChanged(index, i));
    }
  }
  else {
    parameter(index, dataset)->setValue(&valueFor(index, dataset), str);
    emit(parameterChanged(index, dataset));
  }
}

QString FitWorkspace::getTextValue(int index, int dataset) const
{
  if(dataset < 0 || isGlobal(index))
    dataset = 0;                // global;
  return parameter(index, dataset)->textValue(valueFor(index, dataset));
}


void FitWorkspace::selectDataSet(int ds, bool silent)
{
  if(ds < 0 || ds >= datasets) {
    if(! silent)
      throw RuntimeError("Dataset index out of bounds: %1 (0 <= ds < %2)").
        arg(ds).arg(datasets);
    return;
  }
  if(ds == currentDS)
    return;
  currentDS = ds;
  emit(datasetChanged(ds));
}

int FitWorkspace::currentDataset() const
{
  return currentDS;
}

QList<QPair<int, int> > FitWorkspace::findLinearParameters(Vector * deltas,
                                                           double threshold)
{
  QList<QPair<int, int> > rv;
  sendDataParameters();
  fitData->initializeParameters();
  
  QVarLengthArray<double, 1000> params(fitData->freeParameters());
  int sz = fitData->freeParameters();
  if(sz == 0)
    return rv;

  gsl_vector_view v = gsl_vector_view_array(params.data(), sz);
  fitData->packParameters(values, &v.vector);
  GSLVector cl(fitData->dataPoints());

  SparseJacobian j1(fitData);
  SparseJacobian j2(fitData);
  
  fitData->f(&v.vector, cl, true);
  gsl_vector_scale(cl, -1);
  fitData->df(&v.vector, &j1);
  fitData->df(&v.vector, &j2, 1e5);
  j2.addJacobian(j1, -1);

  QList<QList<int> > linearParams;
  int maxParams = 0;
  int maxDs = 0;
  if(deltas)
    *deltas = Vector(datasets * nbParameters, 0.0);
  for(int j = 0; j < datasets; j++) {
    linearParams << QList<int>();
    for(int i = 0; i < nbParameters; i++) {
      gsl_vector * v1 = j1.parameterVector(i, j);
      if(! v1)
        continue;
      if(v1->size > maxDs)
        maxDs = v1->size;
      gsl_vector * v2 = j2.parameterVector(i, j);
      double nrm1 = gsl_blas_dnrm2(v1),
        nrm2 = gsl_blas_dnrm2(v2);
      if(deltas)
        (*deltas)[j * nbParameters + i] = nrm2/nrm1;
      if(nrm2 < nrm1 * threshold) {
        rv << QPair<int, int>(i, j);
        linearParams[j] << i;
      }
    }
    if(maxParams < linearParams[j].size())
      maxParams = linearParams[j].size();
  }
  
  if((!deltas) && maxParams > 0) {
    // Allocate just for the biggest
    gsl_multifit_linear_workspace * ws =
      gsl_multifit_linear_alloc(maxDs, maxParams);
    for(int j = 0; j < datasets; j++) {
      if(linearParams[j].size() == 0)
        continue;
      int sz = j1.parameterVector(linearParams[j].first(), j)->size;
      int nbp = linearParams[j].size();

      GSLVector params(nbp);
      GSLMatrix cov(nbp, nbp);
      GSLMatrix jc(sz, nbp);
      for(int i = 0; i < nbp; i++) {
        gsl_vector * src = j1.parameterVector(linearParams[j][i], j);
        gsl_vector_view v = gsl_matrix_column(jc, i);
        gsl_vector_memcpy(&v.vector, src);
      }
      gsl_vector_view yv = fitData->viewForDataset(j, cl);
      double chi;
      int status = gsl_multifit_linear(jc, &yv.vector, params, cov, &chi, ws);
      // Feeding them back
      for(int i = 0; i < nbp; i++) {
        int idx = linearParams[j][i];
        values[j*nbParameters + idx] += gsl_vector_get(params, i);
        emit(parameterChanged(idx, j));
      }
    }
    gsl_multifit_linear_free(ws);
  }

  return rv;
}

void FitWorkspace::computeAndPushJacobian()
{
  QVarLengthArray<double, 1000> params(fitData->freeParameters());
  int sz = fitData->freeParameters();
  if(sz == 0)
    return;                     


  gsl_vector_view v = gsl_vector_view_array(params.data(), sz);
  fitData->packParameters(values, &v.vector);


  GSLMatrix mt(fitData->dataPoints(), sz);
  GSLVector cl(fitData->dataPoints());

  SparseJacobian jacobian(fitData, false, mt);
  
  fitData->f(&v.vector, cl, false);
  fitData->df(&v.vector, &jacobian);

  // Write out the name of the parameters:
  QList<const FitParameter *> ps;
  for(int i = 0; i < fitData->freeParameters(); i++)
    ps << NULL;
  
  for(int i = 0; i < fitData->currentParameters().size(); i++) {
    const FitParameter * fp = fitData->currentParameters()[i];
    if(fp->fitIndex >= 0)
      ps[fp->fitIndex] = fp;
  }

  for(int i = 0; i < ps.size(); i++) {
    const FitParameter * fp = ps[i];
    QString n = fitData->parameterDefinitions[fp->paramIndex].name;
    if(fp->dsIndex >= 0)
      n += QString("[#%1]").arg(fp->dsIndex);
    Terminal::out << "#" << i << " (y" << i+2 << ") - " << n << endl;
  }
    
  int nbds = fitData->datasets.size();
  for(int i = 0; i < nbds; i++) {
    QList<Vector> cols;
    cols << fitData->datasets[i]->x();
    gsl_vector_view v2 = fitData->viewForDataset(i, cl);
    cols << Vector::fromGSLVector(&v2.vector);
    for(int j = 0; j < sz; j++) {
      gsl_vector_view v3 = gsl_matrix_column(mt, j);
      v2 = fitData->viewForDataset(i, &v3.vector);
      cols << Vector::fromGSLVector(&v2.vector);
    }
    soas().pushDataSet(fitData->datasets[i]->
                       derivedDataSet(cols, "_jacobian.dat"));
  }

}


void FitWorkspace::setBufferWeight(int dataset, double val)
{
  if(dataset < 0) {
    for(int i = 0; i < datasets; i++)
      fitData->weightsPerBuffer[i] = val;
  }
  else {
    if(dataset >= datasets)
      throw InternalError("Out of bounds weight setting: %1 for %2").
        arg(dataset).arg(datasets);
    fitData->weightsPerBuffer[dataset] = val;
  }

  /// @todo Signal !
}

double FitWorkspace::getBufferWeight(int dataset) const
{
  return fitData->weightsPerBuffer[dataset];
}


void FitWorkspace::startFit()
{
  
  if(! fitData->checkWeightsConsistency() && warnings) {
    if(! warnings->warnOnce("error-bars",
                           "Error bar inconsistency" ,
                           "You are about to fit multiple buffers where some of the buffers have error bars and some others don't.\n\nErrors will NOT be taken into account !\nStart fit again to ignore"))
      throw RuntimeError("Cancelled");

    
  }

  sendDataParameters();
  int freeParams = fitData->freeParameters();
  if(! fitData->independentDataSets() &&
     freeParams > 80 &&
     fitData->datasets.size() > FitEngine::buffersForMulti &&
     !fitData->engineFactory->isMultiCapable && warnings) {
    if(! warnings->warnOnce(QString("massive-mfit-%1").
                            arg(fitData->engineFactory->name),
                            QString("Fit engine %1 not adapted").
                            arg(fitData->engineFactory->description),
                            QString("The fit engine %1 is not adapted to massive multifits, it is better to use the Multi fit engine").
                            arg(fitData->engineFactory->description)))
      throw RuntimeError("Cancelled");
  }
  fitStartTime = QDateTime::currentDateTime();
  soas().shouldStopFit = false;
  prepareFit(fitEngineParameterValues.value(fitData->engineFactory, NULL));
  parametersBackup = saveParameterValues();
  shouldCancelFit = false;
  freeParams = fitData->freeParameters();

  recompute(true);
  if(!std::isfinite(overallChiSquared))
    throw RuntimeError("Starting residuals are not finite, not point going on");
  
  QString params;
  if(fitData->independentDataSets())
    params = QString("%1 %2 %3").
      arg(freeParams / (1.0 * fitData->datasets.size())).
      arg(QChar(0xD7)).arg(fitData->datasets.size());
  else {

    int glb = 0, loc = 0;
    for(const FreeParameter * fp : fitData->allParameters) {
      if(fp->global())
        ++glb;
      else
        ++loc;
    }

    params = QString("%1 free parameters (%2 global, %3 local per dataset)").
      arg(freeParams).arg(glb).arg(loc/(1.0 * datasetNumber()));
  }
  
  Terminal::out << "Starting fit '" << fitName() << "' on "
                << fitData->datasets.size() << " datasets with "
                << params 
                << " using the '" << fitData->engineFactory->name
                << "' fit engine; initial residuals: "
                << formatResiduals(overallPointResiduals)
                << endl;
  fitEnding = Running;
  emit(startedFitting(freeParams));
}

double FitWorkspace::elapsedTime() const
{
  return fitStartTime.msecsTo(QDateTime::currentDateTime()) * 1e-3;
}

int FitWorkspace::nextIteration()
{
  int status = fitData->iterate();
  residuals = fitData->residuals();

  /// Signal at the end of each iteration
  retrieveParameters();
  emit(iterated(fitData->nbIterations,
                residuals,
                saveParameterValues()
                ));
    
  lastResiduals = residuals;
  return status;
}

QString FitWorkspace::endingDescription(FitWorkspace::Ending end)
{
  return FitTrajectory::endingName(end);
}

QColor FitWorkspace::endingColor(FitWorkspace::Ending end)
{
  switch(end) {
  case FitWorkspace::Converged:
    return QColor::fromRgb(0,128,0);
  case FitWorkspace::Cancelled:
    return QColor::fromHsv(270, 255, 255);
  case FitWorkspace::TimeOut:
    return QColor::fromHsv(359, 255, 255);
  case FitWorkspace::Error:
  case FitWorkspace::Exception:
  case FitWorkspace::NonFinite:
    return QColor::fromHsv(359, 255, 255);
  default:
    ;
  }
  return QColor();
}

void FitWorkspace::setTracing(QTextStream * target)
{
  tracingStream = target;
}

void FitWorkspace::traceFit()
{
  if(! tracingStream)
    return;
  ValueHash trace;

  trace << "iteration" << fitData->nbIterations;
  trace << "residuals" << formatResiduals(fitData->residuals());

  // OK, I thought this would look better, but, weel.
  QList<const FitParameter *> ps;
  for(int i = 0; i < fitData->freeParameters(); i++)
    ps << NULL;
  
  for(int i = 0; i < fitData->currentParameters().size(); i++) {
    const FitParameter * fp = fitData->currentParameters()[i];
    if(fp->fitIndex >= 0)
      ps[fp->fitIndex] = fp;
  }
  
  for(const FitParameter * fp : ps) {
    QString n = fitData->parameterDefinitions[fp->paramIndex].name;
    if(fp->dsIndex >= 0)
      n += QString("[#%1]").arg(fp->dsIndex);
    int idx = (fp->dsIndex >= 0 ? fp->dsIndex : 0) * nbParameters +
      fp->paramIndex;
    trace << n << values[idx];
  }
  if(fitData->nbIterations == 0) {
    QString hd = QString("## %1").arg(trace.keyOrder.join("\t"));
    (*tracingStream) << hd << "\n";
  }
  (*tracingStream) << trace.toString("\t", "x", true) << endl;
}

FitWorkspace::Ending FitWorkspace::runFit(int iterationLimit)
{
  try {
    startFit();
    traceFit();
    int status;
    do {
      status = nextIteration();
      traceFit();
      if(shouldCancelFit || soas().shouldStopFit) {
        fitEnding = Cancelled;
        break;
      };
      if(fitData->nbIterations >= iterationLimit) {
        fitEnding = TimeOut;
        break;
      }
      if(status != GSL_CONTINUE) {
        if(status == GSL_SUCCESS)
          fitEnding = Converged;
        else
          fitEnding = Error;
        break;
      }
    } while(true);
  }
  catch(::Exception & e) {
    Terminal::out << "Fit aborted with error: " << e.message() << endl;
    fitEnding = Exception;
    if(fitData->debug > 0)
      Debug::debug() << "Fit aborted with exception: " << e.message() << endl
                     << "\nBacktrace:\n\t"
                     << e.exceptionBacktrace().join("\n\t") << endl;

  }

  endFit(fitEnding);
  return fitEnding;
}


void FitWorkspace::endFit(FitWorkspace::Ending ending)
{
  fitEnding = ending;
  double tm = fitStartTime.msecsTo(QDateTime::currentDateTime()) * 1e-3;
  Terminal::out << "Fitting took an overall " << tm
                << " seconds, with " << fitData->evaluationNumber 
                << " evaluations" << endl;

  /// @todo Here: a second computation of the covariance matrix...
  recomputeErrors();
  recompute(true);              // Make sure the residuals are
                                // properly computed.

  /// @todo Here: first computation of the covariance matrix...
  writeToTerminal();


  trajectories << 
    FitTrajectory(parametersBackup, saveParameterValues(),
                  saveParameterErrors(),
                  overallPointResiduals,
                  overallRelativeResiduals,
                  residuals,
                  lastResiduals-residuals,
                  pointResiduals,
                  fitData->engineFactory->name,
                  fitStartTime, fitData);
  FitTrajectory & trj = trajectories.last();
  trj.ending = ending;
  trj.flags = currentFlags;
  trj.weights = fitData->weightsPerBuffer;
  
  fitData->doneFitting();
  emit(finishedFitting(ending));
  if(ending == Cancelled)       // break out of any enclosing context,
                                // including scripts.
    throw RuntimeError("Fit cancelled");
}

void FitWorkspace::cancelFit()
{
  shouldCancelFit = true;
}

void FitWorkspace::quit()
{
  emit(quitWorkspace());
}


CommandOptions * FitWorkspace::fitEngineParameters(FitEngineFactoryItem * engine)
{
  
  if(! fitEngineParameterValues.value(engine, NULL)) {
    FitEngine * f = engine->creator(fitData);
    fitEngineParameterValues[engine] =
      new CommandOptions(f->getEngineParameters());
  }
  return fitEngineParameterValues[engine]; 
}


void FitWorkspace::setFitEngineFactory(FitEngineFactoryItem * item,
                                       const CommandOptions & options)
{
  fitData->engineFactory = item;
  for(const QString & n : options.keys())
    (*fitEngineParameters(item))[n] = options[n]->dup();
  emit(fitEngineFactoryChanged(item));
}

void FitWorkspace::setTrajectoryFlags(const QSet<QString> & flags)
{
  currentFlags = flags;
}

const FitTrajectory & FitWorkspace::lastTrajectory() const
{
  if(trajectories.size() < 1)
    throw InternalError("No trajectories when trying to get the last one");
  return trajectories.last();
}

mrb_value FitWorkspace::parametersToRuby(const Vector & values) const
{
  QStringList names = parameterNames();
  if(values.size() != datasets * names.size())
    throw InternalError("Wrong number of parameters: %1 for %2").
      arg(values.size()).arg(datasets * names.size());
  MRuby * mr = MRuby::ruby();
  mrb_value rv = mr->newArray();
  for(int i = 0; i < datasets; i++) {
    mrb_value hsh = mr->newFancyHash();

    for(int j = 0; j < names.size(); j++) {
      mrb_value k = mr->fromQString(names[j]);
      mrb_value v = mr->newFloat(values[i*names.size() + j]);
      mr->fancyHashSet(hsh, k, v);
    }
    mr->arrayPush(rv, hsh);
  }
  return rv;
}


//////////////////////////////////////////////////////////////////////

CovarianceMatrixDisplay::CovarianceMatrixDisplay(FitWorkspace * params, 
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
  File f(file, File::TextOverwrite);
  QTextStream o(f);
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
