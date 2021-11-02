/**
   \file gcguard.hh
   A class to handle all calls to protect MRuby objects against GC
   Copyright 2021 by CNRS/AMU

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
#ifndef __GCGUARD_HH
#define __GCGUARD_HH

/// Protects the given objects from MRuby garbage collection. All the
/// objects are released upon destruction.
class GCGuard  {
  mrb_value array;
public:
  GCGuard();
  ~GCGuard();

  /// Protects the given object from garbage collection.
  void protect(mrb_value object);
};

#endif
