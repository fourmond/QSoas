/**
   \file parameterspaceexplorer.hh
   Parameter space explorers
   Copyright 2018 by Vincent Fourmond

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
#ifndef __PARAMETERSPACEEXPLORER_HH
#define __PARAMETERSPACEEXPLORER_HH

#include <argumentmarshaller.hh>
#include <argumentlist.hh>
#include <factory.hh>
#include <vector.hh>

class ParameterSpaceExplorer;
class FitWorkspace;
class FitTrajectory;
class Command;
class CommandEffector;

/// The parameter specification. It is in fact a parameter range
/// specification: the parameter comes with a range and a lin/log
/// qualification
class ParameterRangeSpec {
public:
  /// The target, like what is returned by
  /// FitWorkspace::parseParameterList()
  QPair<int, int> parameter;

  /// The lower end of the range
  double low;

  /// The higher end of the range
  double high;

  /// Whether the range is logarithmic or not
  bool log;

  /// Whether or not the selection is global (i.e. all local
  /// parameters are set to the same starting value)
  bool uniform;


  /// Can be used by the explorers to store whatever.
  double storage;


  /// The center of the range
  double center() const;
  
  /// Returns 1/2 of the witdth of the interval (log10 or lin).
  double sigma() const;

  /// Returns the given value, trimmed to be within the range.
  double trim(double val) const;

  /// Returns the distance between the two values, normalized by the range
  /// (and taking into account the log vs lin).
  /// That value is always positive
  double distance(double x1, double x2) const;

  /// Parses the parameter list
  static QList<ParameterRangeSpec> parseSpecs(const QStringList & specs,
                                              FitWorkspace * workSpace,
                                              QStringList * unknowns);

  /// Computes the distance between the two trajectories, using the
  /// list of parameters as measure of distance.
  /// Any global parameter is taken to mean all the parameters.
  /// Any parameter not listed in the range specs is ignored
  static double trajectoryDistance(const QList<ParameterRangeSpec> & specs,
                                   const FitTrajectory & a,
                                   const FitTrajectory & b,
                                   const FitWorkspace * workspace,
                                   bool useFinal = true);

  /// Distance of parameter vectors
  static double trajectoryDistance(const QList<ParameterRangeSpec> & specs,
                                   const Vector & a,
                                   const Vector & b,
                                   const FitWorkspace * workspace);

  /// Averages the given trajectories according to the specifications
  /// (only the lin vs log is taken into consideration).
  /// All other parameters are silently ignored
  static Vector averageParameters(const QList<ParameterRangeSpec> & specs,
                                  const QList<Vector> & parameters,
                                  const FitWorkspace * workspace);
};


//////////////////////////////////////////////////////////////////////

class ParameterSpaceExplorerFactoryItem :
  public Factory<ParameterSpaceExplorer, FitWorkspace *> {
  Command * cmd;
public:


  /// An alias for the description field
  QString & publicName;
  
  /// Creates and register a factory item.
  ParameterSpaceExplorerFactoryItem(const QString & n, 
                                    const QString & pn,
                                    const ArgumentList &args,
                                    const ArgumentList &opts,
                                    Creator c);
};


//////////////////////////////////////////////////////////////////////

/// Generates a style for the next curve.
class ParameterSpaceExplorer {

  friend class ParameterSpaceExplorerFactoryItem;

  static ParameterSpaceExplorerFactoryItem * namedFactoryItem(const QString & name);

protected:
  /// The underlying workspace
  FitWorkspace * workSpace;


  static CommandEffector * explorerEffector(const QString & n);

  /// A list of prefit-hooks. If any of them does not return true, the
  /// fit is aborted.
  QList<std::function<bool ()> > preFitHooks;

public:

  /// If this is true, a linear pre-fit is run
  bool linearPreFit;

protected:

  /// Run all the hooks, and returns true only if one should proceed.
  bool runHooks() const;


  /// Writes the parameters given in the vector to the terminal:
  void writeParametersVector(const Vector & parameters) const;


  /// @name Disabling/enabling of the buffers
  ///
  /// Enables/disables buffers from the fit. Disabling sets their
  /// weight to 0 and fixes all the local parameters. Re-enabling
  /// reset the weight to the original one re-frees initially free
  /// local parameters.
  ///
  /// This function relies on the state at the creation of the
  /// ParameterSpaceExplorer.
  ///
  /// @{

  /// The buffer weights at creation time
  Vector savedWeights;

  /// The status fixed/free (fixed is true) for each parameter.
  QList<bool> savedFixed;

  /// Enables or disables the numbered buffer.
  void enableBuffer(int nb, bool enable = true);
  /// @}

  
public:

  /// Adds a hook to run before launching the script
  void addHook(const std::function<bool ()> & func);

  /// Clears all the hooks.
  void clearHooks();
    

  /// The item used to create the 
  ParameterSpaceExplorerFactoryItem * createdFrom;

  /// Creates the parameter space exploration
  ParameterSpaceExplorer(FitWorkspace * workSpace);
  virtual ~ParameterSpaceExplorer();

  /// A hash public name -> real name for the explorers.
  static QHash<QString, QString> availableExplorers();

  /// Creates the named parameter space explorer
  static ParameterSpaceExplorer * createExplorer(const QString & name,
                                                 FitWorkspace * ws);
  

  /// @name Public interface
  ///
  /// @{

  /// Setup the explorer with the given arguments and options.
  virtual void setup(const CommandArguments & args,
                     const CommandOptions & options) = 0;

  /// Runs the next explorer iteration. Returns true if there are
  /// still iteratons available, and false if not.
  ///
  /// What is meant by running the next iteration is the following:
  /// @li preparing the initial parameters.
  /// @li running a fit (or several, or many), unless @a justPick is true
  ///
  /// The user will have the opportunity to execute a script right @b
  /// after and also @b before each iteration.
  virtual bool iterate(bool justPick = false) = 0;

  /// Returns the current progress
  virtual QString progressText() const = 0;

  /// @}
};


#endif
