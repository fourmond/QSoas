/*
  datastackhelper.cc: implementation of the DataStackHelper class
  Copyright 2016 by CNRS/AMU

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
#include <datastackhelper.hh>
#include <datastack.hh>
#include <soas.hh>
#include <curveview.hh>
#include <stylegenerator.hh>

#include <stylegenerator.hh>
#include <general-arguments.hh>

DataStackHelper::DataStackHelper(const CommandOptions & opts,
                                 bool upt, bool def) :
  deferred(def), update(upt)
{
  updateFromOptions(opts, "flags", flags);
  updateFromOptions(opts, "style", style);
}

DataStackHelper::~DataStackHelper()
{
  flush();
}

QList<Argument *> DataStackHelper::helperOptions()
{
  QList<Argument *> args;
  args << new SeveralStringsArgument(QRegExp("\\s*,\\s*"),
                                     "flags", 
                                     "Flags",
                                     "Flags to set on the new buffers")
       << new StyleGeneratorArgument("style", 
                                     "Style",
                                     "Style for the displayed curves");
  return args;
}

void DataStackHelper::pushDataSet(DataSet * ds)
{
  if(deferred)
    datasets << ds;
  else
    pushOne(ds, NULL);
}


void DataStackHelper::pushDataSets(const QList<DataSet *> & dss)
{
  for(int i = 0; i < dss.size(); i++)
    pushDataSet(dss[i]);
}

DataStackHelper & DataStackHelper::operator <<(DataSet * ds)
{
  pushDataSet(ds);
  return *this;
}

void DataStackHelper::pushOne(DataSet * ds, StyleGenerator * gen)
{
  if(flags.size() > 0)
    ds->setFlags(flags);

  soas().stack().pushDataSet(ds, true); // use the silent version
                                // as we display ourselves
  if(update)
    soas().view().addDataSet(ds, gen);
  else
    soas().view().showDataSet(ds, gen);
  update = true;
}

void DataStackHelper::flush()
{
  int sz = datasets.size();
  QScopedPointer<StyleGenerator> 
    gen(StyleGenerator::fromText(style, sz));
  for(int i = 0; i < sz; i++)
    pushOne(datasets[i], gen.data());
  datasets.clear();
}
