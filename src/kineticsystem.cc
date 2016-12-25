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

/// A species involved in the reactions.
class Species {
public:
    
  /// Name
  QString name;

  /// The indices of the reactions the species takes part in
  QVector<int> reactions;

  /// The type of diffusion.
  KineticSystem::Diffusion diffusion;

  Species(const QString & n) : name(n), diffusion(KineticSystem::None) {;};
    
};


/// A reaction. It is considered an elementary reaction, in the
/// sense that the rate constants are provided, which are mutiplied
/// by the concentration to the power of the stoechiometry. However,
/// it is still possible to cancel out some terms in that by using
/// the appropriate concentration factor in the rates.
///
/// @todo subclasses and possibly even the implementation should be
/// private only in the .cc file
class Reaction {
protected:

  /// Expression for the forward rate
  QString forwardRate;

  /// Expression for the reverse rate (if empty, then the reaction
  /// is irreversible)
  QString backwardRate;

    
  Expression * forward;
  Expression * backward;

public:
  /// List of indices of the reactants or products
  QVector<int> speciesIndices;

  /// Their stoechiometry (negative if on the reactants side,
  /// positive if on the products size). In the same order as
  /// speciesIndices.
  QVector<int> speciesStoechiometry;

  /// the number of electrons (counted negatively if on the left)
  int electrons;

  /// A storage space for caching stuff -- this is hanlded at the
  /// KineticSystem level.
  double cache[2];

  /// Returns true when:
  /// * the stoechiometry is one for each reactant
  /// * the rate constants are constants
  ///
  /// @todo In time, this will have to include the case when a rate
  /// constant is not a constant, but does not depend on the
  /// concentrations.
  virtual bool isLinear() const;

  Reaction(const QString & fd, const QString & bd = "" );

  /// A copy constructor
  Reaction(const Reaction & other);

  virtual void clearExpressions();

  virtual ~Reaction() {
    clearExpressions();
  };

  /// Computes the expressions
  virtual void makeExpressions(const QStringList & vars = QStringList());

  /// Sets the parameters of the expressions
  virtual void setParameters(const QStringList & parameters);

  /// Returns the parameters needed by the rates
  virtual QSet<QString> parameters() const;


  /// Computes both the forward and backward rates
  ///
  /// In fact, these are rate CONSTANTS !
  virtual void computeRateConstants(const double * vals, 
                                    double * forward, double * backward) const;

  /// String depiction of the equation (along with the rates)
  virtual QString toString(const QList<Species> & species) const;

  /// Returns the expression of the exchange rate for the
  /// reaction. Returns an empty string if there isn't an exchange
  /// rate.
  virtual QString exchangeRate() const;

  /// Stores useful values in the cache. Not used as of now.
  virtual void computeCache(const double * vals);

  /// Returns a true duplicate of the object pointed to by
  /// Reaction. It also works for subclasses
  virtual Reaction * dup() const;
};




Reaction::Reaction(const QString & fd, const QString & bd) :
  forwardRate(fd), backwardRate(bd), forward(NULL), backward(NULL), 
  electrons(0)
{
  
}

void Reaction::computeCache(const double * /*vals*/)
{
  // Nothing to do here.
}


void Reaction::makeExpressions(const QStringList & vars)
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

QSet<QString> Reaction::parameters() const
{
  
  if(! forward)
    return QSet<QString>();
  QSet<QString> params = QSet<QString>::fromList(forward->naturalVariables());
  if(backward)
    params += QSet<QString>::fromList(backward->naturalVariables());
  return params;
}


void Reaction::clearExpressions() 
{
  delete forward;
  forward = NULL;
  delete backward;
  backward = NULL;
}

void Reaction::setParameters(const QStringList & parameters)
{
  if(! forward)
    return;
  forward->setVariables(parameters);
  if(! backward)
    return;
  backward->setVariables(parameters);
}

void Reaction::computeRateConstants(const double * vals, 
                                           double * fd, 
                                           double * bd) const
{
  *fd = forward->evaluate(vals);
  if(backward)
    *bd = backward->evaluate(vals);
  else
    *bd = 0;
}

QString Reaction::toString(const QList<Species> & species) const
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

QString Reaction::exchangeRate() const
{
  return QString();
}

bool Reaction::isLinear() const
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

Reaction::Reaction(const Reaction & o) :
  forwardRate(o.forwardRate), backwardRate(o.backwardRate),
  speciesIndices(o.speciesIndices),
  speciesStoechiometry(o.speciesStoechiometry),
  electrons(o.electrons)
{
  forward = o.forward ? new Expression(*o.forward) : NULL;
  backward = o.backward ? new Expression(*o.backward) : NULL;

  memcpy(cache, o.cache, sizeof(o.cache));
}


Reaction * Reaction::dup() const
{
  return new Reaction(*this);
}

//////////////////////////////////////////////////////////////////////

/// This reaction is a simple butler-volmer reaction with alpha = 0.5
class RedoxReaction : public Reaction {
public:
  /// Index of the potential in the parameters
  int potentialIndex;

  /// Index of the temperature in the parameters
  int temperatureIndex;

  // We reuse the forward/backward stuff but with a different
  // meaning.
  RedoxReaction(int els, const QString & e0, const QString & k0);

  /// A copy constructor
  RedoxReaction(const RedoxReaction & other);

  virtual void setParameters(const QStringList & parameters);

  virtual QSet<QString> parameters() const;

  virtual void computeRateConstants(const double * vals, 
                                    double * forward, double * backward) const;
  virtual QString exchangeRate() const;

  virtual Reaction * dup() const;

  /// Stores useful values in the cache. Stores:
  /// * exp(fara * 0.5 * electrons * (- e0));
  /// * k0
  virtual void computeCache(const double * vals);
};



RedoxReaction::RedoxReaction(int els, const QString & e0, 
                                            const QString & k0) :
  Reaction(e0, k0)
{
  electrons = els;
}

QSet<QString> RedoxReaction::parameters() const
{
  QSet<QString> params = Reaction::parameters();
  params += "e";                // We always need the potential !
  params += "temperature";                // We always need the potential !
  return params;
}

void RedoxReaction::setParameters(const QStringList & parameters)
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

void RedoxReaction::computeRateConstants(const double * vals, 
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

void RedoxReaction::computeCache(const double * vals)
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

QString RedoxReaction::exchangeRate() const
{
  return backwardRate;
}

Reaction * RedoxReaction::dup() const
{
  return new RedoxReaction(*this);
}

RedoxReaction::RedoxReaction(const RedoxReaction & o) :
  Reaction(o), potentialIndex(o.potentialIndex),
  temperatureIndex(o.temperatureIndex)
{
  ;
}

//////////////////////////////////////////////////////////////////////

/// This reaction is a full Butler-Volmer reaction, with values of alpha
class BVRedoxReaction : public RedoxReaction {

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
  
  virtual void clearExpressions() {
    Reaction::clearExpressions();
    delete alpha;
    alpha = NULL;
  }


  virtual void computeRateConstants(const double * vals, 
                                    double * fd, double * bd) const
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

  virtual Reaction * dup() const {
    return new BVRedoxReaction(*this);
  }

  virtual QSet<QString> parameters() const {
    QSet<QString> params = RedoxReaction::parameters();
    params += QSet<QString>::fromList(alpha->naturalVariables());
    return params;
  };

  void setParameters(const QStringList & parameters) override {
    RedoxReaction::setParameters(parameters);
    alpha->setVariables(parameters);
  };

};

//////////////////////////////////////////////////////////////////////

/// This reaction is a full Butler-Volmer reaction, with values of alpha
class MarcusRedoxReaction : public RedoxReaction {

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
  
  virtual void clearExpressions() {
    Reaction::clearExpressions();
    delete lambda;
    lambda = NULL;
  }


  virtual void computeRateConstants(const double * vals, 
                                    double * fd, double * bd) const
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

  virtual Reaction * dup() const {
    return new MarcusRedoxReaction(*this);
  }

  virtual QSet<QString> parameters() const {
    QSet<QString> params = RedoxReaction::parameters();
    params += QSet<QString>::fromList(lambda->naturalVariables());
    return params;
  };

  void setParameters(const QStringList & parameters) override {
    RedoxReaction::setParameters(parameters);
    lambda->setVariables(parameters);
  };

};



//////////////////////////////////////////////////////////////////////


KineticSystem::KineticSystem() : 
  linear(false), checkRange(true), redoxReactionScaling(1),
  reporterExpression(NULL)
  
{
  cachedJacobian = NULL;
}

KineticSystem::KineticSystem(const KineticSystem & o) : 
  linear(o.linear), species(o.species),
  speciesLookup(o.speciesLookup),
  parameters(o.parameters),
  checkRange(o.checkRange),
  redoxReactionScaling(o.redoxReactionScaling)
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
                                          const double * params,
                                          gsl_vector * coeffs) const
{
  if(! linear)
    throw InternalError("Using a function for linear systems on non-linear ones !");
  // Now starts the real fun: evaluating all the derivatives !
  // QVarLengthArray<double, 800> vals(parameters.size());

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
    double forwardRate, backwardRate;
    rc->computeRateConstants(params, &forwardRate, &backwardRate);

    int l = (rc->speciesStoechiometry[0] == -1 ? rc->speciesIndices[0] : rc->speciesIndices[1]);
    int r = (rc->speciesStoechiometry[0] == -1 ? rc->speciesIndices[1] : rc->speciesIndices[0]);

    // DO NOT FORGET SCALING !
    if(rc->electrons) {
      forwardRate *= redoxReactionScaling;
      backwardRate *= redoxReactionScaling;
      if(coeffs) {
        *gsl_vector_ptr(coeffs, l) -= forwardRate;
        *gsl_vector_ptr(coeffs, r) += backwardRate;
      }
    }
    // Debug::debug() << "Reaction #" << i << 
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
  QRegExp reactionRE("^\\s*(.*)(<=>|->)(\\([^)]+\\))?\\s*(.*)$");

  QRegExp reporterRE("^\\s*y\\s*=\\s*(.*)$");

  QRegExp doubleBraceRE("\\[\\[(.*)\\]\\]");
  doubleBraceRE.setMinimal(true);

  QRegExp singleBraceRE("\\[([^\\]]*)\\]");
  
  int number = 0;

  int reaction = 0;

  QTextStream o(stdout);
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

    o << "Line: " << line << endl;


    if(line.contains("[["))
      literals = Utils::extractMatches(line, doubleBraceRE, 1);
    else 
      literals = Utils::extractMatches(line, singleBraceRE, 1);

    o << "Literals: " << literals.join(", ") << endl;

    
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
            throw RuntimeError("Line %1: '%2' -- electrons cannot appear "
                               "more than once in a reaction").
              arg(number).arg(orig);
          els = stoechiometry[i];
          reactants.takeAt(i);
          stoechiometry.takeAt(i);
          --i;
        }
      }

      addReaction(reactants, stoechiometry, els, literals, arrow, options);
    }
    else
      throw RuntimeError(QString("Line %1: '%2' not valid").
                         arg(number).arg(orig));
  }
  if(reaction == 0)
    throw RuntimeError("Could not parse any reaction from file %1").
      arg(Utils::fileName(device));
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
