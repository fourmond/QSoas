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
#include <soas.hh>

CommandPrompt::CommandPrompt() : nbSuccessiveTabs(0), historyItem(-1)
{
}

CommandPrompt::~CommandPrompt()
{
}

void CommandPrompt::replaceWord(const CompletionContext & w, const QString & str, 
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

void CommandPrompt::doCompletion()
{
  nbSuccessiveTabs++;
  CompletionContext c = getCompletionContext();
  QString reason = tr("No completion");
  bool complete = true;
  completions = getCompletions(c, &reason, &complete);
  if(completions.size() == 0) {
    soas().showMessage(reason);
    return;
  }
  if(completions.size() == 1) {
    nbSuccessiveTabs = 0;
    replaceWord(c, Command::quoteString(completions.first()), 
                complete);
  }
  else {
    QString common = Utils::commonBeginning(completions);
    if(common == c.word) {
      QString str = tr("%1 completions: %2").
        arg(completions.size()).arg(completions.join(", "));
      soas().showMessage(str);
    }
    else {
      replaceWord(c, Command::quoteString(common), false);
    }
  }

}

void CommandPrompt::keyPressEvent(QKeyEvent * event)
{
  if(event->key() == Qt::Key_Tab) {
    event->accept();
    doCompletion();
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

CommandPrompt::CompletionContext CommandPrompt::getCompletionContext() const
{
  QList<int> beg, end, ann;
  QString str = text();
  QStringList splitted = Command::wordSplit(str, &beg, &end);
  bool pendingOption = false;
  QPair<QStringList, QHash<QString, QString> > p = 
    Command::splitArgumentsAndOptions(splitted.mid(1), &ann, &pendingOption);
  int i = 0;
  for(; i < end.size(); i++)
    if(end[i] >= cursorPosition())
      break;
  return CompletionContext(splitted.value(i), 
                           beg.value(i, str.size()),
                           end.value(i, str.size()), 
                           beg.value(i+1, str.size()),
                           i, p.first.size(), pendingOption,
                           splitted, ann);
}


QStringList CommandPrompt::getCompletions(const CompletionContext & c, 
                                          QString * reason, 
                                          bool * complete) const
{
  *complete = true;
  if(c.index == 0)
    return Utils::stringsStartingWith(Command::allCommands(), c.word);
  Command * cmd = Command::namedCommand(c.allWords[0]);
  if(! cmd) {
    *reason = tr("Unkown command: %1").arg(c.allWords[0]);
    return QStringList();
  }
  // Here, be more clever.
  
  int argid = c.annotations.value(c.index - 1, c.argumentNumber);
  Argument * arg = NULL;
  if(argid < 0 || c.word == "/" || c.unfinishedOption) {
    if(! cmd->commandOptions()) {
      *reason = tr("Command %1 takes no options").
        arg(c.allWords[0]);
      return QStringList();
    }

    // Big question: do we complete on the option name or the option
    // value ?
    QRegExp optionRE("^/([a-zA-Z-]*)\\s*(=\\s*(.*))?");
    if(optionRE.indexIn(c.word) >= 0) {
      if(optionRE.cap(2).isEmpty()) {
        *complete = false;
        // We are completing on option name
        QStringList values = 
          Utils::stringsStartingWith(cmd->commandOptions()->argumentNames(),
                                     optionRE.cap(1));
        for(int i = 0; i < values.size(); i++)
          values[i] = QString("/%1=").arg(values[i]);
        return values;
      }
      else {
        // We complete on option value.
        QString optionName = optionRE.cap(1); // 
        arg = cmd->commandOptions()->namedArgument(optionName);
        QStringList values;
        if(!arg) {
          *reason = tr("No such option: %1").arg(optionName);
          return values;
        }
        values = arg->proposeCompletion(optionRE.cap(3));
        for(int i = 0; i < values.size(); i++) {
          values[i] = QString("/%1=%2").
            arg(optionName).arg(values[i]);
        }
        return values;
      }
    }
    else {
      // We need to look at the arguments before
      if(optionRE.indexIn(c.allWords[c.index-1]) < 0)
        optionRE.indexIn(c.allWords[c.index-2]);
      QString optionName = optionRE.cap(1); // 
      arg = cmd->commandOptions()->namedArgument(optionName);
      if(!arg)
        *reason = tr("No such option: %1").arg(optionName);
    }
  }
  else {
    if(! cmd->commandArguments())
      *reason = tr("Command %1 takes no arguments").
        arg(c.allWords[0]);
    else 
      arg = cmd->commandArguments()->
        whichArgument(argid, c.argumentNumber);
  }
  
  if(! arg)
    return QStringList();
  return arg->proposeCompletion(c.word);
}
