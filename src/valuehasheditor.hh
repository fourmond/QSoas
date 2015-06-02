/**
   \file valuehasheditor.hh
   A widget to edit ValueHash
   Copyright 2015 by CNRS/AMU

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
#ifndef __VALUEHASHEDITOR_HH
#define __VALUEHASHEDITOR_HH

#include <valuehash.hh>

class LabeledEdit;
class FlowingGridLayout;

class ValueHashEditor : public QWidget {
  // QVBoxLayout * vLayout;
  FlowingGridLayout * layout;

  QHash<QString, LabeledEdit *>  editors;

  /// Returns the given editor for the key, creating it if necessary.
  LabeledEdit * editorForKey(const QString & key);

  /// 
public:
  ValueHashEditor(QWidget * parent = 0);

public slots:
  /// Sets the values of the editors from the given hash.
  void setFromHash(const ValueHash & src);

  /// Sets the values currently in the editor to the target hash. Does
  /// not touch values in the target hash that have no editor.
  void updateHash(ValueHash * tgt) const;
  
};


#endif
