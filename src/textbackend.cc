/*
  textbackend.cc: implementation of the TextBackend class
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
#include <databackend.hh>
#include <dataset.hh>

#include <argumentlist.hh>
#include <argumentmarshaller.hh>
#include <general-arguments.hh>

/// A general-purpose text files reader.
class TextBackend : public DataBackend {
protected:

  /// The column separator
  QRegExp separator;

  /// The comments lines
  QRegExp comments;
  virtual int couldBeMine(const QByteArray & peek, 
                          const QString & fileName) const;

  virtual ArgumentList * loadOptions() const;

  virtual QList<DataSet *> readFromStream(QIODevice * stream,
                                   const QString & fileName,
                                   const CommandOptions & opts) const;

public:
  TextBackend(const QRegExp & sep,
              const char * n, const char * pn, const char * d = "");
};


TextBackend::TextBackend(const QRegExp & sep,
                         const char * n, const char * pn, const char * d) : 
  DataBackend(n, pn, d), separator(sep), comments("^\\s*#") {
}

int TextBackend::couldBeMine(const QByteArray & peek, 
                             const QString & /*fileName*/) const
{
  int nbSpaces = 0, nbSpecials = 0, nbRet = 0;
  for(int i = 0; i < peek.size(); i++) {
    unsigned char c = peek[i];
    switch(c) {
    case '\n': 
      nbRet++;
      break;
    case ' ':
    case '\t':
      nbSpaces++;
      break;
    case '\r': // ignoring that guy, especially useful for things
               // coming from windows.
      break;
    default:
      if(c < 32)
        nbSpecials++;
    }
  }
  if(nbSpecials > peek.size()/100)
    return 0;                   // Most probably not
  QString str(peek);
  int nbSeparatorMatches = str.split(separator).size();
  /// Heuristic: we must have one or more separators per line.
  if(nbSeparatorMatches > nbRet) {
    if(separator.indexIn(" \t") >= 0)
      return 2*nbSeparatorMatches - (5 * nbRet)/3;
    else
      return 2*nbSeparatorMatches - nbRet;
  }
  return 0;
}

static ArgumentList 
textLoadOptions(QList<Argument *>() 
                << new StringArgument("separator", 
                                      "Separator",
                                      "Separator regular expression")
                << new StringArgument("separator-exact", 
                                      "Exact separator",
                                      "Exact separator")
                << new SeveralIntegersArgument("columns", 
                                               "Columns",
                                               "Order of the columns")
                );

ArgumentList * TextBackend::loadOptions() const
{
  return &textLoadOptions;
}

QList<DataSet *> TextBackend::readFromStream(QIODevice * stream,
                                             const QString & fileName,
                                             const CommandOptions & opts) const
{
  QString sep;
  if(opts.contains("separator-exact"))
    sep = QRegExp::escape(opts["separator-exact"]->value<QString>());
  else if(opts.contains("separator"))
    sep = opts["separator"]->value<QString>();
  QList<int> cols;
  updateFromOptions(opts, "columns", cols);

  QRegExp s = separator;
  if(! sep.isEmpty()) {
    s = QRegExp(sep);
    QTextStream o(stdout);
    o << "Using separator: " << sep <<  " -- " << s.pattern() << endl;
  }
  
  /// @todo implement comments / header parsing... BTW, maybe
  /// Vector::readFromStream could be more verbose about the nature of
  /// the comments (the exact line, for instance ?)


  QList<QList<Vector> > allColumns = Vector::readFromStream(stream, s, 
                                                            comments);

  QList<DataSet *> ret;

  int total = allColumns.size();
  for(int j = 0; j < total; j++) {
    QList<Vector> & columns = allColumns[j];

    /// @todo Allow 0 as column containing the index ?
    QSet<int> cl = QSet<int>::fromList(cols);
    QList<int> colOrder;
    for(int i = 0; i < cols.size(); i++)
      colOrder << cols[i] - 1;
    
    for(int i = 0; i < columns.size(); i++) {
      if(! cl.contains(i + 1))
        colOrder << i;
    }
    
    QList<Vector> finalColumns;
    for(int i = 0; i < colOrder.size(); i++)
      finalColumns << columns[colOrder[i]];

    DataSet * ds = new DataSet(finalColumns);
    ds->name = QDir::cleanPath(fileName);
    if(total > 1) {
      ds->name += QString("#%1").arg(j);
    }
    setMetaDataForFile(ds, fileName);
    ret << ds;
  }
  return ret;
}





/// @todo This is not a real CSV backend, as it will not parse
/// properly files with quoted text that contains delimitedrs.
TextBackend csv(QRegExp("\\s*[;,]\\s*"),
                "csv",
                "CSV files",
                "Backend to read CSV files");


TextBackend text(QRegExp("\\s+"),
                 "text",
                 "Text files",
                 "Backend to read plain text files");
                
