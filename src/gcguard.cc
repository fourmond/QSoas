/*
  gcguard.cc: implementation of the GCGuard class
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
#include <mruby.hh>
#include <gcguard.hh>

#include <mruby/array.h>
#include <mruby/numeric.h>

GCGuard::GCGuard()
{
  MRuby * mr = MRuby::ruby();
  array = mr->newArray();
  mr->gcRegister(array);
}

GCGuard::~GCGuard()
{
  MRuby::ruby()->gcUnregister(array);
}

void GCGuard::protect(mrb_value value)
{
  MRuby::ruby()->arrayPush(array, value);
}
