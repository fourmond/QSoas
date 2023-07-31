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
#include <fitworkspace.hh>

class FitData;
class ColumnBasedFormat;

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

  /// The average point residuals for each buffer
  Vector pointResiduals;

  /// The buffer weights
  Vector weights;

  /// Whether the parameters are fixed or not
  QVector<bool> fixed;

  /// How the fit ended.
  FitWorkspace::Ending ending;

  /// The residuals of the final parameters
  double residuals;

  /// The (relative) residuals
  double relativeResiduals;

  /// The internal residuals
  double internalResiduals;

  /// The delta between the residuals of the last two iterations
  double residualsDelta;

  /// The fit engine
  QString engine;

  /// Starting date/time
  QDateTime startTime;

  /// Ending date/time
  QDateTime endTime;

  /// The number of iterations
  int iterations;

  /// The number of function evaluations
  int evaluations;

  /// Series of flags attached to the trajectory. Flags cannot contain
  /// commas.
  QSet<QString> flags;

  qint64 pid;

  FitTrajectory() {
  };

  FitTrajectory(const Vector & init, const Vector & final,
                const Vector & errors, 
                double res, double rr, double intr, double delta,
                const Vector & pointRes, // The residuals for each buffer
                const QString & eng,
                const QDateTime & start,
                const FitData * data,
                const QDateTime & end = QDateTime());

  /// Comparison by residuals.
  bool operator<(const FitTrajectory & o) const {
    if(std::isfinite(relativeResiduals) && ! std::isfinite(o.relativeResiduals))
      return true;
    if(! std::isfinite(relativeResiduals) && std::isfinite(o.relativeResiduals))
      return false;
    return relativeResiduals < o.relativeResiduals;
  };

  bool operator==(const FitTrajectory & o) const;

  /// Returns whether the trajectory has the given flag or not
  bool flagged(const QString & flag) const;


  /// Returns true if the argument is within the error range of this
  /// one (that does not necessarily mean that the reverse is true).
  bool isWithinErrorRange(const FitTrajectory & o) const;

  /// Returns a list of strings suitable for export.
  ///
  /// Format is: initial parameters, status, final parameters together
  /// with errors, then... The full order is that given by
  /// exportHeaders.
  QStringList exportColumns(const QStringList & names) const;

  /// Does the reverse of exportColumns().
  /// 
  /// If the reading fails because of missing columns, the ok flag is
  /// set to false
  void loadFromColumns(const QHash<QString, QString> & cols,
                       const QStringList & parameters, int datasets,
                       bool *ok = NULL);

  static QStringList exportHeaders(const QStringList & paramNames, int nb);

  static QString endingName(FitWorkspace::Ending end);

  static FitWorkspace::Ending endingFromName(const QString & n);

protected:
  /// Returns the format for reading/writing this trajectory.
  ColumnBasedFormat * formatForTrajectory(const QStringList & paramNames,
                                          int nb = -1); 
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

  explicit FitTrajectoryCluster(const FitTrajectory & seed);

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
