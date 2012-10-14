/*
  kineticsystem.cc: implementation of KineticSystem
  Copyright 2012 by Vincent Fourmond

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


//////////////////////////////////////////////////////////////////////

KineticSystem::KineticSystem()
{
}

KineticSystem::~KineticSystem()
{
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
                                const QString & forward, 
                                const QString & backward)
{
  parameters.clear();
  int reactionIndex = reactions.size();
  reactions.append(Reaction());
  Reaction & reac = reactions[reactionIndex];
  reac.electrons = 0;
  for(int i = 0; i < species.size(); i++) {
    if(species[i] == "e-")
      reac.electrons = stoechiometry[i];
    else {
      int spi = speciesIndex(species[i]);
      reac.speciesIndices.append(spi);
      this->species[spi].reactions.append(reactionIndex);
      reac.speciesStoechiometry.append(stoechiometry[i]);
    }
  }
  reac.forwardRate = forward;
  reac.backwardRate = backward;
}

void KineticSystem::ensureReady()
{
  QSet<QString> params;
  
  for(int i = 0; i < reactions.size(); i++) {
    reactions[i].makeExpressions();
    params += reactions[i].parameters();
  }

  // We remove the concentration parameters, as we want those to come
  // first, for the sake of simplicity ?
  QStringList conc;
  QStringList conc0;
  for(int i = 0; i < species.size(); i++) {
    QString str = QString("c0_%1").arg(species[i].name);
    params.remove(str);
    conc0.append(str);
    str = QString("c_%1").arg(species[i].name);
    params.remove(str);
    conc.append(str);
  }
  parameters.clear();
  parameters = params.toList();
  qSort(parameters);
  // First concentrations, then initial concentrations, then the
  // remaining parameters.
  parameters = conc + conc0 + parameters;
}

void KineticSystem::prepare()
{
  ensureReady();
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

void KineticSystem::computeDerivatives(double * target, 
                                       const double * concentrations,
                                       const double * params) const
{
  // Now starts the real fun: evaluating all the derivatives !
  QVarLengthArray<double, 800> vals(parameters.size());

  // We put all the eggs in the same basket.
  for(int i = 0; i < species.size(); i++) {
    vals[i] = concentrations[i];
    target[i] = 0;
  }
  for(int i = species.size(); i < parameters.size(); i++)
    vals[i] = params[i - species.size()];

  // Now, we compute the forward and reverse rates of all reactions
  for(int i = 0; i < reactions.size(); i++) {
    const Reaction & r = reactions[i];
    double forwardRate = r.forward->evaluate(vals.data());
    double backwardRate = (r.backward ? r.backward->evaluate(vals.data()) : 0);

    for(int j = 0; j < r.speciesStoechiometry.size(); j++) {
      int s = r.speciesStoechiometry[j];
      if(s < 0)
        forwardRate *= gsl_pow_int(concentrations[r.speciesIndices[j]], -s);
      else
        backwardRate *= gsl_pow_int(concentrations[r.speciesIndices[j]], s);
    }
    double rate = forwardRate - backwardRate;
    for(int j = 0; j < r.speciesStoechiometry.size(); j++) {
      target[r.speciesIndices[j]] += r.speciesStoechiometry[j] * rate;
    }
  }
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
  QRegExp irreversibleRE("^\\s*(.*)->\\[(.*)\\]\\s*(.*)$");
  QRegExp reversibleRE("^\\s*(.*)<=>\\[(.*)\\]\\s*\\[(.*)\\]\\s*(.*)$");
  
  int number = 0;
  while(true) {
    QString line = in.readLine();
    ++number;
    if(line.isNull())
      break;
    if(blankRE.indexIn(line) >= 0)
      continue;
    
    QString fr;
    QString br;
    QString left;
    QString right;

    if(irreversibleRE.indexIn(line) == 0) {
      left = irreversibleRE.cap(1);
      fr = irreversibleRE.cap(2);
      right = irreversibleRE.cap(2);
    }
    else if(reversibleRE.indexIn(line) == 0) {
      left = reversibleRE.cap(1);
      fr = reversibleRE.cap(2);
      br = reversibleRE.cap(3);
      right = reversibleRE.cap(3);
    }
    else
      throw RuntimeError(QString("Line %1: '%2' not valid").
                         arg(number).arg(line));

    QStringList reactants;
    QList<int> stoechiometry;

    parseReactants(left, &reactants, &stoechiometry, -1, number);
    parseReactants(right, &reactants, &stoechiometry, 1, number);
    addReaction(reactants, stoechiometry, fr, br);
  }
}
