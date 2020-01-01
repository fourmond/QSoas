/**
   \file msimplexfitengine.cc
   A Major/minro Nelder-Mead fit engine for QSoas
   Copyright 2013, 2018, 2019 by CNRS/AMU

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
#include <simplex.hh>

#include <fitengine.hh>
#include <vector.hh>

#include <argument.hh>
#include <general-arguments.hh>

#include <utils.hh>
#include <debug.hh>


// Helper function
static Vector linearCombination(double f1, const Vector & v1, double f2, const Vector & v2)
{
  Vector rv = v1;
  for(int i = 0; i < v1.size(); i++)
    rv[i] = f1*v1[i] + f2*v2[i];
  return rv;
}

Simplex::Simplex(Function fun) :
  function(fun),
  alpha(1),
  beta(2),
  gamma(0.5),
  delta(0.5),
  threshold(1e-4),
  debug(0)
{
}


                               

StoredParameters Simplex::compute(const Vector & v) const
{
  double r = function(v);
  return StoredParameters(v, r);
}

void Simplex::sort()
{
  std::sort(vertices.begin(), vertices.end());
}
    
StoredParameters Simplex::stepTowards(const Vector & origin,
                                      const Vector & dest,
                                      double thresh) const
{
  double factor = 1;
  while(factor >= thresh) {
    Vector cur = ::linearCombination(factor, dest, 1 - factor, origin);      
    try {
      return compute(cur);
    }
    catch(RuntimeError & e) {
      // Try smaller
      factor *= 0.5;
    }
  }

  throw RuntimeError("Could not find suitable step");

  // Never reached ?
  return StoredParameters();
}

StoredParameters Simplex::initialVertex(const Vector & center, int idx,
                                        double factor) const
{
  Vector t = center;
  if(t[idx] != 0)
    t[idx] *= factor;
  else
    t[idx] = factor;
  try {
    return stepTowards(center, t);
  }
  catch(const RuntimeError & e) {
    if(t[idx] != 0)
      t[idx] = center[idx]/factor;
    else
      t[idx] = 1/factor;
  }
  return stepTowards(center, t);
}

int Simplex::iterate()
{
  Vector c = vertices[0].parameters;
  for(int i = 1; i < vertices.size(); i++)
    c += vertices[i].parameters;
  c /= vertices.size();
  return iterate(c);
}


int Simplex::iterate(const Vector & c)
{
  StoredParameters best = vertices.first();
  Vector t;
  if(debug > 0) {
    Debug::debug() << "Starting simplex iteration around centroid: "
                   << Utils::vectorString(c) << endl;
    int idx = 0;
    Debug::debug() << "Vertices: " << endl;
    for(const StoredParameters & s : vertices)
      Debug::debug() << " #" << idx++ << " : "
                     << s.toString() << endl;
  }

  ////////////////////////////////////////
  // Reflection step:
  // x_r = c + alpha*(c - x_n+1)
  t = ::linearCombination(1 + alpha, c, -alpha, vertices.last().parameters);

  /// @todo Big question: do we step from the centroid or from the best ?
  StoredParameters r = stepTowards(c, t);
  if(debug > 0)
    Debug::debug() << "Reflection step: " << r.toString() << endl;
  
  ////////////////////////////////////////
  // Expansion if the reflection is better than the best
  if(r < best) {
    if(debug > 0) 
      Debug::debug() << " -> accepted reflection" << endl;
    
    // x_e = c + beta * (x_r - c)
    t = ::linearCombination(1 - beta, c, beta, r.parameters);
    try {
      StoredParameters e = stepTowards(c, t);
      if(debug > 0)
        Debug::debug() << "Expansion step: " << e.toString() << endl;

      if(e < r)
        r = e;
    }
    catch(const RuntimeError &) {
    }
    vertices.last() = r;
  }
  else {
    // Reflection failed to improve the best.

    bool doShrink = false;
    StoredParameters nt;
    ////////////////////////////////////////
    // Outer contraction
    if(vertices[vertices.size()-1] < r) {
      // x_oc = c + gamma * (x_r - c)
      t = ::linearCombination(1 - gamma, c, gamma, r.parameters);
      nt = stepTowards(c, t);
      if(debug > 0)
        Debug::debug() << "Outer contraction: " << nt.toString() << endl;

      if(nt < r)
        vertices.last() = nt;
      else
        doShrink = true;
    }
    ////////////////////////////////////////
    // Inner contraction
    else {
      // x_ic = c - gamma * (x_r - c)
      t = ::linearCombination(1 + gamma, c, -gamma, r.parameters);
      nt = stepTowards(c, t);
      if(debug > 0)
        Debug::debug() << "Inner contraction: " << nt.toString() << endl;
      if(nt < vertices.last())
        vertices.last() = nt;
      else
        doShrink = true;
    }
    if(doShrink) {
      ////////////////////////////////////////
      // Shrink !
      if(debug > 0)
        Debug::debug() << "Performing shrink" << endl;
      for(int i = 1; i < vertices.size(); i++) {
        // x_i = x_1 + delta * (x_i - x_1)
        t = ::linearCombination(1 - delta, best.parameters,
                                delta, vertices[i].parameters);
        vertices[i] = stepTowards(best.parameters, t);
      }
    }
  }
  sort();

  // We didn't improve the best
  if(vertices[0].parameters == best.parameters)
    return GSL_CONTINUE;        // We didn't win

  int nb = best.parameters.size();
  for(int i = 0; i < nb; i++)
    if(fabs((vertices[0].parameters[i] - best.parameters[i])/
            best.parameters[i]) > threshold)
      return GSL_CONTINUE;

  return GSL_SUCCESS;
}

QList<Argument *> Simplex::simplexOptions()
{
  static QList<Argument *> simplexOptions(QList<Argument*>()
               << new NumberArgument("alpha", "Reflection factor")
               << new NumberArgument("beta", "Expansion factor")
               << new NumberArgument("gamma", "Contraction factor")
               << new NumberArgument("delta", "Shrink factor")
               << new NumberArgument("end-threshold", "Threshold for ending")
               ); 
  return simplexOptions;
}


CommandOptions Simplex::getParameters() const
{
  CommandOptions val;
  updateOptions(val, "alpha", alpha);
  updateOptions(val, "beta", beta);
  updateOptions(val, "gamma", gamma);
  updateOptions(val, "delta", delta);
  updateOptions(val, "end-threshold", threshold);

  return val;
}


void Simplex::setParameters(const CommandOptions & val)
{
  updateFromOptions(val, "alpha", alpha);
  updateFromOptions(val, "beta", beta);
  updateFromOptions(val, "gamma", gamma);
  updateFromOptions(val, "delta", delta);
  updateFromOptions(val, "end-threshold", threshold);
}
