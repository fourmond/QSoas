/*
  kineticsystem.cc: implementation of KineticSystem
  Copyright 2012, 2013, 2014, 2015 by CNRS/AMU

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
#include <kineticsystem.hh>

#include <expression.hh>
#include <exceptions.hh>
#include <utils.hh>

#include <gsl/gsl_const_mksa.h>

KineticSystem::Reaction::Reaction(const QString & fd, const QString & bd) :
  forwardRate(fd), backwardRate(bd), forward(NULL), backward(NULL), 
  electrons(0)
{
}


void KineticSystem::Reaction::makeExpressions(const QStringList & vars)
{
  clearExpressions();
  forward = new Expression(forwardRate);
  if(! backwardRate.isEmpty())
    backward = new Expression(backwardRate);
  if(! vars.isEmpty()) {
    forward->setVariables(vars);
    if(backward)
      backward->setVariables(vars);
  }
}

QSet<QString> KineticSystem::Reaction::parameters() const
{
  
  if(! forward)
    return QSet<QString>();
  QSet<QString> params = QSet<QString>::fromList(forward->naturalVariables());
  if(backward)
    params += QSet<QString>::fromList(backward->naturalVariables());
  return params;
}


void KineticSystem::Reaction::clearExpressions() 
{
  delete forward;
  forward = NULL;
  delete backward;
  backward = NULL;
}

void KineticSystem::Reaction::setParameters(const QStringList & parameters)
{
  if(! forward)
    return;
  forward->setVariables(parameters);
  if(! backward)
    return;
  backward->setVariables(parameters);
}

void KineticSystem::Reaction::computeRateConstants(const double * vals, 
                                           double * fd, 
                                           double * bd) const
{
  *fd = forward->evaluate(vals);
  if(backward)
    *bd = backward->evaluate(vals);
  else
    *bd = 0;
}

QString KineticSystem::Reaction::toString(const QList<Species> & species) const
{
  QString str;
  QStringList reactants;
  QStringList products;
  for(int j = 0; j < speciesIndices.size(); j++) {
    int idx = speciesIndices[j];
    int s = speciesStoechiometry[j];
    QString n = QString("%1 %2").arg(abs(s)).arg(species[idx].name);
    if(s > 0)
      products << n;
    else
      reactants << n;
  }
  return reactants.join(" + ")  + 
    (backwardRate.isEmpty() ? " -> " : " <=> ") + 
    products.join(" + ") + " -- forward: " + 
    forwardRate + " -- backward: " + backwardRate;
}

QString KineticSystem::Reaction::exchangeRate() const
{
  return QString();
}

bool KineticSystem::Reaction::isLinear() const
{
  if(speciesIndices.size() != 2)
    return false;
  if(abs(speciesStoechiometry[0]) != 1 || 
     abs(speciesStoechiometry[1]) != 1)
    return false;
  
  if(!forward->isAVariable())
    return false;
  if(! backward)
    return true;
  return backward->isAVariable();
}

//////////////////////////////////////////////////////////////////////


KineticSystem::RedoxReaction::RedoxReaction(int els, const QString & e0, 
                                            const QString & k0) :
  Reaction(e0, k0)
{
  electrons = els;
}

QSet<QString> KineticSystem::RedoxReaction::parameters() const
{
  QSet<QString> params = Reaction::parameters();
  params += "e";                // We always need the potential !
  params += "temperature";                // We always need the potential !
  return params;
}

void KineticSystem::RedoxReaction::setParameters(const QStringList & parameters)
{
  Reaction::setParameters(parameters);
  potentialIndex = -1;
  temperatureIndex = -1;
  for(int i = 0; i < parameters.size(); i++) {
    if(parameters[i] == "e") {
      potentialIndex = i;
    }
    if(parameters[i] == "temperature") {
      temperatureIndex = i;
    }
  }
  if(potentialIndex < 0)
    throw InternalError("Somehow there is no 'e' parameter "
                        "for a redox reaction");
  if(temperatureIndex < 0)
    throw InternalError("Somehow there is no 'temperature' parameter "
                        "for a redox reaction");
}

void KineticSystem::RedoxReaction::computeRateConstants(const double * vals, 
                                                double * fd, 
                                                double * bd) const
{
  double e0, k0;                // e0 is forward, k0 is backward
  Reaction::computeRateConstants(vals, &e0, &k0);
  double e = vals[potentialIndex];
  double fara = GSL_CONST_MKSA_FARADAY /
    (vals[temperatureIndex] * GSL_CONST_MKSA_MOLAR_GAS);

  double ex = exp(fara * 0.5 * electrons * (e - e0));

  *fd = k0 * ex;
  *bd = k0 / ex;
}

QString KineticSystem::RedoxReaction::exchangeRate() const
{
  return backwardRate;
}



//////////////////////////////////////////////////////////////////////

KineticSystem::KineticSystem() : 
  redoxReactionScaling(1), reporterExpression(NULL), linear(false)
{
}

KineticSystem::~KineticSystem()
{
  for(int i = 0; i < reactions.size(); i++)
    delete reactions[i];
  delete reporterExpression;
}

int KineticSystem::speciesNumber() const
{
  return species.size();
}

int KineticSystem::speciesIndex(const QString & str)
{
  if(speciesLookup.contains(str))
    return speciesLookup[str];
  
  int sz = species.size();
  species.append(Species(str));
  speciesLookup[str] = sz;
  return sz;
}

void KineticSystem::addReaction(QList<QString> species, 
                                QList<int> stoechiometry,
                                int els, 
                                const QString & forward, 
                                const QString & backward)
{
  parameters.clear();
  int reactionIndex = reactions.size();
  Reaction * reac = (els ? 
                     new RedoxReaction(els, forward, backward) 
                     :
                     new Reaction(forward, backward));
  for(int i = 0; i < species.size(); i++) {
    int spi = speciesIndex(species[i]);
    reac->speciesIndices.append(spi);
    this->species[spi].reactions.append(reactionIndex);
    reac->speciesStoechiometry.append(stoechiometry[i]);
  }
  reactions.append(reac);
}

void KineticSystem::ensureReady(const QStringList & add)
{
  QSet<QString> params;
  
  for(int i = 0; i < reactions.size(); i++) {
    reactions[i]->makeExpressions();
    params += reactions[i]->parameters();
  }

  if(reporterExpression)
    params += QSet<QString>::fromList(reporterExpression->currentVariables());


  // We remove the concentrations
  QStringList conc;
  for(int i = 0; i < species.size(); i++) {
    QString str = QString("c_%1").arg(species[i].name);
    params.remove(str);
    conc.append(str);
  }

  for(int i = 0; i < add.size(); i++)
    params.remove(add[i]);

  parameters.clear();
  parameters = params.toList();
  qSort(parameters);

  // First concentrations, then additional parameters, then the
  // remaining parameters.
  parameters = conc + add + parameters;

  for(int i = 0; i < reactions.size(); i++)
    reactions[i]->setParameters(parameters);
  if(reporterExpression)
    reporterExpression->setVariables(parameters);

  checkLinearity();
}

void KineticSystem::checkLinearity()
{
  // Is this a linear system ?
  linear = true;
  for(int i = 0; i < reactions.size(); i++) {
    if(! reactions[i]->isLinear()) {
      linear = false;
      break;
    }
  }
}

void KineticSystem::prepareForTimeEvolution(const QStringList & extra)
{
  // initial concentrations
  QStringList conc0;
  for(int i = 0; i < species.size(); i++)
    conc0.append(QString("c0_%1").arg(species[i].name));
  conc0 << extra;

  ensureReady(conc0);
}


void KineticSystem::prepareForSteadyState(const QStringList & extra)
{
  // initial concentrations
  QStringList params;
  params << "temperature" << "e" << "c_tot";
  params << extra;

  ensureReady(params);
}

QStringList KineticSystem::allParameters() const
{
  if(parameters.isEmpty())
    throw InternalError("Looks like you called allParameters() on an "
                        "unready KineticSystem object");

  // We only return the "external parameters", ie we leave the current
  // concentrations out.
  return parameters.mid(species.size());
}

QStringList KineticSystem::allSpecies() const
{
  QStringList ret;
  for(int i = 0; i < species.size(); i++)
    ret += species[i].name;
  return ret;
}

void KineticSystem::setInitialConcentrations(double * target, 
                                             const double * parameters) const
{
  for(int i = 0; i < species.size(); i++)
    target[i] = parameters[i];
}

double KineticSystem::computeDerivatives(double * target, 
                                       const double * concentrations,
                                       const double * params) const
{
  gsl_vector_view tg = gsl_vector_view_array(target, species.size());
  gsl_vector_const_view conc = 
    gsl_vector_const_view_array(concentrations, species.size());
  return computeDerivatives(&tg.vector, &conc.vector, params);
}

void KineticSystem::computeLinearJacobian(gsl_matrix * target,
                                          const double * params) const
{
  if(! linear)
    throw InternalError("Using a function for linear systems on non-linear ones !");
  // Now starts the real fun: evaluating all the derivatives !
  // QVarLengthArray<double, 800> vals(parameters.size());

  int nbs = species.size();
  int nbp = parameters.size();
  int nbr = reactions.size();


  params -= nbs;
  
  gsl_matrix_set_zero(target);

  // Now, we compute the forward and reverse rates of all reactions
  for(int i = 0; i < nbr; i++) {
    const Reaction * rc = reactions[i];
    double forwardRate, backwardRate;
    rc->computeRateConstants(params, &forwardRate, &backwardRate);
    int l = (rc->speciesStoechiometry[0] == -1 ? rc->speciesIndices[0] : rc->speciesIndices[1]);
    int r = (rc->speciesStoechiometry[0] == -1 ? rc->speciesIndices[1] : rc->speciesIndices[0]);
    gsl_matrix_set(target, l, l, gsl_matrix_get(target, l, l) - forwardRate);
    gsl_matrix_set(target, r, l, gsl_matrix_get(target, r, l) + forwardRate);
    gsl_matrix_set(target, r, r, gsl_matrix_get(target, r, r) - backwardRate);
    gsl_matrix_set(target, l, r, gsl_matrix_get(target, l, r) + backwardRate);
  }
}


double KineticSystem::computeDerivatives(gsl_vector * target, 
                                       const gsl_vector * concentrations,
                                       const double * params) const
{
  int nbParams = parameters.size();
  int nbSpecies = species.size();
  int nbReactions = reactions.size();
  // Now starts the real fun: evaluating all the derivatives !
  QVarLengthArray<double, 800> vals(nbParams);

  // We put all the eggs in the same basket.
  for(int i = 0; i < nbSpecies; i++) {
    vals[i] = gsl_vector_get(concentrations, i);
    if(target)
      gsl_vector_set(target, i, 0);
  }
  for(int i = nbSpecies; i < nbParams; i++)
    vals[i] = params[i - nbSpecies];

  double current = 0;

  // Now, we compute the forward and reverse rates of all reactions
  for(int i = 0; i < nbReactions; i++) {
    const Reaction * r = reactions[i];
    double forwardRate, backwardRate;

    // Do not compute if we don't need !
    if(! target && r->electrons == 0)
      continue;

    r->computeRateConstants(vals.data(), &forwardRate, &backwardRate);

    int sts = r->speciesStoechiometry.size();
    const int * stoech = r->speciesStoechiometry.data();
    const int * indices = r->speciesIndices.data();

    
    for(int j = 0; j < sts; j++) {
      int s = stoech[j];
      if(s < 0)
        forwardRate *= 
          gsl_pow_int(gsl_vector_get(concentrations, indices[j]), -s);
      else
        backwardRate *= 
          gsl_pow_int(gsl_vector_get(concentrations, indices[j]), s);
    }
    double rate = forwardRate - backwardRate;
    // We scale the rate of redox reactions
    if(r->electrons) {
      rate *= redoxReactionScaling;
      current += r->electrons * rate;
    }

    if(target) {
      for(int j = 0; j < sts; j++) {
        int idx = indices[j];
        gsl_vector_set(target, idx,
                       gsl_vector_get(target, idx) +
                       + stoech[j] * rate);
      }
    }
  }
  return current;
}


void KineticSystem::parseFile(const QString & fileName)
{
  QFile f(fileName);
  Utils::open(&f, QIODevice::ReadOnly);
  parseFile(&f);
}


/// Parses a reactant list
static void parseReactants(const QString & reactants,
                           QStringList * target,
                           QList<int> * stoechiometry,
                           int sign = 1,
                           int line = 0)
{
  QString s = reactants.trimmed();
  QStringList in = s.split(QRegExp("\\s*\\+\\s*"));
  QRegExp one("^(\\d+)?\\s*(\\w+|e-)$");
  for(int i = 0; i < in.size(); i++) {
    if(one.indexIn(in[i]) != 0)
      throw RuntimeError(QString("invalid species: '%1' at line %2").
                         arg(in[i]).arg(line));
    target->append(one.cap(2));
    if(one.cap(1).isEmpty())
      stoechiometry->append(sign);
    else
      stoechiometry->append(one.cap(1).toInt() * sign);
  }
}

void KineticSystem::parseFile(QIODevice * device)
{

  
  QTextStream in(device);

  QRegExp blankRE("^\\s*(#|$)");
  QRegExp reactionRE("^\\s*(.*)(<=>|->)\\s*(.*)$");

  QRegExp reporterRE("^\\s*y\\s*=\\s*(.*)$");

  QRegExp doubleBraceRE("\\[\\[(.*)\\]\\]");
  doubleBraceRE.setMinimal(true);

  QRegExp singleBraceRE("\\[([^\\]]*)\\]");
  
  int number = 0;

  int reaction = 0;
  while(true) {
    QString line = in.readLine();
    QString orig = line;
    ++number;
    if(line.isNull())
      break;
    if(blankRE.indexIn(line) == 0)
      continue;

    if(reporterRE.indexIn(line) == 0) {
      if(reporterExpression)
        throw RuntimeError("Trying to set two reporters");
      reporterExpression = new Expression(reporterRE.cap(1));
      continue;
    }

    // The groups containing the parameter expressions.
    QStringList literals;

    // We strip the line of those before.

    // To allow for complex expressions, if there is an occurrence of
    // [[ in the line, we only capture groups within [[ ]] (which are
    // not Ruby-specific idioms)


    if(line.contains("[["))
      literals = Utils::extractMatches(line, doubleBraceRE, 1);
    else 
      literals = Utils::extractMatches(line, singleBraceRE, 1);

    
    if(reactionRE.indexIn(line) ==  0) {
      ++reaction;
      QString left = reactionRE.cap(1);
      QString right = reactionRE.cap(3);
      bool reversible = (reactionRE.cap(2) == "<=>");

      QString fr;
      QString br;

      QStringList reactants;
      QList<int> stoechiometry;

      parseReactants(left, &reactants, &stoechiometry, -1, number);
      parseReactants(right, &reactants, &stoechiometry, 1, number);

      int els = 0;

      for(int i = 0; i < reactants.size(); i++) {
        if(reactants[i] == "e-") {
          if(els != 0)
            throw RuntimeError("Line %1: '%2' -- electrons cannot appear "
                               "more than once in a reaction").
              arg(number).arg(orig);
          els = stoechiometry[i];
          reactants.takeAt(i);
          stoechiometry.takeAt(i);
          --i;
        }
      }

      if(literals.size() > 0)
        fr = literals.takeFirst();
      else 
        fr = QString(els != 0 ? "e0_%1" : "k_%1").arg(reaction);
      if(reversible) {
        if(literals.size() > 0)
          br = literals.takeFirst();
        else 
          br = QString(els != 0 ? "k0_%1" : "k_m%1").arg(reaction);
      }

      addReaction(reactants, stoechiometry, els, fr, br);
    }
    else
      throw RuntimeError(QString("Line %1: '%2' not valid").
                         arg(number).arg(orig));
  }

}

void KineticSystem::dump(QTextStream & o) const
{
  o << "Species: \n";
  for(int i = 0; i < species.size(); i++)
    o << " * " << species[i].name << "\n";

  o << "Reactions: \n";
  for(int i = 0; i < reactions.size(); i++)
    o << " * " << reactions[i]->toString(species) << "\n";

  o << "System is " << (linear ? "linear" : "non-linear") << "\n";
}

QString KineticSystem::toString() const
{
  QString s;
  QTextStream o(&s);
  dump(o);
  return s;
}


QSet<QString> KineticSystem::exchangeRates() const
{
  QSet<QString> ret;

  for(int i = 0; i < reactions.size(); i++) {
    QString er = reactions[i]->exchangeRate();
    if(er.isEmpty())
      continue;
    ret << er.trimmed();
  }

  return ret;
}
