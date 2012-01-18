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
  if(pats[0].size() == 0) {
    pats.takeFirst();
    pats[0] = "/" + pats[0];
  }

  QFileInfo info(pats.first());
  QStringList directories;
  directories << info.dir().path();
  for(int i = 0; i < pats.size() - 1; i++) {
    // o << "Pattern:" << pats[i] 
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

double Utils::roundValue(double value, int rank)
{
  double rk = ceil(log10(value));
  double v = value/pow(10, rk-rank);
  return round(v) * pow(10, rk-rank);
}
