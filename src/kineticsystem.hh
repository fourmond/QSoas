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

  /// A species involved in the reactions.
  class Species {
  public:
    
    /// Name
    QString name;

    /// The indices of the reactions the species takes part in
    QVector<int> reactions;

    /// The type of diffusion.
    Diffusion diffusion;

    Species(const QString & n) : name(n), diffusion(None) {;};
    
  };

  /// A reaction. It is considered an elementary reaction, in the
  /// sense that the rate constants are provided, which are mutiplied
  /// by the concentration to the power of the stoechiometry. However,
  /// it is still possible to cancel out some terms in that by using
  /// the appropriate concentration factor in the rates.
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

    void clearExpressions();

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
