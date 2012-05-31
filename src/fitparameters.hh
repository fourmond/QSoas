/**
   \file fitparameters.hh
   The FitParameters class, handling fit parameters as the user sees them
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

#ifndef __FITPARAMETERS_HH
#define __FITPARAMETERS_HH

#include <possessive-containers.hh>

class FitData;
class FitParameter;
class OutFile;

class FitParametersFile;

/// Holds parameters of a fit (possibly multi-buffer), the way the
/// user edits them.
///
/// This class is also responsible for saving/loading the parameters,
/// for exporting (and importing ?) them and so on.
///
/// @todo Handle import (which is quite different from load).
class FitParameters {

  /// The underlying FitData object.
  FitData * fitData;

  /// A cache parameter name -> index
  QHash<QString, int> parameterIndices;

  /// The number of datasets
  int datasets;

  /// Parameter values (same size as fixed)
  double * values;

  /// This list has the same size as values.
  PossessiveList<FitParameter> parameters;

  /// The number of parameters for each dataset in the "unpacked" data
  int nbParameters;

  friend class FitParameterEditor;

  /// Returns the given parameter
  const FitParameter * parameter(int idx, int ds) const {
    return parameters[idx + nbParameters * ds];
  };

  /// Returns the given parameter
  FitParameter * &parameter(int idx, int ds) {
    return parameters[idx + nbParameters * ds];
  };

  double valueFor(int index, int dataset) const {
    return values[dataset * nbParameters + (index % nbParameters)];
  };

  double & valueFor(int index, int dataset) {
    return values[dataset * nbParameters + (index % nbParameters)];
  };

  void dump() const;

  /// Clears the parameter list
  void clear();

  /// The name of the numbered parameter
  QString parameterName(int idx) const;

  void prepareExport(QStringList & headers, QString & lines, 
                     bool exportErrors = false);

  /// Loads parameters from a parsed parameters file.
  ///
  /// If \a targetDS isn't negative, we copy only from \a sourceDS to
  /// the given targetDS.
  void loadParameters(FitParametersFile & params, int targetDS = -1, 
                      int sourceDS = 0);


  /// This updates the parameters values, by packing from values and
  /// unpacking back to values. This takes care of the
  void updateParameterValues();

public:

  FitParameters(FitData * data);
  ~FitParameters();

  /// Whether or not the given parameter is global
  bool isGlobal(int index) const;

  /// Whether or not the given parameter is fixed
  bool isFixed(int index, int ds) const;
  
  /// Setting a global parameter effectively sets all the parameters !
  void setValue(int index, int dataset, double val) {
    if(isGlobal(index)) {
      for(int i = 0; i < datasets; i++)
        values[index % nbParameters + i*nbParameters] = val;
    }
    else
      values[dataset * nbParameters + (index % nbParameters)] = val;
  };

  /// Sets the value by name
  void setValue(const QString & name, double value);

  /// Gets the value of the given parameter
  double getValue(int index, int dataset) const {
    if(dataset == 0 || isGlobal(index))
      return values[index % nbParameters];
    else
      return values[dataset * nbParameters + (index % nbParameters)];
  };

  /// Returns the underlying data
  const FitData * data() const {
    return fitData;
  };

  /// Various utility functions:


  /// Recompute data stored in the storage vector of fitData
  void recompute();

  /// Translates the current state into FitData parameters.
  void sendDataParameters();

  /// Prepare fit
  void prepareFit();

  /// Retrieve parameters from the fit
  void retrieveParameters();

  /// Resets all parameters to the initial guess
  void resetAllToInitialGuess();

  /// Resets the parameters for the current dataset to the initial
  /// guess.
  void resetToInitialGuess(int ds);

  /// Export to the target stream. This essentially exports raw
  /// values, thereby losing information about what was fixed and what
  /// wasn't.
  ///
  /// On the other hand, it may be easier to work with than the output
  /// of saveParameters (which is closer to the original Soas
  /// version).
  void exportParameters(QIODevice * out, bool writeError = false);

  /// Write the result of the fits to the terminal, as those can be
  /// quite interesting... It is different from exportParameters in
  /// the sense that it is optimized for human reading.
  ///
  /// @todo Write the correlation matrix ? Write the confidence matrix ?
  void writeToTerminal(bool writeMatrix = false);

  /// Export parameters to the given output file (or the default one)
  void exportToOutFile(bool writeError = false, OutFile * out = NULL);

  /// Save to the given stream
  void saveParameters(QIODevice * out) const;

  /// Load from the given stream
  void loadParameters(QIODevice * out, int targetDS = -1, int sourceDS = 0);


  /// Fill up a QTableWidget with the contents of the covariance
  /// matrix
  void setupWithCovarianceMatrix(QTableWidget * widget);
  
};

#endif
