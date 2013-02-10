/*
  eventhandler.cc: implementation of the EventHandler class
  Copyright 2013 by Vincent Fourmond

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
#include <eventhandler.hh>
#include <curveeventloop.hh>

EventHandler::EventHandler(const QString & cmd) :
  commandName(cmd)
{
  
}

EventHandler::~EventHandler()
{
}

EventHandler & EventHandler::addKey(int key, int action, const QString & help)
{
  keyActions[key] = action;
  if(! helpTexts.contains(action))
    helpTexts[action] = help;
  return *this;
}

EventHandler & EventHandler::addClick(Qt::MouseButton button, 
                                      int action, const QString & help)
{
  clickActions[button] = action;
  if(! helpTexts.contains(action))
    helpTexts[action] = help;
  return *this;
}


int EventHandler::nextAction(const CurveEventLoop & loop) const
{
  switch(loop.type()) {
  case QEvent::MouseButtonPress: 
    return clickActions.value(loop.button(), -1);
  case QEvent::KeyPress:
    return keyActions.value(loop.key(), -1);
  default:
    return -1;
  }
  return -1;
}

