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
#include <utils.hh>

#include <debug.hh>

DataSetExpression::DataSetExpression(const DataSet * ds,
                                     bool uS, bool uM, bool uN) :
  dataset(ds), index(-1), 
  expr(NULL), useStats(uS),
  useMeta(uM), useNames(uN),
  useRealColNames(false)
{
}




DataSetExpression::~DataSetExpression()
{
  delete expr;
}

void DataSetExpression::setGlobal(const char * name, mrb_value value)
{
  MRuby * mr = MRuby::ruby();
  globals[name] = new SaveGlobal(name);
  mr->setGlobal(name, value);
}

void DataSetExpression::prepareVariables()
{
  MRuby * mr = MRuby::ruby();
  if(useStats && !globals.contains("$stats")) {
    Statistics st(dataset);
    setGlobal("$stats", st.toRuby());
  }

  if(useMeta && !globals.contains("$meta"))
    setGlobal("$meta", dataset->getMetaData().toRuby());

  if(useNames && !globals.contains("$row_names") ) {
    mrb_value mrn = mr->newArray();
    mrb_value mcn = mr->newArray();
    bool mu = false;
    QStringList cn = dataset->mainColumnNames(&mu);
    if(! mu) {
      for(const QString & n : cn)
        mr->arrayPush(mcn, mr->fromQString(n));
    }
    QStringList rn = dataset->mainRowNames();
    for(const QString & n : rn)
      mr->arrayPush(mrn, mr->fromQString(n));
    setGlobal("$row_names", mrn);
    setGlobal("$col_names", mcn); 
    setGlobal("$row_name", mrb_nil_value()); 
    setGlobal("$c", mrb_nil_value()); 
  }
}

mrb_value DataSetExpression::evaluate(const QString & str)
{
  MRuby * mr = MRuby::ruby();
  prepareVariables();
  return mr->eval(str);
}

void DataSetExpression::prepareExpression(const QString & formula,
                                          int extraCols,
                                          QStringList * extraParams)
{
  if(expr)
    throw InternalError("prepareExpression() on an already prepared object");

  MRuby * mr = MRuby::ruby();
  prepareVariables();

  QStringList vars = dataSetParameters(extraCols);
  // vars += extraParameters;
  // QTextStream o(stdout);
  // o << "Preparing DS expression (nb:" << vars.size() << "): " << vars.join(", ") << endl;

  // Setting the global vars ahead may help...

  

  if(extraParams) {
    expr = new Expression(formula);
    QStringList prs = vars + expr->naturalVariables();
    Utils::makeUnique(prs);
    *extraParams = prs.mid(vars.size());
    expr->setVariables(prs);
  }
  else {
    QStringList vn = Expression::variablesNeeded(formula, vars);
    if(vn.size() > 0) {
      throw RuntimeError("Expression '%1' references unknown variables %2 (available variables: %3)").
        arg(formula).arg(vn.join(", ")).arg(vars.join(", "));
    }
    expr = new Expression(formula, vars);
  }

}

Expression & DataSetExpression::expression()
{
  if(! expr)
    throw InternalError("expression() on an unprepared object");
  return *expr;
}



QStringList DataSetExpression::dataSetParameters(int extra, QStringList * cn)
{
  return dataSetParameters(dataset, extra, cn, useRealColNames);
}


QStringList DataSetExpression::dataSetParameters(const DataSet * ds,
                                                 int extra, QStringList * cn,
                                                 bool useRealNames)
{
  QStringList vars;
  vars << "i" << "seg" << "x_0" << "i_0";
  QStringList colNames;
  if(useRealNames) {
    colNames = ds->mainColumnNames();
    QRegExp valid("^[_a-z]\\w*$");
    for(const QString & n : colNames) {
      if(valid.indexIn(n) != 0)
        throw RuntimeError("Column name '%1' is not a valid variable name").
          arg(n);
    }
    for(int i = 0; i < extra; i++)
      colNames << DataSet::standardNameForColumn(colNames.size());
  }
  else {
    for(int i = 0; i < ds->nbColumns() + extra; i++)
      colNames << DataSet::standardNameForColumn(i);
  }
  vars += colNames;
  if(cn)
    *cn = colNames;
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

  if(useNames && dataset->rowNames.size() > 0) {
    MRuby * mr = MRuby::ruby();
    mr->setGlobal("$row_name", mr->fromQString(dataset->rowNames[0].
                                               value(index, QString())));
  }
  if(useNames && dataset->columnNames.size() > 0 && dataset->checkColNames()) {
    MRuby * mr = MRuby::ruby();
    ValueHash c;
    for(int i = 0; i < dataset->columnNames[0].size(); i++) {
      c[dataset->columnNames[0][i]] = dataset->column(i)[index];
    }
    mr->setGlobal("$c", c.toRuby());
  }
  return true;
}
