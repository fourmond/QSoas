/**
   \file fitparameter.hh
   Implementation of FitParameter and its subclasses
   Copyright 2011, 2012 by Vincent Fourmond

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


#ifndef __FITPARAMETER_HH
#define __FITPARAMETER_HH

// For the expression handling with the fit parameters...
#include <expression.hh>

class FitData;

/// Base class for effective parameters
class FitParameter {
public:
  /// The index of the parameters from within the Fit::parameters()
  /// return value.
  int paramIndex;

  /// The index of the dataset (-1) for global parameters.
  int dsIndex;

  /// The index of the parameter in the fit vector (-1 if not part of
  /// the fit vector)
  int fitIndex;

  FitParameter(int p, int ds) : paramIndex(p), dsIndex(ds), 
                                fitIndex(-1) {;};

  /// Sets the target parameters (natural parameters) from the fit
  /// parameters (the GSL vector).
  virtual void copyToUnpacked(double * target, const gsl_vector * fit, 
                             int nbdatasets, int nb_per_dataset) const = 0;

  /// Sets values of the fit parameters (the GSL vector) from the
  /// natural parameters
  virtual void copyToPacked(gsl_vector * fit, const double * unpacked,
                            int nbdatasets, int nb_per_dataset) const;

  /// Whether or not the parameter is fixed
  virtual bool fixed() const { return false;};

  /// Whether the parameter is global or dataset-local
  virtual bool global() const { return dsIndex < 0; };

  /// Performs one-time setup at fit initialization
  virtual void initialize(FitData * data);

  /// Whether or not the parameter needs a second pass.
  virtual bool needSecondPass() const { return false; };

  /// Returns a duplicate of the object.
  virtual FitParameter * dup() const = 0;

  /// Returns the text value for the given parameter.
  virtual QString textValue(double value) const {
    return QString::number(value);
  };

  /// Sets the value corresponding to the given string.
  ///
  /// Most children will update \a target, but some may not (think
  /// about formulas...)
  virtual void setValue(double * target, const QString & value) {
    bool ok;
    double v = value.toDouble(&ok);
    if(ok)
      *target = v;
  };

  /// Saves the parameter as a string. It is the second part of the
  /// output of FitParameters::saveParameters().
  ///
  /// Its goal isn't to save context information (indices) but raw
  /// FitParameter information. FitParameters::saveParameters takes
  /// care of the context.
  virtual QString saveAsString(double value) const;

  /// It is the reverse of saveAsString().
  static FitParameter * loadFromString(const QString & str, 
                                       double * target, int paramIndex, 
                                       int dsIndex);

  virtual ~FitParameter();

  /// Whether or not this parameter needs to be initialize'd again.
  virtual bool needsInit() const {
    return false;
  };

protected:

  /// This returns extra parameters information that must be
  /// saved. For instance, that is the way Bijection is stored for
  /// FreeParameter.
  virtual QString saveExtraInfo() const;

  /// Load the information as saved by saveExtraInfo().
  virtual void loadExtraInfo(const QString & str);

};

class Bijection;

/// A parameter, once it's in use. A list of that can be used to
/// convert GSL parameters to dataset-specific parameter values.
class FreeParameter : public FitParameter {

  /// The factor used for derivation (when that applies)
  double derivationFactor;
    
  /// The minimum step used for derivation
  ///
  /// (unused for now)
  double minDerivationStep;

public:

  virtual void copyToUnpacked(double * target, const gsl_vector * fit, 
                             int nbdatasets, int nb_per_dataset) const;


  virtual void copyToPacked(gsl_vector * fit, const double * unpacked,
                            int nbdatasets, int nb_per_dataset) const;

  /// Takes the current value and computes a rough estimate of a good
  /// derivation step ?
  ///
  /// @todo While the default heuristics should work for non-bijected
  /// parameters, bijected parameters are more delicate, as a small
  /// variation of \a value will only translate linearly onto the
  /// target, which isn't what we want at all.
  double derivationStep(double value) const;

  /// The transformation applied to the parameter to go from the GSL
  /// parameter space to the user- and fit- visible parameters.
  ///
  /// Will be NULL most of the times.
  Bijection * bijection;
  
  FreeParameter(int p, int ds, double dev = 1e-4) :
    FitParameter(p, ds), derivationFactor(dev), 
    minDerivationStep(1e-8), bijection(NULL) {;};

  virtual FitParameter * dup() const;

  virtual ~FreeParameter();

protected:
  virtual QString saveExtraInfo() const;
  virtual void loadExtraInfo(const QString & str);
  
};

/// A fixed parameter, ie a parameter whose value is fixed, and
/// hence isn't part of the gsl_vector of parameters.
class FixedParameter : public FitParameter {
public:
  /// The actual value
  mutable double value;

  virtual void copyToUnpacked(double * target, const gsl_vector * fit, 
                             int nbdatasets, int nb_per_dataset) const;


  virtual void copyToPacked(gsl_vector * fit, const double * unpacked,
                            int nbdatasets, int nb_per_dataset) const;


  virtual bool fixed() const { return true;};

  FixedParameter(int p, int ds, double v)  :
    FitParameter(p, ds), value(v) {;};

  virtual FitParameter * dup() const {
    return new FixedParameter(*this);
  };
};

/// A parameter whose value is defined as a function of other
/// parameters.
class FormulaParameter : public FitParameter {
  /// The expression we're using
  Expression * expression;

  /// The parameter dependencies (to ensure they are computed in the
  /// correct order). It is also the argument list to the block
  QStringList dependencies;

  /// The same thing as dependencies, but with their index (in the
  /// parametersDefinition)
  QVector<int> depsIndex;



  QString formula;

  bool needsUpdate;

  // The last value this stuff took... (could be useful ?)
  mutable double lastValue;

public:


  virtual void copyToUnpacked(double * target, const gsl_vector * fit, 
                             int nbdatasets, int nb_per_dataset) const;


  virtual void copyToPacked(gsl_vector * fit, const double * unpacked,
                            int nbdatasets, int nb_per_dataset) const;


  virtual bool fixed() const { return true;};

  FormulaParameter(int p, int ds, const QString & f)  :
    FitParameter(p, ds), expression(NULL), formula(f), needsUpdate(true) {;};

  virtual void initialize(FitData * data);

  virtual bool needSecondPass() const { return true; };

  virtual FitParameter * dup() const {
    return new FormulaParameter(paramIndex, dsIndex, formula);
  };

  virtual QString textValue(double value) const;

  virtual void setValue(double * target, const QString & value);

  virtual bool needsInit() const {
    return needsUpdate;
  };

  virtual ~FormulaParameter();

};

#endif
