/*
  datasetwriter.cc: save dataset to files
  Copyright 2019 by CNRS/AMU

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
#include <datasetwriter.hh>
#include <dataset.hh>
#include <metadatafile.hh>
#include <file.hh>

#include <utils.hh>

#include <general-arguments.hh>



DataSetWriter::DataSetWriter() :
  writeRowNames(false), separator("\t"), commentPrefix("#"),
  columnNamesPrefix("##")
{
}

void DataSetWriter::writeData(QIODevice * target,
                              const DataSet * ds) const
{
  QTextStream o(target);
  o << commentPrefix  <<" saved from Soas buffer name " << ds->name << endl;

  // Here, we add the 
  
  // Writing column names
  QList<QStringList> ls = ds->columnNames;
  while(ls.size() > 0) {
    QStringList names = ls.takeLast();
    if(writeRowNames)
      names.insert(0, "row-names");
    o << columnNamesPrefix << names.join(separator) << "\n";
  }

  /// @todo write row names.
  ls = ds->rowNames;
  
  int nb = ds->nbRows();
  for(int i = 0; i < nb; i++) {
    if(writeRowNames) {
      for(int rn = 0; rn < ls.size(); rn++) {
        o << ls[rn].value(i,"") << separator;
      }
    }
    for(int j = 0; j < ds->columns.size(); j++) {
      if(j)
        o << separator;
      if(format.isEmpty())
        o << ds->columns[j][i];
      else
        o << Utils::safeAsprintf(format, ds->columns[j][i]);
    }
    o << "\n";
  }
}


void DataSetWriter::writeDataSet(File * file,
                                 const DataSet * dataset) const
{
  writeData(*file, dataset);
  writeDataSetMeta(file->info().absoluteFilePath(), dataset);
}


void DataSetWriter::writeDataSetMeta(const QString & file,
                                     const DataSet * dataset) const
{
  MetaDataFile mf(file);
  mf.metaData = dataset->metaData;
  if(dataset->segments.size() > 0) {
    QList<QVariant> segs;
    for(int i : dataset->segments)
      segs << i;
    mf.metaData["__segments__"] = segs;
  }
  // We remove the meta-data that was automatically added and that
  // does not make much sense to save...
  mf.metaData.remove("age");
  mf.metaData.remove("name");
  mf.metaData.remove("backend");
  mf.metaData.remove("file_date");
  
  if(mf.metaData.size() > 0)
    mf.write();
}


QList<Argument *> DataSetWriter::writeOptions()
{
  QList<Argument *> rv;
  rv << new BoolArgument("row-names",
                         "Row names",
                         "Wether to write row names or not")
     << new StringArgument("separator",
                           "Separator",
                           "column separator (default: tab)")
     << new StringArgument("number-format",
                           "Number format",
                           "printf-like format string for numbers")
     << new StringArgument("comments",
                           "Comments",
                           "prefix for the comments")
    ;
  return rv;
}


void DataSetWriter::setFromOptions(const CommandOptions & opts)
{
  updateFromOptions(opts, "row-names", writeRowNames);
  updateFromOptions(opts, "separator", separator);
  updateFromOptions(opts, "number-format", format);
  updateFromOptions(opts, "comments", commentPrefix);
}
