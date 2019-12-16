/**
   \file fwexpression.hh
   Expression attached to a FitWorkspace
   Copyright 2018 by CNRS/AMU

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
#ifndef __FWEXPRESSION_HH
#define __FWEXPRESSION_HH

#include <expression.hh>

class FitWorkspace;

/// This class represents an expression to be used within a
/// FitWorkspace context.
///
/// It provides:
///
///  * as such, the parameters of the *current* dataset, in plain
///    names (with a bit of hocus pocus for changing names when necessary,
///    as in FormulaParameter)
///  * the current value of z, as z (lower case)
///  * the parameters of all the buffers, as a $parameters[dataset][name] hash
///
///  ... and some more !
///  
class FWExpression  {

private:

  /// Disallow copy construction
  FWExpression(const FWExpression &);

  /// The underlying DataSet
  FitWorkspace * workSpace;

  /// The internal expression object !
  Expression * expr;

  /// The real formula
  QString realFormula;

  /// The fit parameters, tweaked
  QStringList fitParameters;


  /// The overall final parameters
  QStringList finalParameters;

  

  /// The list of extra parameters
  QStringList extraParameters;
public:

  /// Creates an expression object
  FWExpression(const QString & expression,
               FitWorkspace * ws, const QStringList & extra = QStringList());

  /// Frees up all associated storage
  ~FWExpression();

  /// Evaluates the expression. As a double.
  ///
  /// If dataset is -1, the current dataset is used.
  mrb_value evaluate(int dataset = -1, const double * extra = NULL);


  double evaluateAsDouble(int dataset = -1, const double * extra = NULL);

};

#endif
