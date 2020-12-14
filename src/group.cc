/*
  group.cc: implementation of the Group class
  Copyright 2011 by Vincent Fourmond
            2012 by CNRS/AMU

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
#include <group.hh>
#include <command.hh>

#include <commandlineparser.hh>

QHash<QString, Group*> * Group::availableGroups = NULL;

void Group::registerGroup(Group * grp)
{
  if(! availableGroups)
    availableGroups = new QHash<QString, Group*>;

  if(availableGroups->contains(grp->groupName()))
    throw InternalError(QObject::tr("Duplicate group name : %1").
                        arg(grp->groupName()));

  (*availableGroups)[grp->groupName()] = grp;
}

Group * Group::namedGroup(const QString & grp)
{
  if(! availableGroups)
    return NULL;
  return availableGroups->value(grp, NULL);
}

static bool compareCommands(const Command * a, const Command * b)
{
  return a->commandName() < b->commandName();
}

static bool compareGroups(const Group * a, const Group * b)
{
  if(a->priority != b->priority)
    return a->priority < b->priority;
  return a->groupName() < b->groupName();
}

QAction * Group::actionForGroup(QObject * parent, 
                                const CommandContext * context) const
{
  QList<Command *> cmds = commands;
  qSort(cmds.begin(), cmds.end(), compareCommands);
  QAction * action = new QAction(parent);
  action->setText(publicName());
  action->setStatusTip(shortDescription());
  action->setToolTip(shortDescription()); // probably useless.
  QMenu * menu = new QMenu(); // Leaks ? Isn't that a Qt bug ?

  QList<Group *> grps = subGroups;
  qSort(grps.begin(), grps.end(), compareGroups);


  for(int i = 0; i < grps.size(); i++) {
    QAction * ac = grps[i]->actionForGroup(parent, context);
    menu->addAction(ac);
#ifdef Q_OS_MAC
    QMenu * sub = ac->menu();
    // This is a workaround for bug https://bugreports.qt.nokia.com/browse/QTBUG-19920
    // Hmm, now https://bugreports.qt.io/browse/QTBUG-19920
    menu->connect(sub, SIGNAL(triggered(QAction*)), 
                  SIGNAL(triggered(QAction*)));
#endif

  }
  if(grps.size() > 0)
    menu->addSeparator();

  for(int i = 0; i < cmds.size(); i++) {
    if(cmds[i]->commandContext() == context) {
      QAction * act = cmds[i]->actionForCommand(parent);
      if(cmds[i]->commandName() == "quit")
        act->setMenuRole(QAction::QuitRole);
      else
        act->setMenuRole(QAction::NoRole);
      menu->addAction(act);
    }
  }

  action->setMenu(menu);
  return action;
}


void Group::fillMenuBar(QMenuBar * menu, const CommandContext * context)
{
  if(! availableGroups)
    return;
  QList<Group *> groups = availableGroups->values();
  qSort(groups.begin(), groups.end(), compareGroups);
  for(int i = 0; i < groups.size(); i++) {
    if(groups[i]->parent)
      continue;
    QAction * action = groups[i]->actionForGroup(menu->parent(), context);
    menu->addAction(action);
#ifdef Q_OS_MAC
    QMenu * sub = action->menu();
    // This is a workaround for bug https://bugreports.qt.nokia.com/browse/QTBUG-19920
    // Hmm, now https://bugreports.qt.io/browse/QTBUG-19920
    menu->connect(sub, SIGNAL(triggered(QAction*)), 
                  SIGNAL(triggered(QAction*)));
#endif

  }
}
