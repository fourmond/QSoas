/**
   \file utils.hh
   Generally useful code
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


#ifndef __UTILS_HH
#define __UTILS_HH


/// Comes in useful for call to member functions
#define CALL_MEMBER_FN(object,ptrToMember) ((object).*(ptrToMember))

/// Various generally useful functions.
namespace Utils {

  /// Returns a list of file names matching the given glob.
  ///
  /// If \p trim is true, returns an empty list if no file matches,
  /// else returns the pattern.
  ///
  /// For now, it does not support recursive globs ("*/*", or even
  /// "**/*"). It may, one day...
  ///
  /// @todo Support selecting only files, hiding hidden files, and so
  /// on...
  ///
  /// @todo Support */ as a glob (not currently supported). That one
  /// may come in nicely as a side effect of the above transformation.
  QStringList glob(const QString & pattern, 
                   bool trim = true);

  /// Returns all the strings from the given QStringList that start
  /// with the given string.
  QStringList stringsStartingWith(const QStringList & strings, 
                                  const QString & start); 

  /// Returns the common part at the beginning of the given string
  QString commonBeginning(const QStringList & strings);

  /// Dumps the contents of a rectangle to the target stream
  template<typename T, typename Rect> void dumpRectangle(T & stream, 
                                                         Rect & r) {
    stream << r.x() << "," << r.y() << " to " 
           << r.x() + r.width() << "," << r.y() + r.height();
  };

  /// Applies the margins \m to a rectangle \r (ie remove them),
  /// and return the adjusted value.
  inline QRect applyMargins(const QRect & r, 
                            const QMargins & m) {
    return r.adjusted(m.left(), m.top(), -m.right(), -m.bottom());
  };

  /// Prompts the user for confirmation for something. For now,
  /// through the use of a dialog box, but that may change some time
  /// later.
  bool askConfirmation(const QString & what, 
                       const QString & title = QString());

  /// Returns a new version of the rectangle scaled around the given
  /// point by the given quantities. The idea is that \p point should
  /// stay at the same relative position.
  QRectF scaledAround(const QRectF & rect, const QPointF & point,
                      double xscale, double yscale);

  /// Returns the string Delta something. Pityful, isn't it ;-) ?
  QString deltaStr(const QString & w);
};

#endif
