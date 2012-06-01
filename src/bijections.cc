/*
  bijection.cc: implementation of common bijections
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
#include <bijection.hh>

class LogBijection : public Bijection {
public:
  virtual QString name() const {
    return "log";
  };

  virtual QString publicName() const {
    return "Log";
  };

  virtual double forward(double x) const {
    return exp(x);
  };

  
  virtual double backward(double y) const {
    if(y <= 0)
      return -200;                 // if negative, go for 0 !
    return log(y);
  };

  virtual Bijection * dup() const {
    return new LogBijection(*this);
  };

  
  // virtual double derivationStep(double /*value*/) const {
  //   return 0.1;                        // should be decent ?
  // };

};

static Bijection * createLog() {
  return new LogBijection;
}

BijectionFactoryItem logBijection(createLog);

//////////////////////////////////////////////////////////////////////

class HyperbolicRangeBijection : public Bijection {

  void prepare(double & a, double & b) const {
    a = parameterValues[0];
    b = parameterValues[1];
    if(a > b)
      std::swap(a, b);
    if(a == b)
      b = a + 1;                // Whatever, anyway that doesn't make
                                // sense.
  };
public:
  
  HyperbolicRangeBijection() {
    parameterValues.resize(2);  // Two parameters
  };

  virtual QStringList parameters() const {
    return QStringList() << "min" << "max";
  };

  virtual QString name() const {
    return "hyper-range";
  };

  virtual QString publicName() const {
    return "Range (hyperbolic)";
  };

  virtual double forward(double x) const {
    double a,b;
    prepare(a,b);
    return 0.5 * (a + b) + 0.5 * (b - a) * tanh(x);
  };

  
  virtual double backward(double y) const {
    double a,b;
    prepare(a,b);
    if(y < a)
      return -80;
    if(y > b) 
      return +80;

    double sc = 2 * (y - 0.5 * (a+b))/(b - a);

    /// @todo We should check for range errors here.
    return atanh(sc);
  };

  virtual Bijection * dup() const {
    return new HyperbolicRangeBijection(*this);
  };

};

static Bijection * createHBR() {
  return new HyperbolicRangeBijection;
}

BijectionFactoryItem hbBijection(createHBR);
