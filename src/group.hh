/**
   \file group.hh
   Command group handling for QSoas.
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
#ifndef __GROUP_HH
#define __GROUP_HH

class Command;
class CommandContext;

/// A group. In principle, there shouldn't be need for derived
/// classes, but, well.
class Group {
protected:

  QString grpName;

  const char * pubName;

  const char * shortDesc;
  
  /// A global hash holding a correspondance name->group
  static QHash<QString, Group*> * availableGroups;

  /// Registers the given group to the static registry
  static void registerGroup(Group * grp);

  QList<Group *> subGroups;

  /// The parent group (for submenu classification)
  Group * parent;

public:

  /// Group are sorted by priority in the status bar.
  int priority;

  /// Specifies the various elements linked to the Group.
  ///
  /// \warning Group doesn't take ownership of the three last
  /// strings, which should therefore point to locations that will not
  /// move, ideally constant strings.
  Group(const char * cn, int p, const char * pn,
        const char * sd, 
        Group * prt = NULL, bool autoRegister = true) : 
    grpName(cn), pubName(pn), 
    shortDesc(sd), parent(prt), priority(p) {
    if(autoRegister)
      registerGroup(this);
    if(parent) {
      parent->subGroups.append(this);
    }
  }; 
  

  /// The group name, for internal use only.
  ///
  /// This name will not be translated.
  ///
  /// \warning If you reimplement this function, you should set the
  /// the autoRegister parameter to false and do the registration
  /// yourself.
  virtual QString groupName() const {
    return grpName;
  };


  /// The public name, the one to be used in the menus. This one gets
  /// translated, which means that one should use QT_TRANSLATE_NOOP
  /// macro for setting it.
  virtual QString publicName() const {
    return QObject::tr(pubName);
  };

  /// A short description, typically to be used for the status bar.
  virtual QString shortDescription() const {
    return QObject::tr(shortDesc);
  };


  /// The commands that belong to this group.
  QList<Command*> commands;

  /// Returns the named group.
  static Group * namedGroup(const QString & grp);

  /// Makes up an action for the group, ie, popping up a submenu
  QAction * actionForGroup(QObject * parent,
                           const CommandContext * context) const;

  /// Fills a menu with all the groups informations for the given
  /// CommandContext
  static void fillMenuBar(QMenuBar * menu,
                          const CommandContext * context);

};

#endif
