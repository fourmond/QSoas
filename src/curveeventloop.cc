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
#include <commandwidget.hh>
#include <lineedit.hh>
#include <dataset.hh>

#include <terminal.hh>
#include <utils.hh>


CurveEventLoop::CurveEventLoop(CurveView * v) : 
  lastEventType(QEvent::None),
  k(0), done(false), prompt(NULL),
  printingAllowed(true)
{
  if(! v)
    v = &(soas().view());
  view = v;
  view->enterLoop(this);

  // Then, we switch the command widget to loop mode
  soas().prompt().setLoopMode(true);
  setHelpString(tr("To escape, \n hit ESC"));

  connect(qApp, SIGNAL(lastWindowClosed()), 
          SLOT(onLastWindowClosed()));
}

void CurveEventLoop::setHelpString(const QString & str)
{
  soas().prompt().setSideBarLabel(str);
}

CurveEventLoop::~CurveEventLoop()
{
  soas().prompt().setLoopMode(false);
  view->leaveLoop();

  /// @todo Send back all pending events ?

  view->viewport()->repaint();
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
  pos = event->pos();
  bt = event->button();
  updateMessage();
}

void CurveEventLoop::updateMessage()
{
  QPointF p = position();
  soas().showMessage(QString("X: %1, Y: %2 %3 %4").
                     arg(p.x()).arg(p.y()).
                     arg(ppMessage).
                     arg(message));
}

void CurveEventLoop::processKeyEvent(QKeyEvent * event)
{
  k = event->key() | event->modifiers();

  if(printingAllowed && k == Qt::Key_Print) {
    QPrinter p;
    p.setOrientation(QPrinter::Landscape);
    QString file;
    int i = 0;
    while(true) {
      file = QString("print-%1.pdf").arg(i);
      if(! QFile::exists(file))
        break;
      i++;
    }
    p.setOutputFileName(file);
    QPainter painter;
    Terminal::out << "Saving view as PDF file: " << file << endl;
    painter.begin(&p);
    view->render(&painter, 500,
                 p.pageRect());
  }

  /// Convert most usual keys to their ASCII value
  if(event->text().size() == 1) {
    int newk = event->text().toLocal8Bit()[0];
    if(newk >= 32 && newk != 0x7F)
      k = newk;
  }
}

bool CurveEventLoop::finished()
{
  if(done)
    return true;

  view->viewport()->repaint();
  while(pendingEvents.size() == 0) {
    QCoreApplication::processEvents(QEventLoop::AllEvents|QEventLoop::WaitForMoreEvents, 10);
    Utils::msleep(10);
  }

  QInputEvent * ie = pendingEvents.takeFirst();
  processInputEvent(ie);
  delete ie;
  return done;
}

bool CurveEventLoop::isConventionalAccept() const
{
  if(type() == QEvent::MouseButtonPress && button() == Qt::MiddleButton)
    return true;

  if(type() == QEvent::KeyPress && (key() == 'q' || key() == 'Q'))
    return true;
  return false;
}


QPointF CurveEventLoop::position(CurvePanel * panel)
{
  if(! panel)
    panel = &view->panel;        // Else, it can segfault.
  return panel->fromWidget(pos);
}

bool CurveEventLoop::eventFilter(QObject *, QEvent * event)
{
  /// This function is only called during the event loop

  switch(event->type()) {
  case QEvent::MouseButtonPress:
  case QEvent::MouseButtonRelease:
  case QEvent::MouseMove:
    receiveMouseEvent(static_cast<QMouseEvent*>(event));
    return true;
  case QEvent::KeyPress:
    {
      QKeyEvent * ev = static_cast<QKeyEvent*>(event);
      if(prompt) {
        switch(ev->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
          promptOK = true;
          prompt->recordHistory(); // has to be done manually...
        case Qt::Key_Escape:
          inputText = prompt->text();
          prompt = NULL;
          return true;
        default: 
          return false;
        }
      }
      else {
        receiveKeyEvent(ev);
        return true;
      }
    }
  default:
    return false;               // Propagate other events
  }
  return false;
}

QString CurveEventLoop::promptForString(const QString & pr, bool * ok,
                                        const QString & init)
{
  prompt = soas().prompt().enterPromptMode(pr, init);
  promptOK = false;

  while(prompt)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

  soas().prompt().leavePromptMode();
  if(ok)
    *ok = promptOK;
  return (promptOK ? inputText : QString());
}

QPair<double, int> CurveEventLoop::distanceToDataSet(const DataSet * ds, 
                                                     const QPointF & scale)
{
  /// @todo Be carefull when using several datasets ?
  QPointF scales = view->panel.scaleFactors();
  if(! scale.isNull()) {
    scales.setX(scales.x() * scale.x());
    scales.setY(scales.y() * scale.y());
  }
  return ds->distanceTo(position(), scales.x(), scales.y());
}

CurvePanel * CurveEventLoop::currentPanel()
{
  return view->panelAt(pos);
}

void CurveEventLoop::onLastWindowClosed()
{
  done = true;
}
