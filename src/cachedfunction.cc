/*
  cachedfunction.cc: implementation of functions with cache
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
#include <cachedfunction.hh>

unsigned qHash(const double &key) {
  union { double a;
    unsigned b;
  } c;
  c.a = key;
  return c.b;
}

double CachedFunction::value(double v)
{
  // QHash<double, double>::iterator it = cache.find(v);
  // if(it != cache.end())
  //   return it.value();
  double rv = func(v);
  //  cache[v] = rv;
  return rv;
}

