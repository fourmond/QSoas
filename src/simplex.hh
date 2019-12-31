/**
   \file simplex.hh
   A simple simplex wrapper
   Copyright 2019 by CNRS/AMU

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

class Vector;
class StoredParameters;

class Argument;
#include <argumentmarshaller.hh>


/// The simplex algorithm in its simplest expression.
///
/// The function to minimize is simply provided via the function member.
///
/// The algorithm makes no assumption regarding the dimension. It is
/// up to the user to set up the initial vertices.
class Simplex {
public:

  /// @name Parameters
  ///
  /// The parameters of the simplex.
  ///
  /// @{

  typedef std::function<double (const Vector & params)> Function;
  
  /// The funtion to minimize.
  Function function;

  /// The n + 1 vertices
  QList<StoredParameters> vertices;

  /// The factor for computing a vertex from the initial parameters.
  double initialVertexFactor;

  /// Reflection factor
  double alpha;

  /// Expansion factor
  double beta;

  /// Contraction factor
  double gamma;

  /// Shrink factor
  double delta;

  /// Final threshold
  double threshold;

  /// @}

  Simplex(Function f);

  /// Compute the residuals at the given position
  StoredParameters compute(const Vector & v) const;


  /// Sorts the vertices.
  void sort();
    

  /// @name The algorithm
  ///
  /// Functions to carry out the minimization
  /// @{

  /// Makes a step towards the destination. If the parameters in @a
  /// dest are not acceptable, that is if function throws an
  /// exception, then the step is shortened until either the call to
  /// function succeeds or the step is shorter than thresh times the
  /// initial difference.
  /// 
  StoredParameters stepTowards(const Vector & origin, const Vector & dest,
                               double thresh = 1e-3) const;

  /// Creates an initial vertex by varying the parameter of index @a
  /// idx. Increases by default, decreases if no suitable parameter is
  /// found.
  StoredParameters initialVertex(const Vector & center, int idx,
                                 double factor = 2) const;

  /// Performs a single iteration, and returns GSL_CONTINUE if it should go on, or GSL_SUCCESS if the iteration has converged.
  int iterate();
  
  /// Performs a single iteration, and update the vertices.
  ///
  /// Assumes the vertices are sorted at the beginning of the
  /// iteration (they are sorted at the end).
  ///
  /// The iteration revolves around the given center. See iterate() to
  /// automatically compute the center.
  int iterate(const Vector & c);
  /// @}

  /// Returns the options suitable for setting
  static QList<Argument *> simplexOptions();

  /// Returns the CommandOptions corresponding to the current parameters
  ///
  /// @todo Add a prefix ?
  CommandOptions getParameters() const;

  /// Sets the parameters from the options
  void setParameters(const CommandOptions & opts);


  

};
