/**
   \file widgets.hh
   A few general-purpose widgets
   Copyright 2014 by CNRS/AMU

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
#ifndef __WIDGETS_HH
#define __WIDGETS_HH

/// A line edit with a label !
class LabeledEdit : public QWidget {
  Q_OBJECT;

  /// The underlying label
  QLabel * _label;

  /// The line edit
  QLineEdit * _edit;
public:
  LabeledEdit(const QString & label, const QString & contents = "", 
              QWidget * parent = NULL);

  /// Changes the label
  void setLabelText(const QString & str);

  /// Changes the edit text
  void setText(const QString & txt);

  /// Gets the edit text
  QString text() const;

  /// Provides direct access to the label
  QLabel * label();

  /// Provides direct access to the line edit
  QLineEdit * lineEdit();
};

#endif
