/*
  datasetlist.cc: implementation of the DataSetList class
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
#include <datasetlist.hh>
#include <datastack.hh>
#include <soas.hh>
#include <general-arguments.hh>

DataSetList::DataSetList(const CommandOptions & opts, bool all) 
{
  DataStack & s = soas().stack();
  if(! opts.contains("buffers") && ! s.currentDataSet(true)) {
    noDataSet = true;
    return;
  }
  
  noDataSet = false;
  
  if(opts.contains("buffers"))
    updateFromOptions(opts, "buffers", datasets);
  else {
    if(all)
      datasets = s.allDataSets();
    else
      datasets << s.currentDataSet();
  }

  QString frm;
  updateFromOptions(opts, "for-which", frm);
  if(! frm.isEmpty()) {
    QList<const DataSet *> nl = datasets;
    datasets.clear();
    for(const DataSet * s : nl) {
      if(s->matches(frm))
        datasets << s;
    }
  }
}

DataSetList::~DataSetList()
{
  
}


bool DataSetList::dataSetsSpecified() const
{
  return !noDataSet;
}

QList<const DataSet *>::const_iterator DataSetList::begin() const
{
  return datasets.begin();
}

QList<const DataSet *>::const_iterator DataSetList::end() const
{
  return datasets.end();
}

QList<Argument *> DataSetList::listOptions(const QString & txt, bool def)
{
  QList<Argument *> args;
  args << new SeveralDataSetArgument("buffers", 
                                     "Buffers",
                                     txt.toLocal8Bit(), true, def)
       << new CodeArgument("for-which", 
                           "For which",
                           "Only act on buffers matching the code");
  return args;
}

