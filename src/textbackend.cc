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
#include <textbackend.hh>
#include <dataset.hh>

#include <argumentlist.hh>
#include <argumentmarshaller.hh>
#include <general-arguments.hh>

#include <utils.hh>
#include <regex.hh>
#include <exceptions.hh>


static void countChars(const QByteArray & peek,
                       int & nbSpaces,
                       int & nbZeros,
                       int & nbSpecials,
                       int & nbRet,
                       int & nbQuotes)
{
  nbSpaces = 0;
  nbZeros = 0;
  nbSpecials = 0;
  nbRet = 0;
  nbQuotes = 0;
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
    case '"':
    case '\'':
      lastRet = false;
      nbQuotes++;
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
}

TextBackend::TextBackend(const QString & sep,
                         const char * n, const char * pn, const char * d) : 
  DataBackend(n, pn, d), separator(sep), comments("{auto}") {
}

int TextBackend::couldBeMine(const QByteArray & peek, 
                             const QString & /*fileName*/) const
{
  int nbSpaces = 0, nbZeros = 0, nbSpecials = 0, nbRet = 0, nbQuotes;
  countChars(peek, nbSpaces, nbZeros, nbSpecials, nbRet, nbQuotes);
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
      return (2*nbSeps - (5 * nbLines)/3)*200/peek.size();
    else
      return (5*(2*nbSeps - nbLines)/3)*200/peek.size();
  }
  return 0;
}

static ArgumentList 
textLoadOptions(QList<Argument *>() 
                << new RegexArgument("separator", 
                                     "Separator",
                                     "separator between columns")
                << new StringArgument("decimal", 
                                      "Decimal separator",
                                      "decimal separator")
                << new IntegerArgument("skip", 
                                       "skip lines",
                                       "skip that many lines at beginning")
                << new RegexArgument("comments", 
                                     "Comments",
                                     "pattern for comment lines")
                << new SeveralIntegersArgument("columns", 
                                               "Columns",
                                               "columns loaded from the file")
                << new SeveralIntegersArgument("text-columns", 
                                               "Text columns",
                                               "text columns ")
                << new BoolArgument("auto-split", 
                                    "auto split",
                                    "if on, create a new dataset at every fully blank line (off by default)")
                );

ArgumentList TextBackend::loadOptions() const
{
  return textLoadOptions;
}

TextFileHeader TextBackend::parseComments(const QStringList & comments) const
{
  TextFileHeader rv;

  // Parse comments for a = b stuff
  QRegExp cmtval("([\\w-]+)\\s*=\\s*(.*)");

  QRegExp coln("^## *");

  for(const QString & s: comments) {
    if(coln.indexIn(s) == 0) {
      rv.columnNames = s.mid(coln.cap(0).size()).split('\t');
    }
    else {
      if(cmtval.indexIn(s) >= 0) {
        QString key = cmtval.cap(1);
        QString value = cmtval.cap(2);
        bool ok;
        double v = value.toDouble(&ok);
        if(ok)
          rv.meta[key] = v;
        else
          rv.meta[key] = value;
      }
    }
  }
  if(rv.meta.contains("Scan Rate (V/s)"))
    rv.meta["sr"] = rv.meta["Scan Rate (V/s)"];
  return rv;
}


QList<QList<Vector> > TextBackend::readColumns(QTextStream & s,
                                               const CommandOptions & opts,
                                               QStringList * cmts,
                                               const QList<int> & textColumns,
                                               QList<QList<QStringList> > * savedTexts) const
{
  Regex cmt = comments;
  updateFromOptions(opts, "comments", cmt);


  // look in the first lines to see if any starts with #, and use it
  // if(cmt.patternString() == "{auto}") {
  QString peek = s.device()->peek(1000);
  if(cmt.patternString() == "{auto}") {
    QRegExp tryCmts("(^|\n)\\s*#");
    if(tryCmts.indexIn(peek) >= 0)
      cmt = Regex("/^#/");
    else
      cmt = Regex("{text-line}");
  }

  Regex sep = separator;
  // If we have a line starting with ##, we set {tabs} as default
  QRegExp tryCmts2("(^|\n)\\s*##");

  
  bool trim = true;

  if(tryCmts2.indexIn(peek) >= 0) {
    // Using "strict" mode
    sep = Regex("\t");
    trim = false;
  }

  updateFromOptions(opts, "separator", sep);

  QString dSep;
  updateFromOptions(opts, "decimal", dSep);

  int skip = 0;
  updateFromOptions(opts, "skip", skip);

  bool autoSplit = false;
  updateFromOptions(opts, "auto-split", autoSplit);

  return Vector::readFromStream(&s, sep.toQRegExp(), 
                                cmt.toQRegExp(), autoSplit, dSep,
                                QRegExp("^\\s*$"), cmts, skip,
                                textColumns, savedTexts, trim);
}

QList<DataSet *> TextBackend::readFromStream(QIODevice * stream,
                                             const QString & fileName,
                                             const CommandOptions & opts) const
{

  QList<int> cols;
  updateFromOptions(opts, "columns", cols);

  /// @todo implement comments / header parsing... BTW, maybe
  /// Vector::readFromStream could be more verbose about the nature of
  /// the comments (the exact line, for instance ?)

  QByteArray bt = stream->peek(300);
  QTextCodec * cd = Utils::autoDetectCodec(bt);
  QTextStream s(stream);
  s.setCodec(cd);

  QStringList comments;
  QList<QList<QStringList> > savedTexts;

  QList<int> textColumns;
  updateFromOptions(opts, "text-columns", textColumns);

  std::sort(textColumns.begin(), textColumns.end());
  std::reverse(textColumns.begin(), textColumns.end());

  // Here, shift the columns to remove text columns
  for(int & col : textColumns) {
    for(int & i : cols) {
      if(i == col) {
        throw RuntimeError("Cannot specify a column both as /column and as /text-column");
      }
      if(i > col)
        i--;
    }
    col -= 1;                   // So we keep the logic 1 = first column
    if(col < 0)
      throw RuntimeError("Text column cannot number be smaller than 1: %1").
        arg(col+1);
  }

  QList<QList<Vector> > allColumns = readColumns(s, opts, &comments,
                                                 textColumns,
                                                 &savedTexts);
  TextFileHeader hd = parseComments(comments);

  QList<DataSet *> ret;

  for(int col : textColumns) {
    if(hd.columnNames.size() > col)
      hd.columnNames.takeAt(col);
  }


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
    QStringList cn;
    for(int i = 0; i < colOrder.size(); i++) {
      int col = colOrder[i];
      if(col >= columns.size() || col < 0)
        throw RuntimeError("There is no column #%1 in file '%2'").
          arg(col+1).arg(fileName);
      finalColumns << columns[col];
      if(hd.columnNames.size() > col)
        cn << hd.columnNames[col];
    }

    DataSet * ds = new DataSet(finalColumns);
    ds->name = QDir::cleanPath(fileName);
    if(total > 1) {
      ds->name += QString("#%1").arg(j);
    }
    ds->addMetaData(hd.meta);
    setMetaDataForFile(ds, fileName);
    ds->columnNames.clear();
    ds->columnNames << cn;
    if(savedTexts.size() > j)
      ds->rowNames = savedTexts[j];

    ret << ds;
  }
  return ret;
}

TextBackend text("/\\s+/",
                 "text",
                 "Text files",
                 "Backend to read plain text files");

//////////////////////////////////////////////////////////////////////

class CSVBackend : public TextBackend {
protected:

  static QStringList splitCSVLine(const QString &s,
                                  QRegExp & re, QChar quote) {
    QStringList fields;
    int cur = 0;
    int size = s.size();

    while(cur < size) {
      if(s[cur] == quote) {     // a quoted field
        int ci = cur+1;         // just after the quote
        QString field;
        while(true) {
          int nxt = s.indexOf(quote, ci);
          if(nxt < 0) 
            nxt = size;
          field += s.mid(ci, nxt-ci);
          ci = nxt+1;
          if(ci >= size)
            break;
          if(s[ci] == quote) {
            field += quote;
            ++ci;
          }
          else
            break;
        }
        if(ci < size) {
          int idx = re.indexIn(s, ci);
          if(idx < 0)
            idx = size;
          field += s.mid(ci, idx-ci);
          ci = idx + std::max(re.matchedLength(), 0);
        }
        fields << field;
        cur = ci;
      }
      else {
        int idx = re.indexIn(s, cur);
        if(idx < 0)
          idx = size;
        fields << s.mid(cur, idx - cur);
        cur = idx + std::max(re.matchedLength(), 0);
      }
    }
    // QTextStream o(stdout);
    // o << "'" << fields.join("', '") << "'" << endl;
    return fields;
  };
  
  virtual QList<QList<Vector> > readColumns(QTextStream & s,
                                            const CommandOptions & opts,
                                            QStringList * cmts,
                                            const QList<int> & textColumns,
                                            QList<QList<QStringList> > * savedTexts) const override {

    Regex sep = separator;
    updateFromOptions(opts, "separator", sep);

    Regex cmt = comments;
    updateFromOptions(opts, "comments", cmt);

    QString dSep;
    updateFromOptions(opts, "decimal", dSep);

    int skip = 0;
    updateFromOptions(opts, "skip", skip);

    bool autoSplit = false;
    updateFromOptions(opts, "auto-split", autoSplit);

    QRegExp sp = sep.toQRegExp();
    QChar quote = '"';

    return
      Vector::readFromStream(&s,
                             [&quote, &sp, this](const QString & str) -> QStringList {
                               return CSVBackend::splitCSVLine(str, sp, quote);
                             }, 
                             cmt.toQRegExp(), autoSplit, dSep,
                             QRegExp("^\\s*$"), cmts, skip,
                             textColumns, savedTexts);
  };
  
  virtual ArgumentList loadOptions() const override {
    return TextBackend::loadOptions();
  };

public:
  CSVBackend(const QString & sep,
             const char * n, const char * pn, const char * d = "") :
    TextBackend(sep, n, pn, d)
  {
    comments = Regex("/^#/");  // We don't want to potentially comment out every single line that starts with a quote !
  };
  
};
                


CSVBackend csv("/[;,]/",
               "csv",
               "CSV files",
               "Backend to read CSV files");
