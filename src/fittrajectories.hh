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

  /// Sort by residuals
  void sortByResiduals();

public:
  FitTrajectories(const FitWorkspace * ws);

  /// Exports the fit trajectory to the given file.
  void exportToFile(QTextStream & out);

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

  /// Returns the size of the list
  int size() const;

  /// Removes all the elements whose residuals are more than @a factor
  /// times that of the lowest
  void trim(double factor);

  /// Returns the nth element
  const FitTrajectory & operator[](int idx) const;

  /// Returns the best fit, or the nth best fit.
  const FitTrajectory & best(int nth = 0) const;

  FitTrajectory & last();


  /// Iteration
  QList<FitTrajectory>::const_iterator begin() const;
  QList<FitTrajectory>::const_iterator end() const;
};

#endif
