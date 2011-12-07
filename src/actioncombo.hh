/**
   \file actioncombo.hh
   The Actioncombo class, providing a combo box triggering actions.
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

#ifndef __ACTIONCOMBO_HH
#define __ACTIONCOMBO_HH

/// A combo box that triggers an action whenever an element is clicked
/// (saved the first one)
class ActionCombo : public QComboBox {

  Q_OBJECT;

  /// The actions that get triggered upon selecting the elements.
  QList<QAction *> actions;
public:
  
  ActionCombo(const QString & title);

  /// Adds an action with the given name, connected to the \a slot of
  /// the \a receiver.
  void addAction(const QString & name, QObject * receiver, 
                 const char * slot, 
                 const QKeySequence & shortCut = QKeySequence());

  /// A helper function that creates actions. Used by ActionCombo, but
  /// can be used by others too...
  static QAction * createAction(const QString & name, 
                                QObject * receiver, 
                                const char * slot, 
                                const QKeySequence & shortCut = 
                                QKeySequence(),
                                QObject * parent = NULL);

protected slots:

  void elementSelected(int idx);
};

#endif
