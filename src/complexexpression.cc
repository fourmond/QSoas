/*
  complexexpression.cc: implementation of the mathematical complexexpressions
  Copyright 2012, 2013, 2014 by CNRS/AMU

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
#include <complexexpression.hh>
#include <exceptions.hh>

#include <mruby.hh>

#include <utils.hh>


void ComplexExpression::buildArgs()
{
  MRuby * mr = MRuby::ruby();
  delete[] args;
  argsSize = effectiveMinVars.size();
  args = new mrb_value[argsSize];

  gsl_complex a;
  GSL_SET_COMPLEX(&a, 0, 0);
  for(int i = 0; i < argsSize; i++) {
    if(i == 0)
      args[i] = mr->newComplex(a);
    else
      args[i] = mr->newFloat(0.0);
    guard.protect(args[i]);
  }

  // Update the cache
  //
  // NOTE that this DOES NOT reflect the complex variable
  delete[] indexInVariables;
  indexInVariables = new int[argsSize];
  for(int i = 0; i < argsSize; i++) {
    indexInVariables[i] = -1;
    for(int j = 0; j < variables.size(); j++) {
      if(variables[j] == effectiveMinVars[i]) {
        indexInVariables[i] = j;
        break;
      }
    }
    if(indexInVariables[i] < 0)
      throw InternalError("One of the natural variables, '%1' was not found, shouldn't happen. Variables: %2").arg(effectiveMinVars[i]).arg(variables.join(","));
    
  }
}

void ComplexExpression::buildCode()
{
  Expression::buildCode();
  buildArgs();
}


ComplexExpression::ComplexExpression(const QString & variable,
                                     const QString & expr) :
  Expression(expr)
{
  effectiveMinVars = minimalVariables;
  effectiveMinVars.insert(0, variable);
  Utils::makeUnique(effectiveMinVars);
  Expression::setVariables(effectiveMinVars);
  buildArgs();
}


void ComplexExpression::setVariables(const QStringList & vars)
{
  variables = vars;
  variables.insert(0, variable);
  Utils::makeUnique(variables);
  buildCode();
}


mrb_value ComplexExpression::rubyEvaluation(const double * values) const
{
  // Should this be cached at the Complexexpression level ?
  MRuby * mr = MRuby::ruby();
  for(int i = 0; i < argsSize; i++) {
    if(i == 0) {
      gsl_complex * c = mr->complexInternal(args[i]);
      GSL_SET_COMPLEX(c, values[0], values[1]);
    }
    else
      // +1 since the first variable takes two spots
      SET_FLOAT_VALUE(mr->mrb, args[i], values[indexInVariables[i] + 1]);
  }

  mrb_value rv = mr->funcall(code, callSym(), argsSize, args);
  return rv;
}

mrb_value ComplexExpression::evaluateAsRuby(const double * values) const
{
  return rubyEvaluation(values);
}

gsl_complex ComplexExpression::evaluate(const double * values) const
{
  MRuby * mr = MRuby::ruby();
  MRubyArenaContext c(mr);  
  return mr->complexValue(rubyEvaluation(values));
}


void ComplexExpression::reverseLaplace(const double * parameters,
                                       const double * xvalues,
                                       gsl_vector * target,
                                       int steps)
{
  /// We sum:
  /// $$\frac{h}{2\pi i} \sum_{k=-N}{N} exp(z(u_k)t) F(z(u_k)) z'(u_k)$$
  ///
  /// With
  /// $$z(u_k) = \mu (i u +1)^2$$
  /// and thus:
  /// $$z'(u_k) = 2 \mu i (i u +1)$$
  ///
  /// So that:
  /// $$z'(u_k) \times \frac{h}{2\pi i} = \mu h (i u +1)/\pi$$

  int nbt = target->size;
  int cur_k = 0;

  int zero_index = -1;
  MRuby * mr = MRuby::ruby();

  // QTextStream o(stdout);

  // Copying the parameters
  double argvs[variables.size() + 1];
  for(int i = 0; i < variables.size() - 1; i++)
    argvs[i+2] = parameters[i];

  for(int i = 0; i < nbt; i++) {
    double t = xvalues[i];
    if(t < 0)
      continue;
    if(t == 0) {
      zero_index = i;
      continue;
    }
    double step = 3.0/steps;
    double mu = M_PI * steps/(12*t);
    double val = 0;
    // o << "Dealing with time t = " << t
    //   << "\n -> step = " << step << "\tmu = " << mu << endl;

    for(int cur_k = 0; cur_k < steps; cur_k++) {
      // No optimization for now
      gsl_complex zv, zp, fn, ex;
      double args[2];
      double uk = step*cur_k;
      GSL_SET_COMPLEX(&zv, 1, uk);
      zv = gsl_complex_mul(zv, zv);
      zv = gsl_complex_mul_real(zv, mu);
      argvs[0] = GSL_REAL(zv);
      argvs[1] = GSL_IMAG(zv);
      fn = evaluate(argvs);

      // o << "k:\t" << cur_k << "\n -> Z = " << args[0]
      //   << "\t+ " << args[1] << "\tI\n"
      //   << " -> F = " << GSL_REAL(fn) << "\t+ "
      //   << GSL_IMAG(fn) << "\tI" << endl;

      GSL_SET_COMPLEX(&zp, step * mu/M_PI, step * mu*uk/M_PI);
      ex = gsl_complex_mul_real(zv, t);
      ex = gsl_complex_exp(ex);
      ex = gsl_complex_mul(ex, fn);
      ex = gsl_complex_mul(ex, zp);
      if(cur_k > 0)
        val += 2 * GSL_REAL(ex);
      else
        val = GSL_REAL(ex);
    }
    gsl_vector_set(target, i, val);
  }

  // Limit at t = 0
  if(zero_index >= 0) {
    // We multiply by two until the value doesn't change that much
    double args[2];
    double curx = 1e4;
    argvs[1] = 0;
    argvs[0] = 0;

    int nb = 0;
    double prev = 0, cur = 0;

    // o << "Looking at t = 0" << endl;
    while(nb < 30) {
      argvs[0] = curx;
      argvs[1] = 0;
      gsl_complex z = evaluate(argvs);
      double fnv = GSL_REAL(z);
      cur = curx * fnv;
      // o << "Iteration " << nb << ": arg\t" << curx
      //   << "\n -> val " << fnv
      //   << "\t -> limit " << cur << endl;
      // if(nb > 0) {
      //   if(fabs(prev/cur) < 1e-5 * fabs(cur))
      //     break;
      // }

      curx *= 2;
      prev = cur;
      nb++;
    }
    gsl_vector_set(target, zero_index, cur);
    // ny[zero_index] = cur;
  }

}
