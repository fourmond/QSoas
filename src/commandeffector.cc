/*
  commandeffector.cc: implementation of the CommandEffector base class
  Copyright 2011 by Vincent Fourmond
            2012, 2013 by CNRS/AMU

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
#include <commandeffector.hh>
#include <exceptions.hh>

void CommandEffector::runWithLoop(CurveEventLoop & /*loop*/,
                                  const QString & /*commandName*/, 
                                  const CommandArguments & /*arguments*/,
                                  const CommandOptions & /*options*/)
{
  throw InternalError("Attempting to run a non-interactive effector "
                      "with an event loop");
}

CommandEffector::~CommandEffector()
{
}
