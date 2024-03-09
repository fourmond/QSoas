/*
  datasetoptions.cc: implementation of DatasetOptions
  Copyright 2013, 2014 by CNRS/AMU

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
#include <curveview.hh>
#include <datastack.hh>

DatasetOptions::DatasetOptions() :
  yErrors(-1), histogram(false), jagThreshold(100)
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
  return fabs(ds->column(yErrors).value(idx, 0));
}

void DatasetOptions::setYErrors(int col)
{
  yErrors = col;
}

void DatasetOptions::setYError(DataSet * ds, int idx, double val) const
{
  if(! hasYErrors(ds))
    return;                     // but should throw ?
  ds->column(yErrors)[idx] = val;
}

ArgumentList DatasetOptions::optionList() 
{
  return 
    ArgumentList(QList<Argument *>() 
                 << new ColumnArgument("yerrors", 
                                       "Y errors",
                                       "name of the column containing y errors", false, true)
                 << new BoolArgument("histogram", 
                                     "Histogram",
                                     "whether to show as a histogram (defaults to false)")
                 );
}


void DatasetOptions::setDatasetOptions(DataSet * ds, 
                                       const CommandOptions & opts)
{
  if(opts.contains("yerrors")) {
    ColumnSpecification col;
    updateFromOptions(opts, "yerrors", col);
    ds->options.setYErrors(col.getValue(ds));
  }
  updateFromOptions(opts, "histogram", ds->options.histogram);
}

bool DatasetOptions::isJaggy(const DataSet * ds) const
{
  if(jagThreshold <= 0)
    return false;
  if(ds->x().isJaggy(jagThreshold))
    return true;
  if(ds->y().isJaggy(jagThreshold))
    return true;
  return false;

}

bool DatasetOptions::shouldDrawMarkers(const DataSet * ds) const
{
  return isJaggy(ds);
}

bool DatasetOptions::shouldDrawLines(const DataSet * ds) const
{
  return ! isJaggy(ds);
}




//////////////////////////////////////////////////////////////////////

QDataStream & operator<<(QDataStream & out, const DatasetOptions & ds)
{
  out << ds.yErrors;
  out << ds.histogram;

  return out;
}


QDataStream & operator>>(QDataStream & in, DatasetOptions & ds)
{
  in >> ds.yErrors;
  in >> ds.histogram;

  return in;
}

//////////////////////////////////////////////////////////////////////


static void optionsCommand(const QString &, const CommandOptions & opts)
{
  /// @todo Make that a new dataset ?
  DataSet * ds = soas().currentDataSet();
  DatasetOptions::setDatasetOptions(ds, opts);

  soas().view().repaint();
}

static Command 
opts("dataset-options",            // command name
     effector(optionsCommand),     // action
     "buffer",                     // group name
     ArgumentList(),               // arguments
     DatasetOptions::optionList(), // options
     "Options",
     "Set dataset options");

