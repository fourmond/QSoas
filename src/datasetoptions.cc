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

ArgumentList * DatasetOptions::optionList() 
{
  return 
    new ArgumentList(QList<Argument *>() 
                     << new IntegerArgument("yerrors", 
                                            "Y errors",
                                            "Column containing y errors")
                     << new BoolArgument("histogram", 
                                         "Histogram",
                                         "Show as a histogram")
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
  updateFromOptions(opts, "histogram", ds->options.histogram);
}

bool DatasetOptions::isJaggy(const DataSet * ds) const
{
  if(jagThreshold <= 0)
    return false;
  QTextStream o(stdout);
  double dx = ds->x().deltaSum()/(ds->x().max() - ds->x().min());
  if(dx > jagThreshold)
    return true;
  dx = ds->y().deltaSum()/(ds->y().max() - ds->y().min());
  if(dx > jagThreshold)
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
     "stack",                      // group name
     NULL,                         // arguments
     DatasetOptions::optionList(), // options
     "Options",
     "Set dataset options");

