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
                       CommandEffector * multiFit)
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
  
  
  new Command((const char*)(QString("fit-") + name).toLocal8Bit(),
              singleFit ? singleFit : 
              optionLessEffector(this, &Fit::runFitCurrentDataSet),
              "fits", fal, NULL, pn, sd, ld);
  
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
              optionLessEffector(this, &Fit::runFit),
              "fits", al, NULL, pn, sd, ld);
}



void Fit::runFitCurrentDataSet(const QString & n)
{
  QList<const DataSet *> ds;
  /// @todo Implement correctly the "minimumDataSet thing". The trick
  /// is to implement it only once, and not in every single complex
  /// fit.
  // for(int i = 0; i < 
  ds << soas().currentDataSet();
  runFit(n, ds);
}

void Fit::runFit(const QString &, QList<const DataSet *> datasets)
{
  FitData data(this, datasets);
  FitDialog dlg(&data);
  dlg.exec();
}


Fit::~Fit()
{
}


QString Fit::annotateDataSet(int idx) const
{
  return QString();
}
