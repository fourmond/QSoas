/**
   \file checkablewidget.hh
   A checkable widget that embeds 
   Copyright 2013 by CNRS/AMU

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
#ifndef __CHECKABLEWIDGET_HH
#define __CHECKABLEWIDGET_HH

/// This widgets embeds another widget while giving it the possibility
/// to be checked.
///
/// @todo Add the possibility to 
class CheckableWidget : public QFrame {
  Q_OBJECT;

  /// The underlying widget
  QWidget * widget;

  /// A check box
  QCheckBox * checkBox;

  /// If this is not NULL, then the check state of the widget is taken
  /// from the presence of the number @a index in the given set.
  QSet<int> * present;

  /// The index for the @a present member.
  int index;
public:
  explicit CheckableWidget(QWidget * sub,
                           QWidget * parent = NULL);

  /// when this is used, then the QSet and the check state of the
  /// widget are bound together.
  void useSet(QSet<int> * tgt, int index);

  /// Whether the widget is checked or not.
  bool isChecked() const;

  /// Returns the subwidget to play with. Templated for type-safety.
  template <class T> T * subWidget() {
    return dynamic_cast<T*>(widget);
  };
protected slots:

  /// Called when the check state has changed
  void cbStateChanged(int state);

signals:

  void stateChanged(int state);
  
};

#endif
