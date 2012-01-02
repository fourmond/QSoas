/*
  peaks.cc: implementation of the Peaks class
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
#include <peaks.hh>

#include <dataset.hh>

Peaks::Peaks(const Vector & cx, const Vector & cy, int w) :
  x(cx), y(cy), window(w)
  
{
}

Peaks::Peaks(const DataSet * ds, int w) :
  x(ds->x()), y(ds->y()), window(w)
{
}

QList<PeakInfo> Peaks::findPeaks()
{
  QList<PeakInfo> peaks;
  if(extrema.size() == 0)
    extrema = y.extrema(window);


  for(int i = 0; i < extrema.size(); i++) {
    /// @todo Thresholding could come in here.
    int idx = extrema[i];
    PeakInfo info;
    info.isMin = idx < 0;
    idx = abs(idx) - 1;
    info.index = idx;
    info.x = x[idx];
    info.y = y[idx];
    peaks << info;
  }
  return peaks;
}
