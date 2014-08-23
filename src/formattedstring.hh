/**
   \file formattedstring.hh
   String with formatting information
   Copyright 2012 by CNRS/AMU

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
#ifndef __FORMATTEDSTRING_HH
#define __FORMATTEDSTRING_HH

/// This class (and its children) represent various formatted string
/// that can be converted to various formats, namely:
/// 
/// @li plain text
/// @li HTML (possibly with the on-demand or once-for-all generation of
/// image files, for equations for instance ?) 
/// @li LaTeX
/// @li QTextBrowser internal format
///
/// This is done through a basic markup that will get documented here.
///
/// Or, maybe, I should use a functional approach to that ? (ie
/// FormattedString s + FormartedString::bold("stuff") + ... ?
///
/// In principle, no pointer to children should leak from this class.
class FormattedString : public QSharedData {

protected:

  /// Sub-nodes, for the top-level object and for nodes for which it
  /// matters.
  ///
  /// We use QExplicitlySharedDataPointer to avoid segfaults with copy
  /// constructors...
  QList<QExplicitlySharedDataPointer<FormattedString> > nodes;

  FormattedString();

  
  /// Parses a given string and constructs the corresponding node
  /// list.
  static QList<QExplicitlySharedDataPointer<FormattedString> > parseString(const QString & str);
public:

  virtual ~FormattedString();

  
  /// Constructs a formatted string with the given marked up string.
  FormattedString(const QString & str);

  /// Converts the string to plain text
  virtual QString toText() const;

  /// Converts the formatted string to HTML
  virtual QString toHTML() const;

  /// Converts the formatted string to LaTeX
  virtual QString toLaTeX() const;

  /// @todo Conversion to QTextBrowser internals

  /// @todo basic formatting functions, returning bold, plain, other
  /// texts... + operators (+, for instance)

};

#endif
