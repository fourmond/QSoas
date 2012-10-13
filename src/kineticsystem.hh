/**
   \file kineticsystem.hh 
   Handling of arbitrary kinetic systems

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
#ifndef __KINETICSYSTEM_HH
#define __KINETICSYSTEM_HH

/// The KineticSystem, a class that parses a chemical description of a
/// full kinetic system, be it or not, and provide functions for
/// solving the concentration of species over time or at the
/// steady-state.
///
/// It parses reactions with arbitrary formulas as rate constants. The
/// concentration of the species are reffered to as c_species from
/// within the formulas.
///
/// It feels a little like
class KineticSystem {
public:

  /// A subclass to store species.
  class Species {
  public:
    
    /// Name
    QString name;

    /// The indices of the reactions the species takes part in
    QList<int> reactions;

    Species(const QString & n) : name(n) {;};
    
  };

  /// A reaction. It is considered an elementary reaction, in the
  /// sense that 
  class Reaction {
  public:
    /// List of indices of the reactants or products
    QList<int> speciesIndices;

    /// Their stoechiometry (negative if on the reactants side,
    /// positive if on the products size). In the same order as
    /// speciesIndices.
    QList<int> speciesStoechiometry;

    /// the number of electrons (counted positively if on the left)
    int electrons;

    /// Expression for the forward rate
    QString forwardRate;

    /// Expression for the reverse rate (if empty, then the reaction
    /// is irreversible)
    QString backwardRate;
  };

protected:

  /// List of species
  QList<Species> species;

  /// List of reactions
  QList<Reaction> reactions;

  /// A hash species name -> species index
  QHash<QString, int> speciesLookup;

  /// Returns the index of the species, and registers it if needed.
  int speciesIndex(const QString & str);
  
public:

  /// Builds an empty KineticSystem
  KineticSystem();
  ~KineticSystem();

  void addReaction(QList<QString> species, QList<int> stoechiometry, 
                   const QString & forward, 
                   const QString & backward = "");
  
};

#endif
