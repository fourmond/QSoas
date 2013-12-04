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
#include <baselinehandler.hh>
#include <pointpicker.hh>

#include <exceptions.hh>

QHash<QString, EventHandler *> * EventHandler::registeredHandlers = NULL;


EventHandler::EventHandler(const QString & cmd) :
  commandName(cmd), lastAction(-1)
{
}

EventHandler::EventHandler(const EventHandler & o) :
  commandName(o.commandName), helpTexts(o.helpTexts),
  clickActions(o.clickActions), keyActions(o.keyActions),
  lastAction(o.lastAction)
{
  // We only register in the copy constructor, as all the baseline
  // handlers are built using a copy constructor !
  registerSelf();
}

EventHandler::~EventHandler()
{
}

void EventHandler::registerSelf()
{
  if(! registeredHandlers)
    registeredHandlers = new QHash<QString, EventHandler *>();
  if(registeredHandlers->contains(commandName))
    throw InternalError("Two handlers for the same command: %1").
      arg(commandName);

  (*registeredHandlers)[commandName] = this;
}

EventHandler * EventHandler::handlerForCommand(const QString & cmd)
{
  if(! registeredHandlers)
    return NULL;
  return registeredHandlers->value(cmd, NULL);
}

QString EventHandler::keyString(int key)
{
  // Gets it wrong for the upper-case/lowercase stuff
  if(key > 64 && key < 128)
    return QChar(key);
  QKeySequence seq(key);
  return seq.toString();
}

QString EventHandler::clickString(Qt::MouseButton button)
{
  switch(button) {
  case Qt::LeftButton:
    return "left";
  case Qt::RightButton:
    return "right";
  case Qt::MidButton:
    return "mid";
  default:
    return "?";
  }
  return QString();
}

EventHandler & EventHandler::addPointPicker()
{
  PointPicker::addToHandler(*this);
  return *this;
}

int EventHandler::normalizeKey(int key)
{
  if( (key&255) > 96 && (key&255) <= 96+26 
     && (key & (Qt::CTRL|Qt::ALT|Qt::META)))
     key -= 32;                 // Use uppercase !
  return key;
}

EventHandler & EventHandler::addKey(int key, int action, const QString & help)
{
  key = normalizeKey(key);
  if(keyActions.contains(key))
    throw InternalError("Multiply defined key: %1").arg(key);
  keyActions[key] = action;
  if(! helpTexts.contains(action))
    helpTexts[action] = help;
  lastAction = action;
  return *this;
}

EventHandler & EventHandler::alsoKey(int key)
{
  key = normalizeKey(key);
  if(keyActions.contains(key))
    throw InternalError("Multiply defined key: %1").arg(key);
  keyActions[key] = lastAction;
  return *this;
}

EventHandler & EventHandler::addClick(Qt::MouseButton button, 
                                      int action, const QString & help)
{
  if(clickActions.contains(button))
    throw InternalError("Multiply defined click: %1").arg(button);
  clickActions[button] = action;
  if(! helpTexts.contains(action))
    helpTexts[action] = help;
  lastAction = action;
  return *this;
}

EventHandler & EventHandler::alsoClick(Qt::MouseButton button)
{
  clickActions[button] = lastAction;
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

QString EventHandler::buildHelpString(bool /*useHTML*/) const
{
  QList<int> actions = helpTexts.keys();
  qSort(actions);

  QString text;

  for(int i = 0; i < actions.size(); i++) {
    // First get all the actions corresponding to 
    int action = actions[i];
    QStringList shortcuts;
    
    for(QHash<Qt::MouseButton, int>::const_iterator it = clickActions.begin();
        it != clickActions.end(); it++)
      if(it.value() == action)
        shortcuts << clickString(it.key());

    for(QHash<int, int>::const_iterator it = keyActions.begin();
        it != keyActions.end(); it++)
      if(it.value() == action)
        shortcuts << keyString(it.key());
    
    text.append(QString("%1: %2\n").arg(shortcuts.join(", ")).
                arg(helpTexts[action]));
  }
  return text;
  
}

QString EventHandler::buildSpec() const
{
  QList<int> actions = helpTexts.keys();
  qSort(actions);

  QStringList sc;

  for(int i = 0; i < actions.size(); i++) {
    // First get all the actions corresponding to 
    int action = actions[i];
    QStringList shortcuts;
    
    for(QHash<Qt::MouseButton, int>::const_iterator it = clickActions.begin();
        it != clickActions.end(); it++)
      if(it.value() == action)
        shortcuts << clickString(it.key());

    for(QHash<int, int>::const_iterator it = keyActions.begin();
        it != keyActions.end(); it++)
      if(it.value() == action)
        shortcuts << keyString(it.key());
    
    sc.append(QString("(%1: %2)").
                arg(shortcuts.join(", ")).
                arg(action));
  }
  return sc.join(" ");
  
}

EventHandler & EventHandler::conventionalAccept(int action, 
                                                const QString & help)
{
  addClick(Qt::MidButton, action, help);
  addKey('q', action);
  return addKey('Q', action);
}

EventHandler & EventHandler::baselineHandler(BaselineHandler::Options opts)
{
  return BaselineHandler::addToEventHandler(*this, opts);
}
