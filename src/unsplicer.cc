/*
  unsplicer.cc: implementation of the unsplicer
  Copyright 2019 by CNRS/AMU

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
#include <unsplicer.hh>

#include <dataset.hh>


UnsplicedData::UnsplicedData(double x, double y)
{
  xv << x;
  yv << y;
  dy << 0;
}

int UnsplicedData::size() const {
  return xv.size();
};

int UnsplicedData::indexOfX(double x) const {
  int idx = 0; 
  while(idx < xv.size()) {
    if(xv[idx] >= x)
      break;
    ++idx;
  }
  return idx;
}

int UnsplicedData::magnitudesAround(double x, double * ym,
                                    double * dym, int window) const {
  int idx = indexOfX(x);
  int min = std::max(idx - window, 0);
  int max = std::min(idx + window+1, yv.size());
  *ym = 0;
  *dym = 0;
  for(int i = min; i < max; i++) {
    *ym += yv[i];
    if(dy.size() > 0)
      *dym += dy[i];
  }
  *ym /= max - min;
  *dym /= max - min;
  return idx;
};


bool UnsplicedData::isMine(double x, double y, double tolerance,
                           double dtol) const {
  double ym, dym;
  int idx = magnitudesAround(x, &ym, &dym);
    
  if(fabs(y - ym) > tolerance * fabs(ym))
    return false;             // can't be.

  if(dy.size() > 1) {
    int didx = idx;
    if(didx == xv.size())
      --didx;
    double dyv = (y - yv[didx])/(x - xv[didx]);
    if(fabs(dyv - dym) > tolerance * fabs(dym))
      return false;
  }
  return true;
};


int UnsplicedData::insertPoint(double x, double y) {
  int idx = indexOfX(x);
  xv.insert(idx, x);
  yv.insert(idx, y);

  // Now update the derivative
  int order = std::min(xv.size() - 1, 7); // Should be 2 !
  dy = xv;
  try {
    DataSet::arbitraryDerivative(1, order, xv.size(),
                                 xv.data(), yv.data(),
                                 dy.data());
  }
  catch(const RuntimeError & re) {
    // This means we have only duplicate X values, dropping
    dy.clear();
  };
  return idx;
};


//////////////////////////////////////////////////////////////////////

Unsplicer::Unsplicer(const Vector & x, const Vector & y) :
  xv(x), yv(y), unspliced(false)
{
}



void Unsplicer::unsplice()
{
  if(unspliced)
    throw InternalError("Cannot run unsplice() twice");
  double tol = 0.2, dtol = 0.4;
  for(int i = 0; i < xv.size(); i++) {
    bool found = false;
    double x = xv[i], y = yv[i];
    for(UnsplicedData & t : trnds) {
      if(t.isMine(x,y,tol,dtol)) {
        t.insertPoint(x, y);
        found = true;
        break;
      }
    }
    if(!found) {
      trnds << UnsplicedData(x, y);
    }
  }

  // we do that twice
  for(int i = 0; i < 2; i++) {
    int thresh = std::max(1, (int) (xv.size() / trnds.size() * 0.1));
    for(int i = 0; i < trnds.size(); i++) {
      const UnsplicedData & d = trnds[i];
      if(d.size() <= thresh) {
        lx << d.xv;
        ly << d.yv;
        trnds.takeAt(i);
        --i;
      }
    }

    // Go in reverse order.
    for(int i = lx.size()-1; i >= 0; i--) {
      bool found = false;
      double x = lx[i], y = ly[i];
      for(UnsplicedData & t : trnds) {
        if(t.isMine(x,y,tol,dtol)) {
          t.insertPoint(x, y);
          found = true;
          break;
        }
      }
      if(found) {
        lx.takeAt(i);
        ly.takeAt(i);
        --i;
      }
    }
  }
  unspliced = true;
}

QList<UnsplicedData> Unsplicer::trends() const
{
  if(! unspliced)
    throw InternalError("Must run unsplice() before using trends()");
  return trnds;
}

QList<Vector> Unsplicer::leftovers() const
{
  if(! unspliced)
    throw InternalError("Must run unsplice() before using leftover()");
  QList<Vector> lst;
  lst << lx << ly;
  return lst;
}
