/*
  hook.cc: implementation of application-wide hooks
  Copyright 2014 by CNRS/AMU

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
#include <hook.hh>

QList<Hook*> * Hook::hooks = NULL;

void Hook::registerHook(Hook * s)
{
  if(! hooks)
    hooks = new QList<Hook*>;
  *hooks << s;
}

Hook::Hook(const std::function<void ()> & fnc) :
  function(fnc)
{
  registerHook(this);
}

void Hook::runHooks()
{
  if(! hooks)
    return;
  for(int i = 0; i < hooks->size(); i++)
    hooks->value(i)->function();
}
