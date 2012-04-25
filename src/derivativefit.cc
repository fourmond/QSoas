/*
  derivativefit.cc: fit with automatic fitting of the derivative
  Copyright 2012 by Vincent Fourmond

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
#include <derivativefit.hh>

#include <fitdata.hh>
#include <dataset.hh>
#include <perdatasetfit.hh>

void DerivativeFit::processOptions(const CommandOptions & opts)
{
  /// @todo add options to the command definition
  Fit::processOptions(underlyingFit, opts);
}


QString DerivativeFit::optionsString() const
{
  return Fit::optionsString(underlyingFit) + " -- derivative";
}

void DerivativeFit::checkDatasets(const FitData * data) const
{
  if(data->datasets.size() != 2)
    throw RuntimeError("Fit " + name + " needs exactly two buffers");
  if(data->datasets[0]->x() != data->datasets[1]->x())
    throw RuntimeError("Fit " + name + " works under the assumption that both the derivative and the function datasets hold the same X values");
    
}

QList<ParameterDefinition> DerivativeFit::parameters() const
{
  return underlyingFit->parameters(); // Nothing else, heh ?
}

void DerivativeFit::initialGuess(FitData * data, double * guess)
{
  underlyingFit->initialGuess(data, guess);
}

QString DerivativeFit::annotateDataSet(int idx) const
{
  if(idx == 0)
    return "function";
  return "derivative";
}

DerivativeFit::~DerivativeFit()
{
}

void DerivativeFit::function(const double * parameters,
                             FitData * data, gsl_vector * target)
{
  gsl_vector_view fnView = data->viewForDataset(0, target);
  gsl_vector_view derView = data->viewForDataset(1, target);
  const DataSet * baseDS = data->datasets[0];

  // Only compute the function once !
  underlyingFit->function(parameters, data,
                          baseDS, &fnView.vector);
  DataSet::firstDerivative(baseDS->x().data(), 1, 
                           fnView.vector.data, fnView.vector.stride,
                           derView.vector.data, derView.vector.stride,
                           baseDS->x().size());
                           
}

