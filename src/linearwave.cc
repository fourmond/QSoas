/*
  linearwave.cc: implementation of LinearWave
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

#include <exceptions.hh>
#include <linearwave.hh>

#include <terminal.hh>
#include <utils.hh>


// Temporary includes for commands
#include <command.hh>
#include <vector.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_math.h>

#include <soas.hh>
#include <dataset.hh>
#include <datastack.hh>
#include <curveview.hh>

class LWSpecies {
public:
  /// The name of the species
  QString name;

  /// @todo cross-refs

  LWSpecies() { ; };
  LWSpecies(const QString & n) : name(n) {;};
};


//////////////////////////////////////////////////////////////////////

class LWReactionBase {
public:
  int reactantIndex;
  int productIndex;
  bool reversible;

  LWReactionBase() {;};
  LWReactionBase(int ri, int pi, bool rev) : 
    reactantIndex(ri), 
    productIndex(pi), reversible(rev) {;};

};

//////////////////////////////////////////////////////////////////////

class LWReaction : public LWReactionBase {
public:
  int forwardRateIndex;
  int backwardRateIndex;

  LWReaction() {;};
  LWReaction(int ri, int pi, bool rev) : 
    LWReactionBase(ri, pi, rev) {;};
};

//////////////////////////////////////////////////////////////////////

class LWRedoxReaction : public LWReactionBase {
public:
  int potentialIndex;
  int rateIndex;
  int asymmetryIndex;

  int electrons;

  LWRedoxReaction() {;};
  LWRedoxReaction(int ri, int pi, bool rev) : 
    LWReactionBase(ri, pi, rev) {;};
};

//////////////////////////////////////////////////////////////////////

class LWParameter  {
public:
  QString name;

  LinearWave::ParameterType type;
    

  LWParameter() { ; };
  LWParameter(const QString & n, LinearWave::ParameterType t = 
              LinearWave::Rate) : name(n), type(t) {;};
};

//////////////////////////////////////////////////////////////////////

LinearWave::LinearWave()
{
  systemMatrix = NULL;
}

LinearWave::~LinearWave()
{
  if(systemMatrix) {
    gsl_matrix_free(systemMatrix);
    gsl_vector_free(concStorage);
    gsl_vector_free(curStorage);
  }
}

int LinearWave::addParameter(const QString & name, LinearWave::ParameterType t)
{
  if(parametersIndex.contains(name))
    return parametersIndex[name];
  int id = parameters.size();
  parameters << LWParameter(name, t);
  parametersIndex[name] = id;
  return id;
}


int LinearWave::addSpecies(const QString & name)
{
  if(speciesIndex.contains(name))
    return speciesIndex[name];
  int id = species.size();
  species << LWSpecies(name);
  speciesIndex[name] = id;
  return id;
}

void LinearWave::parseSystem(QIODevice * dev)
{
  QTextStream s(dev);
  parseSystem(&s);
}

void LinearWave::parseSystem(QTextStream * s)
{
  if(systemMatrix)
    throw RuntimeError("Cannot re-use an already defined LinearWave system");

  QRegExp commentRE("^\\s*(?:$|#)"); // blank or comment
  QRegExp reactionRE("^(.*)\\s*(<=>|->)\\s*([^[]+)\\s*(?:\\[(.*)\\])?$");
  QRegExp redoxRE("^(.*)\\+\\s*(\\d+)?e-\\s*$");
  
  int nb;
  while(true) {
    QString line = s->readLine();
    if(line.isNull())
      break;                    // Reached end of file
    ++nb;
    
    if(commentRE.indexIn(line, 0) == 0)
      continue;

    if(reactionRE.indexIn(line, 0) != 0) {
      // We have a wrong line
      throw RuntimeError(QString("Incorrect line: '%2' for linear wave spec (line #%1)").arg(nb).arg(line.trimmed()));
    }

    QString reactant = reactionRE.cap(1).trimmed();
    QString product = reactionRE.cap(3).trimmed();
    QStringList constantNames = reactionRE.cap(4).
      split(QRegExp("\\s*,\\s*"), QString::SkipEmptyParts);
    bool reversible = (reactionRE.cap(2) == "<=>");
    int electrons = 0;

    if(redoxRE.indexIn(reactant) == 0) {
      if(redoxRE.cap(2).size() > 0)
        electrons = redoxRE.cap(2).toInt();
      else
        electrons = 1;
      reactant = redoxRE.cap(1).trimmed();
    }

    int rindex = addSpecies(reactant);
    int pindex = addSpecies(product);

    if(electrons > 0) {
      redoxReactions << LWRedoxReaction(rindex, pindex, reversible);

      LWRedoxReaction & react = redoxReactions.last();
      react.electrons = electrons;

      /// @todo in real, irreversible reactions are only supported for
      /// reductions.
      if(constantNames.size() != (reversible ? 3 : 2)) {
        int id = redoxReactions.size();
        constantNames.clear();
        if(reversible)
          constantNames << QString("E0_%1").arg(id);
        constantNames << QString("k0_%1").arg(id);
        constantNames << QString("alpha_%1").arg(id);
      }
      if(reversible)
        react.potentialIndex = addParameter(constantNames.takeFirst(), 
                                            Potential);
      react.rateIndex = addParameter(constantNames[0]);
      react.asymmetryIndex = addParameter(constantNames[1], Alpha);
    }
    else {
      reactions << LWReaction(rindex, pindex, reversible);

      LWReaction & react = reactions.last();

      // Now, either read rate constant names or make them up:
      if(constantNames.size() != (reversible ? 2 : 1)) {
        int id = reactions.size();
        constantNames.clear();
        constantNames << QString("k_%1").arg(id);
        if(reversible)
          constantNames << QString("k_m%1").arg(id);
      }
      react.forwardRateIndex = addParameter(constantNames[0]);
      if(reversible)            /// @todo fail when not present
        react.backwardRateIndex = addParameter(constantNames[1]);
    }    
  }


  systemMatrix = gsl_matrix_alloc(speciesNumber(), speciesNumber());
  concStorage = gsl_vector_alloc(speciesNumber());
  curStorage = gsl_vector_alloc(speciesNumber());
}


void LinearWave::dumpSystem() const
{

  for(int i = 0; i < species.size(); i++)
    Terminal::out << "Species #" << i << ": " << species[i].name << endl;

  for(int i = 0; i < reactions.size(); i++)
    Terminal::out << "Reaction #" << i << ": " << reactions[i].reactantIndex 
                  << " to " << reactions[i].productIndex 
                  << "\tf: #" << reactions[i].forwardRateIndex << endl;

  for(int i = 0; i < redoxReactions.size(); i++)
    Terminal::out << "Reaction redox #" << i << ": " << redoxReactions[i].reactantIndex 
                  << " to " << redoxReactions[i].productIndex 
                  << " els: " << redoxReactions[i].electrons << endl;

  for(int i = 0; i < parameters.size(); i++)
    Terminal::out << "Parameter #" << i << ": " 
                  << parameters[i].name << endl; 
}




QStringList LinearWave::parameterNames() const
{
  QStringList params;
  for(int i = 0; i < parameters.size(); i++)
    params << parameters[i].name;
  return params;
}


static inline void matrixAdd(gsl_matrix * m, int i, int j, double val)
{
  gsl_matrix_set(m, i, j, val + gsl_matrix_get(m, i, j));
}

static inline void matrixAddReaction(gsl_matrix * m, int from, int to, 
                                     double val)
{
  matrixAdd(m, from, from, - val);
  matrixAdd(m, to, from, + val);
}

void LinearWave::computeEchemRateConstants(double fara, double potential, 
                                           const LWRedoxReaction & react,
                                           const double * parameters,
                                           double * kforw, double * kback)
{
  double pot = parameters[react.potentialIndex];
  double k0 = parameters[react.rateIndex];
  double alpha = parameters[react.asymmetryIndex];
  int nb = react.electrons;

  /// @todo handle irreversible reactions ?

  // Reduction
  *kforw = exp(fara * (pot - potential) * alpha * nb) * k0;
  *kback = exp(fara * (potential - pot) * (1 - alpha) * nb) * k0;
}

void LinearWave::prepareMatrix(double potential, double temperature, 
                               const double * parameters)
{
  // First, we must clear the matrix.
  gsl_matrix_set_zero(systemMatrix);


  /// @todo add a function that computes a vector that gives the
  /// current by scalar multiplication with the concentrations

  for(int i = 0; i < reactions.size(); i++) {
    const LWReaction & react = reactions[i];
    int ri = react.reactantIndex, pi = react.productIndex, 
      fi = react.forwardRateIndex, bi = react.backwardRateIndex;
    matrixAddReaction(systemMatrix, ri, pi, parameters[fi]);

    if(react.reversible)
      matrixAddReaction(systemMatrix, pi, ri, parameters[bi]);
  }

  double fara = GSL_CONST_MKSA_FARADAY/ 
    (temperature * GSL_CONST_MKSA_MOLAR_GAS);

  for(int i = 0; i < redoxReactions.size(); i++) {
    const LWRedoxReaction & react = redoxReactions[i];
    int ri = react.reactantIndex, pi = react.productIndex;

    if(! react.reversible)
      throw RuntimeError("Irreversible redox reactions not implemented");

    /// @todo Handle irreversible reactions ?? 

    double kf, kb;
    computeEchemRateConstants(fara, potential, react,
                              parameters, &kf, &kb);

    matrixAddReaction(systemMatrix, ri, pi, kf);
    matrixAddReaction(systemMatrix, pi, ri, kb);
  }
}


static inline void vectorAdd(gsl_vector * v, int i, double val)
{
  gsl_vector_set(v, i, val + gsl_vector_get(v, i));
}

void LinearWave::computeCurrentVector(double potential, double temperature, 
                                      const double * parameters,
                                      gsl_vector * result)
{
  gsl_vector_set_zero(result);

  double fara = GSL_CONST_MKSA_FARADAY/ 
    (temperature * GSL_CONST_MKSA_MOLAR_GAS);

  for(int i = 0; i < redoxReactions.size(); i++) {
    const LWRedoxReaction & react = redoxReactions[i];
    int ri = react.reactantIndex, pi = react.productIndex;
    double kf, kb;
    computeEchemRateConstants(fara, potential, react,
                              parameters, &kf, &kb);
    vectorAdd(result, ri, - react.electrons * kf);
    vectorAdd(result, pi, react.electrons * kb);
  }
}

void LinearWave::computeConcentrations(double potential, double temperature,
                                       const double * parameters,
                                       gsl_vector * result)
{
  // First, we need to prepare the matrix:
  prepareMatrix(potential, temperature, parameters);

  gsl_permutation * p = gsl_permutation_alloc(speciesNumber());

  // Now, we set the first line to 0:
  for(int i = 0; i < speciesNumber(); i++)
    gsl_matrix_set(systemMatrix, 0, i, 1);

  int sgn;
  gsl_linalg_LU_decomp(systemMatrix, p, &sgn);

  gsl_vector_set_zero(result);
  gsl_vector_set(result, 0, 1);

  gsl_linalg_LU_svx(systemMatrix, p, result);

  gsl_permutation_free(p);
}

double LinearWave::computeCurrent(double potential, double temperature,
                                  const double * parameters)
{
  computeConcentrations(potential, temperature, parameters, concStorage);
  computeCurrentVector(potential, temperature, parameters, curStorage);
  double val;
  gsl_blas_ddot(concStorage, curStorage, &val);
  return val;
}


QString LinearWave::getMatrix(double potential, double temperature,
                              const double * parameters)
{
  prepareMatrix(potential, temperature, parameters);
  return Utils::matrixString(systemMatrix);
}

QString LinearWave::lastConcentrations() const
{
    return Utils::vectorString(concStorage);
}

QString LinearWave::lastCurrent() const 
{
  return Utils::vectorString(curStorage);
}

LinearWave::ParameterType LinearWave::parameterType(int index) const
{
  return parameters[index].type;
}

QList<int> LinearWave::typedParameters(LinearWave::ParameterType type) const
{
  QList<int> ret;
  for(int i = 0; i < parameters.size(); i++)
    if(parameters[i].type == type)
      ret << i;
  return ret;
}






//////////////////////////////////////////////////////////////////////

static ArgumentList 
testLWArgs(QList<Argument *>() 
           << new FileArgument("file", 
                               "File",
                               "Files to load !"));


static void testLWCommand(const QString &, QString arg)
{
  QFile f(arg);
  Utils::open(&f, QIODevice::ReadOnly);
  LinearWave lw;
  lw.parseSystem(&f);
  lw.dumpSystem();
}


static Command 
testLW("test-lw", // command name
       optionLessEffector(testLWCommand), // action
       "simulations",  // group name
       &testLWArgs, // arguments
       NULL, // options
       "TestLW",
       "TestLW test command",
       "TestLW command for testing purposes");


//////////////////////////////////////////////////////////////////////

static ArgumentList 
simLWArgs(QList<Argument *>() 
           << new FileArgument("file", 
                               "System",
                               "System to load")
           << new SeveralNumbersArgument("parameters", 
                                         "Parameters")
           );


static void simulateLWCommand(const QString &, QString file, 
                              QList<double> consts)
{
  QFile f(file);
  Utils::open(&f, QIODevice::ReadOnly);
  LinearWave lw;
  lw.parseSystem(&f);

  QStringList pn = lw.parameterNames();

  while(consts.size() < lw.parametersNumber())
    consts << 0;

  Terminal::out << "Parameters are: " << endl;
  for(int i = 0; i < pn.size(); i++)
    Terminal::out << " - " << pn[i] << " = " << consts[i] << endl;

  Vector params = consts.toVector();

  double xstart = 0;
  double xend = -1;
  int nb = 1000;
  double temperature = soas().temperature();

  Vector x;
  Vector y;
  for(int i = 0; i < nb; i++) {
    double p = xstart + ((xend - xstart) * i)/(nb - 1);
    x << p;
    y << lw.computeCurrent(p, temperature, params.data());
  }

  DataSet * ds = new DataSet(x, y);
  ds->name = "linear-wave-simul.dat";
  soas().stack().pushDataSet(ds); 
}


static Command 
simLW("sim-lw", // command name
       optionLessEffector(simulateLWCommand), // action
       "simulations",  // group name
       &simLWArgs, // arguments
       NULL, // options
       "Simulate linear wave",
       "Simulate linear wave based on kinetic model",
       "...");
