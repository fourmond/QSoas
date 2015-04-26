/**
   \file fitparameters.hh
   The FitParameters class, handling fit parameters as the user sees them
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014 by CNRS/AMU

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
#ifndef __FITPARAMETERS_HH
#define __FITPARAMETERS_HH


#include <fitengine.hh>
#include <possessive-containers.hh>
#include <vector.hh>

class FitData;
class FitParameter;
class OutFile;

class FitParametersFile;

class CurveData;

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
                     bool exportErrors = false, bool exportMeta = true);

  /// Loads parameters from a parsed parameters file.
  ///
  /// If \a targetDS isn't negative, we copy only from \a sourceDS to
  /// the given targetDS.
  void loadParameters(FitParametersFile & params, int targetDS = -1, 
                      int sourceDS = 0);


  /// This updates the parameters values, by packing from values and
  /// unpacking back to values. This takes care of the
  void updateParameterValues();

  /// Two temporary storage for the covariance matrix
  gsl_matrix * rawCVMatrix;
  gsl_matrix * cookedCVMatrix;

  /// Frees the matrices
  void freeMatrices();

  /// Allocates and computes the covariance matrices.
  void computeMatrices();

  /// Writes the text to the stream-like target
  template <typename T>  void writeText(T & target, 
                                        bool writeMatrix = false,
                                        const QString & prefix = "") const;

public:

  /// @name Residuals
  ///
  /// Residuals-related storage
  /// @{


  /// The point-average residuals for each dataset
  Vector pointResiduals;

  /// The overall point residuals
  double overallPointResiduals;

  /// The relative residuals for each dataset
  Vector relativeResiduals;

  /// The overall relative residuals;
  double overallRelativeResiduals;

  /// Recomputes all the residuals -- assumes that the
  /// fitData->storage contains the right parameters.
  void computeResiduals();
  
  /// @}

  /// The perpendicular coordinates. FitDialog should make them up
  /// when appropriate. There are as many elements as the number of
  /// datasets, or 0 elements if there are no relevant perpendicular
  /// coordinates.
  Vector perpendicularCoordinates;

  bool hasPerpendicularCoordinates() const;


  FitParameters(FitData * data);
  ~FitParameters();

  /// Whether or not the given parameter is global
  bool isGlobal(int index) const;

  /// Whether or not the given parameter is fixed
  bool isFixed(int index, int ds) const;
  
  /// Setting a global parameter effectively sets all the parameters !
  void setValue(int index, int dataset, double val);
  
  /// Sets the value by name. 
  void setValue(const QString & name, double value, int ds = -1);

  /// Gets the value of the given parameter
  double getValue(int index, int dataset) const {
    if(dataset == 0 || isGlobal(index))
      return values[index % nbParameters];
    else
      return values[dataset * nbParameters + (index % nbParameters)];
  };

  /// Returns the value of the \b relative error for the given parameter:
  double getParameterError(int index, int dataset, 
                           double confidenceThreshold = 0.975) const;

  /// Returns the underlying data
  const FitData * data() const {
    return fitData;
  };


  /// Returns the parameters for the numbered dataset
  QHash<QString, double> parametersForDataset(int ds) const;

  /// Various utility functions:


  /// Recompute data stored in the storage vector of fitData. Also
  /// updates the residuals.
  void recompute();

  /// Force the recomputation of the jacobian, useful to ensure that
  /// the errors are up-to-date. Use with caution
  void recomputeJacobian();

  /// Returns the computed subfunctions
  QList<Vector> computeSubFunctions();

  /// Translates the current state into FitData parameters.
  void sendDataParameters();

  /// Prepare fit
  void prepareFit(FitEngineFactoryItem * fitEngine = NULL);

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
  /// matrix. If not in raw mode, display the correlation coefficients
  /// and the square root of the diagonal parameters.
  ///
  /// A side effect of this function (is that a hack ?)
  void setupWithCovarianceMatrix(QTableWidget * widget, bool raw = false);


  /// Writes the covariance matrix to a file
  void writeCovarianceMatrix(QTextStream & out,  bool raw = false);

  /// Writes the covariance matrix in latex-friendly form to a file.
  void writeCovarianceMatrixLatex(QTextStream & out,  bool raw = false);



  /// @name Functions to backup parameters...
  ///
  /// @{

  /// Saves the current parameters as a Vector
  Vector saveParameterValues();

  /// Returns the \b relative errors on the parameters as a Vector
  Vector saveParameterErrors(double confidenceThreshold = 0.975);

  /// Restores the previously saved values.
  void restoreParameterValues(const Vector & values);


  /// @}
  
};


/// This class wraps up a QTableWidget to display covariance matrices
/// (and possibly save them up ?)
class CovarianceMatrixDisplay : public QDialog {
  Q_OBJECT;

  FitParameters * parameters;
  
  QTableWidget * widget;

  void setupFrame();
public:
  CovarianceMatrixDisplay(FitParameters * params, QWidget * parent = 0);

public slots:

  /// Export to file
  void exportToFile();

  /// Export as a LaTeX table ?
  void exportAsLatex();
};

#endif
