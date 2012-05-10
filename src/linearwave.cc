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
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>


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

  LWParameter() { ; };
  LWParameter(const QString & n) : name(n) {;};
};

//////////////////////////////////////////////////////////////////////

LinearWave::LinearWave()
{
}

LinearWave::~LinearWave()
{
}

int LinearWave::addParameter(const QString & name)
{
  if(parametersIndex.contains(name))
    return parametersIndex[name];
  int id = parameters.size();
  parameters << LWParameter(name);
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

      if(constantNames.size() != (reversible ? 3 : 2)) {
        int id = redoxReactions.size();
        constantNames.clear();
        if(reversible)
          constantNames << QString("E0_%1").arg(id);
        constantNames << QString("k0_%1").arg(id);
        constantNames << QString("alpha_%1").arg(id);
      }
      if(reversible)
        react.potentialIndex = addParameter(constantNames.takeFirst());
      react.rateIndex = addParameter(constantNames[0]);
      react.asymmetryIndex = addParameter(constantNames[1]);
    }
    else {
      reactions << LWReaction(rindex, pindex, reversible);

      LWReaction & react = reactions.last();

      // Now, either read rate constant names or make them up:
      if(constantNames.size() != (reversible ? 2 : 1)) {
        int id = redoxReactions.size();
        constantNames.clear();
        constantNames << QString("k_%1").arg(id);
        if(reversible)
          constantNames << QString("k_m%1").arg(id);
      }
      react.forwardRateIndex = addParameter(constantNames[0]);
      if(reversible)            /// @todo fail when not present
        react.backwardRateIndex = addParameter(constantNames[1]);
    }    
    // see later for echem stuff
  }

  // Now, we build the parameter list.
  

  for(int i = 0; i < species.size(); i++)
    Terminal::out << "Species #" << i << ": " << species[i].name << endl;

  for(int i = 0; i < reactions.size(); i++)
    Terminal::out << "Reaction #" << i << ": " << reactions[i].reactantIndex 
                  << " to " << reactions[i].productIndex << endl;

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
