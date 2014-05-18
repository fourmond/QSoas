/*
  lineedit.cc: implementation of LineEdit
  Copyright 2011 by Vincent Fourmond
  Copyright 2014 by AMU/CNRS

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
#include <lineedit.hh>
#include <command.hh>
#include <utils.hh>
#include <argument.hh>
#include <argumentlist.hh>
#include <mainwin.hh>
#include <soas.hh>

#include <terminal.hh>

LineEdit::LineEdit() : historyItem(-1), autoSaveHistory(true)
{
  connect(this, SIGNAL(returnPressed()), SLOT(recordHistory()));
}

LineEdit::~LineEdit()
{
}


void LineEdit::keyPressEvent(QKeyEvent * event)
{

  switch(event->key()) {
  case Qt::Key_Up: 
    event->accept();
    if(historyItem < 0)
      savedLine = text();
    if(historyItem + 1 < savedHistory.size()) {
      historyItem++;
      setText(savedHistory[historyItem]);
    }
    break;
  case Qt::Key_Down:
    event->accept();
    if(historyItem < 0)
      break;
    historyItem--;
    if(historyItem >= 0)
      setText(savedHistory[historyItem]);
    else
      setText(savedLine);
    break;
  case Qt::Key_Enter:
  case Qt::Key_Return:
    historyItem = -1;         // Reset history.
    doHistoryExpansion();
  default:
    QLineEdit::keyPressEvent(event);
  }
}

void LineEdit::recordHistory()
{
  if(autoSaveHistory)
    addHistoryItem(text());
}

void LineEdit::addHistoryItem(const QString & str)
{
  savedHistory.prepend(str);
}

QStringList LineEdit::historyMatching(const QString & str) const
{
  return Utils::stringsStartingWith(savedHistory, str);
}

void LineEdit::doHistoryExpansion()
{
  QString curText = text();
  if(curText[0] == QChar('!')) {
    QStringList matching = historyMatching(curText.mid(1));
    if(matching.size() >= 1) 
      setText(matching.first());
  }
}
