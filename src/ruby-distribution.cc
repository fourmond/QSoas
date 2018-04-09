/*
  ruby-distribution.cc: parameter distributions defined by ruby code
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
#include <distribfit.hh>
#include <expression.hh>

#include <gsl/gsl_integration.h>

#include <command.hh>
#include <argumentlist.hh>
#include <general-arguments.hh>
#include <commandeffector-templates.hh>

static double lambda_integrand(double x, void * params)
{
  std::function<double (double)> * f =
    reinterpret_cast<std::function<double (double)> *>(params);
  return (*f)(x);
}


class MRubyDistribution : public Distribution {
  gsl_integration_workspace * ws;

  /// List of the parameter name suffixes. They also are the names
  /// used in the expressions.
  QStringList parameterSuffixes;


  /// Lower value of the integration range
  Expression * rangeMin;

  /// Upper value of the integration range
  Expression * rangeMax;

  /// Expression for the weight
  Expression * weightExpression;

public:

  MRubyDistribution(const QString & name,
                    const QStringList & params, const QString & weight,
                    const QString & min, const QString & max) :
    Distribution(name)
  {
    parameterSuffixes = params;
    QStringList ps = params;
    ps.insert(0, "x");
    rangeMin = new Expression(min, params);
    rangeMax = new Expression(max, params);
    weightExpression = new Expression(weight, ps);
    ws = gsl_integration_workspace_alloc(400);
  }

  ~MRubyDistribution() {
    delete rangeMin;
    delete rangeMax;
    delete weightExpression;
    gsl_integration_workspace_free(ws);
  }
  

  virtual QList<ParameterDefinition> parameters(const QString & param)
    const override {
    QList<ParameterDefinition> ret;
    for(const QString n : parameterSuffixes)
      ret << ParameterDefinition(QString("%1_%2").arg(param).arg(n));
    return ret;
  };

  void range(const double * parameters, double * first,
             double * last) const override {
    *first = rangeMin->evaluate(parameters);
    *last = rangeMax->evaluate(parameters);
  };

  
  virtual double weight(const double * parameters, double x) const override {
    double ps[parameterSuffixes.size() + 1];
    ps[0] = x;
    for(int i = 0; i < parameterSuffixes.size(); i++)
      ps[i+1] = parameters[i];
    return weightExpression->evaluate(ps);
  };
  

  double rangeWeight(const double * parameters) const
  {
    double l, r;
    range(parameters, &l, &r);
    
    std::function<double (double)> fn = [this, parameters](double x) -> double {
      return weight(parameters, x);
    };
  
    gsl_function f;
    f.function = &::lambda_integrand;
    f.params = &fn;
    double rv;
    double er;
    gsl_integration_qag(&f, l, r, 1e-5, 1e-5, 400, GSL_INTEG_GAUSS31, ws,
                        &rv, &er);
    return rv;
  }

  virtual void initialGuess(double * parameters, double value) const {
    for(int i = 0; i < parameterSuffixes.size(); i++)
      parameters[i] = i ? 1 : value;
  };

  
};


//////////////////////////////////////////////////////////////////////
// Now, the command !

static void defineDistribution(const QString &, QString name,
                               QStringList parameters,
                               QString weight,
                               QString min,
                               QString max,
                               const CommandOptions & opts)
{
  new MRubyDistribution(name, parameters, weight, min, max);
}

static ArgumentList 
dfA(QList<Argument *>() 
    << new StringArgument("name", "Name",
                          "name of the new distribution")
    << new SeveralStringsArgument("parameters", "Parameters",
                                  "parameters of the distribution")
    << new StringArgument("weight", "Weight",
                          "expression for the weight")
    << new StringArgument("left", "Left bound",
                          "expression for the left boundary")
    << new StringArgument("right", "Right bound",
                          "expression for the right boundary")
    );

// static ArgumentList 
// dfO(QList<Argument *>() 
//     );

static Command 
df("define-distribution", // command name
    effector(defineDistribution), // action
    "fits",  // group name
    &dfA, // arguments
    NULL, // options
    "Define new parameter distribution",
    "Defines a new parameter distribution");
