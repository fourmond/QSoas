/**
   \file onetimewarnings.hh
   Facility to display series of warnings one
   Copyright 2017 by CNRS/AMU

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
#ifndef __ONETIMEWARNINGS_HH
#define __ONETIMEWARNINGS_HH

/// Displays warnings only once
class OneTimeWarnings {
  /// Whether the given warning was shown
  QHash<QString, bool> warningShown;

  QWidget * base;

public:

  OneTimeWarnings(QWidget * parent = NULL);


  

  /// Prompts for the named warning (code name), and returns true if
  /// the warning was already shown or if it was ignored, and false if
  /// Abort was clicked.
  bool warnOnce(QWidget * parent,
                const QString & codeName, const QString & title,
                const QString & warning);

  bool warnOnce(const QString & codeName, const QString & title,
                const QString & warning);
};

#endif
