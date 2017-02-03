/*
  xyiterable.cc: implementation of the XYIterable class hierarchy
  Copyright 2017 by CNRS/AMU

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
#include <xyiterable.hh>
#include <dataset.hh>


QString XYIterable::dataName() const
{
  return QString();
}

bool XYIterable::nextWithErrors(double * x, double * y, double * err)
{
  bool v = next(x, y);
  if(v)
    *err = 0;
  return v;
}

QRectF XYIterable::boundingBox()
{
  reset();
  double xmin = 0, xmax = 0, ymin = 0, ymax = 0;
  double x, y;
  bool first = true;
  while(next(&x, &y)) {
    if(! (std::isfinite(x) && std::isfinite(y)))
      continue;
    if(first) {
      xmin = xmax = x;
      ymin = ymax = y;
      first = false;
    }
    xmin = std::min(xmin, x);
    xmax = std::max(xmax, x);
    ymin = std::min(ymin, y);
    ymax = std::max(ymax, y);
  }
  return QRectF(xmin, ymin, xmax - xmin, ymax - ymin);
}

//////////////////////////////////////////////////////////////////////

XYIDataSet::XYIDataSet(const DataSet * ds) :
  dataset(ds), cur(0)
{
  hasErrors = dataset->options.hasYErrors();
}
  
bool XYIDataSet::next(double * x, double * y)
{
  int sz = dataset->nbRows();
  if(cur < sz) {
    *x = dataset->x()[cur];
    *y = dataset->y()[cur];
    ++cur;
    return true;
  }
  return false;
}

void XYIDataSet::reset()
{
  cur = 0;
}

QString XYIDataSet::dataName() const
{
  return dataset->name;
}

bool XYIDataSet::nextWithErrors(double * x, double * y, double * err)
{
  if(! hasErrors)
    return XYIterable::nextWithErrors(x, y, err);

  int sz = dataset->nbRows();
  if(cur < sz) {
    *x = dataset->x()[cur];
    *y = dataset->y()[cur];
    *err = dataset->yError(cur);
    ++cur;
    return true;
  }
  return false;
}

//////////////////////////////////////////////////////////////////////

XYIGSLVectors::XYIGSLVectors(const gsl_vector * xvs,
                             const gsl_vector * yvs,
                             const gsl_vector * evs) :
  xv(xvs), yv(yvs), ev(evs), cur(0)
{
}

bool XYIGSLVectors::next(double * x, double * y)
{
  if(cur < xv->size) {
    *x = gsl_vector_get(xv, cur);
    *y = gsl_vector_get(yv, cur);
    ++cur;
    return true;
  }
  return false;
}

void XYIGSLVectors::reset()
{
  cur = 0;
}

QString XYIGSLVectors::dataName() const
{
  return name;
}

bool XYIGSLVectors::nextWithErrors(double * x, double * y, double * err)
{
  if(cur < xv->size) {
    *x = gsl_vector_get(xv, cur);
    *y = gsl_vector_get(yv, cur);
    if(ev)
      *err = gsl_vector_get(ev, cur);
    else
      *err = 0;
    ++cur;
    return true;
  }
  return false;
}

