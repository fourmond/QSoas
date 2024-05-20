/**
   \file parameterrangespec.hh
   Parameter range spec, along with distance and clustering facilities
   Copyright 2024 by Vincent Fourmond

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
#ifndef __PARAMETERRANGESPEC_HH
#define __PARAMETERRANGESPEC_HH

#include <vector.hh>

class FitWorkspace;
class FitTrajectory;
class Vector;

/// The parameter specification. It is in fact a parameter range
/// specification: the parameter comes with a range and a lin/log
/// qualification
class ParameterRangeSpec {
public:
  /// The target, like what is returned by
  /// FitWorkspace::parseParameterList()
  QPair<int, int> parameter;

  /// The lower end of the range
  double low;

  /// The higher end of the range
  double high;

  /// Whether the range is logarithmic or not
  bool log;

  /// Whether or not the selection is global (i.e. all local
  /// parameters are set to the same starting value)
  bool uniform;


  /// Can be used by the explorers to store whatever.
  double storage;


  /// The center of the range
  double center() const;
  
  /// Returns 1/2 of the witdth of the interval (log10 or lin).
  double sigma() const;

  /// Returns the given value, trimmed to be within the range.
  double trim(double val) const;

  /// Returns the distance between the two values, normalized by the range
  /// (and taking into account the log vs lin).
  /// That value is always positive
  double distance(double x1, double x2) const;

  /// Parses the parameter list
  static QList<ParameterRangeSpec> parseSpecs(const QStringList & specs,
                                              FitWorkspace * workSpace,
                                              QStringList * unknowns);

  /// Computes the distance between the two trajectories, using the
  /// list of parameters as measure of distance.
  /// Any global parameter is taken to mean all the parameters.
  /// Any parameter not listed in the range specs is ignored
  static double trajectoryDistance(const QList<ParameterRangeSpec> & specs,
                                   const FitTrajectory & a,
                                   const FitTrajectory & b,
                                   const FitWorkspace * workspace,
                                   bool useFinal = true);

  /// Distance of parameter vectors
  static double trajectoryDistance(const QList<ParameterRangeSpec> & specs,
                                   const Vector & a,
                                   const Vector & b,
                                   const FitWorkspace * workspace);

  /// Averages the given trajectories according to the specifications
  /// (only the lin vs log is taken into consideration).
  /// All other parameters are silently ignored
  static Vector averageParameters(const QList<ParameterRangeSpec> & specs,
                                  const QList<Vector> & parameters,
                                  const FitWorkspace * workspace);
};

//////////////////////////////////////////////////////////////////////

/// This class handles a whole series of ParameterRangeSpec, including
/// utilities like:
/// @li distance computation
/// @li averaging
/// @li formatting ?
/// @li clustering ? (or that is only in the private part ?)
class ParameterRangeSpecs : public QList<ParameterRangeSpec> {
protected:

  /// The underlying workspace
  FitWorkspace * workspace;

public:

  ParameterRangeSpecs(const QList<ParameterRangeSpec> & lst,
                      FitWorkspace * ws);

  /// Parses the parameter list
  static ParameterRangeSpecs parseSpecs(const QStringList & specs,
                                        FitWorkspace * workSpace,
                                        QStringList * unknowns);


  /// Returns an average of the parameter vectors using the range
  /// information. The parameters not in the range are set to 0.
  ///
  /// @todo Optionally change this ?
  Vector averageParameters(const QList<Vector> & parameters) const;

  /// A class for the results of parametersDispersion
  class DispersionStats {
  public:

    /// The center of the parameters
    Vector center;


    /// Average of the squares of the distances to the center
    double moment2 = 0;

    /// Average of the power 4 of the distances to the center
    double moment4 = 0;
  };

  /// Returns some stastistics on the dispersion
  DispersionStats parametersDispersion(const QList<Vector> & parameters) const;


  /// Returns the distance from the two parameter vectors
  double trajectoryDistance(const Vector & a, const Vector & b) const;


  /// Returns a string representing the parameters, one parameter definition
  /// per line, a TAB-separated list per buffer
  QString parametersString(const Vector & params) const;


};

#endif
