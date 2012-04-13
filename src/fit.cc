/*
  fit.cc: implementation of the fits interface
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
#include <fit.hh>
#include <dataset.hh>

#include <soas.hh>
#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>

#include <exceptions.hh>

#include <fitdata.hh>
#include <fitdialog.hh>

static Group fits("fits", 0,
                  "Fits",
                  "Fitting of data");

/// @todo It may be relatively easy to copy/paste the lmder solver and
/// tweak it in order to provide better handling of domain errors...
int Fit::parametersCheck(const double * /*parameters*/,
                         FitData * /*data*/)
{
  return GSL_SUCCESS;
}

void Fit::functionForDataset(const double * parameters,
                             FitData * data, gsl_vector * target, 
                             int /*dataset*/)
{
  /// Defaults to all datasets at once
  function(parameters, data, target);
}

void Fit::makeCommands(ArgumentList * args, 
                       CommandEffector * singleFit,
                       CommandEffector * multiFit,
                       ArgumentList * originalOptions)
{

  QByteArray pn = "Fit: ";
  pn += shortDesc;
  QByteArray sd = "Single buffer fit: ";
  sd += shortDesc;
  QByteArray ld = "Single buffer fit:\n";
  ld += longDesc;

  ArgumentList * fal = NULL;
  if(args) 
    fal = new ArgumentList(*args);

  ArgumentList * options;
  if(! originalOptions) 
    options = new ArgumentList();
  else 
    options = new ArgumentList(*originalOptions);
  *options << new FileArgument("parameters", 
                               "Parameters",
                               "Pre-loads parameters");

  // We don't declare the fit command when multiple datasets are
  // necessary.
  if(minDataSets == 1)          
    new Command((const char*)(QString("fit-") + name).toLocal8Bit(),
                singleFit ? singleFit : 
                effector(this, &Fit::runFitCurrentDataSet),
                "fits", fal, options, pn, sd, ld);
  
  ArgumentList * al = NULL;
  if(args) {
    al = args;
    (*al) << new SeveralDataSetArgument("datasets", 
                                        "Dataset",
                                        "Datasets to fit",
                                        true);
  }
  else
    al = new 
      ArgumentList(QList<Argument *>()
                   << new SeveralDataSetArgument("datasets", 
                                                 "Dataset",
                                                 "Datasets to fit",
                                                 true));

  pn = "Multi fit: ";
  pn += shortDesc;
  sd = "multi buffer fit: ";
  sd += shortDesc;
  ld = "multi buffer fit:\n";
  ld += longDesc;
  new Command((const char*)(QString("mfit-") + name).toLocal8Bit(),
              multiFit ? multiFit : 
              effector(this, &Fit::runFit),
              "fits", al, options, pn, sd, ld);

  if(! multiFit) {
    /// @todo handle the case when there is a fit-specified effector.
    pn = "Fit: ";
    pn += shortDesc;;
    sd = "fit simulation: ";
    sd += shortDesc;
    ld = "fit simulation:\n";
    ld += longDesc;
    ArgumentList * al2 = new ArgumentList(*al);
    al2->insert(al2->size()-1, 
                new FileArgument("parameters", 
                                 "Parameters",
                                 "File to load parameters from"));
    new Command((const char*)(QString("sim-") + name).toLocal8Bit(),
                effector(this, &Fit::computeFit),
                "simulations", al2, originalOptions, pn, sd, ld);
    
  }
}

void Fit::processOptions(const CommandOptions & /*opts*/)
{
  // No specific option to process by default.
}


void Fit::checkDatasets(const FitData * data) const
{
  int nb = data->datasets.size();
  if(nb < minDataSets)
    throw RuntimeError(QString("Fit %1 expects at least %2 datasets, "
                               "but found only %3").
                       arg(name).arg(minDataSets).arg(nb));
  if(maxDataSets > 0 && nb > maxDataSets)
    throw RuntimeError(QString("Fit %1 expects at most %2 datasets, "
                               "but found %3").
                       arg(name).arg(maxDataSets).arg(nb));
}


void Fit::runFitCurrentDataSet(const QString & n, const CommandOptions & opts)
{
  QList<const DataSet *> ds;
  ds << soas().currentDataSet();
  runFit(n, ds, opts);
}

void Fit::runFit(const QString &, QList<const DataSet *> datasets,
                 const CommandOptions & opts)
{
  processOptions(opts);
  FitData data(this, datasets);
  checkDatasets(&data);
  FitDialog dlg(&data);

  QString loadParameters;
  updateFromOptions(opts, "parameters", loadParameters);
  if(! loadParameters.isEmpty())
    dlg.loadParametersFile(loadParameters);
  dlg.exec();
}

void Fit::computeFit(const QString &, QString file,
                     QList<const DataSet *> datasets,
                     const CommandOptions & opts)
{
  processOptions(opts);
  FitData data(this, datasets);
  checkDatasets(&data);
  FitDialog dlg(&data);
  dlg.loadParametersFile(file);
  dlg.pushSimulatedCurves();
}


Fit::~Fit()
{
}


QString Fit::annotateDataSet(int idx) const
{
  return QString();
}
