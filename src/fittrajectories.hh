/**
   \file fittrajectories.hh
   List of FitTrajectory
   Copyright 2018 by CNRS/AMU

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
#ifndef __FITTRAJECTORIES_HH
#define __FITTRAJECTORIES_HH

#include <vector.hh>

class FitTrajectory;
class FitWorkspace;

/// This class represents a list of FitTrajectory items, and provide
/// tools to work with them.
///
/// The natural order for the trajectories is to be sorted by
/// time. The class also provide means to retrieve the trajectories
/// based on the residuals.
class FitTrajectories {
  /// The underlying fit workspace
  const FitWorkspace * workSpace;

  /// The list of trajectories
  QList<FitTrajectory> trajectories;

  /// The list of indices of the trajectories, sorted by residuals.
  /// Cleared very often.
  mutable QList<int> residualsOrder;

  void clearCache() const;

  /// Update the residualsOrder cache
  void updateCache() const;


public:
  explicit FitTrajectories(const FitWorkspace * ws);

  /// Exports the fit trajectory to the given file.
  void exportToFile(QTextStream & out) const;

  /// Imports fit trajectories from the given file.
  int importFromFile(QTextStream & in);

  /// Merges the given trajectory into this one, making sure there are
  /// no duplicates.
  ///
  /// Also sorts according to time.
  void merge(const FitTrajectories & other);

  /// Appends to the list of trajectories
  FitTrajectories & operator<<(const FitTrajectory & trj);

  /// Sorts the list of trajectories by startTime.
  void sort();

  /// Sort by residuals
  void sortByResiduals();

  /// Sort with an arbitrary comparison function
  void sort(std::function<bool (const FitTrajectory &a,
                                const FitTrajectory & b)> cmp);

  /// Returns the size of the list
  int size() const;

  /// Removes all the elements whose residuals are more than @a factor
  /// times that of the lowest. Returns the number of trajectories
  /// trimmed.
  int trim(double factor);

  /// Removes all but the nb best trajectories. Returns the number of
  /// trajectories trimmmed.
  int keepBestTrajectories(int nb);

  /// Returns the nth element
  const FitTrajectory & operator[](int idx) const;

  /// Returns the nth element
  FitTrajectory & operator[](int idx);

  /// Returns the best fit, or the nth best fit.
  const FitTrajectory & best(int nth = 0) const;

  /// Removes the nth element
  void remove(int nth);

  FitTrajectory & last();

  const FitTrajectory & last() const;

  /// Clears the trajectories
  void clear();

  /// Returns the trajectories that contain the given flag, or the
  /// trajectories bearing any flag if flag is empty.
  ///
  /// If @a flagged is false, then it returns the trajectories that do
  /// NOT have this flag.
  ///
  /// If both @a flag is empty and @a flagged is false, this returns
  /// all the trajectories that have NO flags.
  FitTrajectories flaggedTrajectories(const QString & flag,
                                      bool flagged = true) const;

  /// Returns the list of flags
  QSet<QString> allFlags() const;

  /// Summarizes all the trajectories, i.e. computes a weighted
  /// average/standard deviation of all the parameters, using a
  /// weights a decreasing exponential of the residuals with unit the
  /// best residuals.
  ///
  /// Returns a list: parameters, stddev, average error, and
  /// optionally the total weight in @p weight.
  QList<Vector> summarizeTrajectories(double * weight = NULL) const;
  


  /// Iteration
  QList<FitTrajectory>::const_iterator begin() const;
  QList<FitTrajectory>::const_iterator end() const;

  QList<FitTrajectory>::iterator begin();
  QList<FitTrajectory>::iterator end();
};

#endif
