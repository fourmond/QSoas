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

class FitData;
class OutFile;

/// Holds parameters of a fit (possibly multi-buffer), the way the
/// user edits them
class FitParameters {

  /// The underlying FitData object.
  FitData * fitData;

  /// The number of datasets
  int datasets;

  /// Parameters are global ? (nb parameters)
  bool * global;

  /// Parameters are fixed ? (nb parameters x nb buffers)
  bool * fixed;

  /// Parameter values (same size as fixed)
  double * values;

  /// The number of parameters for each dataset in the "unpacked" data
  int nbParameters;

public:

  FitParameters(FitData * data);
  ~FitParameters();

  /// Sets the global status of the given parameter
  void setGlobal(int index, bool glbl) {
    global[index % nbParameters] = glbl;
  };

  /// Whether or not the given parameter is global
  bool isGlobal(int index) const {
    return global[index % nbParameters];
  };
  
  /// Sets the fixed status of the given parameter
  void setFixed(int index, int dataset, bool fixd) {
    if(isGlobal(index))
      fixed[index % nbParameters] = fixd;
    else
      fixed[dataset * nbParameters + (index % nbParameters)] = fixd;
  };

  /// Queries the fixed status of the given parameter
  bool isFixed(int index, int dataset) const {
    if(isGlobal(index))
      return fixed[index % nbParameters];
    else
      return fixed[dataset * nbParameters + (index % nbParameters)];
  };

  /// Sets the value of the given parameter
  void setValue(int index, int dataset, double val) {
    if(isGlobal(index))
      values[index % nbParameters] = val;
    else
      values[dataset * nbParameters + (index % nbParameters)] = val;
  };

  /// Gets the value of the given parameter
  double getValue(int index, int dataset) const {
    if(isGlobal(index))
      return values[index % nbParameters];
    else
      return values[dataset * nbParameters + (index % nbParameters)];
  };

  /// Various utility functions:


  /// Recompute data stored in the storage vector of fitData
  void recompute();

  /// Translates the current state into FitData parameters.
  void setDataParameters();

  /// Prepare fit
  void prepareFit();

  /// Retrieve parameters from the fit
  void retrieveParameters();

  /// Resets all parameters to the initial guess
  void resetAllToInitialGuess();

  /// Resets the parameters for the current dataset to the initial
  /// guess.
  void resetToInitialGuess(int ds);

    

  // /// Performs the fit
  // void doFit();

  /// Export to the target stream. This essentially exports raw
  /// values, thereby losing information about what was fixed and what
  /// wasn't.
  ///
  /// On the other hand, it may be easier to work with than the output
  /// of saveParameters (which is closer to the original Soas
  /// version).
  void exportParameters(QIODevice * out) const;


  /// Export parameters to the given output file (or the default one)
  void exportToOutFile(OutFile * out = NULL) const;

  /// Save to the given stream
  void saveParameters(QIODevice * out) const;
  
};

#endif
