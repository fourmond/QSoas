/*
  peaks.cc: peak detection
  Copyright 2012, 2014 by CNRS/AMU

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
#include <peaks.hh>

#include <exceptions.hh>

#include <dataset.hh>
#include <debug.hh>

bool PeakInfo::comparePeakMagnitude(const PeakInfo &a, const PeakInfo & b)
{
  return a.magnitude > b.magnitude;
}

void PeakInfo::sortByMagnitude(QList<PeakInfo> & peaks)
{
  qSort(peaks.begin(), peaks.end(), &PeakInfo::comparePeakMagnitude);
}

bool PeakInfo::comparePeakPosition(const PeakInfo &a, const PeakInfo & b)
{
  return a.x < b.x;
}

void PeakInfo::sortByPosition(QList<PeakInfo> & peaks)
{
  qSort(peaks.begin(), peaks.end(), &PeakInfo::comparePeakPosition);
}

void PeakInfo::removeMinMax(QList<PeakInfo> & peaks, bool removeMax)
{
  for(int i = 0; i < peaks.size(); i++) {
    if(!removeMax != !peaks[i].isMin) {
      peaks.takeAt(i);
      i--;
    }
  }
}

void PeakInfo::computeArea(QList<PeakInfo> & peaks, const Vector & x, const Vector & y)
{
  QList<PeakInfo> pk = peaks;
  for(int i = 0; i < pk.size(); i++)
    pk[i].area = i;             // Boaf
  PeakInfo::sortByPosition(peaks);
  Vector iy = Vector::integrateVector(x,y);

  int left = 0;
  for(int i = 0; i < pk.size(); i++) {
    int right; 
    if(i < pk.size() - 1) {
      double xt = 0.5 *
        (pk[i].x + pk[i].rightHHWidth +
         pk[i+1].x - pk[i+1].leftHHWidth);
      right = x.findCrossing(pk[i].index, xt);
    }
    else
      right = x.size()-1;

    peaks[static_cast<int>(pk[i].area)].area = iy[right] - iy[left];
    left = right;
  }
}


//////////////////////////////////////////////////////////////////////


Peaks::Peaks(const DataSet * ds, int w) :
  x(ds->x()), y(ds->y()), window(w), dataset(ds)
{
}

QList<PeakInfo> Peaks::findPeaks(bool includeBorders)
{
  QList<PeakInfo> peaks;
  if(extrema.size() == 0)
    extrema = y.extrema(window);
  double avg, var;
  y.stats(&avg, &var);

  for(int i = 0; i < extrema.size(); i++) {
    /// @todo Thresholding could come in here.
    int idx = extrema[i];
    PeakInfo info;
    info.isMin = idx < 0;
    idx = abs(idx) - 1;
    if(! includeBorders && (idx == 0 || idx == y.size() - 1))
      continue;
    info.index = idx;
    info.x = x[idx];
    info.y = y[idx];
    info.magnitude = fabs(avg - info.y);

    int j = y.findCrossing(idx, y[idx]/2, -1);
    info.leftHHWidth = info.x - x.value(j, 0.0/0.0);
    j = y.findCrossing(idx, y[idx]/2, 1);
    info.rightHHWidth = x.value(j, 0.0/0.0) - info.x;
    info.area = 0;              // Defaults to 0...

    // Now, we look for the right and left half widths
    peaks << info;
  }
  return peaks;
}


QList<EchemPeakPair> Peaks::findPeakPairs()
{
  QList<EchemPeakPair> pairs;
  // First, look at the first sign change
  int delta = dataset->deltaSignChange(0);

  if(delta <= 0)
    throw RuntimeError("The dataset must hold both forward and reverse scans");

  double sign = x[1] - x[0];
  
  
  QList<PeakInfo> peaks = findPeaks(false);
  QList<PeakInfo> forward;
  int breakPoint = 0;           // Will be the index of the first
                                // backward peak.
  for(; breakPoint < peaks.size(); breakPoint++) {
    if(peaks[breakPoint].index >= delta)
      break;
    forward << peaks[breakPoint];
  }
  QList<PeakInfo> backward = peaks.mid(breakPoint);
  PeakInfo::sortByMagnitude(forward);
  // o << "Forward: " << forward.size() << " -- backward: " 
  //   << backward.size() << endl;
  
  bool polarity = forward[0].isMin;

  for(int i = 0; i < forward.size(); i++) {
    if(forward[i].isMin != polarity) // XOR isn't it ?
      break;                         // We stop at the first false positive ?
    EchemPeakPair pair;
    pair.forward = forward[i];
    pair.backward.index = -1;   // No backward peak for now

    double x = pair.forward.x;

    // o << "Looking for reverse peak: " << endl;
    // Now, we look for the matching pair on the reverse scan, ie the
    // first peak of the opposite direction right after this one.
    for(int j = 0; j < backward.size(); j++) {
      // o << " -> index " << backward[j].index << " (x = " 
      //   << backward[j].x << ")" << endl;
      if(backward[j].isMin == polarity)
        continue;               // Can't be this one.

      // o << " -> correct polarity " << (x - backward[j].x) 
      //   << " | " << sign << " -- " <<  (x - backward[j].x) * sign 
      //   << endl;
    
      // We pick the first one after the forward scan
      if((x - backward[j].x) * sign >= 0) {
        /// @todo Limit the maximum possible
        pair.backward = backward[j];
        backward[j].isMin = polarity; // Exclude it from next scans
        break;
      }
    }
    pairs << pair;
  }

  return pairs;
}
