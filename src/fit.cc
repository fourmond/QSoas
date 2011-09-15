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

int Fit::f(const gsl_vector * parameters, 
           FitData * data, gsl_vector * target_f)
{
  QVarLengthArray<double, 1024> params(data->fullParameterNumber());
  data->unpackParameters(parameters, params.data());

  // First, compute the value in place
  function(params.data(), data, target_f);
  // Then, subtract data.
  data->subtractData(target_f);
  /// @todo Data weighting ?

  return GSL_SUCCESS;
}

/// @todo Maybe these functions or part of them should join FitData ?
int Fit::df(const gsl_vector * parameters, 
           FitData * data, gsl_matrix * target_df)
{
  QVarLengthArray<double, 1024> params(data->fullParameterNumber());
  data->unpackParameters(parameters, params.data());

  int nbparams = data->parameters.size();
  if(parameters->size != nbparams)
    throw InternalError("Size mismatch between GSL parameters "
                        "and FitData data");


  /// First, compute the common value, and store it in... the storage
  /// place.
  function(params.data(), data, data->storage);

  /// Hmmm...
  for(int i = 0; i < nbparams; i++) {

    int idx = data->packedToUnpackedIndex(i);
    double saved = params[idx];
    double step = data->parameters[i].derivationFactor * saved;
    /// @todo minimum step

    gsl_vector_view v = gsl_matrix_column(target_df, i);
    if(data->parameters[i].dsIndex < 0) {
      int nb_ds_params = data->parameterDefinitions.size();
      int nb_datasets = data->datasets.size();
      for(int j = 0; j < nb_datasets; j++)
        params[idx + j * nb_ds_params] = saved + step;
      function(params.data(), data, &v.vector);
    }
    else {
      params[idx] = saved + step;
      // First copy around to avoid spurious data in the end.

      /// @todo We could still take down the number of operations by
      /// working only on the target subvector.
      gsl_vector_memcpy(&v.vector, data->storage);
      functionForDataset(params.data(), data, &v.vector, 
                         data->parameters[i].dsIndex);
    }
    gsl_vector_sub(&v.vector, data->storage);
    gsl_vector_scale(&v.vector, 1/step);
    if(data->parameters[i].dsIndex < 0) {
      int nb_ds_params = data->parameterDefinitions.size();
      int nb_datasets = data->datasets.size();
      for(int j = 0; j < nb_datasets; j++)
        params[idx + j * nb_ds_params] = saved;
    }
    else 
      params[idx] = saved;        // Don't forget to restore !
  }
  return GSL_SUCCESS;
}

int Fit::fdf(const gsl_vector * parameters,  FitData * data, 
             gsl_vector * target_f, gsl_matrix * target_J) 
{
  /// @todo Here, we can save one funcall...
  ///
  /// That dosen't seem too expensive in the end, anyway...
  f(parameters, data, target_f);
  df(parameters, data, target_J);
  return GSL_SUCCESS;
}

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
