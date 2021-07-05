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

#include <idioms.hh>
#include <debug.hh>
#include <gsl/gsl_const_mksa.h>

// for the Marcus-Hush-Chidsey equation
#include <functions.hh>
#include <file.hh>






KineticSystem::Reaction::Reaction(const QString & fd, const QString & bd) :
  forwardRate(fd), backwardRate(bd), forward(NULL), backward(NULL), 
  electrons(0)
{
  
}

void KineticSystem::Reaction::computeCache(const double * /*vals*/)
{
  // Nothing to do here.
}


void KineticSystem::Reaction::makeExpressions(const QStringList & vars)
{
  clearExpressions();
  if(! forwardRate.startsWith("cycle:")) 
    forward = new Expression(forwardRate);
  if(! (backwardRate.isEmpty() || backwardRate.startsWith("cycle:")))
    backward = new Expression(backwardRate);
  if(! vars.isEmpty()) {
    if(forward)
      forward->setVariables(vars);
    if(backward)
      backward->setVariables(vars);
  }
}

QSet<QString> KineticSystem::Reaction::parameters() const
{

  QSet<QString> params;
  if(forward)
    params += QSet<QString>::fromList(forward->naturalVariables());
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


int KineticSystem::Reaction::stoechiometry(int species) const
{
  int fnd = -1;
  for(int i = 0; i < speciesIndices.size(); i++) {
    if(speciesIndices[i] == species) {
      fnd = i;
      break;
    }
  }
  if(fnd >= 0)
    return speciesStoechiometry[fnd];
  return 0;
}

QList<int> KineticSystem::Reaction::products() const
{
  QList<int> rv;
  for(int i = 0; i < speciesIndices.size(); i++) {
    if(speciesStoechiometry[i] > 0)
      rv << speciesIndices[i];
  }
  return rv;
}

QList<int> KineticSystem::Reaction::reactants() const
{
  QList<int> rv;
  for(int i = 0; i < speciesIndices.size(); i++) {
    if(speciesStoechiometry[i] < 0)
      rv << speciesIndices[i];
  }
  return rv;
}

bool KineticSystem::Reaction::isReversible() const
{
  return !(forwardRate.isEmpty() || backwardRate.isEmpty());
}


KineticSystem::Reaction::Reaction(const Reaction & o) :
  forwardRate(o.forwardRate), backwardRate(o.backwardRate),
  speciesIndices(o.speciesIndices),
  speciesStoechiometry(o.speciesStoechiometry),
  forwardCache(o.forwardCache), backwardCache(o.backwardCache),
  electrons(o.electrons)
{
  forward = o.forward ? new Expression(*o.forward) : NULL;
  backward = o.backward ? new Expression(*o.backward) : NULL;

}


KineticSystem::Reaction * KineticSystem::Reaction::dup() const
{
  return new Reaction(*this);
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

void KineticSystem::RedoxReaction::computeCache(const double * vals)
{
  double e0, k0;                // e0 is forward, k0 is backward
  Reaction::computeRateConstants(vals, &e0, &k0);
  // Debug::debug() << "Cache: " << k0 << " - " << e0 << endl;
  cache[1] = k0;
  double fara = GSL_CONST_MKSA_FARADAY /
    (vals[temperatureIndex] * GSL_CONST_MKSA_MOLAR_GAS);

  double ex = exp(-fara * 0.5 * electrons * e0);
  cache[0] = ex;
}

QString KineticSystem::RedoxReaction::exchangeRate() const
{
  return backwardRate;
}

KineticSystem::Reaction * KineticSystem::RedoxReaction::dup() const
{
  return new RedoxReaction(*this);
}

KineticSystem::RedoxReaction::RedoxReaction(const RedoxReaction & o) :
  Reaction(o), potentialIndex(o.potentialIndex),
  temperatureIndex(o.temperatureIndex)
{
  ;
}

//////////////////////////////////////////////////////////////////////

/// This reaction is a full Butler-Volmer reaction, with values of alpha
class BVRedoxReaction : public KineticSystem::RedoxReaction {

  /// Expression for the value of alpha
  QString alphaValue;

  /// and the corresponding Expression object
  Expression * alpha;


public:

  // We reuse the forward/backward stuff but with a different
  // meaning.
  BVRedoxReaction(int els, const QString & e0, const QString & k0, const QString & alph) :
    RedoxReaction(els, e0, k0), alphaValue(alph), alpha(NULL) {
  };

  /// A copy constructor
  BVRedoxReaction(const BVRedoxReaction & o) :
    RedoxReaction(o), alphaValue(o.alphaValue) {
    alpha = o.alpha ? new Expression(*o.alpha) : NULL;
  }

  virtual void makeExpressions(const QStringList & vars) override {
    Reaction::makeExpressions(vars);
    alpha = new Expression(alphaValue);
    if(! vars.isEmpty())
      alpha->setVariables(vars);
  }
  
  virtual void clearExpressions() override {
    Reaction::clearExpressions();
    delete alpha;
    alpha = NULL;
  }


  virtual void computeRateConstants(const double * vals, 
                                    double * fd, double * bd) const override
  {
    double e0, k0, al;
    e0 = forward->evaluate(vals);
    k0 = backward->evaluate(vals);
    al = alpha->evaluate(vals);

    double e = vals[potentialIndex];
    double fara = GSL_CONST_MKSA_FARADAY /
      (vals[temperatureIndex] * GSL_CONST_MKSA_MOLAR_GAS);

    *fd = k0 * exp(fara * al * electrons * (e - e0));
    *bd = k0 * exp(fara * (1 - al) * electrons * (e0 - e));
  }

  virtual Reaction * dup() const override {
    return new BVRedoxReaction(*this);
  }

  virtual QSet<QString> parameters() const {
    QSet<QString> params = KineticSystem::RedoxReaction::parameters();
    params += QSet<QString>::fromList(alpha->naturalVariables());
    return params;
  };

  void setParameters(const QStringList & parameters) override {
    KineticSystem::RedoxReaction::setParameters(parameters);
    alpha->setVariables(parameters);
  };

};

//////////////////////////////////////////////////////////////////////

/// This reaction is a full Butler-Volmer reaction, with values of alpha
class MarcusRedoxReaction : public KineticSystem::RedoxReaction {

  /// Expression for the value of alpha
  QString lambdaValue;

  /// and the corresponding Expression object
  Expression * lambda;


public:

  // We reuse the forward/backward stuff but with a different
  // meaning.
  MarcusRedoxReaction(int els, const QString & e0, const QString & k0, const QString & lambd) :
    RedoxReaction(els, e0, k0), lambdaValue(lambd), lambda(NULL) {
  };

  /// A copy constructor
  MarcusRedoxReaction(const MarcusRedoxReaction & o) :
    RedoxReaction(o), lambdaValue(o.lambdaValue) {
    lambda = o.lambda ? new Expression(*o.lambda) : NULL;
  }

  virtual void makeExpressions(const QStringList & vars) override {
    Reaction::makeExpressions(vars);
    lambda = new Expression(lambdaValue);
    if(! vars.isEmpty())
      lambda->setVariables(vars);
  }
  
  virtual void clearExpressions() override {
    Reaction::clearExpressions();
    delete lambda;
    lambda = NULL;
  }


  virtual void computeRateConstants(const double * vals, 
                                    double * fd, double * bd) const override
  {
    double e0, k0, lb;
    e0 = forward->evaluate(vals);
    k0 = backward->evaluate(vals);
    lb = lambda->evaluate(vals);

    double e = vals[potentialIndex];
    double fara = GSL_CONST_MKSA_FARADAY /
      (vals[temperatureIndex] * GSL_CONST_MKSA_MOLAR_GAS);

    // Normalizing factor so that k0 is always the exchange rate (at 0
    // overpotential)
    k0 /= Functions::marcusHushChidseyZeng(fara * lb, 0);

    *fd = k0 * Functions::marcusHushChidseyZeng(fara * lb, fara * electrons * (e-e0));
    *bd = k0 * Functions::marcusHushChidseyZeng(fara * lb, fara * electrons  * (e0-e));
  }

  virtual Reaction * dup() const override {
    return new MarcusRedoxReaction(*this);
  }

  virtual QSet<QString> parameters() const {
    QSet<QString> params = KineticSystem::RedoxReaction::parameters();
    params += QSet<QString>::fromList(lambda->naturalVariables());
    return params;
  };

  void setParameters(const QStringList & parameters) override {
    KineticSystem::RedoxReaction::setParameters(parameters);
    lambda->setVariables(parameters);
  };

};


//////////////////////////////////////////////////////////////////////

void KineticSystem::Cycle::computeRateConstant() const
{
  double forw = 1, back = 1;
  for(int i = 1; i < reactions.size(); i++) {
    const Reaction * r = reactions[i];
    if(directions[i] > 0) {
      forw *= r->forwardCache;
      back *= r->backwardCache;
    }
    else {
      back *= r->forwardCache;
      forw *= r->backwardCache;
    }
  }
  Reaction * r = reactions[0];

  // In a complete thermodynamic cycle, the product of all the forward
  // over the product of all the backwards is one.
  if(directions[0] > 0)
    // The forward direction is under the control of the cycle
    r->forwardCache = r->backwardCache * back/forw;
  else 
    r->backwardCache = r->forwardCache * forw/back;
}

//////////////////////////////////////////////////////////////////////


KineticSystem::KineticSystem() : 
  linear(false), checkRange(true), redoxReactionScaling(1),
  reporterExpression(NULL), reporterUseCurrent(false)
  
{
  cachedJacobian = NULL;
}

KineticSystem::KineticSystem(const KineticSystem & o) : 
  linear(o.linear), species(o.species),
  speciesLookup(o.speciesLookup),
  parameters(o.parameters),
  checkRange(o.checkRange),
  redoxReactionScaling(o.redoxReactionScaling),
  reporterUseCurrent(o.reporterUseCurrent)
{
  cachedJacobian = NULL;
  if(o.cachedJacobian) {
    cachedJacobian = gsl_matrix_alloc(o.cachedJacobian->size1, o.cachedJacobian->size2);
    gsl_matrix_memcpy(cachedJacobian, o.cachedJacobian);
  }
  reporterExpression = o.reporterExpression ?
    new Expression(*o.reporterExpression) : NULL;
  for(int i = 0; i < o.reactions.size(); i++)
    reactions << o.reactions[i]->dup();

  for(int i = 0; i < o.redoxReactions.size(); i++)
    redoxReactions << static_cast<RedoxReaction*>(o.redoxReactions[i]->dup());

}

  
KineticSystem::~KineticSystem()
{
  for(int i = 0; i < reactions.size(); i++)
    delete reactions[i];
  delete reporterExpression;
  if(cachedJacobian)
    gsl_matrix_free(cachedJacobian);
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

void KineticSystem::parseCycle(Reaction * start)
{
  QString cycle;
  int dir = 0;
  QRegExp cre("^\\s*cycle:\\s*(\\S.*)");
  if(cre.indexIn(start->forwardRate) >= 0) {
    cycle = cre.cap(1);
    dir = 1;
  }
  if(cre.indexIn(start->backwardRate) >= 0) {
    cycle = cre.cap(1);
    if(dir > 0)
      throw RuntimeError("Cannot have two 'cycle:' specifications in a single reaction, '%1'").arg(start->toString(species));
    dir = -1;
  }
  if(dir == 0)
    return;

  QList<int> s = start->products();
  if(s.size() > 1)
    throw RuntimeError("Cannot use thermodynamic cycles with more than one species as of now");

  int curSpec = s[0];
  QStringList spc = cycle.split(QRegExp("\\s+"));
  s = start->reactants();
  if(s.size() > 1)
    throw RuntimeError("Cannot use thermodynamic cycles with more than one species as of now");
  spc << species[s[0]].name;

  Cycle c;
  c.reactions << start;
  c.directions << dir;
  
  for(const QString & s : spc) {
    int ns = speciesLookup.value(s, -1);
    if(ns < 0)
      throw RuntimeError("Unknown species: '%1'").arg(s);
    Reaction * nr = NULL;
    int st = 0;
    for(int ridx : species[curSpec].reactions) {
      nr = reactions[ridx];
      if(! nr->isReversible())
        continue;
      st = nr->stoechiometry(ns);
      if(st != 0)
        break;
    }
    if(st == 0 || nr == NULL)
      throw RuntimeError("Could not find a reversible reaction from '%1' to '%2'").arg(species[curSpec].name).arg(s);
    c.reactions << nr;
    c.directions << st;
    curSpec = ns;
  }
  cycles << c;
}


void KineticSystem::ensureReady(const QStringList & add)
{
  QSet<QString> params;

  for(Reaction * r : reactions) {
    // Here, we parse cycles
    r->makeExpressions();
    parseCycle(r);
    params += r->parameters();
  }

  if(reporterExpression) {
    QSet<QString> reporterVariables =
      QSet<QString>::fromList(reporterExpression->currentVariables());
    if(reporterVariables.contains("j_elec")) {
      reporterUseCurrent = true;
      reporterVariables.remove("j_elec");
    }
    else
      reporterUseCurrent = false;
    params += reporterVariables;
  }


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
  QStringList rep = parameters;

  for(int i = 0; i < reactions.size(); i++)
    reactions[i]->setParameters(parameters);
  if(reporterExpression) {
    if(reporterUseCurrent)
      rep.insert(speciesNumber(), "j_elec");
    reporterExpression->setVariables(rep);
  }

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


void KineticSystem::cacheRateConstants(const gsl_vector * concentrations,
                                       const double * params) const
{
  const double * p;
  int nbSpecies = species.size();
  int nbParams = parameters.size();
  
  QVarLengthArray<double, 800> vals(nbParams);
  if(isLinear()) {
    /// @hack This is acutally undefined behaviour if the original
    /// params array does not extend before params - nbSpecies.
    p = params - nbSpecies;
  }
  else {
    // We put all the eggs in the same basket.
    for(int i = 0; i < nbSpecies; i++)
      vals[i] = gsl_vector_get(concentrations, i);
    for(int i = nbSpecies; i < nbParams; i++)
      vals[i] = params[i - nbSpecies];
    p = vals.data();
  }
  
  for(Reaction * rc : reactions) {
    rc->computeRateConstants(p, &rc->forwardCache,
                             &rc->backwardCache);
    if(checkRange) {
      if(rc->forwardCache < 0)
        throw RangeError("Negative forward rate constant for reaction '%1'").
          arg(rc->toString(species));

      if(rc->backwardCache < 0)
        throw RangeError("Negative backward rate constant for reaction '%1'").
          arg(rc->toString(species));
    }
  }
  
  // Compute the cycles
  for(const Cycle & c : cycles)
    c.computeRateConstant();
  
}

void KineticSystem::computeLinearJacobian(gsl_matrix * target,
                                          const double * params,
                                          gsl_vector * coeffs) const
{
  if(! linear)
    throw InternalError("Using a function for linear systems on non-linear ones !");
  // Now starts the real fun: evaluating all the derivatives !
  // QVarLengthArray<double, 800> vals(parameters.size());

  cacheRateConstants(NULL, params);

  int nbs = species.size();
  int nbp = parameters.size();
  int nbr = reactions.size();

  // We send pointers with uncontrolled data, but by assumption, this
  // data cannot be used, so that it does not matter so much.
  params -= nbs;
  
  gsl_matrix_set_zero(target);
  if(coeffs)
    gsl_vector_set_zero(coeffs);

  // Now, we compute the forward and reverse rates of all reactions
  for(int i = 0; i < nbr; i++) {
    const Reaction * rc = reactions[i];
    double forwardRate = rc->forwardCache,
      backwardRate = rc->backwardCache;

    int l = (rc->speciesStoechiometry[0] == -1 ? rc->speciesIndices[0] : rc->speciesIndices[1]);
    int r = (rc->speciesStoechiometry[0] == -1 ? rc->speciesIndices[1] : rc->speciesIndices[0]);

    // DO NOT FORGET SCALING !
    if(rc->electrons) {
      forwardRate *= redoxReactionScaling;
      backwardRate *= redoxReactionScaling;
      if(coeffs) {
        *gsl_vector_ptr(coeffs, l) -= forwardRate * rc->electrons;
        *gsl_vector_ptr(coeffs, r) += backwardRate * rc->electrons;
      }
    }
    // Debug::debug() << checkRange << " \tReaction #" << i << 
    //   ": " << forwardRate << " -- " << backwardRate << endl;
    

    if(checkRange && (forwardRate < 0 || backwardRate < 0))
      throw RangeError("Negative rate constant for reaction #%1").
        arg(i);

    *gsl_matrix_ptr(target, l, l) -= forwardRate;
    *gsl_matrix_ptr(target, r, l) += forwardRate;
    *gsl_matrix_ptr(target, r, r) -= backwardRate;
    *gsl_matrix_ptr(target, l, r) += backwardRate;
  }
}


/// The cache is setup assuming that ALL the non-redox reactions have
/// constant rates. This is true as of now if the isLinear() returns
/// true.
bool KineticSystem::setupCache(const double * params)
{
  if(! isLinear())
    return false;

  if(! cachedJacobian)
    cachedJacobian = gsl_matrix_alloc(species.size(), species.size());

  // Debug::debug() << "Trying to set up cache" <<  endl;

  { 
    TemporaryChange<double> t(redoxReactionScaling, 0);
    computeLinearJacobian(cachedJacobian, params, NULL);
  }
  // Debug::debug() << "Setting up cache" <<  endl;

  // We send pointers with uncontrolled data, but by assumption, this
  // data cannot be used, so that it does not matter so much.
  int nbs = species.size();
  params -= nbs;

  // Now, we look at all the redox reactions, store
  redoxReactions.clear();
  int nbr = reactions.size();
  for(int i = 0; i < nbr; i++) {
    Reaction * rc = reactions[i];
    if(rc->electrons) {
      // Debug::debug() << "Computing cache for reaction " << i << endl;
      redoxReactions.append(static_cast<RedoxReaction*>(rc));
      rc->computeCache(params);
    }
  }

  return true;
}



void KineticSystem::computeCachedLinearJacobian(gsl_matrix * target,
                                                const double * params,
                                                gsl_vector * coeffs) const
{
  if(! cachedJacobian)
    throw InternalError("Using a cache that was not setup");

  int nbs = species.size();

  // We send pointers with uncontrolled data, but by assumption, this
  // data cannot be used, so that it does not matter so much.
  params -= nbs;
  
  gsl_matrix_memcpy(target, cachedJacobian);
  if(coeffs)
    gsl_vector_set_zero(coeffs);

  int nbr = redoxReactions.size();
  double ex; 

  // Now, we compute the forward and reverse rates of all reactions
  for(int i = 0; i < nbr; i++) {
    const RedoxReaction * rc = redoxReactions[i];
    if(! i) {
      // Compute ex the first time
      double e = params[rc->potentialIndex];
      double fara = GSL_CONST_MKSA_FARADAY /
        (params[rc->temperatureIndex] * GSL_CONST_MKSA_MOLAR_GAS);
      ex = exp(fara * 0.5 * e);
    }

    double tmp = pow(ex, rc->electrons) * rc->cache[0];
    double tmp2 = rc->cache[1] * redoxReactionScaling;
    double forwardRate, backwardRate;
    forwardRate = tmp2 * tmp;
    backwardRate = tmp2/ tmp;

    int l = (rc->speciesStoechiometry[0] == -1 ? rc->speciesIndices[0] : rc->speciesIndices[1]);
    int r = (rc->speciesStoechiometry[0] == -1 ? rc->speciesIndices[1] : rc->speciesIndices[0]);

    if(coeffs) {
      *gsl_vector_ptr(coeffs, l) -= forwardRate;
      *gsl_vector_ptr(coeffs, r) += backwardRate;
    }

    if(checkRange && (forwardRate < 0 || backwardRate < 0))
      throw RangeError("Negative rate constant for reaction #%1").
        arg(i);

    *gsl_matrix_ptr(target, l, l) -= forwardRate;
    *gsl_matrix_ptr(target, r, l) += forwardRate;
    *gsl_matrix_ptr(target, r, r) -= backwardRate;
    *gsl_matrix_ptr(target, l, r) += backwardRate;
  }
}



double KineticSystem::computeDerivatives(gsl_vector * target, 
                                         const gsl_vector * concentrations,
                                         const double * params) const
{
  int nbParams = parameters.size();
  int nbSpecies = species.size();
  int nbReactions = reactions.size();

  
  cacheRateConstants(concentrations, params);

  // We put all the eggs in the same basket.
  if(target)
    gsl_vector_set_zero(target);

  double current = 0;

  // Now, we compute the forward and reverse rates of all reactions
  for(int i = 0; i < nbReactions; i++) {
    const Reaction * r = reactions[i];
    double forwardRate = r->forwardCache,
      backwardRate = r->backwardCache;

    // Do not compute if we don't need !
    if(! target && r->electrons == 0)
      continue;

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
  File f(fileName, File::TextRead);
  parseFile(f, fileName);
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

void KineticSystem::parseFile(QIODevice * device, const QString & n)
{
  QTextStream in(device);

  QRegExp blankRE("^\\s*(#|$)");
  QRegExp reactionRE("^\\s*(.*)(<=>|->)(\\([^)]+\\))?\\s*(.*)$");

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
      QString right = reactionRE.cap(4);
      QString arrow = reactionRE.cap(2);
      QString options = reactionRE.cap(3);

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
            throw RuntimeError("Kinetic file %1 -- line %2:\n '%3' -- electrons cannot appear "
                               "more than once in a reaction").
              arg(Utils::fileName(device)).arg(number).arg(orig);
          els = stoechiometry[i];
          reactants.takeAt(i);
          stoechiometry.takeAt(i);
          --i;
        }
      }

      addReaction(reactants, stoechiometry, els, literals, arrow, options);
    }
    else
      throw RuntimeError(QString("Kinetic file %1 -- line %2:\n '%3' not valid").
                         arg(Utils::fileName(device)).
                         arg(number).arg(orig));
  }
  if(reaction == 0)
    throw RuntimeError("Could not parse any reaction from file %1").
      arg(Utils::fileName(device));
  fileName = n;
}

void KineticSystem::addReaction(QList<QString> species,
                                QList<int> stoechiometry, 
                                int els, 
                                const QStringList & lits,
                                const QString & arrow,
                                const QString & opts)
{
  parameters.clear();
  QStringList literals = lits;
  int reactionIndex = reactions.size();
  bool reversible = arrow == "<=>";

  if(literals.size() < 1)
    literals << QString(els != 0 ? "e0_%1" : "k_%1").arg(reactionIndex);
  if(reversible && literals.size() < 2)
    literals << QString(els != 0 ? "k0_%1" : "k_m%1").arg(reactionIndex);

  int type = 0;
  if(els != 0) {
    type = 1;
    if(opts == "(BV)")
      type = 2;
    if(opts == "(M)")
      type = 3;
  }

  Reaction * reac;
  switch(type) {
  case 0:                       // plain chemical reaction
    reac = new Reaction(literals.value(0),
                        literals.value(1));
    break;
  case 1:                       // plain electrochemical (BV, alpha = 1/2) reaction
    reac = new RedoxReaction(els, literals.value(0),
                             literals.value(1));
    break;
  case 2:
    if(literals.size() < 3)
      literals << QString("alpha_%1").arg(reactionIndex);
    reac = new BVRedoxReaction(els, literals.value(0),
                               literals.value(1), literals.value(2));
    break;
  case 3:
    if(literals.size() < 3)
      literals << QString("lambda_%1").arg(reactionIndex);
    reac = new MarcusRedoxReaction(els, literals.value(0),
                                   literals.value(1), literals.value(2));
    break;

  default:
    reac = NULL;
  }
                     
  for(int i = 0; i < species.size(); i++) {
    int spi = speciesIndex(species[i]);
    reac->speciesIndices.append(spi);
    this->species[spi].reactions.append(reactionIndex);
    reac->speciesStoechiometry.append(stoechiometry[i]);
  }
  reactions.append(reac);
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
