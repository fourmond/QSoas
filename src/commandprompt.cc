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
#include <command.hh>
#include <utils.hh>
#include <argument.hh>
#include <argumentlist.hh>
#include <mainwin.hh>

CommandPrompt::CommandPrompt() : nbSuccessiveTabs(0), historyItem(-1)
{
}

CommandPrompt::~CommandPrompt()
{
}

void CommandPrompt::replaceWord(const Word & w, const QString & str, 
                                bool full)
{
  QString t = text();
  QString r = str;
  if(full)
    r += " ";
  int end = (full ? w.next : w.end);
  t.replace(w.begin, end - w.begin, r);
  setText(t);
  setCursorPosition(w.begin + r.size());
}

void CommandPrompt::keyPressEvent(QKeyEvent * event)
{
  if(event->key() == Qt::Key_Tab) {
    event->accept();
    nbSuccessiveTabs++;
    Word w = getCurrentWord();
    QTextStream o(stdout);
    // o << "Completion for word #" << w.index 
    //   << ": '" << w.word << "' = '"
    //   << text().mid(w.begin, w.end - w.begin) << "'" << endl;
    completions = getCompletions(w);
    // o << "Completions :" << completions.join(", ") << endl;
    if(completions.size() == 1) {
      nbSuccessiveTabs = 0;
      replaceWord(w, Command::quoteString(completions.first()));
    }
    else {
      QString common = Utils::commonBeginning(completions);
      if(common == w.word) {
        QString str = QObject::tr("%1 completions: %2").
          arg(completions.size()).arg(completions.join(", "));
        MainWin::showMessage(str);
      }
      else {
        replaceWord(w, Command::quoteString(common), false);
      }
    }
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

CommandPrompt::Word CommandPrompt::getCurrentWord() const
{
  QList<int> beg, end;
  QString str = text();
  QStringList splitted = Command::wordSplit(str, &beg, &end);
  int i = 0;
  for(; i < end.size(); i++)
    if(end[i] >= cursorPosition())
      break;
  return Word(splitted.value(i), 
              beg.value(i, str.size()),
              end.value(i, str.size()), 
              beg.value(i+1, str.size()),
              i, splitted);
}


QStringList CommandPrompt::getCompletions(const Word & w) const
{
  if(w.index == 0)
    return Utils::stringsStartingWith(Command::allCommands(), w.word);
  Command * cmd = Command::namedCommand(w.allWords[0]);
  if(! cmd || ! cmd->commandArguments())
    return QStringList();       // Possibly even send a warning ?
  Argument * arg = cmd->commandArguments()->
    whichArgument(w.index-1, w.allWords.size());
  if(! arg)
    return QStringList();
  /// @todo This really doesn't handle options gracefully. But that
  /// one is going to be tough...
  return arg->proposeCompletion(w.word);
}
