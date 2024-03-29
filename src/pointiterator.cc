/*
  pointiterator.cc: implementation of the PointIterator class.
  Copyright 2013 by CNRS/AMU

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
#include <pointiterator.hh>
#include <vector.hh>
#include <dataset.hh>

#include <exceptions.hh>

PointIterator::PointIterator(const Vector & xv, 
                             const Vector & yvo,
                             PointIterator::Type t) :
  x(xv), yv(yvo), ds(NULL), y_vect(NULL), type(t), index(0), sub(0)
{
  total = std::min(x.size(), yv.size());
  if(type == Errors || type == ErrorsAbove || type == ErrorsBelow)
    throw InternalError("Wrong iterator type: errors need a dataset");
}

PointIterator::PointIterator(const DataSet *d, PointIterator::Type t) :
  x(d->x()), yv(d->y()), ds(d), y_vect(NULL), type(t), index(0), sub(0)
{
  total = ds->nbRows();
}

PointIterator::PointIterator(const gsl_vector * yvalues, const DataSet * d,
                             bool res, PointIterator::Type t) :
  x(d->x()), yv(d->y()), ds(d), residuals(res), y_vect(yvalues), type(t), index(0), sub(0)
{
  total = ds->nbRows();
}

bool PointIterator::hasNext() const
{
  switch(type) {
  case Errors:
    return ! ((sub == 1) && (index < 0));
  default:
    ;
  }
  return index < total;
}

QPointF PointIterator::next()
{
  QPointF n = peek();
  advance();
  return n;
}

double PointIterator::y(int index) const
{
  if(y_vect) {
    if(residuals)
      return yv[index] - gsl_vector_get(y_vect, index);
    else
      return gsl_vector_get(y_vect, index);
  }
  return yv[index];
}
 

QPointF PointIterator::peek() const
{
  switch(type) {
  case ErrorsAbove: 
    return QPointF(x[index], y(index) + ds->yError(index));
  case ErrorsBelow: 
    return QPointF(x[index], y(index) - ds->yError(index));
  case Errors:
    if(sub == 1)                // backwards
      return QPointF(x[index], y(index) - ds->yError(index));
    else
      return QPointF(x[index], y(index) + ds->yError(index));
  case Steps: {
    // here is a little more complex...
    // sub == 0 -> left, sub == 1 -> right
    if(sub == 0) {              // left
      if(index > 0)
        return QPointF(0.5 * (x[index-1] + x[index]), y(index));
      else
        return QPointF(1.5 * x[index] - 0.5 * x[index+1], y(index));
    }
    else {                      // right
      if(index < total - 1)
        return QPointF(0.5 * (x[index] + x[index+1]), y(index));
      else
        return QPointF(1.5 * x[index] - 0.5 * x[index-1], y(index));
    }
  }
  default:
    ;
  }
  return QPointF(x[index], y(index));
}


void PointIterator::advance()
{
  switch(type) {
  case Steps:
    if(sub) {
      sub = 0;
      advanceIndex();
    }
    else
      sub = 1;
    break;
  case Errors:
    if(sub == 0) {
      advanceIndex(); 
      if(index >= total) {
        sub = 1;
        index = total - 1;
      }
    }
    else
      advanceIndex(-1);
    break;
  default:
    advanceIndex();
  }
}

QPointF PointIterator::next(const QTransform & trans)
{
  return trans.map(next());
}

void PointIterator::addToPath(QPainterPath & path, const QTransform & trans)
{
  bool first = true;
  while(hasNext()) {
    if(first) {
      path.moveTo(next(trans));
      first = false;
    }
    else 
      path.lineTo(next(trans));
  }
}


void PointIterator::advanceIndex(int di)
{
  while(true) {
    index += di;
    if(index < 0 || index >= total)
      break;
    if(std::isfinite(x[index]) && std::isfinite(y(index))) {
      break;
    }
    else
      nonFinitePoints << index;
  }
}
