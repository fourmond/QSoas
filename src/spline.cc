/*
  spline.cc: implementation of the Spline class
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014 by CNRS/AMU

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
      i != values.end(); ++i)
    points << QPointF(i.key(), i.value());
  return points;
}

int Spline::size() const
{
  return values.size();
}

void Spline::remove(double x)
{
  if(values.size() == 0)
    return;
  QMap<double, double>::const_iterator i = values.begin();
  double closest = i.key();
  double delta = fabs(x - i.key());
  ++i;
  for(; i != values.end(); ++i) {
    double d = fabs(x - i.key());
    if(d < delta) {
      delta = d;
      closest = i.key();
    }
  }
  values.remove(closest);
}

/// Assumes that the xvalues are sorted !
void Spline::preparePoints(Vector * xt,
                           Vector * xv, Vector * yv)
{
  qSort(*xt);
  xv->clear();
  yv->clear();

  for(QMap<double, double>::const_iterator i = values.begin();
      i != values.end(); ++i) {
    *xv << i.key();
    *yv << i.value();
  }
  if((xv->size() == 0) || (xv->value(0) > xt->value(0))) {
    // Padding:
    xv->prepend(xt->value(0));
    yv->prepend(yv->value(0));
  }
  if((xv->size() == 0) || (xv->last() < xt->last())) {
    xv->append(xt->last());
    yv->append(yv->last());
  }
}

gsl_interp * Spline::prepareInterpolant(Type type, const Vector & xvals, 
                                        const Vector & yvals)
{
  const gsl_interp_type * t = interpolationType(type);
  if(gsl_interp_type_min_size(t) > xvals.size())
    return NULL;                // We can't do anything here.

  gsl_interp * interp = gsl_interp_alloc(t, xvals.size());
  gsl_interp_init(interp, xvals.data(), yvals.data(), xvals.size());
  return interp;
}

Vector Spline::evaluate(const Vector & xvalues, Type type)
{
  return innerEvaluation(xvalues, type, gsl_interp_eval);
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

Vector Spline::derivative(const Vector & xvalues, Type type)
{
  return innerEvaluation(xvalues, type, gsl_interp_eval_deriv);
}

Vector Spline::secondDerivative(const Vector & xvalues, Type type)
{
  return innerEvaluation(xvalues, type, gsl_interp_eval_deriv2);
}

Vector Spline::innerEvaluation(const Vector & xvalues, Type type,
                               double (*f)(const gsl_interp *, 
                                           const double x[], const double ya[],
                                           double, gsl_interp_accel *))
{
  Vector ret = xvalues;
  Vector xvals;
  Vector yvals;
  preparePoints(&ret, &xvals, &yvals);
  gsl_interp * interp = prepareInterpolant(type, xvals, yvals);
  if(! interp) {
    ret.clear();                // No data points
    return ret;
  }

  gsl_interp_accel * accel = gsl_interp_accel_alloc();
  for(int i = 0; i < ret.size(); i++)
    ret[i] = f(interp, xvals.data(), yvals.data(),
               xvalues[i], accel);

  gsl_interp_accel_free(accel);
  gsl_interp_free(interp);
  return ret;
}


Spline::Type Spline::nextType(Spline::Type type)
{
  int t = (int) type + 1;
  if(t >= SplineTypes)
    t = Linear;
  return (Type) t;
}



QHash<QString, Spline::Type> Spline::interpolationTypes()
{
  Type t = Linear;
  QHash<QString, Spline::Type> ret;
  do {
    ret[typeName(t)] = t;
    t = nextType(t);
  } while(t != Linear);
  return ret;
}

QString Spline::typeName(Spline::Type type)
{
  switch(type) {
  case Linear:
    return "linear";
  case Polynomial:
    return "polynomial";
  case CSpline:
    return "spline";
  case Akima:
    return "akima";
  default:
    ;
  }
  return QString();
}
