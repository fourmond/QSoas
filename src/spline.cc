/*
  spline.cc: implementation of the Spline class
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
#include <spline.hh>

#include <dataset.hh>

void Spline::insert(const QPointF & pos)
{
  values[pos.x()] = pos.y();
}

QList<QPointF> Spline::pointList() const
{
  QList<QPointF> points;
  for(QMap<double, double>::const_iterator i = values.begin();
      i != values.end(); i++)
    points << QPointF(i.key(), i.value());
  return points;
}

void Spline::remove(double x)
{
  if(values.size() == 0)
    return;
  QMap<double, double>::const_iterator i = values.begin();
  double closest = i.key();
  double delta = fabs(x - i.key());
  i++;
  for(; i != values.end(); i++) {
    double d = fabs(x - i.key());
    if(d < delta) {
      delta = d;
      closest = i.key();
    }
  }
  values.remove(closest);
}

Vector Spline::evaluate(const Vector & xvalues, Type type)
{
  Vector ret = xvalues;
  qSort(ret);                   // Work on sorted values.
  Vector xvals;
  Vector yvals;

  for(QMap<double, double>::const_iterator i = values.begin();
      i != values.end(); i++) {
    xvals << i.key();
    yvals << i.value();
  }
  if(xvals[0] > ret[0]) {
    // Padding:
    xvals.prepend(ret[0]);
    yvals.prepend(yvals[0]);
  }
  if(xvals.last() < ret.last()) {
    xvals.append(ret.last());
    yvals.append(yvals.last());
  }
  gsl_interp * interp = gsl_interp_alloc(interpolationType(type), 
                                         xvals.size());
  gsl_interp_init(interp, xvals.data(), yvals.data(), xvals.size());
  gsl_interp_accel * accel = gsl_interp_accel_alloc();
  for(int i = 0; i < ret.size(); i++)
    ret[i] = gsl_interp_eval(interp, xvals.data(), yvals.data(),
                             ret[i], accel);

  gsl_interp_accel_free(accel);
  gsl_interp_free(interp);
  return ret;
}


const gsl_interp_type * Spline::interpolationType(Type type) const
{
  switch(type) {
  case Linear:
    return gsl_interp_linear;
  case Polynomial:
    return gsl_interp_polynomial;
  default:
  case CSpline:
    return gsl_interp_cspline;
  case Akima:
    return gsl_interp_akima;
  }
}

