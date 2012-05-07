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


// Temporary includes for commands


class LWSpecies {
public:
  /// The name of the species
  QString name;

  /// @todo cross-refs
};


//////////////////////////////////////////////////////////////////////

LinearWave::LinearWave()
{
}

LinearWave::~LinearWave()
{
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
      throw RuntimeError(QString("Incorrect line for linear wave spec (line #%1)").arg(nb));
    }

    QString reactant = reactionRE.cap(1);
    QString product = reactionRE.cap(3);
    bool reversible = (reactionRE.cap(2) == "<=>");
    QString add = reactionRE.cap(4);

    // see later for echem stuff
    
    
  }
}




//////////////////////////////////////////////////////////////////////

