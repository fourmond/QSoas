/**
   \file regex.hh
   A thin wrapper around Qt regular expressions
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
#ifndef __REGEX_HH
#define __REGEX_HH

/// This class provides a thin wrapper around Qt regular expressions
/// that make it easy to:
/// @li use common idioms
/// @li use plain text
/// @li use perl-like regular expressions
///
/// @question Maybe commonly used regex idioms should be included
/// here.
class Regex {

  /// Original pattern (ie, the argument of the constructor)
  QString originalPattern;
 
  /// The real pattern
  QString pattern;

  /// Default case sensitivity
  Qt::CaseSensitivity cs;
public:

  /// Constructs a
  Regex(const QString & pattern);


  /// Transforms a Regex pattern into a valid QRegExp pattern
  static QString parsePattern(const QString & pattern, 
                              Qt::CaseSensitivity * cs);

  /// Returns a fresh ready-to-use QRegExp object
  QRegExp toQRegExp() const;
};

#endif
