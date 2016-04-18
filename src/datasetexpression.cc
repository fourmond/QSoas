/*
  datasetexpression.cc: implementation of the DataSetExpression class
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
#include <datasetexpression.hh>
#include <exceptions.hh>
#include <dataset.hh>

#include <statistics.hh>
#include <idioms.hh>

DataSetExpression::DataSetExpression(const QString & expression) :
  Expression(expression), dataset(NULL), index(-1), sStats(NULL),
  sMeta(NULL), useStats(false),
  useMeta(false)
{
}




DataSetExpression::~DataSetExpression()
{
  delete sStats;
  delete sMeta;
}

void DataSetExpression::prepareExpression(const DataSet * ds,
                                          const QStringList & extraParameters,
                                          int extraCols)
{
  if(dataset)
    throw InternalError("prepareExpression() on an already prepared object");
  dataset = ds;
  QStringList vars = dataSetParameters(ds, extraCols);
  vars += extraParameters;
  setVariables(vars);

  /// @todo Save the value somehow ?
  if(useStats) {
    sStats = new SaveGlobal("$stats");
  
    Statistics st(ds);
    rbw_gv_set("$stats", st.toRuby());
  }

  if(useMeta) {
    sMeta = new SaveGlobal("$meta");
    rbw_gv_set("$meta", ds->getMetaData().toRuby());
  }

}




QStringList DataSetExpression::dataSetParameters(const DataSet * ds,
                                                 int extra)
{
  QStringList vars;
  vars << "i" << "seg" << "x_0" << "i_0";
  QStringList colNames;
  colNames << "x" << "y";
  for(int i = 2; i < ds->nbColumns() + extra; i++)
    colNames << QString("y%1").arg(i);
  vars += colNames;
  return vars;
}

bool DataSetExpression::nextValues(double * args, int * idx)
{
  if(! dataset)
    throw InternalError("nextValues() on an unprepared expression");
  index++;
  if(index >= dataset->nbRows())
    return false;
  if(idx)
    *idx = index;

  int seg = 0;
  while(seg < dataset->segments.size() && index >= dataset->segments[seg]) {
      seg++;
  }
  
  args[0] = index;                // the index !
  args[1] = seg;
  int ib = dataset->segments.value(seg-1, 0);
  args[2] = dataset->x().value(ib);
  args[3] = ib;

  for(int j = 0; j < dataset->nbColumns(); j++)
    args[j+4] = dataset->column(j)[index];
    
  return true;
}
