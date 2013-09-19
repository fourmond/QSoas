/**
   \file utils.hh
   Generally useful code
   Copyright 2011, 2012 by Vincent Fourmond

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
                   bool trim = true, bool isDir = false);


  /// @name String-related functiosn
  ///
  /// A series of functions dealing with strings
  ///
  /// @{

  /// Returns all the strings from the given QStringList that start
  /// with the given string.
  QStringList stringsStartingWith(const QStringList & strings, 
                                  const QString & start); 

  /// Returns the common part at the beginning of the given string
  QString commonBeginning(const QStringList & strings);


  /// Returns the string Delta something. Pityful, isn't it ;-) ?
  QString deltaStr(const QString & w);

  /// Abbreviates the string so that it fits within the given
  /// number. It should also sanitize the string to some extent.
  QString abbreviateString(const QString & str, int nb = 50);

  /// Shortens the string so that it has at most \a len characters,
  /// out of which @a last are from the end.
  QString shortenString(const QString & str, int len = 50, int last = 10);

  /// Splits a list of strings into several sublist of strings,
  /// separated by stings that match the given regular expression. Can
  /// be nicely combined with the parseConfigurationFile() function.
  QList<QStringList> splitOn(const QStringList & lines,
                             const QRegExp & re);

  /// Updates a string so that anything between \a begin and \a end
  /// are replaced with \a newText. Returns true if any replacement
  /// was performed.
  bool updateWithin(QString & str, const QString & begin, 
                    const QString & end, const QString & newText, 
                    bool appendIfNotFound = true);


  /// A function to safely parse a double from a string -- or raise an
  /// exception
  double stringToDouble(const QString & str);

  /// A function to safely parse an int from a string -- or raise an
  /// exception
  int stringToInt(const QString & str);


  /// Extract all the strings matching the given regular expression
  /// while removing them from the original string.
  ///
  /// The returned list is the list of the \a group-th capturing
  /// group. If -1 is specified, then all capturing groups are
  /// returned.
  QStringList extractMatches(QString & str, const QRegExp & re, 
                             int group = 0);


  /// @}

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


  /// @name File-related functions
  ///
  /// A bunch of file-related functions
  ///
  /// @{

  /// Opens the given file with the target mode, raising a
  /// RuntimeError if that couldn't happen for some reason.
  ///
  /// Unless \a expandTilde is set to false, tilde are automatically
  /// expanded.
  void open(QFile * file, QIODevice::OpenMode mode, 
            bool expandTilde = true);

  /// Asks confirmation before overwriting the target file if it exists.
  ///
  /// Throws an exception if user cancelled unless \a silent is true.
  bool confirmOverwrite(const QString & fileName, bool silent = false);

  /// Expands a leading ~ into the home directory of the user.
  QString expandTilde(const QString & name);

  /// Reads a text line from the next file, treating any of the
  /// standard newline delimiters as such.
  ///
  /// Returns the line, terminated by a newline character (even if
  /// there wasn't one in the first place).
  QString readTextLine(QIODevice * device);

  /// @}


  /// @name GSL-related functions
  ///
  /// Various bunch of useful GSL-related functions.
  ///
  /// @{
  
  /// Converts the given matrix to a string
  QString matrixString(const gsl_matrix * matrix);
  QString matrixString(const gsl_matrix_complex * matrix);

  QString vectorString(const gsl_vector * vect);
  QString vectorString(const gsl_vector_complex * vect);

  /// Inverts the given matrix using SV decomposition.
  /// The original matrix is destroyed in the process.
  ///
  /// If threshold is a positive value, any singular value smaller
  /// than threshold times the large one is considered to have that
  /// value. 
  void invertMatrix(gsl_matrix  * mat, gsl_matrix * target,
                    double threshold = -1);

  /// @}

  /// Rounds \a value as if it had only \a ranks digits before the
  /// decimal point.
  double roundValue(double value, int ranks = 1);


  /// Connects the given shortcut to the given receiver. If parent is
  /// NULL, it is taken equal to the receiver.
  void registerShortCut(const QKeySequence & seq, QObject * receiver, 
                        const char * fn, QWidget * parent = NULL,
                        Qt::ShortcutContext context = Qt::WindowShortcut);


  /// Makes the items in the list unique while keeping the order
  template <typename T> void makeUnique(QList<T> & list) {
    QSet<T> elems;
    for(int i = 0; i < list.size(); i++) {
      const T & t = list[i];
      if(elems.contains(t)) {
          list.takeAt(i);
          --i;
      }
      else
        elems.insert(t);
    }
  };

  /// This function parses a configuration file, stripping out
  /// comments and concatenating lines when applicable, ie for lines
  /// finishing with "\"
  ///
  /// If \a keepCR is on, the final \n isn't stripped from escaped
  /// lines. If \a comments isn't NULL, comments are stored there. If
  /// \a lineNumbers isn't NULL, it is filled with the begin/end line
  /// numbers from the original file (for the corresponding line)
  QStringList parseConfigurationFile(QIODevice * source, 
                                     bool keepCR = false,
                                     QStringList * comments = NULL,
                                     QList< QPair<int, int> > * lineNumbers = NULL,
                                     bool stripBlank = false);




  /// This function returns a caller stack trace. It is not guaranteed
  /// to be accurate, or even to be of any use at all. But, it will
  /// probably work on linux and MacOS even.
  ///
  /// One string per stack frame
  QStringList backtrace(int maxframes = 20);

  /// Sleeps that many milliseconds
  void msleep(unsigned long msecs);

};

#endif
