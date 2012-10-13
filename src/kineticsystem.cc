/*
  kineticsystem.cc: implementation of KineticSystem
  Copyright 2011 by Vincent Fourmond

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

#include <exceptions.hh>
#include <utils.hh>

KineticSystem::KineticSystem()
{
}

KineticSystem::~KineticSystem()
{
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
