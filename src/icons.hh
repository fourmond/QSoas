/**
   \file icons.hh
   A central class for loading icons
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
#ifndef __ICONS_HH
#define __ICONS_HH

/// This class provides (static) functions to load icons.
class Icons {
public:

  /// Returns the icon for the given name.
  ///
  /// Loads from the theme, and if that fails, falls back to 
  static QIcon namedIcon(const QString & name);
};


#endif
