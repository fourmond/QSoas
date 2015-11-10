/**
   \file fittrajectory.hh
   Definition of fit trajectories.
   Copyright 2013, 2015 by CNRS/AMU

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
#ifndef __FITTRAJECTORY_HH
#define __FITTRAJECTORY_HH

#include <vector.hh>

/// This class represents a "fit operation", ie what happens every
/// time the user clicks on the fit button.
///
/// @todo Add storage of integers for indexing ?
class FitTrajectory {
public:


  /// The initial parameters
  Vector initialParameters;

  /// The final parameters
  Vector finalParameters;

  /// The errors on the final parameters
  Vector parameterErrors;

  typedef enum {
    Converged,
    Cancelled,
    TimeOut,
    NonFinite,
    Error,
    Invalid
  } Ending;

  /// How the fit ended.
  Ending ending;

  /// The residuals of the final parameters
  double residuals;

  /// The (relative) residuals
  double relativeResiduals;

  /// The internal residuals
  double internalResiduals;

  /// The fit engine
  QString engine;

  /// Starting date/time
  QDateTime startTime;

  /// Ending date/time
  QDateTime endTime;

  FitTrajectory() {
  };

  FitTrajectory(const Vector & init, const Vector & final,
                const Vector & errors, 
                double res, double rr, double intr,
                const QString & eng, const QDateTime & start,
                const QDateTime & end = QDateTime()) :
    initialParameters(init), finalParameters(final), 
    parameterErrors(errors),
    ending(Converged), residuals(res), relativeResiduals(rr),
    internalResiduals(intr),
    engine(eng), startTime(start) {
    if(end.isValid())
      endTime = end;
    else
      endTime = QDateTime::currentDateTime();
    if(! final.allFinite())
      ending = NonFinite;

  };

  /// Comparison by residuals.
  bool operator<(const FitTrajectory & o) const {
    if(std::isfinite(relativeResiduals) && ! std::isfinite(o.relativeResiduals))
      return true;
    if(! std::isfinite(relativeResiduals) && std::isfinite(o.relativeResiduals))
      return false;
    return relativeResiduals < o.relativeResiduals;
  };


  /// Returns true if the argument is within the error range of this
  /// one (that does not necessarily mean that the reverse is true).
  bool isWithinErrorRange(const FitTrajectory & o) const;

  /// Returns a list of strings suitable for export.
  ///
  /// Format is: initial parameters, status, final parameters together
  /// with errors, then... The full order is that given by
  /// exportHeaders.
  QStringList exportColumns() const;

  /// Does the reverse of exportColumns().
  void loadFromColumns(const QStringList & cols, int nb);

  static QStringList exportHeaders(const QStringList & paramNames, int nb);

  static QString endingName(Ending end);

  static Ending endingFromName(const QString & n);
};


/// A series of trajectories grouped together.
class FitTrajectoryCluster {

  /// Whether the given trajectory should belong to the cluster or
  /// not.
  bool belongsToCluster(const FitTrajectory & traj) const;


  /// Merges the given cluster into this one.
  void mergeCluster(const FitTrajectoryCluster & other);
  
public:

  /// The trajectories that belong to this cluster.
  QList<FitTrajectory> trajectories;

  FitTrajectoryCluster(const FitTrajectory & seed);

  /// Cluster the given trajectories.
  ///
  /// A trajectory belongs to a cluster if it is within the error
  /// range of a trajectory already within that cluster.
  static QList<FitTrajectoryCluster> clusterTrajectories(const QList<FitTrajectory> * trajectories);

  /// Returns a description of the cluster as a small text. Should be
  /// used after sorting.
  ///
  /// @todo This can't be the right thing, as we're not handling named
  /// parameters.
  QString dump() const;
};

#endif
