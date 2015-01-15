/**
  \file timedependentparameters.hh
  A collection of TimeDependentParameters
  Copyright 2015 by CNRS/AMU

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
#ifndef __TIMEDEPENDENTPARAMETERS_HH
#define __TIMEDEPENDENTPARAMETERS_HH

#include <vector.hh>


class TimeDependentParameter;
class ParameterDefinition;
class DataSet;

/// This class represents a series of TimeDependentParameter and how
/// they apply to an array of doubles (and from an array of double).
///
/// It is a hash of TimeDependentParameter indexed on the index of the
/// target point to modify in the target array.
class TimeDependentParameters : public QHash<int, TimeDependentParameter*> {
  QList<ParameterDefinition> parameters;

  /// This is a correspondance parameter name -> index.
  QHash<QString, int> parameterNames;
public:
  /// Clears
  void clear();

  /// Returns the parameters to be added to the fit parameters
  QList<ParameterDefinition> fitParameters() const;

  /// Evaluate all the functions into the target array.
  void computeValues(double t, double * target, const double * params) const;

  /// Parses a series of parameters from a string list.
  ///
  /// In addition to the string list, it requires a function to map
  /// from a name to an index.
  ///
  /// It just adds to the list, so you'd better call clear() first if
  /// you want to set from a string. Returning a negative value will
  /// throw an exception.
  void parseFromStrings(const QStringList & specs, const std::function<int (const QString &)> & indices);

  /// Returns true if the named parameter is one of the parameters
  /// this class is handling.
  bool contains(const QString & name) const;

  /// Sets all the initial guesses
  void setInitialGuesses(double * target, const DataSet * ds) const;

  /// Returns the list of discontinuities
  Vector discontinuities(const double * parameters) const;
  
};




#endif
