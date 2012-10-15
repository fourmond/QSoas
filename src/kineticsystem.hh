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

class Expression;

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
  /// sense that the rate constants are provided, which are mutiplied
  /// by the concentration to the power of the stoechiometry. However,
  /// it is still possible to cancel out some terms in that by using
  /// the appropriate concentration factor in the rates.
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

    
    Expression * forward;
    Expression * backward;

    Reaction() : forward(NULL), backward(NULL) {;};

    void clearExpressions();

    ~Reaction() {
      clearExpressions();
    };

    /// Computes the expressions
    void makeExpressions(const QStringList & vars = QStringList());

    /// Sets the parameters of the expressions
    void setParameters(const QStringList & parameters);

    /// Returns the parameters needed by the rates
    QSet<QString> parameters() const;

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


  /// All the external parameters of the system. Includes initial
  /// concentrations. The parameters value must be provided \b in \b
  /// the \b same \b order !
  QStringList parameters;

  /// Ensures all the reactions are ready for evaluation, computing
  /// the parameters as a side effect.
  ///
  /// For now, no checking for work already done.
  void ensureReady();
  
public:

  /// Builds an empty KineticSystem
  KineticSystem();
  ~KineticSystem();

  /// Adds a reaction to the system
  void addReaction(QList<QString> species, QList<int> stoechiometry, 
                   const QString & forward, 
                   const QString & backward = "");

  /// Ensures the object is ready to perform computations.
  void prepare();

  /// Returns all the parameters
  QStringList allParameters() const;

  /// Returns the name of all the species, in the order in which they
  /// come in the parameters.
  QStringList allSpecies() const;

  /// The number of species in the system...
  int speciesNumber() const;

  /// Sets the initial concentrations from the parameter list
  void setInitialConcentrations(double * target, 
                                const double * parameters) const;

  /// Compute the derivatives of the concentrations for the given
  /// conditions, and store them in \a target.
  ///
  /// @question Shall we explicitly include the time here ?  Doesn't
  /// make too much sense to me.
  ///
  /// @todo Make a function taking only a const double *
  /// concentrations_followed_by_parameters ?
  void computeDerivatives(double * target, const double * concentrations,
                          const double * parameters) const;


  /// Reads reactions from a file, and add them to the current system.
  void parseFile(QIODevice * stream);


  /// Reads directly the file
  void parseFile(const QString & fileName);

  /// Dump into a file
  void dump(QTextStream & out) const;

  /// Dumps as a string
  QString toString() const;
};

#endif
