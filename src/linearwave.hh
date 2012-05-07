/**
   \file linearwave.hh 
   The LinearWave, a system for computing the current of a
   steady-state catalytic wave of an adsorbed catalyst

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

#ifndef __LINEARWAVE_HH
#define __LINEARWAVE_HH


class LWSpecies;
class LWReaction;
class LWRedoxReaction;

/// This class computes the catalytic wave of an arbitrary linear
/// catalyst (ie all reactions can be represented by first-order or
/// pseudo first-order kinetics) adsorbed onto an electrode.
///
/// Such as system is constructed from the parsing of a reaction file,
/// containing lines in the spirit of:
///   A + e- <=> B [E0, k0, alpha]
///   B -> C [kcat!]
///
/// Here, kcat is made the 'main' reaction...
///
/// @todo Think of a way to do "fast equilibrium for redox species"
/// conditions (although it may just involve systematically chosing
/// the echem rate constants so that they are 100 higher than any of
/// the other ones ?)
///
/// @todo Try to go with the k0 dispersion scheme later on.
class LinearWave {

  /// The various species within the catalytic cycle.
  QList<LWSpecies> species;

  /// An index species name -> index in the species list
  QHash<QString, int> speciesIndex;

  /// The list of (non-redox) reactions
  QList<LWReaction> reactions;

  /// The list of redox reactions
  QList<LWRedoxReaction> redoxReactions;
  
public:

  LinearWave();
  ~LinearWave();

  /// Parses the system in the file pointed to by \a dev.
  ///
  /// Only one system can be parsed for a given LinearWave
  void parseSystem(QIODevice * dev);

  /// Parses straight from a QTextStream
  void parseSystem(QTextStream * s);
};

#endif
