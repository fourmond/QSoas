/*
  commandprompt.cc: Main window for QSoas
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
#include <commandprompt.hh>

CommandPrompt::CommandPrompt() : nbSuccessiveTabs(0), historyItem(-1)
{
}

CommandPrompt::~CommandPrompt()
{
}

void CommandPrompt::keyPressEvent(QKeyEvent * event)
{
  if(event->key() == Qt::Key_Tab) {
    event->accept();
    nbSuccessiveTabs++;
    QTextStream o(stdout);
    o << "Completion requested..." << endl;
  }
  else if(event->key() == Qt::Key_Up) {
    event->accept();
    if(historyItem < 0)
      savedLine = text();
    if(historyItem + 1 < savedHistory.size()) {
      historyItem++;
      setText(savedHistory[historyItem]);
    }
  }
  else if(event->key() == Qt::Key_Down) {
    event->accept();
    if(historyItem < 0)
      return;
    historyItem--;
    if(historyItem >= 0)
      setText(savedHistory[historyItem]);
    else
      setText(savedLine);
  }
  else {
    nbSuccessiveTabs = 0;
    if(event->key() == Qt::Key_Enter ||
       event->key() == Qt::Key_Return) {
      historyItem = -1;         // Reset history.
    }
    QLineEdit::keyPressEvent(event);
  }
}

void CommandPrompt::addHistoryItem(const QString & str)
{
  savedHistory.prepend(str);
}
