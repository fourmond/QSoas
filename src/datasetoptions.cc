/*
  datasetoptions.cc: implementation of DatasetOptions
  Copyright 2013 by Vincent Fourmond

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
#include <datasetoptions.hh>
#include <dataset.hh>


#include <command.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <soas.hh>

DatasetOptions::DatasetOptions() :
  yErrors(-1)
{
}

bool DatasetOptions::hasYErrors() const
{
  return yErrors >= 0;
}

bool DatasetOptions::hasYErrors(const DataSet * ds) const
{
  return yErrors >= 0 && yErrors < ds->nbColumns();
}

double DatasetOptions::yError(const DataSet * ds, int idx) const
{
  if(! hasYErrors(ds))
    return 0;
  return ds->column(yErrors).value(idx, 0);
}

void DatasetOptions::setYErrors(int col)
{
  yErrors = col;
}

ArgumentList * DatasetOptions::optionList() 
{
  return 
    new ArgumentList(QList<Argument *>() 
                     << new IntegerArgument("yerrors", 
                                            "Y errors",
                                            "Column containing y errors")
                     );
}


void DatasetOptions::setDatasetOptions(DataSet * ds, 
                                       const CommandOptions & opts)
{
  if(opts.contains("yerrors")) {
    int col = 0;
    updateFromOptions(opts, "yerrors", col);
    ds->options.setYErrors(col);
  }
}



//////////////////////////////////////////////////////////////////////

QDataStream & operator<<(QDataStream & out, const DatasetOptions & ds)
{
  out << ds.yErrors;
  
  return out;
}


QDataStream & operator>>(QDataStream & in, DatasetOptions & ds)
{
  in >> ds.yErrors;
  // if(DataStack::serializationVersion >= 1)
  return in;
}

//////////////////////////////////////////////////////////////////////


static void optionsCommand(const QString &, const CommandOptions & opts)
{
  /// @todo Make that a new dataset ?
  DataSet * ds = soas().currentDataSet();
  DatasetOptions::setDatasetOptions(ds, opts);
}

static Command 
opts("dataset-options",            // command name
     effector(optionsCommand),     // action
     "stack",                      // group name
     NULL,                         // arguments
     DatasetOptions::optionList(), // options
     "Options",
     "Set dataset options");

