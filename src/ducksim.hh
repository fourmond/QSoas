/**
   @file duckskim.hh
   DuckSim-based fits
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
#include <perdatasetfit.hh>

/// This fit interfaces (badly !!) with duck-sim to provide fits for
/// diffusive electrochemical data.
///
/// @todo For now, the fit relies on temporary files and
/// writing/reading of text files, which is quite stupid
///
/// @toto In addition, it doesn't support at all multi-threading,
/// which is quite stupid too. (but it's not too slow)
class DuckSimFit : public PerDatasetFit {

  /// Full path to the duck-sim executable.
  static QString fullPath;


  /// The instance of the fit
  static DuckSimFit * theInstance;

  /// The parameters of the fit 
  QStringList fitParameters;

  /// The corresponding initial values
  Vector initialValues;

  /// And whether or not they are fixed by default
  QList<bool> fixedParameters;

  /// The temporary directory currently in use
  QString tempDir;

  /// The specification of the system, as an argument to duck-sim
  QStringList systemSpec;

  /// The system we're using
  QString system;

  /// Dispose of the temporary directory
  void removeTemporaryDirectory();

  /// Additional parameter specs
  QString additionalParameters;

  /// Runs duck-sim with the given arguments, and returns the output.
  QString runDuckSim(const QStringList & args, bool * success = NULL);

protected:

  virtual void processOptions(const CommandOptions & opts);

  virtual QString optionsString() const;


public:

  virtual QList<ParameterDefinition> parameters() const;
  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target);

  virtual void initialGuess(FitData * params, 
                            const DataSet *ds,
                            double * a);

  DuckSimFit();

  /**
     This function searches for DuckSim and registers the fit if it
     finds it.
  */
  static void initialize();
};

