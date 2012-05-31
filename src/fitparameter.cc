/*
  fitparameter.cc: implementation of FitParameter and derived classes
  Copyright 2011, 2012 by Vincent Fourmond

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
#include <fitparameter.hh>
#include <fit.hh>
#include <fitdata.hh>
#include <utils.hh>
#include <bijection.hh>


void FitParameter::initialize(FitData * /*data*/)
{
  
}

void FitParameter::copyToPacked(gsl_vector * /*fit*/, 
                                const double * /*unpacked*/,
                                int /*nbdatasets*/, 
                                int /*nb_per_dataset*/) const
{
  // Nothing to do !
}


QString FitParameter::saveExtraInfo() const
{
  return QString();
}

void FitParameter::loadExtraInfo(const QString & /*str*/)
{
  // Nothing to do by default
}

QString FitParameter::saveAsString(double value) const
{
  QString str = QString("%1\t!\t%2").
    arg(textValue(value)).arg(fixed() ? "0" : "1");
  QString extra = saveExtraInfo();
  if(! extra.isEmpty()) 
    str += "\t!\t" + extra;
  return str;
}

FitParameter * FitParameter::loadFromString(const QString & str, 
                                            double * target, 
                                            int paramIndex, int dsIndex)
{
  QStringList lst = str.split("\t!\t");
  if(lst.size() < 2) 
    throw RuntimeError(QString("Invalid parameter specification '%1'").
                       arg(Utils::abbreviateString(str)));
  
  bool fixed = (lst[1] == "0");

  FitParameter * p;
  
  if(fixed) {
    if(lst[0].startsWith("=")) {
      FormulaParameter * fm = 
        new FormulaParameter(paramIndex, dsIndex,lst[0].mid(1));
      p = fm;
    }
    else {
      FixedParameter * pm = new FixedParameter(paramIndex, dsIndex, 0);
      pm->setValue(target, lst[0]);
      pm->value = *target;        /// @todo This should move to setValue
      p = pm;
    }
  }
  else {
    FreeParameter * pm = new FreeParameter(paramIndex, dsIndex);
    pm->setValue(target, lst[0]);
    p =  pm;
  }
  p->loadExtraInfo(lst.value(2, QString()));
  return p;
}

FitParameter::~FitParameter()
{
}

//////////////////////////////////////////////////////////////////////



void FreeParameter::copyToUnpacked(double * target, const gsl_vector * fit, 
                                  int nb_datasets, int nb_per_dataset) const
{
  /// @todo check that fitIndex is positive ??
  double value = gsl_vector_get(fit, fitIndex);

  if(bijection)
    value = bijection->forward(value);
  if(dsIndex >= 0)
    target[paramIndex + dsIndex * nb_per_dataset] = value;
  else
    for(int j = 0; j < nb_datasets; j++)
      target[paramIndex + j * nb_per_dataset] = value;
}

void FreeParameter::copyToPacked(gsl_vector * target, const double * unpacked,
                                 int /*nbdatasets*/, int nb_per_dataset) const
{
  double value = unpacked[paramIndex + (dsIndex < 0 ? 0 : dsIndex) * 
                          nb_per_dataset];
  if(bijection)
    value = bijection->backward(value);

  gsl_vector_set(target, fitIndex, value);
}

QString FreeParameter::saveExtraInfo() const
{
  if(bijection)
    return bijection->saveAsText();
  return QString();
}

void FreeParameter::loadExtraInfo(const QString & str)
{
  delete bijection;
  bijection = Bijection::loadFromText(str); // Can return NULL, that
                                            // isn't a problem.
}

FreeParameter::~FreeParameter()
{
  delete bijection;             // This is where the fun starts...
}

FitParameter * FreeParameter::dup() const
{
  FreeParameter *p = new FreeParameter(*this);
  if(bijection)
    p->bijection = bijection->dup();
  return p;
}

//////////////////////////////////////////////////////////////////////

void FixedParameter::copyToUnpacked(double * target, const gsl_vector * fit, 
                                   int nb_datasets, int nb_per_dataset) const
{
  if(dsIndex >= 0)
    target[paramIndex + dsIndex * nb_per_dataset] = value;
  else
    for(int j = 0; j < nb_datasets; j++)
      target[paramIndex + j * nb_per_dataset] = value;
}

void FixedParameter::copyToPacked(gsl_vector * target, const double * unpacked,
                                  int /*nbdatasets*/, int nb_per_dataset) const
{
  // We retrieve the value from the unpacked parameters
  value = unpacked[paramIndex + (dsIndex < 0 ? 0 : dsIndex) * 
                   nb_per_dataset];
}

//////////////////////////////////////////////////////////////////////

void FormulaParameter::initialize(FitData * data)
{
  if(! needsUpdate)
    return;

  QStringList parameters;
  for(int i = 0; i < data->parameterDefinitions.size(); i++)
    parameters << data->parameterDefinitions[i].name;

  QString exp2 = Expression::rubyIzeExpression(formula, parameters);
  QTextStream o(stdout);
  o << "Tweaked expression: '" << formula << "' -> '" << exp2 << endl;

  delete expression;
  expression = new Expression(exp2);


  // Hmmmm... This means that all the fancy variables with # signs
  // inside will be delicate to handle...
  dependencies = expression->naturalVariables();

  // <unnecessary> I think all this is completely unnecessary...
  for(int j = 0; j < dependencies.size(); j++) {
    int idx = parameters.indexOf(dependencies[j]);
    if(idx < 0)
      throw RuntimeError(QString("In definition of parameter %1: "
                                 "'%2' isn't a parameter name !").
                         arg(data->parameterDefinitions[paramIndex].name).
                         arg(dependencies[j]));
    depsIndex << idx;
  }
  // </unnecessary>
  
  expression->setVariables(parameters);
  needsUpdate = false;
}



void FormulaParameter::copyToUnpacked(double * target, const gsl_vector * fit, 
                                      int nb_datasets, int nb_per_dataset) const
{
  if(dsIndex >= 0) {
    lastValue = expression->evaluate(target + dsIndex * nb_per_dataset);
    target[paramIndex + dsIndex * nb_per_dataset] = lastValue;
  }
  else {
    for(int j = 0; j < nb_datasets; j++) {
      lastValue = expression->evaluate(target + j * nb_per_dataset);
      target[paramIndex + j * nb_per_dataset] = lastValue;
    }
  }
}

void FormulaParameter::copyToPacked(gsl_vector * target, const double * unpacked,
                                    int /*nbdatasets*/, int nb_per_dataset) const
{
  lastValue = unpacked[paramIndex + (dsIndex < 0 ? 0 : dsIndex) * 
                       nb_per_dataset]; // necessary ??
}

void FormulaParameter::setValue(double *, const QString & value)
{
  formula = value.mid(1);
  needsUpdate = true;
}

FormulaParameter::~FormulaParameter()
{
  delete expression;
}

QString FormulaParameter::textValue(double ) const
{
  return "=" + formula;
};
