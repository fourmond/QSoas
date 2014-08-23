/*
  textbackend.cc: implementation of the TextBackend class
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
#include <databackend.hh>
#include <dataset.hh>

#include <argumentlist.hh>
#include <argumentmarshaller.hh>
#include <general-arguments.hh>

#include <utils.hh>
#include <regex.hh>
#include <exceptions.hh>

/// A general-purpose text files reader.
class TextBackend : public DataBackend {
protected:

  /// The column separator
  Regex separator;

  /// The comments lines
  Regex comments;
  virtual int couldBeMine(const QByteArray & peek, 
                          const QString & fileName) const;

  virtual ArgumentList * loadOptions() const;

  virtual QList<DataSet *> readFromStream(QIODevice * stream,
                                   const QString & fileName,
                                   const CommandOptions & opts) const;

public:
  TextBackend(const QString & sep,
              const char * n, const char * pn, const char * d = "");
};


TextBackend::TextBackend(const QString & sep,
                         const char * n, const char * pn, const char * d) : 
  DataBackend(n, pn, d), separator(sep), comments("{text-line}") {
}

int TextBackend::couldBeMine(const QByteArray & peek, 
                             const QString & /*fileName*/) const
{
  int nbSpaces = 0, nbZeros = 0, nbSpecials = 0, nbRet = 0;
  bool lastRet = false;
  for(int i = 0; i < peek.size(); i++) {
    unsigned char c = peek[i];
    switch(c) {
    case '\n': 
      nbRet++;
      lastRet = true;
      break;
    case ' ':
    case '\t':
      lastRet = false;
      nbSpaces++;
      break;
    case '\r': // ignoring that guy, especially useful for things
               // coming from windows.
      if(! lastRet)
        nbRet++;
      lastRet = false;
      break;
    case 0:
      nbZeros++;
      lastRet = false;
      break;
    default:
      if(c < 32)
        nbSpecials++;
      lastRet = false;
    }
  }
  if(nbSpecials > peek.size()/100)
    return 0;                   // Most probably not
  QTextCodec * cd = Utils::autoDetectCodec(peek);
  QString str = cd->toUnicode(peek);

  // We split into lines, el
  QStringList lines = str.split(QRegExp("[\n\r]"));

  int nbSeps = 0;
  int nbLines = 0;
  QRegExp sep = separator.toQRegExp();
  QRegExp cmt = comments.toQRegExp();
  for(int i = 0; i < lines.size(); i++) {
    const QString & l = lines[i];
    if(cmt.indexIn(l) >= 0)
      continue;
    ++nbLines;
    /// @todo Don't use split, but count !
    nbSeps += l.split(sep).size();
  }

  if(nbSeps > nbLines) {
    if(sep.indexIn(" \t") >= 0)
      return 2*nbSeps - (5 * nbLines)/3;
    else
      return 5*(2*nbSeps - nbLines)/3;
  }
  return 0;
}

static ArgumentList 
textLoadOptions(QList<Argument *>() 
                << new RegexArgument("separator", 
                                     "Separator",
                                     "Separator between columns")
                << new StringArgument("decimal", 
                                      "Decimal separator",
                                      "Decimal separator")
                << new RegexArgument("comments", 
                                     "Comments",
                                     "Comment lines")
                << new SeveralIntegersArgument("columns", 
                                               "Columns",
                                               "Order of the columns")
                << new BoolArgument("auto-split", 
                                    "Auto split",
                                    "Split on blank lines")
                );

ArgumentList * TextBackend::loadOptions() const
{
  return &textLoadOptions;
}

QList<DataSet *> TextBackend::readFromStream(QIODevice * stream,
                                             const QString & fileName,
                                             const CommandOptions & opts) const
{
  Regex sep = separator;
  updateFromOptions(opts, "separator", sep);

  Regex cmt = comments;
  updateFromOptions(opts, "comments", cmt);

  QString dSep;
  updateFromOptions(opts, "decimal", dSep);

  QList<int> cols;
  updateFromOptions(opts, "columns", cols);

  bool autoSplit = false;
  updateFromOptions(opts, "auto-split", autoSplit);

  /// @todo implement comments / header parsing... BTW, maybe
  /// Vector::readFromStream could be more verbose about the nature of
  /// the comments (the exact line, for instance ?)

  QByteArray bt = stream->peek(300);
  QTextCodec * cd = Utils::autoDetectCodec(bt);
  QTextStream s(stream);
  s.setCodec(cd);

  QStringList comments;

  QList<QList<Vector> > allColumns = 
    Vector::readFromStream(&s, sep.toQRegExp(), 
                           cmt.toQRegExp(), autoSplit, dSep,
                           QRegExp("^\\s*$"), &comments);

  ValueHash meta;

  // Parse comments for a = b stuff
  QRegExp cmtval("([\\w-]+)\\s*=\\s*(.*)");
  for(int i = 0; i < comments.size(); i++) {
    const QString & s = comments[i];
    if(cmtval.indexIn(s) >= 0) {
      QString key = cmtval.cap(1);
      QString value = cmtval.cap(2);
      bool ok;
      double v = value.toDouble(&ok);
      if(ok)
        meta[key] = v;
      else
        meta[key] = value;
    }
  }

  QList<DataSet *> ret;

  int total = allColumns.size();
  for(int j = 0; j < total; j++) {
    QList<Vector> & columns = allColumns[j];

    /// @todo Allow 0 as column containing the index ?
    QList<int> colOrder;

    if(cols.size() > 0) {
      for(int i = 0; i < cols.size(); i++)
        colOrder << cols[i] - 1;
    }
    else {
      for(int i = 0; i < columns.size(); i++) {
        colOrder << i;
      }
    }
    
    QList<Vector> finalColumns;
    for(int i = 0; i < colOrder.size(); i++) {
      if(colOrder[i] >= columns.size())
        throw RuntimeError("There are not %1 columns in file %2").
          arg(colOrder[i]+1).arg(fileName);
      finalColumns << columns[colOrder[i]];
    }

    DataSet * ds = new DataSet(finalColumns);
    ds->name = QDir::cleanPath(fileName);
    if(total > 1) {
      ds->name += QString("#%1").arg(j);
    }
    ds->addMetaData(meta);
    setMetaDataForFile(ds, fileName);
    ret << ds;
  }
  return ret;
}





/// @todo This is not a real CSV backend, as it will not parse
/// properly files with quoted text that contains delimitedrs.
TextBackend csv("/\\s*[;,]\\s*/",
                "csv",
                "CSV files",
                "Backend to read CSV files");


TextBackend text("/\\s+/",
                 "text",
                 "Text files",
                 "Backend to read plain text files");
                
