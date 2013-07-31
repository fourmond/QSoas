/*
  utils.cc: various utility functions
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


#include <headers.hh>
#include <utils.hh>
#include <exceptions.hh>

#include <vector.hh>

/// Helper function for globs
static QStringList dirGlob(QString directory, QString str,
                           bool isDir, bool parentOK = false)
{
  QDir dir(directory);
  QStringList rets;
  QDir::Filters filters = QDir::Dirs | QDir::CaseSensitive | QDir::NoDot;
  if(! isDir)
    filters |= QDir::Files;
  if(! ((isDir && parentOK) || str.startsWith(".")) )
    filters |= QDir::NoDotDot;
  if(str.size() == 0)
    str = "*";
  QStringList entries = dir.entryList(QStringList() << str, filters);
  QTextStream o(stdout);
  for(int j = 0; j < entries.size(); j++)
    rets << dir.filePath(entries[j]);
  
  return rets;
}

QStringList Utils::glob(const QString & pattern, bool trim, bool isDir)
{
  
  QStringList pats = QDir::fromNativeSeparators(pattern).split("/");
  QTextStream o(stdout);
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

QString Utils::deltaStr(const QString & w)
{
  QString ret = QChar(0x0394);
  ret += w;
  return ret;
}


void Utils::open(QFile * file, QIODevice::OpenMode mode)
{
  /// @todo Implement parsing of ~ ? (seems rather useless)
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
    for(size_t j = 0; j < matrix->size2; j++)
      lst << QString::number(gsl_matrix_get(matrix, i, j));
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

double Utils::roundValue(double value, int rank)
{
  double rk = ceil(log10(value));
  double v = value/pow(10, rk-rank);
  return round(v) * pow(10, rk-rank);
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



QStringList Utils::parseConfigurationFile(QIODevice * source, 
                                          bool keepCR, QStringList * tComments,
                                          QList< QPair<int, int> > * 
                                          lineNumbers, bool stripBlank)
{
  QTextStream in(source);
  QString line;
  QRegExp comment(stripBlank ? "^\\s*#|^\\s*$"
                  : "^\\s*#");    // Comment, possibly with blank line
  QStringList validLines;
  QList< QPair<int, int> > numbers;
  QStringList comments;
  int ln = 0;
  QString contLine;
  int contFrom = -1;
  
  do {
    line = in.readLine();
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
    
  } while(! line.isNull());

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
  int v = str.toInt(&ok);
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
