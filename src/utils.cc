/*
  utils.cc: various utility functions
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
#include <utils.hh>
#include <exceptions.hh>

#include <vector.hh>
#include <linereader.hh>

/// Helper function for globs
static QStringList dirGlob(QString directory, QString str,
                           bool isDir, bool parentOK = false)
{
  QDir dir(directory);
  QStringList rets;
  if(isDir && str == ".") {
    rets << dir.filePath(".");
    return rets;
  }
  QDir::Filters filters = QDir::Dirs | QDir::NoDot;

#ifdef Q_OS_LINUX
  filters |= QDir::CaseSensitive;
#endif
  
  if(! isDir)
    filters |= QDir::Files;
  if(! ((isDir && parentOK) || str.startsWith(".")) )
    filters |= QDir::NoDotDot;
  if(str.size() == 0)
    str = "*";
  QStringList entries = dir.entryList(QStringList() << str, filters);
  for(int j = 0; j < entries.size(); j++)
    rets << dir.filePath(entries[j]);
  
  return rets;
}

QStringList Utils::glob(const QString & pattern, bool trim, bool isDir)
{
  
  QStringList pats = QDir::fromNativeSeparators(pattern).split("/");
  bool rewriteHome = false;
  QString hp;
  if(pats[0].size() == 0) {
    pats.takeFirst();
    pats[0] = "/" + pats[0];
  }
  else if(pats[0] == "~") {
    pats[0] = QDir::homePath();
    rewriteHome = true;
    hp = pats[0] + "/";
  }

  QFileInfo info(pats.first());
  QStringList directories;
  directories << info.dir().path();
  pats[0] = info.fileName();

  for(int i = 0; i < pats.size() - 1; i++) {
    QStringList newdirs;
    /// @todo There should be a way to implement ** at this stage, but
    /// I don't see a simple one for now.
    for(int j = 0; j < directories.size(); j++)
      newdirs += dirGlob(directories[j], pats[i], true);
    directories = newdirs;
  }


  QStringList lst;
  for(int i = 0; i < directories.size(); i++)
    lst += dirGlob(directories[i], pats.last(), isDir, isDir);

  if(lst.isEmpty() && ! trim)
    lst << pattern;

  // Sounds a little hackish, but should work anyway
  if(rewriteHome) {
    QRegExp rp("^" + QRegExp::escape(hp));
    lst.replaceInStrings(rp, "~/");
  }

  return lst;
}



// #ifdef Q_OS_WIN32
// // as per https://doc.qt.io/qt-5/qfileinfo.html#ntfs-permissions
// // But it does NOT work...
// extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
// #endif

QString Utils::getWritablePath(const QString & file)
{
  QFileInfo info(file);
  if(info.isAbsolute())
    return file;
  
  QDir cur = QDir::current();
  info.setFile(cur, ".");
// #ifdef Q_OS_WIN32
//   qt_ntfs_permission_lookup++; // turn checking on
// #endif
  if(! info.isWritable())
    cur = QDir::home();
// #ifdef Q_OS_WIN32
//   qt_ntfs_permission_lookup--; // turn it off again
// #endif
  return cur.absoluteFilePath(file);
}

QStringList Utils::stringsStartingWith(const QStringList & strings, 
                                       const QString & start)
{
  QStringList ret;
  for(int i = 0; i < strings.size(); i++) {
    if(strings[i].startsWith(start))
      ret << strings[i];
  }
  return ret;
}

QString Utils::escapeHTML(const QString & str)
{
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
  return Qt::escape(str);
#else
  return str.toHtmlEscaped();
#endif
}

QString Utils::reverseString(const QString & str)
{
  QString ret;
  ret.reserve(str.size());
  for(int i = str.size() - 1; i >= 0; i--)
    ret.append(str[i]);
  return ret;
}

QString Utils::commonBeginning(const QStringList & strings)
{
  if(! strings.size())
    return "";

  // First, determine the minimum size of the strings
  int min = strings.first().size();
  for(int i = 1; i < strings.size(); i++)
    if(strings[i].size() < min)
      min = strings[i].size();

  QString ret;
  int i = 0;
  while(i < min) {
    QChar cur = strings.first()[i];
    bool diff = false;
    for(int j = 1; j < strings.size(); j++)
      if( strings[j][i] != cur) {
        diff = true;
        break;
      }
    if(diff)
      break;
    ret.append(cur);
    i++;
  }
  return ret;
}

/// @todo Reversing twice, but, well...
QString Utils::commonEnding(const QStringList & strings)
{
  QStringList revd;
  for(int i = 0; i < strings.size(); i++)
    revd << Utils::reverseString(strings[i]);
  return Utils::reverseString(Utils::commonBeginning(revd));
}

QString Utils::writeBooleans(const QList<bool> & bools,
                             const QChar & tru,
                             const QChar & fals)
{
  QString s;
  for(int i = 0; i < bools.size(); i++)
    s.append(bools[i] ? tru : fals);
  return s;
}

QList<bool> Utils::readBooleans(const QString & bools,
                                const QChar & tru)
{
  QList<bool> rv;
  for(int i = 0; i < bools.size(); i++)
    rv << (bools[i] == tru );
  return rv;
}





QString Utils::smartConcatenate(const QStringList & strings,
                                const QString & join,
                                const QString & bef,
                                const QString & aft)
{
  QString pref = commonBeginning(strings);
  QString suff = commonEnding(strings);
  QStringList mids;
  for(int i = 0; i < strings.size(); i++) {
    const QString & s = strings[i];
    int sz = s.size() - pref.size() - suff.size();
    mids << s.mid(pref.size(), sz);
  }
  return pref + bef + mids.join(join) + aft + suff;
}



QString Utils::joinSortedList(QStringList list, const QString & glue)
{
  qSort(list);
  return list.join(glue);
}

QStringList Utils::nestedSplit(const QString & str, const QChar & delim,
                               const QString & opening,
                               const QString & closing)
{
  QStringList rv;
  QString cur;
  int nest = 0;
  for(int i = 0; i < str.size(); i++) {
    QChar c = str[i];
    if(opening.contains(c)) {
      nest += 1;
    }
    else if(closing.contains(c)) {
      nest -= 1;
    }
    else if(c == delim) {
      if(nest == 0) {
        rv << cur;
        cur = "";
        continue;
      }
    }
    cur.append(c);
  }
  rv << cur;
  return rv;
}


bool Utils::askConfirmation(const QString & what, 
                            const QString & title)
{
  QString t = (title.isEmpty() ? QObject::tr("Please confirm") : title);
  QMessageBox confirmation(QMessageBox::Question, t, what,
                           QMessageBox::Yes | QMessageBox::No);
  confirmation.move(QCursor::pos() + QPoint(-10,-10));
  return confirmation.exec() == QMessageBox::Yes;
}

QRectF Utils::scaledAround(const QRectF & rect, const QPointF & point,
                           double xscale, double yscale)
{
  QRectF nr(point, QSizeF(0,0));
  
  nr.adjust((rect.left() - point.x()) * xscale,
            (rect.top() - point.y()) * yscale,
            (rect.right() - point.x()) * xscale,
            (rect.bottom() - point.y()) * yscale);
  return nr;
}

QRectF Utils::uniteRectangles(const QList<QRectF> rects)
{
  Vector xvals;
  Vector yvals;

  for(int i = 0; i < rects.size(); i++) {
    const QRectF & r = rects[i];
    if(r.isNull())
      continue;                 // Ignore null rectangles, but not
                                // invalid ones, since the latter are
                                // empty but meaningful !
    QPointF p = r.topLeft();
    xvals << p.x();
    yvals << p.y();

    p = r.bottomRight();
    xvals << p.x();
    yvals << p.y();
  }
  if(xvals.size() > 0) {
    QPointF tl(xvals.finiteMin(), yvals.finiteMin());
    QPointF br(xvals.finiteMax(), yvals.finiteMax());
    return QRectF(tl, br);
  }
  return QRectF();
}

QRectF Utils::uniteRectangles(const QRectF & r1, const QRectF & r2)
{
  return uniteRectangles(QList<QRectF>() << r1 << r2);
}


QRectF Utils::sanitizeRectangle(const QRectF & rect)
{
  QRectF ret(rect);
  double d = ret.width();
  if(d == 0 || std::isnan(d)) {
    double x = ret.left();
    if(std::isnan(d) || x == 0.0) {
      ret.moveLeft(-0.1);
      ret.setRight(0.1);
      ret.setLeft(-0.1);
      ret.setRight(0.1);
    }
    else {
      ret.setLeft(0.9 * x);
      ret.setRight(1.1 * x);
    }
  }

  d = ret.height();
  if(d == 0 || std::isnan(d)) {
    double y = ret.top();
    if(std::isnan(d) || y == 0.0) {
      ret.moveTop(0.1);
      ret.setBottom(-0.1);
      ret.setTop(0.1);
      ret.setBottom(-0.1);
    }
    else {
      ret.setTop(0.9 * y);
      ret.setBottom(1.1 * y);
    }
  }
  return ret;
}



QString Utils::deltaStr(const QString & w)
{
  QString ret = QChar(0x0394);
  ret += w;
  return ret;
}

bool Utils::isPointFinite(const QPointF & p)
{
  return std::isfinite(p.x()) && std::isfinite(p.y());
}

void Utils::open(QFile * file, QIODevice::OpenMode mode, bool expand)
{
  if(expand) {
    QString efn = Utils::expandTilde(file->fileName());
    if(efn != file->fileName())
      file->setFileName(efn);
  }
  if(! file->open(mode)) {
    QString mdStr;
    switch(mode & QIODevice::ReadWrite) {
    case QIODevice::ReadOnly:
      mdStr = "for reading";
      break;
    case QIODevice::WriteOnly:
      mdStr = "for writing";
      break;
    case QIODevice::ReadWrite:
      mdStr = "read/write";
      break;
    default:
      mdStr = "??";
    }
    throw RuntimeError(QObject::tr("Could not open '%1' %2: %3").
                       arg(file->fileName()).
                       arg(mdStr).arg(file->errorString()));
  }
}

QString Utils::fileName(const QIODevice * device)
{
  const QFile * fl = dynamic_cast<const QFile *>(device);
  if(fl)
    return fl->fileName();
  return "(device)";
}

QString Utils::fileName(const QDataStream & stream)
{
  return Utils::fileName(stream.device());
}


bool Utils::confirmOverwrite(const QString & fileName, bool silent)
{
  if(QFile::exists(fileName)) {
    QString s = QObject::tr("Overwrite file '%1' ?").
      arg(fileName);
    if(! Utils::askConfirmation(s)) {
      if(silent)
        return false;
      else
        throw RuntimeError("Aborted");
    }
  }
  return true;
}



QString Utils::matrixString(const gsl_matrix * matrix)
{
  QString ret;
  for(size_t i = 0; i < matrix->size1; i++) {
    QStringList lst;
    for(size_t j = 0; j < matrix->size2; j++) {
      QString s("%1");
      lst << s.arg(gsl_matrix_get(matrix, i, j), 10);
    }
    ret += lst.join("\t") + "\n";
  }
  return ret;
}

QString Utils::matrixString(const gsl_matrix_complex * matrix)
{
  QString ret;
  for(size_t i = 0; i < matrix->size1; i++) {
    gsl_vector_complex_const_view v = 
      gsl_matrix_complex_const_row(matrix, i);
    ret += Utils::vectorString(&v.vector) + "\n";
  }
  return ret;
}

QString Utils::vectorString(const gsl_vector_complex * vector)
{
  QStringList lst;
  for(size_t i = 0; i < vector->size; i++) {
    gsl_complex c = gsl_vector_complex_get(vector, i);
    lst << QString("%1 + %2i").arg(GSL_REAL(c)).
      arg(GSL_IMAG(c));
  }
  return lst.join("\t");
}

QString Utils::vectorString(const gsl_vector * vector)
{
  QStringList lst;
  for(size_t i = 0; i < vector->size; i++) {
    double x = gsl_vector_get(vector, i);
    lst << QString("%1").arg(x);
  }
  return lst.join("\t");
}

double Utils::finiteNorm(const gsl_vector * v)
{
  double rv = 0;
  const double * x = v->data;
  size_t s = v->stride;
  for(size_t i = 0; i < v->size; i++, x += s) {
    if(std::isfinite(*x))
      rv += *x * *x;
  }
  return rv;
}

double Utils::finiteProduct(const gsl_vector * v1, const gsl_vector * v2)
{
  double rv = 0;
  const double * x1 = v1->data;
  const double * x2 = v2->data;
  size_t s1 = v1->stride;
  size_t s2 = v2->stride;
  for(size_t i = 0; i < v1->size; i++, x1 += s1, x2 += s2) {
    if(std::isfinite(*x1) && std::isfinite(*x2))
      rv += *x1 * *x2;
  }
  return rv;
}


double Utils::roundValue(double value, int rank)
{
  double rk = ceil(log10(value));
  double v = value/pow(10, rk-rank);
  return round(v) * pow(10, rk-rank);
}


bool Utils::fuzzyCompare(double a, double b, double tolerance)
{
  if(std::isnan(a)) {
    if(std::isnan(b))
      return true;
    else
      return false;
  }
  if(std::isnan(b))
    return false;
  if(a == b)
    return true;
  if(tolerance > 0 && fabs(a - b) <= tolerance)
    return true;
  return false;
}

void Utils::registerShortCut(const QKeySequence & seq, QObject * receiver, 
                             const char * fn, QWidget * parent,
                             Qt::ShortcutContext context)
{
  if(! parent)
    parent = dynamic_cast<QWidget*>(receiver);
  QShortcut * sc = new QShortcut(seq, parent);
  sc->setContext(context);
  receiver->connect(sc, SIGNAL(activated()), fn);
}


QStringList Utils::readAllLines(QTextStream * source, bool keepCR)
{
  QStringList ret;
  LineReader s(source);
  while(! s.atEnd())
    ret << s.readLine(keepCR);
  return ret;
}

QStringList Utils::readAllLines(QIODevice * source, bool keepCR)
{
  QTextStream s(source);
  return readAllLines(&s, keepCR);
}

QStringList Utils::parseConfigurationFile(QIODevice * source, 
                                          bool keepCR, QStringList * tComments,
                                          QList< QPair<int, int> > * 
                                          lineNumbers, bool stripBlank)
{
  QTextStream s(source);
  return Utils::parseConfigurationFile(&s, keepCR, tComments,
                                       lineNumbers, stripBlank);
}

QStringList Utils::parseConfigurationFile(QTextStream * source, 
                                          bool keepCR, QStringList * tComments,
                                          QList< QPair<int, int> > * 
                                          lineNumbers, bool stripBlank)
{
  QString line;
  QRegExp comment(stripBlank ? "^\\s*#|^\\s*$"
                  : "^\\s*#");    // Comment, possibly with blank line
  QStringList validLines;
  QList< QPair<int, int> > numbers;
  QStringList comments;
  int ln = 0;
  QString contLine;
  int contFrom = -1;

  LineReader s(source);
  while(! s.atEnd()) {
    line = s.readLine();
    
    ++ln;
    
    // Avoid comments;
    if(contFrom < 0 && comment.indexIn(line, 0) >= 0)
      comments << line;
    else {
      bool cont = false;
      if(line.endsWith("\\")) {
        line.chop(1);
        cont = true;
        if(keepCR)
          line += "\n";         // well...
        if(contFrom < 0)
          contFrom = ln;
      }
      contLine += line;
      if(! cont) {
        // we have done
        validLines << contLine;
        contLine.clear();
        numbers.append(QPair<int, int>(contFrom < 0 ? ln : contFrom, ln));
        contFrom = -1;
      }
    }
  }

  // Update user-supplied arguments when applicable
  if(lineNumbers)
    *lineNumbers = numbers;

  if(tComments)
    *tComments = comments;

  return validLines;
}


bool Utils::updateWithin(QString & str, const QString & begin, 
                         const QString & end, const QString & newText,
                         bool appendIfNotFound)
{
  QRegExp re(QRegExp::escape(begin) + ".*" +
             QRegExp::escape(end));

  int left = re.indexIn(str, 0);
  int nb = 0;
  if(left < 0) {
    if(! appendIfNotFound)
      return false;
    else
      left = str.size();
  }
  else
    nb = re.cap(0).size();
  str.replace(left, nb, begin + newText + end);
  return true;
}

QTextCodec * Utils::autoDetectCodec(const QByteArray & str)
{
  return QTextCodec::codecForUtfText(str);
}


// For some reason, on win32, truncate is redefined by Ruby, which
// makes a mess
QString Utils::abbreviateString(const QString & str, int nb)
{
  if(nb < 4)
    nb = 4;
  if(str.size() <= nb)
    return str;
  QString s = str;
  s.truncate(nb-3);
  return  s + "...";
}

QString Utils::capitalize(const QString & str)
{
  QString ret = str;
  if(! ret.isEmpty())
    ret[0] = ret[0].toUpper();
  return ret;
}

QString Utils::uncapitalize(const QString & str)
{
  QString ret = str;
  if(! ret.isEmpty())
    ret[0] = ret[0].toLower();
  return ret;
}

double Utils::stringToDouble(const QString & str)
{
  bool ok = false;
  double v = str.toDouble(&ok);
  if(! ok)
    throw RuntimeError(QObject::tr("Not a valid number: '%1'").
                       arg(str));
  return v;
}

int Utils::stringToInt(const QString & str)
{
  bool ok = false;
  int v;
  if(str.startsWith("0x") || str.startsWith("0X")) {
    v = str.mid(2).toInt(&ok, 16);
  }
  else
    v = str.toInt(&ok);
  if(! ok)
    throw RuntimeError(QObject::tr("Not an integer: '%1'").
                       arg(str));
  return v;
}

QString Utils::expandTilde(const QString & name)
{
  if(name == "~")
    return QDir::homePath();

  if(name.startsWith("~/"))
    return QDir::home().filePath(name.mid(2));
  return name;
}

QString Utils::shortenString(const QString & str, int len, int last)
{
  if(str.size() < len)
    return str;
  QString s = str.left(len - 3 - last);
  s += "...";
  s += str.right(last);
  return s;
}

/// @todo error handling
void Utils::rotateFile(const QString & file, int max)
{
  QFileInfo f(file);
  if(! f.exists()) {
    /// @todo Delete dangling symlink ?
    return;
  }
  int cur = 1;
  while(true) {
    QString cf = QString("%2.%1").arg(cur).arg(file);
    QFileInfo f(cf);
    if(! f.exists())
      break;
    cur++;
    if(max >= 0 && cur >= max)
      break;
  }

  // Now move the files, cur is the first target file
  while(cur > 1) {
    QString src = QString("%2.%1").arg(cur-1).arg(file);
    QString target = QString("%2.%1").arg(cur).arg(file);
    QFileInfo f(target);
    if(f.exists())
      QFile::remove(target);
    QFile::rename(src, target);
    cur--;
  }
  QString first = QString("%1.1").arg(file);
  QFile::rename(file, first);
}

void Utils::invertMatrix(gsl_matrix  * mat, gsl_matrix * target,
                         double threshold)
{
  if(mat->size1 != mat->size2)
    throw RuntimeError("Impossible to invert a rectangular matrix: %1 != %2").
      arg(mat->size1).arg(mat->size2);

  int sz = mat->size1;
  Vector sv(sz, 0);             // singular values
  Vector w(sz, 0);              // work
  Vector tmpm(sz*sz, 0);

  gsl_matrix_view m = gsl_matrix_view_array(tmpm.data(), sz, sz);

  gsl_linalg_SV_decomp(mat, &m.matrix, sv.toGSLVector(), w.toGSLVector());

  // Now, we invert the diagonal elements and multiply V by that
  double max = sv[0];
  double t = max * threshold;
  for(int i = 0; i < sz; i++) {
    double elem = sv[i];
    if(t > 0 && elem < t)
      elem = t;
    gsl_vector_view col = gsl_matrix_column(&m.matrix, i);
    gsl_vector_scale(&col.vector, 1/elem);
  }

  // target = V * U^T
  gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, &m.matrix, mat, 
                 0.0, target);
}

class Sleep : public QThread {
public:

  static void msleep(unsigned long ms) {
    QThread::msleep(ms);
  };
};

void Utils::msleep(unsigned long ms)
{
  Sleep::msleep(ms);
}


QList<QStringList> Utils::splitOn(const QStringList & lines,
                                  const QRegExp & re)
{
  QList<QStringList> ret;

  ret << QStringList();
  
  QRegExp sep(re);

  bool lastMatched = true;      // We don't want to create an empty
                                // val on the first line if it
                                // matches.
  for(int i = 0; i < lines.size(); i++) {
    const QString & l = lines[i];
    if(sep.indexIn(l) >= 0) {
      if(! lastMatched) 
        ret << QStringList();
      lastMatched = true;
    }
    else {
      ret.last() << l;
      lastMatched = false;
    }
  }
  return ret;
}


QStringList Utils::extractMatches(QString & str, const QRegExp & ore, 
                                  int group)
{
  QStringList ret;
  int idx = 0;
  QRegExp re(ore);
  
  
  while(re.indexIn(str, idx) >= 0) {
    int idx = re.pos();
    if(group < 0)
      ret << re.capturedTexts();
    else
      ret << re.cap(group);
    str.replace(idx, re.matchedLength(), "");
  }
  return ret;
}

double Utils::random()
{
  double x = ::rand() * 1.0/RAND_MAX;
  /// @todo add more entropy ?
  return x;
}

double Utils::random(double low, double high, bool log)
{
  double x = Utils::random();
  double sign = 1;
  if(low < 0)
    sign = -1;
  if(log) {
    if(high*low < 0)
      throw RuntimeError("Cannot use logarithm when range crosses 0");
    low = ::log(low*sign);
    high = ::log(high*sign);
  }
  x = low + (high - low)*x;
  if(log)
    return sign * exp(x);
  return x;
}

double Utils::magnitude(double value, bool below)
{
  double sgn = (value < 0 ? -1.0 : 1.0);
  double lg = log10(sgn * value);
  lg = (below ? floor(lg) : ceil(lg));
  return sgn * pow(10, lg);
}


void Utils::skippingCopy(const double * source, double * target,
                         int nb, const QSet<int> skipped, bool skipInSource)
{
  int j = 0;
  for(int i = 0; i < nb; i++)
    if(! skipped.contains(i)) {
      if(skipInSource)
        target[j++] = source[i];
      else
        target[i] = source[j++];
    }
}



QColor Utils::gradientColor(double value,
                            const QList<QPair<double, QColor> > & c,
                            bool hsv)
{
  QList<QPair<double, QColor> > colors = c;
  std::sort(colors.begin(), colors.end(), [](const QPair<double, QColor> & a, const QPair<double, QColor> & b) -> bool {
      return a.first < b.first;
    }
    );
  if(value < colors.first().first)
    return colors.first().second;


  for(int i = 1; i < colors.size(); i++) {
    if(value < colors[i].first) {
      double l = colors[i-1].first;
      QColor lc = colors[i-1].second;
      double r = colors[i].first;
      QColor rc = colors[i].second;
      double alpha = (r - value)/(r - l);

      qreal a1,a2,b1,b2,c1,c2;
      if(hsv) {
        lc.getHsvF(&a1, &b1, &c1);
        rc.getHsvF(&a2, &b2, &c2);
      }
      else {
        lc.getRgbF(&a1, &b1, &c1);
        rc.getRgbF(&a2, &b2, &c2);
      }
      a1 = alpha*a1 + (1 - alpha)*a2;
      b1 = alpha*b1 + (1 - alpha)*b2;
      c1 = alpha*c1 + (1 - alpha)*c2;
      if(hsv) {
        lc.setHsvF(a1, b1, c1);
      }
      else {
        lc.setRgbF(a1, b1, c1);
      }
      return lc;
    }
  }

  return colors.last().second;
}


void Utils::drawRichText(QPainter * painter, const QRectF &rectangle,
                         int flags, const QString &text,
                         QRectF *boundingRect)
{
  QStaticText t(text);
  t.setTextWidth(rectangle.width());
  Qt::Alignment al =
    (Qt::Alignment) flags & (Qt::AlignHorizontal_Mask | Qt::AlignVertical_Mask);
  t.setTextOption(QTextOption(al));

  QSizeF sz = t.size();
  painter->drawStaticText(rectangle.topLeft(), t);
}
