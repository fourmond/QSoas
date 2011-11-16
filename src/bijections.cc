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
      return 0;                 // Stupid default value ?
    return log(y);
  };

  virtual Bijection * dup() const {
    return new LogBijection(*this);
  };

};

static Bijection * createLog() {
  return new LogBijection;
}

BijectionFactoryItem logBijection(createLog);
