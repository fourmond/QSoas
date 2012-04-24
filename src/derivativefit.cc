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
#include <perdatasetfit.hh>

void DerivativeFit::processOptions(const CommandOptions & opts)
{
  Fit * f = underlyingFit;
  /// @todo add options !
  Fit::processOptions(f, opts);
}


QString DerivativeFit::optionsString() const
{
  return QString();
  // return underlyingFit->optionsString() + " -- derivative";
}

void DerivativeFit::checkDatasets(const FitData * data) const
{
  if(data->datasets.size() != 2)
    throw RuntimeError("Fit " + name + " needs exactly two buffers");
}
