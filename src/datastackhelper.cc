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
  deferred(def), update(upt), reversed(false), valid(true)
{
  updateFromOptions(opts, "flags", flags);
  updateFromOptions(opts, "style", style);
  updateFromOptions(opts, "set-meta", meta);
  updateFromOptions(opts, "reversed", reversed);
}

DataStackHelper::~DataStackHelper()
{
  if(valid)
    flush();
  else {
    for(DataSet * ds : datasets)
      delete ds;
  }
}

void DataStackHelper::invalidate()
{
  valid = false;
}

void DataStackHelper::validate()
{
  valid = true;
}

const QList<DataSet *> & DataStackHelper::currentDataSets() const
{
  return datasets;
}


QList<Argument *> DataStackHelper::helperOptions()
{
  QList<Argument *> args;
  args << new SeveralStringsArgument(QRegExp("\\s*,\\s*"),
                                     "flags", 
                                     "Flags",
                                     "Flags to set on the newly created datasets")
       << new StyleGeneratorArgument("style", 
                                     "Style",
                                     "Style for the displayed curves")
       << new BoolArgument("reversed", 
                           "Reversed",
                           "Push the datasets in reverse order")
       << new MetaHashArgument("set-meta", 
                               "Meta-data to add",
                               "Meta-data to add to the newly created datasets");
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

  for(auto i = meta.begin(); i != meta.end(); i++)
    ds->setMetaData(i.key(), i.value());

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
    pushOne(datasets[reversed ? sz-1-i: i], gen.data());
  datasets.clear();
}
