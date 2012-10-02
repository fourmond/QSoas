/**
   \file peaks.hh
   The PEAKS class, providing GSL-based fourier transforms
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
#ifndef __PEAKS_HH
#define __PEAKS_HH

#include <vector.hh>

class DataSet;

/// A class representing a peak.
///
/// @todo This is very basic for now...
///
/// @todo For real appropriate peak finding, one should have the
/// derivative, in order to find the inflexion points too. Maybe an
/// idea would have to add the 
class PeakInfo {
public:

  /// Whether it is a minimum of a maximum ?
  bool isMin;
  
  /// The position of the peak
  int index;

  /// The X position of the peak
  double x;

  /// The Y position of the peak
  double y;

  /// The magnitude of the peak, ie the absolute value of the peak's
  /// amplitude with respect to the average.
  ///
  /// @todo Shouldn't it rather be:
  /// @li value - average for max
  /// @li the opposite for a min ?
  /// Or should that be another parameter ?
  ///
  /// @todo A real amplitude should be the (absolute value ?) of the
  /// difference between the extrema and the last extrema of opposed
  /// polarity. This would be actually pretty good.
  ///
  /// @todo This class should hold the actual position of the extremum
  /// of reference.
  double magnitude;

  /// @todo More to come later here, such as witdh and assymetry (but
  /// this will have to be based on fits -- to the extent possible),
  /// surface, half-height width (or the other way around...)


  /// Compare the peaks by reverse magnitude
  static bool comparePeakMagnitude(const PeakInfo &a, const PeakInfo & b);

  /// Sorts the peaks by magnitude
  static void sortByMagnitude(QList<PeakInfo> & peaks);
};

/// A pair of forward and backward peaks, in particular in a
/// voltammogram
class EchemPeakPair {
public:
  /// The forward peak
  PeakInfo forward;

  /// The backward peak. Index < 0 if there is no backward peak
  PeakInfo backward;

  /// @name Data inspection facilities
  ///
  /// Almost all functions only make sense if isReversible() returns
  /// true.
  ///
  /// @{

  /// Is the peak reversible (ie both a forward and a backward peak ?)
  bool isReversible() const {
    return backward.index >= 0;
  };

  /// The delta X
  double deltaX() const {
    return fabs(backward.x - forward.x);
  };

  /// Average X position
  double midX() const {
    return 0.5*(backward.x + forward.x);
  };

  /// The delta Y
  double deltaY() const {
    return fabs(backward.y - forward.y);
  };

  /// @}

  
};


/// This class provides Peak detection facilities.
///
/// @todo Threshold should be built in the Peaks class
class Peaks {

  /// X values
  Vector x;

  /// Y values
  Vector y;

  /// The window to look for extrema
  int window;

  /// The local extrema
  QList<int> extrema;

  /// The original dataset
  const DataSet * dataset;

public:

  Peaks(const DataSet * ds, int window = 8);

  /// Returns a list of possible peaks, based solely on local extrema
  /// heuristics.
  ///
  /// @todo This probably should be cached too, and it should be
  /// possible to refine.
  QList<PeakInfo> findPeaks(bool includeBorders = false);


  /// Returns a list of possible electrochemical peak pairs, in the
  /// order of the magnitude of the forward peak.
  ///
  /// @todo lots of parameters...
  QList<EchemPeakPair> findPeakPairs();
  
};

#endif
