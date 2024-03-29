/**
   \file utils.hh
   Generally useful code
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014 by CNRS/AMU

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



  /// @name String-related functions
  ///
  /// A series of functions dealing with strings
  ///
  /// @{

  /// Returns all the strings from the given QStringList that start
  /// with the given string.
  QStringList stringsStartingWith(const QStringList & strings, 
                                  const QString & start); 

  /// Returns the common part at the beginning of the strings
  QString commonBeginning(const QStringList & strings);

  /// Returns the common part at the end of the strings
  QString commonEnding(const QStringList & strings);

  /// Returns a string in which the changing parts of the each string
  /// are concatenated, and surrounded by the original common parts:
  ///
  ///    a_1_b, a_2_b... -> a_[bef]1[join]2[aft]_b
  QString smartConcatenate(const QStringList & strings,
                           const QString & join,
                           const QString & bef = "",
                           const QString & aft = "");

  /// Returns the same string in the reverse direction
  QString reverseString(const QString & str);

  /// Capitalize the string
  QString capitalize(const QString & str);

  /// Returns the string, but not starting with an uppercase
  QString uncapitalize(const QString & str);

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


  /// Escapes HTML special codes from within the string
  QString escapeHTML(const QString & str);


  /// Attemps to guess the codec of the given byte string.
  QTextCodec * autoDetectCodec(const QByteArray & str);


  /// Converts a list of booleans into a simple two-letter string.
  QString writeBooleans(const QList<bool> & bools,
                        const QChar & tru = 'X',
                        const QChar & fals = 'x');

  /// Converts back from writeBoolean(). All the characters that do
  /// not correspond to true correspond to false.
  QList<bool> readBooleans(const QString& str,
                           const QChar & tru = 'X');

  /// Sort a list and join it
  QString joinSortedList(QStringList list, const QString & glue);

  /// Splits on a char avoiding being within pairs of (any) char
  /// contained in opening and closing. No validation (i.e. are the
  /// delimitors really paire) is performed.
  QStringList nestedSplit(const QString & str, const QChar & delim,
                          const QString & opening, const QString & closing);

  /// Makes a string suitable to represent a pointer
  QString pointerString(const void * pointer);


  /// Safe asprintf using Ruby
  QString safeAsprintf(const QString & format, int value);
  QString safeAsprintf(const QString & format, double value);


  /// @}

  /// @name CSV-related functions

  /// Extracts any tabular data from QMimeData. Returns the data line
  /// by line (i.e. the QList is as big as there are lines)
  QList<QStringList> extractTable(const QMimeData * data);

  /// Splits a CSV line with the given separator and given quote
  /// character.
  ///
  /// Stores the position of the fields into the indices vector.
  void splitCSVLine(const QString &s, QRegExp & re, QChar quote,
                    QVector<int> * indices);

  /// @}



  /// @name Number-related functions
  ///
  /// @{

  /// Returns a random number between 0.0 and 1.0
  double random();

  /// Returns a random number between @a low and @a high, optionally distributed logarithmically
  double random(double low, double high, bool log = false);

  /// Rounds \a value as if it had only \a ranks digits before the
  /// decimal point.
  double roundValue(double value, int ranks = 1);

  /// Returns the lower (or higher, if \a below is false) power of ten
  /// closest to the number.
  double magnitude(double value, bool below = true);


  /// Copies the source to the target, skipping in the target the
  /// indices contained in the set. The total number is the one in the
  /// TARGET (i.e. the number in source + the size of skipped)
  ///
  /// If skipInSource is true, then the holes are in the source rather
  /// than in the target.
  void skippingCopy(const double * source, double * target,
                    int nb, const QSet<int> skipped, bool skipInSource = false);

  /// Compares the two numbers, and returns true if they are "equal", meaning:
  /// @li both NaN
  /// @li equal
  /// @li they difference is smaller than tolerance (if tolerance is
  /// strictly positive)
  bool fuzzyCompare(double a, double b, double tolerance = 0);

  /// @}

  /// @name Geometrical functions
  ///
  /// A series of geometry-related utility functions
  /// 
  /// @{

  /// Dumps the contents of a rectangle to the target stream
  template<typename T, typename Rect> void dumpRectangle(T & stream, 
                                                         const Rect & r) {
    stream << r.x() << "," << r.y() << " to " 
           << r.x() + r.width() << "," << r.y() + r.height();
    if(r.isNull())
      stream << " (null)";
    if(! r.isValid())
      stream << " (invalid)";
  };

  /// Applies the margins \m to a rectangle \r (ie remove them),
  /// and return the adjusted value.
  inline QRect applyMargins(const QRect & r, 
                            const QMargins & m) {
    return r.adjusted(m.left(), m.top(), -m.right(), -m.bottom());
  };

  /// Returns a new version of the rectangle scaled around the given
  /// point by the given quantities. The idea is that \p point should
  /// stay at the same relative position.
  QRectF scaledAround(const QRectF & rect, const QPointF & point,
                      double xscale, double yscale);


  /// Sanitize a rectangle, i.e. make sure it has a non-zero with and
  /// height and it does not have NaNs.
  QRectF sanitizeRectangle(const QRectF & rect);

  /// Unite two rectangles, returning the rectangle that encompasses
  /// both. It ignores non-finite values (unless both rectangles have
  /// non-finite values).
  QRectF uniteRectangles(const QRectF & r1, const QRectF & r2);

  /// Unite several rectangles, returning the rectangle that
  /// encompasses all. It ignores non-finite values. The target
  /// rectangle will always be oriented the right way
  QRectF uniteRectangles(const QList<QRectF> rects);


  /// Checks that the given point only has finite coordinates
  bool isPointFinite(const QPointF & point);

  /// @}

  /// Prompts the user for confirmation for something. For now,
  /// through the use of a dialog box, but that may change some time
  /// later.
  bool askConfirmation(const QString & what, 
                       const QString & title = QString());



  /// @name File-related functions
  ///
  /// A bunch of file-related functions
  ///
  /// @{

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


  /// Opens the given file with the target mode, raising a
  /// RuntimeError if that couldn't happen for some reason.
  ///
  /// Unless \a expandTilde is set to false, tilde are automatically
  /// expanded.
  void open(QFile * file, QIODevice::OpenMode mode, 
            bool expandTilde = true);

  /// If the path is relative, checks wether it is possible to write
  /// in the target directory, or return a suitably modified path
  /// (based on the home dir).
  ///
  /// If absolute, does not change a thing.
  ///
  /// Returns an absolute path.
  QString getWritablePath(const QString & path);

  /// Performs a rotation of given file, is if it is present, rename
  /// it as file.1, renaming a file.1 as file.2 and so on up until the
  /// max number, after which the files are simply deleted.
  ///
  /// If max is negative, there is no maximum file (but this isn't
  /// probably a great idea).
  ///
  /// @todo Move that to File
  void rotateFile(const QString & file, int max);

  /// Asks confirmation before overwriting the target file if it exists.
  ///
  /// Throws an exception if user cancelled unless \a silent is true.
  //bool confirmOverwrite(const QString & fileName, bool silent = false);

  /// Expands a leading ~ into the home directory of the user.
  ///
  /// @todo Move that to File
  QString expandTilde(const QString & name);

  /// Reads all lines from a file, using a LineReader
  QStringList readAllLines(QIODevice * device, bool stripCR = false);

  /// Reads all lines from a file, using a LineReader
  QStringList readAllLines(QTextStream * txt, bool stripCR = false);

  /// Returns the name of a file based from a QIODevice pointer, or a
  /// suitable other string describing the stream.
  QString fileName(const QIODevice * device);


  /// Same thing, but with a QDataStream
  QString fileName(const QDataStream & stream);

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

  /// Returns the norm of the given vector, ignoring any non-finite
  /// number.
  double finiteNorm(const gsl_vector * v);

  /// Returns the scalar product of the two vectors, taking into
  /// account only the finite terms.
  double finiteProduct(const gsl_vector * v1, const gsl_vector * v2);

  /// @}



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
  QStringList parseConfigurationFile(QTextStream * source, 
                                     bool keepCR = false,
                                     QStringList * comments = NULL,
                                     QList< QPair<int, int> > * lineNumbers = NULL,
                                     bool stripBlank = false);

  /// Overloaded function that operates directly on a QIODevice (but
  /// you cannot set the codec in that case).
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


  /// Returns the memory used (hmm, the "resources" ?), in KiB
  int memoryUsed();

  /// Sets the @a user and @a system arguments to the CPU time (in
  /// milliseconds) used in user mode and kernel mode since the
  /// beginning of the program.
  ///
  /// It also provides the number of voluntary and involuntary context
  /// switches.
  void processorUsed(long * user, long * system,
                     long * vCS = NULL, long * iCS = NULL);

  /// Reverses the order of the list, in place.
  template <class T> void reverseList(QList<T> & list) {
    int sz = list.size();
    int l = sz/2;
    for(int i = 0; i < l; i++) 
      list.swap(i, sz - i - 1);
  }


  /// @name Graphical utilities
  ///
  /// @{ 
  
  /// Returns the color corresponding to the value. A value below the
  /// lowest of the list returns the corresponding color. Same for a
  /// value higher than the higher of the list. Anything inbetween is
  /// interpolated, either in the RGB space or in the HSV space.
  QColor gradientColor(double value, const QList<QPair<double, QColor> > & colors, bool hsv = false);

  /// Draw the given rich text, accepting the same arguments as the
  /// corresponding QPainter function, with the addition painter as
  /// first function.
  void drawRichText(QPainter * painter, const QRectF &rectangle,
                    int flags, const QString &text,
                    QRectF *boundingRect = NULL);

  const QStyle::StandardPixmap FreeParameterIcon = QStyle::SP_CustomBase;
  const QStyle::StandardPixmap FixedParameterIcon = static_cast<QStyle::StandardPixmap>(QStyle::SP_CustomBase+1);

  /// Returns the standard icon for the given function
  QIcon standardIcon(QStyle::StandardPixmap standardIcon);

  /// @}
};

#endif
