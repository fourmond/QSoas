/**
   \file kineticsystem.hh 
   Handling of arbitrary kinetic systems

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
#ifndef __KINETICSYSTEM_HH
#define __KINETICSYSTEM_HH

class Expression;


/// These are private subclasses, only visible in the implementation.
class Species;
class Reaction;
class RedoxReaction;


/// The KineticSystem, a class that parses a chemical description of a
/// full kinetic system, be it or not, and provide functions for
/// solving the concentration of species over time or at the
/// steady-state.
///
/// It parses reactions with arbitrary formulas as rate constants. The
/// concentration of the species are reffered to as c_species from
/// within the formulas.
///
/// @todo Add reporters, ie expressions that make up something
class KineticSystem {

  /// Whether the system is linear or not
  bool linear;

  /// Check linearity
  void checkLinearity();

public:

  typedef enum {
    None,                       // no diffusion, ie homogeneous or
                                // adsorbed.
    Forced                      // forced convection, (ie RDE)
  } Diffusion;

protected:

  /// List of species
  QList<Species> species;

  /// List of reactions
  QVector<Reaction *> reactions;

  /// @name Cache
  ///
  /// These attributes are used to setting up and maintaining the cache 
  ///  
  /// @{

  /// The only reactions not handled by the cache
  QVector<RedoxReaction *> redoxReactions;

  /// The cached part of the jacobian, set up by setupCache()
  gsl_matrix * cachedJacobian;

  /// @}

  /// A hash species name -> species index
  QHash<QString, int> speciesLookup;

  /// Returns the index of the species, and registers it if needed.
  int speciesIndex(const QString & str);


  /// All the external parameters of the system. Includes initial
  /// concentrations. The parameters values must be provided \b in \b
  /// the \b same \b order !
  QStringList parameters;

  /// Ensures all the reactions are ready for evaluation, computing
  /// the parameters as a side effect.
  ///
  /// Additional parameters needed (such as initial concentrations for
  /// time evolution, total concentration and potential/temperature
  /// for steady-state current) are given in order.
  void ensureReady(const QStringList & parameters);

  /// Adds a reaction to the system
  void addReaction(QList<QString> species, QList<int> stoechiometry, 
                   int els, 
                   const QStringList & literals,
                   const QString & arrow,
                   const QString & opts = "");



public:

  /// If this is on, then computeLinearJacobian checks that the rate
  /// constants are not negative, and raise an exception if they are.
  ///
  /// On by default.
  bool checkRange;

  /// Builds an empty KineticSystem
  KineticSystem();
  ~KineticSystem();

  KineticSystem(const KineticSystem & other);

  /// Whether or not the system is linear
  bool isLinear() const {
    return linear;
  };


  /// Prepares the system for time evolution, ie using initial
  /// concentrations as additional parameters.
  void prepareForTimeEvolution(const QStringList & extra = QStringList());

  /// Prepares the system for steady-state, using:
  /// @li the potential
  /// @li the temperature
  /// @li and a total concentration (to be changed later on)
  /// 
  /// as the additional parameters
  ///
  /// Takes also an extra list of parameters
  void prepareForSteadyState(const QStringList & extra = QStringList());


  /// Sets up the cache for the linear jacobian. Returns false, if,
  /// for any reason, the cache could not be setup.
  bool setupCache(const double * params);

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


  /// A global scaling factor for all redox reactions. Can be used to
  /// implement very efficiently dispersion of values of k_0.
  double redoxReactionScaling;

  /// Compute the derivatives of the concentrations for the given
  /// conditions, and store them in \a target.
  ///
  /// @question Shall we explicitly include the time here ?  Doesn't
  /// make too much sense to me.
  ///
  /// @todo Make a function taking only a const double *
  /// concentrations_followed_by_parameters ?
  double computeDerivatives(double * target, const double * concentrations,
                            const double * parameters) const;


  /// Computations are performed there.
  ///
  /// It returns the current.
  ///
  /// If \a target is NULL, then only the current is computed.
  double computeDerivatives(gsl_vector * target, 
                            const gsl_vector * concentrations,
                            const double * parameters) const;

  /// Computes the jacobian of a linear system (ie J so that dC/dt = J
  /// C). Will abort if used on non-linear systems.
  ///
  /// If the vector \a coeffs isn't NULL, then is it filled with the
  /// coefficient needed to compute the current (by multiplying with
  /// the concentrations).
  void computeLinearJacobian(gsl_matrix * target,
                             const double * parameters,
                             gsl_vector * coeffs = NULL) const;


  /// Same thing as computeLinearJacobian, but uses the cache.
  void computeCachedLinearJacobian(gsl_matrix * target,
                                   const double * parameters,
                                   gsl_vector * coeffs = NULL) const;


  /// Reads reactions from a file, and add them to the current system.
  void parseFile(QIODevice * stream);


  /// Reads directly the file
  void parseFile(const QString & fileName);

  /// Dump into a file
  void dump(QTextStream & out) const;

  /// Dumps as a string
  QString toString() const;

  /// Returns the exchange rate constants for redox reactions.
  QSet<QString> exchangeRates() const;

  /// A "reporter" expression. If this exists, then the "time value"
  /// of the system is taken to be the result of this expression.
  Expression * reporterExpression;

};

#endif
