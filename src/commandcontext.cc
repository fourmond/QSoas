/*
  command.cc: implementation of the Command class
  Copyright 2018 by CNRS/AMU

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
#include <commandcontext.hh>
#include <command.hh>
#include <group.hh>

QHash<QString, CommandContext*> * CommandContext::availableContexts = NULL;

CommandContext * CommandContext::namedContext(const QString & name,
                                              const QString & prefix,
                                              const QString & rubyClass)
{
  if(! availableContexts)
    availableContexts = new QHash<QString, CommandContext*>;
  if(! availableContexts->contains(name))
    (*availableContexts)[name] = new CommandContext(prefix, rubyClass);
  return (*availableContexts)[name];
}

CommandContext * CommandContext::globalContext()
{
  return namedContext("global", "", "QSoas");
}

CommandContext * CommandContext::fitContext()
{
  return namedContext("fit", "f", "QSoasFit");
}


CommandContext::CommandContext(const QString & p,
                               const QString & cls) :
  prefix(p), rubyClass(cls)
{
}


Command * CommandContext::namedCommand(const QString & cmd,
                                       bool convert) const
{
  QString cnv = cmd;
  QString prf = prefix;
  if(! prf.isEmpty()) {
    prf += "+";
    if(cnv.startsWith(prf))
      cnv = cnv.mid(prf.size());
  }
  if(convert)
    cnv.replace('_', '-');
  return commands.value(cnv, NULL);
}



void CommandContext::registerCommand(Command * cmd)
{

  if(commands.contains(cmd->commandName()))
    throw InternalError(QObject::tr("Duplicate command name : %1").
                        arg(cmd->commandName()));

  commands[cmd->commandName()] = cmd;

  if(cmd->shortCommandName().isEmpty())
    return;
  if(commands.contains(cmd->shortCommandName()))
    throw InternalError(QObject::tr("Duplicate short command name : %1").
                        arg(cmd->shortCommandName()));
  commands[cmd->shortCommandName()] = cmd;
}

void CommandContext::unregisterCommand(Command * cmd)
{
  commands.remove(cmd->commandName());
  if(! cmd->shortCommandName().isEmpty())
    commands.remove(cmd->shortCommandName());

  // Is that all ?
}



QStringList CommandContext::commandNames() const
{
  return commands.keys();
}

QStringList CommandContext::interactiveCommands() const
{
  QStringList ret;
  
  for(QHash<QString, Command*>::const_iterator i = commands.begin();
      i != commands.end(); ++i)
    if(i.value()->isInteractive())
      ret << i.value()->cmdName;
  return ret;
}

QStringList CommandContext::nonInteractiveCommands() const
{
  QStringList ret;
  
  for(QHash<QString, Command*>::const_iterator i = commands.begin();
      i != commands.end(); ++i)
    if(! i.value()->isInteractive())
      ret << i.value()->cmdName;
  return ret;
}


QSet<Command *> CommandContext::availableCommands() const
{
  return QSet<Command * >::fromList(commands.values());
}


bool CommandContext::finishedLoading = false;

void CommandContext::crosslinkCommands()
{
  for(QHash<QString, Command *>::iterator i = commands.begin();
      i != commands.end(); ++i) {
    if(! i.value()->group) {
      Group * grp = Group::namedGroup(i.value()->groupName);
      if(! grp) {
        QTextStream o(stdout);
        o << "Missing group: " << i.value()->groupName << endl;
      }
      else {
        i.value()->group = grp;
        grp->commands.append(i.value());
      }
    }
  }
}

void CommandContext::crosslinkAllCommands()
{
  if(! availableContexts)
    return;
  for(CommandContext * cxt : availableContexts->values())
    cxt->crosslinkCommands();
  CommandContext::finishedLoading = true;
}

void CommandContext::writeSpecFile(QTextStream & out, bool full)
{
  QList<Command *> lst = availableCommands().toList();
  qSort(lst.begin(), lst.end(), ::cmpCommands);

  for(int i = 0; i < lst.size(); i++)
    out << lst[i]->commandSpec(full);

}

QStringList CommandContext::loadDocumentation(const QString & str)
{
  // QRegExp re("\\{::comment\\} description-(start|end):\\s*([0-9a-z-]+)\\s*\\{:/\\}\\s*");

  // QHash<QString, Command *> cmds = *availableCommands;

  // int idx = 0;
  // int nx = 0;

  // int beg = -1;
  // QString cur;
  // while(nx = re.indexIn(str, idx), nx >= 0) {
  //   if(re.cap(1) == "start") {
  //     beg = nx + re.matchedLength();
  //     cur = re.cap(2);
  //   } else {
  //     if(beg >= 0 && cmds.contains(cur)) {
  //       cmds[cur]->longDesc = str.mid(beg, nx - beg);
  //       cmds.remove(cmds[cur]->shortCommandName());
  //       cmds.remove(cur);
  //     }
  //   }
  //   idx = nx + re.matchedLength();
  // }
  // return cmds.keys();
}
