/**
   \file credits.hh
   Code to keep track of dependencies and credits
   Copyright 2013 by Vincent Fourmond

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
#ifndef __CREDITS_HH
#define __CREDITS_HH

/// This class helps to keep track of who should be thanked for what
class Credits {
protected:
  void registerSelf();
public:

  static QList<Credits *> * currentCredits;

  /// Name of the credits
  QString name;

  /// Authors
  QStringList authors;

  /// Urls associated with the credits.
  QStringList urls;

  /// A free text notice
  QString notice;


  Credits(const QString & n, const QString & a, 
          const QString & u, const QString & d);

  Credits(const QString & n, const QStringList & a, 
          const QStringList & u, 
          const QString & d);

  /// Display the credits in the terminal
  static void displayCredits();

};

#endif
