/**
   \file rubysolver.hh
   The Rubysolver class, a one dimensional root finder
   Copyright 2013 by CNRS/AMU

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
#ifndef __RUBYSOLVER_HH
#define __RUBYSOLVER_HH

#include <solver.hh>

class Expression;

/// Solves a 1-dimensional equation from a string expression. The
/// variable is just the only variable.
class RubySolver : public Solver{
protected:

  Expression * formula;

  int nbVariables;

public:
  RubySolver(const QString & expr, 
             const gsl_root_fdfsolver_type * type = 
         gsl_root_fdfsolver_steffenson);

  virtual ~RubySolver();

  virtual double f(double x);
};



#endif
