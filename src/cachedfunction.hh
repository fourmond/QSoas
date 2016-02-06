/**
   \file cachedfunction.hh
   Functions with cache functions
   Copyright 2016 by CNRS/AMU

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
#ifndef __CACHEDFUNCTION_HH
#define __CACHEDFUNCTION_HH

class CachedFunction {
protected:
  QHash<double, double> cache;

  typedef  double (*Function)(double);

  Function func;
public:
  CachedFunction(Function f) : func(f) {;};

  double value(double v);

  double operator()(double v) { return value(v);};
};




#endif
