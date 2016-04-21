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
#include <ruby-templates.hh>

class DataSet;
class SaveGlobal;

/// This class represents an expression meant to act on a DataSet
class DataSetExpression  {

private:

  /// Disallow copy construction
  DataSetExpression(const DataSetExpression &);

  /// The underlying DataSet
  const DataSet * dataset;

  /// Current index in the dataset for iterators.
  int index;

  SaveGlobal * sStats;
  SaveGlobal * sMeta;

  /// The internal expression object !
  Expression * expr;


public:

  /// Whether or not we use $stats
  bool useStats;

  /// Whether or not we use $meta
  bool useMeta;

  /// Creates an expression object
  DataSetExpression(const DataSet * ds);

  /// Prepares the expression for use with the given dataset, possibly
  /// adding the additional parameters.
  void prepareExpression(const QString & formula,
                         const QStringList & extraParameters = QStringList(),
                         int extraCols = 0);

  /// Returns the expression
  Expression & expression();

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
