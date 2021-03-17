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

#include <general-arguments.hh>



DataSetWriter::DataSetWriter() :
  writeRowNames(false)
{
}

void DataSetWriter::writeDataSet(QIODevice * target,
                                 const DataSet * ds) const
{
  QTextStream o(target);
  o << "# saved from Soas buffer name " << ds->name << endl;
  
  /// @todo Write header ?
  o << ds->metaData.prettyPrint(1, "# ", "\n#\t") + "\n";

  // Writing column names
  QList<QStringList> ls = ds->columnNames;
  while(ls.size() > 0) {
    QStringList names = ls.takeLast();
    if(writeRowNames)
      names.insert(0, "row-names");
    o << "## " << names.join("\t") << "\n";
  }

  /// @todo write row names.
  ls = ds->rowNames;
  
  int nb = ds->nbRows();
  for(int i = 0; i < nb; i++) {
    if(writeRowNames) {
      for(int rn = 0; rn < ls.size(); rn++) {
        o << ls[rn].value(i,"") << "\t";
      }
    }
    for(int j = 0; j < ds->columns.size(); j++) {
      if(j)
        o << "\t";
      o << ds->columns[j][i];
    }
    o << "\n";
  }
}

QList<Argument *> DataSetWriter::writeOptions()
{
  QList<Argument *> rv;
  rv << new BoolArgument("row-names",
                         "Row names",
                         "Wether to write row names or not")
    ;
  return rv;
}


void DataSetWriter::setFromOptions(const CommandOptions & opts)
{
  updateFromOptions(opts, "row-names", writeRowNames);
}
