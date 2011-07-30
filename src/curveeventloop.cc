/*
  curveeventloop.cc: implementation of the CurveEventLoop class
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
#include <curveeventloop.hh>
#include <curveview.hh>
#include <soas.hh>

CurveEventLoop::CurveEventLoop(CurveView * v) : 
  lastEventType(QEvent::None),
  k(0), done(false)
{
  if(! v)
    v = &(soas().view());
  view = v;
  view->enterLoop(this);
}

CurveEventLoop::~CurveEventLoop()
{
  view->leaveLoop();

  // Send back all pending events ?
}

void CurveEventLoop::receiveMouseEvent(QMouseEvent * event)
{
  pendingEvents << new QMouseEvent(*event);
}

void CurveEventLoop::receiveKeyEvent(QKeyEvent * event)
{
  pendingEvents << new QKeyEvent(*event);
}

void CurveEventLoop::processInputEvent(QInputEvent * ie)
{
  lastEventType = ie->type();
  mods = ie->modifiers();
  switch(ie->type()) {
  case QEvent::MouseButtonPress:
  case QEvent::MouseButtonRelease:
  case QEvent::MouseMove:
    processMouseEvent(static_cast<QMouseEvent*>(ie));
    break;
  case QEvent::KeyPress:
    processKeyEvent(static_cast<QKeyEvent*>(ie));
    break;
  default:
    // warn ?
    ;
  }
}

void CurveEventLoop::processMouseEvent(QMouseEvent * event)
{
}

void CurveEventLoop::processKeyEvent(QKeyEvent * event)
{
  k = event->key();
  if(k == Qt::Key_Escape)
    done = true;

  if(event->text().size() == 1)
    k = event->text().toLocal8Bit()[0];
}

bool CurveEventLoop::finished()
{
  if(done)
    return true;

  while(pendingEvents.size() == 0) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 0);
  }

  QInputEvent * ie = pendingEvents.takeFirst();
  processInputEvent(ie);
  delete ie;
  return done;
}
