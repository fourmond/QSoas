/*
  command.cc: implementation of the Command class
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
#include <command.hh>
#include <group.hh>
#include <argument.hh>
#include <commandeffector.hh>

#include <possessive-containers.hh>


QHash<QString, Command*> * Command::availableCommands = NULL;

void Command::registerCommand(Command * cmd)
{
  if(! availableCommands)
    availableCommands = new QHash<QString, Command*>;

  if(availableCommands->contains(cmd->commandName())) {
    QString str = "Duplicate command name : " + cmd->commandName();
    throw std::logic_error(str.toStdString());
  }
  (*availableCommands)[cmd->commandName()] = cmd;

  if(cmd->shortCommandName().isEmpty())
    return;
  if(availableCommands->contains(cmd->shortCommandName())) {
    QString str = "Duplicate short command name : " + cmd->shortCommandName();
    throw std::logic_error(str.toStdString());
  }
  (*availableCommands)[cmd->shortCommandName()] = cmd;
}

void Command::crosslinkCommands()
{
  if(! availableCommands)
    return;
  for(QHash<QString, Command *>::iterator i = availableCommands->begin();
      i != availableCommands->end(); i++) {
    if(! i.value()->group) {
      Group * grp = Group::namedGroup(i.value()->groupName);
      i.value()->group = grp;
      grp->commands.append(i.value());
    }
  }
}

/// @todo I think this whole function should move to ArgumentList,
/// probably with much better checking of argument size and slurping
/// arguments.
CommandArguments Command::parseArguments(const QStringList & args,
                                         QWidget * base) const
{
  CommandArguments ret;  
  if(! arguments)
    return ret;
  int size = arguments->size();
  for(int i = 0; i < size; i++) {
    /// @todo I should make provisions one day for slurping arguments,
    /// ie arguments that take more than one word (and that simply add
    /// the results to their first argument). Of course, there may
    /// only be one slurping argument, but its position should not
    /// have to be the last one. This will come in useful for loading
    /// files...
    ///
    /// @todo This functionality should move to ArgumentList
    if(args.size() > i)
      ret.append(arguments->value(i)->fromString(args[i]));
    else
      ret.append(arguments->value(i)->promptForValue(base));
  }
  return ret;
}

CommandOptions Command::parseOptions(const QHash<QString, QString> & opts) const
{
  CommandOptions ret;  
  if(! options)
    return ret;
  for(QHash<QString, QString>::const_iterator i = opts.begin();
      i != opts.end(); i++) {
    Argument * opt = options->namedArgument(i.key());
    if(! opt) {
      QString str = QObject::tr("Unknown option '%1' for command %2").
        arg(i.key()).arg(commandName());
      throw std::runtime_error(str.toStdString());
    }
    ret[i.key()] = opt->fromString(i.value());
  }
  return ret;
}

void Command::runCommand(const QString & commandName, 
                         const QStringList & arguments,
                         QWidget * base)
{
  // First, arguments conversion
  QPair<QStringList, QHash<QString, QString> > split = 
    splitArgumentsAndOptions(arguments);

  PossessiveList<ArgumentMarshaller> args = 
    parseArguments(split.first, base);
  PossessiveHash<QString, ArgumentMarshaller> options = 
    parseOptions(split.second);
  // Then the call !
  effector->runCommand(commandName, args, options);
}

Command * Command::namedCommand(const QString & cmd)
{
  if(! availableCommands)
    return NULL;
  return availableCommands->value(cmd, NULL);
}

void Command::runCommand(const QStringList & cmd,
                         QWidget * base)
{
  QStringList b = cmd;
  QString name = b.takeFirst();
  Command * command = namedCommand(name);
  if(! command) {
    QString str = QObject::tr("Unknown command: '%1'").
      arg(name);
    throw std::runtime_error(str.toStdString());
  }
  command->runCommand(name, b, base);
}

QAction * Command::actionForCommand(QObject * parent) const
{
  QAction * action = new QAction(parent);
  action->setText(publicName());
  action->setStatusTip(QString("%1: %2").
                       arg(commandName()).
                       arg(shortDescription()));
  action->setToolTip(shortDescription()); // probably useless.
  action->setData(QStringList() << commandName());
  return action;
}


QPair<QStringList, QHash<QString, QString> > 
Command::splitArgumentsAndOptions(const QStringList & rawArgs)
{
  QPair<QStringList, QHash<QString, QString> > ret;
  QStringList & args = ret.first;
  QHash<QString, QString> & opts = ret.second;
  int size = rawArgs.size();
  
  QRegExp optionRE("^\\s*/([a-zA-Z-]+)\\s*(?:=?\\s*|=\\s*(.*))$");
  QRegExp equalRE("^\\s*=\\s*(.*)$");

  for(int i = 0; i < size; i++) {
    if(rawArgs[i].startsWith("/!")) {
      args.append(rawArgs[i].mid(2));
      continue;
    }

    if(optionRE.indexIn(rawArgs[i]) == 0) {
      // we found an option
      QString optionName = optionRE.cap(1);
      if(! optionRE.cap(2).isEmpty()) {
        // The most simple case: an option in a single word
        opts[optionName] = optionRE.cap(2);
      }
      else {                    // Looking at next words
        i++;
        QString next = rawArgs.value(i, "");
        if(equalRE.indexIn(next) == 0) {
          if(! equalRE.cap(1).isEmpty())
            opts[optionName] = equalRE.cap(1);
          else {
            i++;
            next = rawArgs.value(i, "");
            opts[optionName] = next;
          }
        }
        else
          opts[optionName] = next;
      }
    }
    else
      args.append(rawArgs[i]);
  }

  return ret;
}

QStringList Command::wordSplit(const QString & str, QList<int> * wordBegin)
{
  int sw = -1;
  QChar quote = 0;
  int size = str.size();
  QString curString;
  QStringList retVal;

  if(wordBegin)
    wordBegin->clear();

  for(int cur = 0; cur < size; cur++) {
    QChar c = str[cur];
    if(quote != 0) {
      if(c != quote)
        curString.append(c);
      else
        quote = 0;
      continue;
    }
    if(c == ' ' || c == '\t') {
      // We flush the last word, if there is one
      if(sw >= 0) {
        retVal << curString;
        if(wordBegin)
          *wordBegin << sw;
        sw = -1;
        curString.clear();
      }
      continue;
    }
    if(c == '"' || c == '\'') {
      quote = c;
      // We start a word
      if(sw < 0)
        sw = cur;
      continue;
    }
    if(sw < 0)
      sw = cur;
    curString.append(c);
  }
  if(sw >= 0) {
    retVal << curString;
    if(wordBegin)
      *wordBegin << sw;
  }
  return retVal;
}
