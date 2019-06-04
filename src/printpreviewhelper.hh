/**
   \file printpreviewhelper.hh
   A small helper class for print preview
   Copyright 2019 by CNRS/AMU

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
#ifndef __PRINTPREVIEWHELPER_HH
#define __PRINTPREVIEWHELPER_HH

/// A helper class to draw print previews
class PrintPreviewHelper : public QObject {

  Q_OBJECT;

  /// The height of the object
  int height;

  /// The page rectangle
  QRect rect;

  /// The title
  QString title;
public:
  PrintPreviewHelper(int h, const QRect & r, const QString & t);

public slots:

  /// Paints the dialog
  void paint(QPaintDevice * target);

  void paintOnPrinter(QPrinter * target);
};

#endif
