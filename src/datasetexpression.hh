/**
   \file datasetexpression.hh
   Expression attached to a DataSet
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
#ifndef __DATASETEXPRESSION_HH
#define __DATASETEXPRESSION_HH

#include <expression.hh>
#include <possessive-containers.hh>

class DataSet;
class SaveGlobal;

/// This class represents an expression meant to act on a DataSet.
///
/// It provides two modes of action:
/// 
/// @li a mode in which one can iterate over the rows of a dataset,
/// evaluating a formula that refers to the values of the columns for
/// each row, through the use of first prepareExpression() and then
/// nextValues()
/// @li another one in which one can just evaluate an expression in
/// the context of the dataset, using evaluate()
class DataSetExpression  {

private:

  /// Disallow copy construction
  DataSetExpression(const DataSetExpression &);

  /// The underlying DataSet
  const DataSet * dataset;

  /// Current index in the dataset for iterators.
  int index;

  /// A hash containing the values
  PossessiveHash<QString, SaveGlobal> globals;

  /// The corresponding helper function
  void setGlobal(const char * name, mrb_value value);

  /// The internal expression object !
  Expression * expr;


  /// Prepares the internal variables for evaluation, but does not
  /// evaluate.
  void prepareVariables();


public:

  /// Whether or not we use $stats
  bool useStats;

  /// Whether or not we use $meta
  bool useMeta;

  /// Whether or not we use $col_names $row_names
  bool useNames;

  /// Creates an expression object
  DataSetExpression(const DataSet * ds, bool useStats = false,
                    bool useMeta = false, bool useNames = false);

  /// Prepares the expression for use with the given dataset, possibly
  /// adding the additional parameters.
  void prepareExpression(const QString & formula,
                         int extraCols = 0,
                         QStringList * extraParams = NULL);

  /// Returns the expression
  Expression & expression();

  /// Evaluates some Ruby code in the dataset context, with access to
  /// meta, stats and row/column names but without access to the
  /// dataset's data.
  mrb_value evaluate(const QString & str);

  /// Frees up all associated storage
  ~DataSetExpression();

  /// Returns the list of the available parameters, in the correct
  /// order.
  QStringList dataSetParameters(int extracols = 0,
                                QStringList * colNames = NULL);

  /// Static version informing on the parameters available for the
  /// given DataSet.
  static QStringList dataSetParameters(const DataSet * ds,
                                       int extracols = 0,
                                       QStringList * colNames = NULL);

  /// Prepares the given argument storage space for the evaluation of
  /// the next step. If returns false, then there is no next step.
  ///
  /// Does not initialize the values of the extra columns.
  bool nextValues(double * storage, int * idx = NULL);
  
};

#endif
