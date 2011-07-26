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

void Command::runCommand(const QString & commandName, 
                         const QStringList & arguments,
                         QWidget * base)
{
  // First, arguments conversion
  CommandArguments args;
  CommandOptions options;
  // if(effector->arguments) {
  // }
  
  effector->runCommand(commandName, args, options);

  // Cleanup after conversion.
  //
  /// @todo This is not exception-safe. To do that, I should simply
  /// provide a thin wrapper around a QList<ArgumentMarshaller *> that
  /// takes object ownership, and can be converted to
  /// QList<ArgumentMarshaller *>. A template class would even be
  /// great !
  for(int i = 0; i < args.size(); i++)
    delete args[i];
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
    /// @todo throw exception !
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
