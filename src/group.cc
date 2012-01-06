/*
  group.cc: implementation of the Group class
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
#include <group.hh>
#include <command.hh>


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

QAction * Group::actionForGroup(QObject * parent) const
{
  QList<Command *> cmds = commands;
  qSort(cmds.begin(), cmds.end(), compareCommands);
  QAction * action = new QAction(parent);
  action->setText(publicName());
  action->setStatusTip(shortDescription());
  action->setToolTip(shortDescription()); // probably useless.
  QMenu * menu = new QMenu();
  for(int i = 0; i < cmds.size(); i++)
    menu->addAction(cmds[i]->actionForCommand(parent));

  action->setMenu(menu);
  return action;
}

static bool compareGroups(const Group * a, const Group * b)
{
  if(a->priority != b->priority)
    return a->priority < b->priority;
  return a->groupName() < b->groupName();
}

void Group::fillMenuBar(QMenuBar * menu)
{
  if(! availableGroups)
    return;
  QList<Group *> groups = availableGroups->values();
  qSort(groups.begin(), groups.end(), compareGroups);
  for(int i = 0; i < groups.size(); i++) {
    QAction * action = groups[i]->actionForGroup(menu->parent());
    QMenu * sub = action->menu();
    menu->addAction(action);
#ifdef Q_WS_MACX
    // This is a workaround for bug https://bugreports.qt.nokia.com/browse/QTBUG-19920
    menu->connect(sub, SIGNAL(triggered(QAction*)), 
                  SIGNAL(triggered(QAction*)));
#endif

  }
}
