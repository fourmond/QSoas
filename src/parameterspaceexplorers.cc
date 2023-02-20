/*
  parameterspaceexplorers.cc: several simple parameter space explorers
  Copyright 2013, 2018, 2019, 2020 by Vincent Fourmond

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
#include <parameterspaceexplorer.hh>
#include <exceptions.hh>

#include <dataset.hh>

#include <fitworkspace.hh>

#include <argumentlist.hh>
#include <general-arguments.hh>

#include <gsl-types.hh>

#include <terminal.hh>
#include <utils.hh>

#include <fitdata.hh>
#include <fittrajectory.hh>

#include <file.hh>

// random generators
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

/// The parameter specification
class ParameterSpec {
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


  /// The center
  double center() const {
    if(log) {
      return sqrt(low * high);
    }
    else
      return 0.5*(low + high);
  };

  /// Returns 1/2 of the witdth of the interval (log10 or lin).
  double sigma() const {
    if(log)
      return 0.5 * (log10(high) - log10(low));
    else
      return 0.5 * (high - low);
  };

  double trim(double val) const {
    if(std::isnan(val))
      return center();          // Safety catch
    return std::max(std::min(val, high), low);
  };

  /// Parses the parameter list
  static QList<ParameterSpec> parseSpecs(const QStringList & specs,
                                         FitWorkspace * workSpace,
                                         QStringList * unknowns) {
    QList<ParameterSpec> parameterSpecs;
    QRegExp re("^\\s*(.*):(u,)?([^:]+)\\.\\.([^:,]+)(,log)?\\s*$");

    for(const QString & s : specs) {
      if(re.indexIn(s) != 0)
        throw RuntimeError("Invalid parameter specification: '%1'").
          arg(s);


      // Now handling: 
      // monte-carlo-explorer tau_1[#0,#1],tau_2[#1]:1e-2..1e2,log
      
      QStringList pars = Utils::nestedSplit(re.cap(1), ',', "[", "]");
      bool uniform = ! re.cap(2).isEmpty();
      double l = re.cap(3).toDouble();
      double h = re.cap(4).toDouble();
      bool log = ! re.cap(5).isEmpty();
      QList<QPair<int, int> > params;
      for(const QString & pa : pars)
        params << workSpace->parseParameterList(pa, unknowns);
      for(const QPair<int, int> & p : params) {
        ParameterSpec sp = {p, l, h, log, uniform, 0};
        parameterSpecs << sp;
      }
    }
    return parameterSpecs;
  };
};

/// A Monte-Carlo parameter space explorer, i.e. an explorer in which
/// one randomly chooses the initial parameters within a given range.
class MonteCarloExplorer : public ParameterSpaceExplorer {

  int iterations;

  int currentIteration;

  int fitIterations;

  int resetFrequency;

  QList<ParameterSpec> parameterSpecs;

  /// @name Gradual exploration
  ///
  /// All the parameters/states/functions necessary for "gradual
  /// exploration", i.e. the exploration starting with a limited
  /// number of buffers, deepening on successful fits.
  ///
  /// Gradual fits go thus:
  /// 
  /// @li we select an initial set of buffers (a small number, at
  /// least two) with which most of the fits are going to be performed
  /// @li to actually select only a subset of buffers, we just fix all
  /// parameters of unselected buffers and set their weights temporarily
  /// to 0. This is done via the selectBuffers() function.
  /// @li When deepening, include one buffer between each previously
  /// selected buffers and just interpolate the monte-carlo
  /// parameters. The deepening only occurs after the fits.
  ///
  /// @{

  /// The number of buffers initially present (parameter)
  int gradualDatasets;

  /// The threshold above the best so far to trigger deepening
  /// (parameter)
  double gradualThreshold;

  /// The list of initial buffers. Empty if no gradual approach.
  QList<int> initialBuffers;

  /// The trajectories so far, by
  QList<FitTrajectories> trajectoriesPerLevel;

  /// Selects the numbered buffers for inclusion in the fit (and
  /// exclude the other ones, then).
  void selectBuffers(const QSet<int> & selected = QSet<int>()) {
    int nbds = workSpace->datasetNumber();
    for(int i = 0 ; i < nbds; i++)
      enableBuffer(i, selected.isEmpty() || selected.contains(i));
  };

  void selectBuffers(const QList<int> & l) {
    selectBuffers(l.toSet());
  };

  /// @}


public:

  static ArgumentList args;
  static ArgumentList opts;

  MonteCarloExplorer(FitWorkspace * ws) :
    ParameterSpaceExplorer(ws), iterations(20),
    currentIteration(0), fitIterations(50), resetFrequency(0) {
  };

  virtual void setup(const CommandArguments & args,
                     const CommandOptions & opts) override {

    QStringList specs = args[0]->value<QStringList>();

    QStringList unknowns;


    parameterSpecs = ParameterSpec::parseSpecs(specs, workSpace, &unknowns);

    if(unknowns.size() > 0)
      Terminal::out << "WARNING: could not understand the following parameters: "
                    << unknowns.join(", ")
                    << endl;
    if(parameterSpecs.size() == 0)
      throw RuntimeError("Could understand no parameters at all");
       

    updateFromOptions(opts, "iterations", iterations);
    updateFromOptions(opts, "fit-iterations", fitIterations);
    updateFromOptions(opts, "reset-frequency", resetFrequency);
    gradualDatasets = -1;
    updateFromOptions(opts, "gradual-datasets", gradualDatasets);
    gradualThreshold = 8;
    updateFromOptions(opts, "gradual-threshold", gradualThreshold);

    Terminal::out << "Setting up monte-carlo explorator with: "
                  << iterations << " iterations and "
                  << fitIterations << " fit iterations" << endl;


    QStringList names = workSpace->parameterNames();
    for(const ParameterSpec & s : parameterSpecs)
      Terminal::out << " * " << names[s.parameter.first]
                    << "[#" << s.parameter.second << "]: "
                    << s.low << " to " << s.high
                    << (s.log ? " log" : " lin")
                    << (s.uniform ? " -- uniform" : "")
                    << endl;

    // Deal with the gradual case
    initialBuffers.clear();
    trajectoriesPerLevel.clear();
    int nbds = workSpace->datasetNumber();
    if(gradualDatasets > 1 && gradualDatasets < nbds) {
      Terminal::out << "Selecting initial buffers for the gradual approach: "
                    << endl;
      for(int i = 0; i < gradualDatasets; i++) {
        int ds = (i*(nbds-1))/(gradualDatasets-1);
        initialBuffers << ds;
        Terminal::out << " * #" << ds << endl;
      }
    }
  };

  virtual bool iterate(bool justPick) override {
    QStringList names = workSpace->parameterNames();
    Terminal::out << "Setting initial parameters: " << endl;
    if(resetFrequency > 0
       && (((currentIteration + 1) % resetFrequency) == 0)) {
      if(workSpace->trajectories.size() < 1)
        Terminal::out << "Cannot reset parameters: no trajectories available so far" << endl;
      else {
        const FitTrajectory & best = workSpace->trajectories.best();
        workSpace->restoreParameterValues(best.finalParameters);
        Terminal::out << "Restoring parameters to the best parameters so far: "
                      << best.residuals << endl;
      }
    }
    
    QHash<int, double> uniformSetValues;
    for(const ParameterSpec & s : parameterSpecs) {
      double v;
      if(s.uniform && uniformSetValues.contains(s.parameter.first))
        v = uniformSetValues[s.parameter.first];
      else
        v = Utils::random(s.low, s.high, s.log);
      if(s.uniform)
        uniformSetValues[s.parameter.first] = v;
      workSpace->setValue(s.parameter.first, s.parameter.second, v);
      Terminal::out << " -> " << names[s.parameter.first]
                    << "[#" << s.parameter.second << "] =  " << v << endl;
    }
    if(! runHooks())
      return false;
    if(! justPick) {
      selectBuffers(initialBuffers);
      workSpace->runFit(fitIterations);
      if(initialBuffers.size() > 0) {
        int nbds = workSpace->datasetNumber();
        int level = 0;
        QList<int> buffers = initialBuffers;
        while(buffers.size() < nbds) {
          const FitTrajectory & latest = workSpace->lastTrajectory();
          if(trajectoriesPerLevel.size() <= level)
            trajectoriesPerLevel << FitTrajectories(workSpace);
          if(trajectoriesPerLevel[level].size() == 0 ||
             latest.residuals <= gradualThreshold * trajectoriesPerLevel[level].best().residuals) {
            trajectoriesPerLevel[level] << latest;
            level++;
            Terminal::out << "Gradual exploration: iteration "
                          << currentIteration + 1
                          << " level " << level << endl;
            // now we insert the previously disabled buffers, and use
            // interpolation to set their starting local free
            // parameters (the fixed parameters are not touched)
            QList<int> nbf;
            for(int i = 1; i < buffers.size(); i++) {
              int ds = buffers[i];
              int prev = buffers[i-1];
              if(ds == prev+1)
                continue;       // nothing to do
              int nds = (ds+prev)/2;
              enableBuffer(nds);
              Terminal::out << "Enabling buffer #" << nds << endl;
              nbf << nds;

              int nbp = workSpace->parametersPerDataset();
              for(int j = 0; j < nbp; j++) {
                if(workSpace->isGlobal(j))
                  continue;
                if(! workSpace->isFixed(j, nds)) {
                  double v = 0.5 * (workSpace->getValue(j, prev) +
                                    workSpace->getValue(j, ds));
                  workSpace->setValue(j, nds, v);
                }
              }
            }
            if(nbf.size() == 0)
              throw InternalError("Could not add anything ?");
            buffers << nbf;
            std::sort(buffers.begin(), buffers.end());
            QString fn = QString("mcg-level-%1").arg(level);
            workSpace->currentFlags.insert(fn);
            workSpace->runFit(fitIterations);
            workSpace->currentFlags.remove(fn);
          }
          else                  // Not improving significantly, do not
                                // deepen
            break;
        }
      }

      currentIteration++;
      selectBuffers();
    }
    return currentIteration < iterations;
  };

  virtual QString progressText() const override {
    return QString("%1/%2").
      arg(currentIteration+1).arg(iterations);
  };


};

ArgumentList
MonteCarloExplorer::args(QList<Argument*>() 
                         << new SeveralStringsArgument("parameters",
                                                       "Parameters",
                                                       "Parameter specification"));

ArgumentList
MonteCarloExplorer::opts(QList<Argument*>() 
                         << new IntegerArgument("iterations",
                                                "Iterations",
                                                "Number of monte-carlo iterations")
                         << new IntegerArgument("fit-iterations",
                                                "Fit iterations",
                                                "Maximum number of fit iterations")
                         << new IntegerArgument("reset-frequency",
                                                "Reset to best frequency",
                                                "If > 0 reset to the best parameters every that many iterations")
                         << new IntegerArgument("gradual-datasets",
                                                "Gradual datasets",
                                                "Number of starting datasets when doing gradual exploration")
                         );

ParameterSpaceExplorerFactoryItem 
montecarlo("monte-carlo", "Monte Carlo",
           MonteCarloExplorer::args,
           MonteCarloExplorer::opts,
           [](FitWorkspace *ws) -> ParameterSpaceExplorer * {
             return new MonteCarloExplorer(ws);
           });


//////////////////////////////////////////////////////////////////////

/// An "adaptive" Monte-Carlo parameter space explorer, as defined
/// (but for a minimization algorithm) in Walter & Pronzato, 1995,
/// section 4.7.3.1.
class AdaptiveMonteCarloExplorer : public ParameterSpaceExplorer {

  /// The number of iterations in a "level" (decreasing for the
  /// exploration part)
  int iterations;

  /// Current iteration within the level
  int currentIteration;

  /// The maximum number of levels
  int maxLevel;

  /// The current level
  int currentLevel;

  /// The scale in sigma between the various levels
  double scale;

  /// The best residuals at the end of the previous cycle
  double previousCycleBest;

  /// Whether we are in the exploration phase or in the exploitation
  /// phase
  bool exploration;

  /// the maximum number of cycles
  int cycles;

  /// The current cycle
  int currentCycle;

  /// The number of useless cycles, i.e. consecutive cycles that yield
  /// no significant improvement to the residuals.
  int uselessCycles;

  /// The number of fit iterations
  int fitIterations;

  /// Whether we use the final paramters or the initial as center for
  /// the next iteration.
  bool isFinal;

  /// The list of the current best trajectories per level;
  QList<FitTrajectory> bestTrajectories;

  /// Initialization: 0, init not done, 1, center not done, 2 = OK.
  int init;

  FitTrajectory currentCenter;

  /// The current specs.
  QList<ParameterSpec> parameterSpecs;

  gsl_rng * rnd;

public:

  static ArgumentList args;
  static ArgumentList opts;

  AdaptiveMonteCarloExplorer(FitWorkspace * ws) :
    ParameterSpaceExplorer(ws),
    iterations(100),
    currentIteration(0),
    maxLevel(5),
    currentLevel(0),
    scale(5),
    exploration(true),
    cycles(5),
    currentCycle(0),
    uselessCycles(0),
    fitIterations(50),
    isFinal(false),
    init(0)
  {
    rnd = gsl_rng_alloc(gsl_rng_mt19937);
  };

  ~AdaptiveMonteCarloExplorer() {
    gsl_rng_free(rnd);
  };

  virtual void setup(const CommandArguments & args,
                     const CommandOptions & opts) override {

    QStringList specs = args[0]->value<QStringList>();

    QStringList unknowns;



    parameterSpecs = ParameterSpec::parseSpecs(specs, workSpace, &unknowns);

    if(unknowns.size() > 0)
      Terminal::out << "WARNING: could not understand the following parameters: "
                    << unknowns.join(", ")
                    << endl;
    if(parameterSpecs.size() == 0)
      throw RuntimeError("Could understand no parameters at all");

    scale = 6;
    updateFromOptions(opts, "fit-iterations", fitIterations);
    updateFromOptions(opts, "use-final", isFinal);
    updateFromOptions(opts, "scale", scale);
    updateFromOptions(opts, "iterations", iterations);

    bool skip = false;
    updateFromOptions(opts, "skip-current", skip);

    // Reset internal parameters
    currentIteration = 0;
    currentCycle = 0;
    currentLevel = 0;
    bestTrajectories.clear();
    exploration = true;
    previousCycleBest = 0;
    uselessCycles = 0;
    init = skip ? 1 : 0;

    Terminal::out << "Setting up the adaptive explorer with the following parameters: " << endl;
    
                                                                                    QStringList names = workSpace->parameterNames();
    for(const ParameterSpec & s : parameterSpecs)
      Terminal::out << " * " << names[s.parameter.first]
                    << "[#" << s.parameter.second << "] : "
                    << s.center() << " +- " << s.sigma()
                    << (s.log ? " (log)" : " (lin)")
                    << (s.uniform ? ",  uniform" : "")
                    << endl;
    
  };

  /// Called whenever one changes the current level, it sets the sigma
  /// values for the level
  void prepareLevel() {
    double curScale = pow(scale, -currentLevel);
    
    QStringList names = workSpace->parameterNames();

    Terminal::out << "Preparing sigmas for level " << currentLevel
                  << ":" << endl;
    for(ParameterSpec & s : parameterSpecs) {
      if(s.log) {
          // The idea is to go from 0.01 at the deepest level to full
          // sigma at 0.
          double slm = 0.01;
          double sigma = std::max(0.1,s.sigma());
          sigma = std::max(sigma-currentLevel * log10(scale), slm);
          s.storage = sigma;
      }
      else {
        s.storage = s.sigma() * curScale;
      }
      Terminal::out << " * " << names[s.parameter.first]
                    << "[#" << s.parameter.second << "] : "
                    << s.storage
                    << endl;
    }
    
  };

  /// Generates a random displacement using the current Level
  Vector addRandomDisplacement(const Vector & src) const {
    Vector dp = src;
    QHash<int, double> uniformSetValues;
    int nbPDs = workSpace->parametersPerDataset();
    for(const ParameterSpec & s : parameterSpecs) {
      double v;
      if(s.uniform && uniformSetValues.contains(s.parameter.first))
        v = uniformSetValues[s.parameter.first];
      else
        v = gsl_ran_gaussian(rnd, s.storage);
      
      if(s.uniform)
        uniformSetValues[s.parameter.first] = v;

      int idx = s.parameter.first + nbPDs*std::max(s.parameter.second,0);
      if(s.log) {
        v += log10(src[idx]);
        dp[idx] = pow(10, v);
      }
      else
        dp[idx] += v;
      // Force the parameter into the specified segment.
      dp[idx] = s.trim(dp[idx]);
    }
    return dp;
  };

  Vector trajectoryParameters(const FitTrajectory & trj) const {
    if(isFinal)
      return trj.finalParameters;
    else
      return trj.initialParameters;
  };

  virtual bool iterate(bool justPick) override {
    Vector base, params;
    int maxIt = iterations;
    if(exploration)
      maxIt = maxIt/(sqrt(currentLevel + 1));
    if(maxIt < 2)               // At least two iterations per level
      maxIt = 2;

    if(init == 0) {             // First step: initial parameters
      Terminal::out << "Initialization: starting parameters" << endl;
      params = workSpace->saveParameterValues();
      base = params;
    }
    if(init == 1) {             // Second step: center
      Terminal::out << "Initialization: center" << endl;
      // Prepare initial parameters.
      int nbPDs = workSpace->parametersPerDataset();
      params = workSpace->saveParameterValues();
      base = params;
      
      for(const ParameterSpec & s : parameterSpecs) {
        params[s.parameter.first +
               nbPDs*std::max(s.parameter.second,0)] =
          s.center();
      }
    }
    if(init > 1) {            
      ++currentIteration;
      Terminal::out << "Iteration " << currentIteration << "/" << maxIt
                    << " of level " << currentLevel << " ("
                    << (exploration ? "exploration" : "exploitation")
                    << " phase)" << " of cycle " << currentCycle
                    << endl;

      // The base is either the current center or the best trajectory
      // of the current level, whichever is the lowest.
      FitTrajectory trj = currentCenter;
      if(bestTrajectories.size() > currentLevel && bestTrajectories[currentLevel] < trj)
        trj = bestTrajectories[currentLevel];

      base = trajectoryParameters(trj);
      params = addRandomDisplacement(base);
    }
    
    Terminal::out << "Parameters:" << endl;
    QStringList names = workSpace->parameterNames();
    int nbPDs = workSpace->parametersPerDataset();
    for(const ParameterSpec & s : parameterSpecs) {
      int idx = s.parameter.first + nbPDs*std::max(s.parameter.second, 0);
      Terminal::out << " * " << names[s.parameter.first]
                    << "[#" << s.parameter.second << "] = "
                    << base[idx] << " -> " 
                    << params[idx]
                    << endl;
    }

    workSpace->restoreParameterValues(params);
    
    if(! runHooks()) {
      Terminal::out << "The hooks failed, ending exploration" << endl;
      return false;
    }
    if(! justPick) {
      workSpace->runFit(fitIterations);
      const FitTrajectory & latest = workSpace->lastTrajectory();

      if(init == 0) {
        currentCenter = latest;
        ++init;
        return true;
      }
      if(init == 1) {
        ++init;
        if(currentCenter.initialParameters.size() == 0)
          currentCenter = latest;
        else
          currentCenter = std::min(latest, currentCenter);
        prepareLevel();
        return true;
      }
      
      // Initialize with the current trajectory
      while(bestTrajectories.size() <= currentLevel)
        bestTrajectories << latest;

      if(latest < bestTrajectories[currentLevel]) {
        Terminal::out << "Improved residuals (level " << currentLevel
                      << ") from "
                      << bestTrajectories[currentLevel].residuals
                      << " to " << latest.residuals << endl;
         bestTrajectories[currentLevel] = latest;
      }

      // Ok, so now we see whether we are at the end of a cycle
      if(currentIteration >= maxIt) {
        if(exploration) {
          ++currentLevel;
          currentIteration = 0;
            
          if(currentLevel >= maxLevel) {
            // We switch to the exploitation phase
            int bestLevel = 0;
            for(int i = 1; i < bestTrajectories.size(); i++) {
              // only select if we significantly decrease the residuals
              if(bestTrajectories[i].residuals < 0.9 * bestTrajectories[bestLevel].residuals)
                bestLevel = i;
            }
            Terminal::out << "End of exploration phase: results:" << endl;
            for(int i = 0; i < bestTrajectories.size(); i++)
              Terminal::out << " * level #" << i << " -> "
                            << bestTrajectories[i].residuals << endl;
              
            Terminal::out << "Switching to exploitation phase around level "
                          << bestLevel << ", using the following center: " << endl;
                                                                              
            currentLevel = bestLevel;
            writeParametersVector(trajectoryParameters(std::min(bestTrajectories[currentLevel], currentCenter)));
            exploration = false;
          }
          prepareLevel();
          return true;
        }
        else {
          Terminal::out << "Best residuals of the cycle: "
                        << bestTrajectories[currentLevel].residuals << endl;
          Terminal::out << "Overall best: " << currentCenter.residuals << endl;
          if(currentCycle > 0) {
            Terminal::out << "Best residuals of the previous cycle: "
                        << previousCycleBest << endl;
            if(previousCycleBest * 0.999 <= bestTrajectories[currentLevel].residuals) {
              uselessCycles++;
              if(uselessCycles >= 3) {
                Terminal::out << "No improvement at all in the last three cycles, stopping here" << endl;
                return false;
              }
            }
            else
              uselessCycles = 0;
          }


          // end of exploitation
          currentCycle++;
          if(currentCycle >= cycles) {
            Terminal::out << "Reached the maximum number of cycles: "
                          << currentCycle << "/" << cycles << endl;
            return false;
          }
            
          currentCenter = std::min(currentCenter,
                                   bestTrajectories[currentLevel]);

          // We restart around the current center
          Terminal::out << "Switching back to exploration phase around the best residuals: "
                        << currentCenter.residuals << endl;
          writeParametersVector(trajectoryParameters(currentCenter));

          
          // Copying the best residuals
          previousCycleBest = currentCenter.residuals;
          bestTrajectories.clear();

          currentIteration = 0;
          currentLevel = 0;
          prepareLevel();
          exploration = true;
        }
      }
      return true;
    }
    else {
      Terminal::out << "Warning: it does not make so much sense to use /just-pick=true for the adaptive explorer" << endl;
        
    }
    return true;
  };

  virtual QString progressText() const override {
    return QString("%1/%2").
      arg(currentIteration+1).arg(iterations);
  };


};

ArgumentList
AdaptiveMonteCarloExplorer::args(QList<Argument*>() 
                                 << new SeveralStringsArgument("parameters",
                                                               "Parameters",
                                                               "Parameter specification"));

ArgumentList
AdaptiveMonteCarloExplorer::opts(QList<Argument*>() 
                                 << new IntegerArgument("iterations",
                                                        "Iterations",
                                                        "Number of monte-carlo iterations per 'level'")
                                 << new NumberArgument("scale",
                                                       "Scaling factor",
                                                       "Scaling factor between the levels")
                                 << new BoolArgument("use-final",
                                                     "Use final",
                                                     "If true, center the exploration around the final parameters rather than the initial (default: false)")
                                 << new BoolArgument("skip-current",
                                                     "Skip current",
                                                     "If true, skips the current parameters in the exploration (default: false)")
                                 << new IntegerArgument("fit-iterations",
                                                        "Fit iterations",
                                                        "Maximum number of fit iterations")
                         );

ParameterSpaceExplorerFactoryItem 
adaptivemontecarlo("adaptive", "Adaptive Monte Carlo",
           AdaptiveMonteCarloExplorer::args,
           AdaptiveMonteCarloExplorer::opts,
           [](FitWorkspace *ws) -> ParameterSpaceExplorer * {
             return new AdaptiveMonteCarloExplorer(ws);
           });



//////////////////////////////////////////////////////////////////////


/// This explorer takes two random trajectories from the trajectory
/// list and mixes them randomly, using either the starting parameters
/// or the final parameters (one after the other).
class ShuffleExplorer : public ParameterSpaceExplorer {

  int iterations;

  int currentIteration;

  int fitIterations;

public:
  static ArgumentList opts;

  ShuffleExplorer(FitWorkspace * ws) :
    ParameterSpaceExplorer(ws), iterations(20),
    currentIteration(0), fitIterations(50) {
  };

  // virtual ArgumentList * explorerArguments() const override {
  //   return NULL;
  // };

  // virtual ArgumentList * explorerOptions() const override {
  //   return &opts;
  // };

  virtual void setup(const CommandArguments & /*args*/,
                     const CommandOptions & opts) override {
    updateFromOptions(opts, "iterations", iterations);
    updateFromOptions(opts, "fit-iterations", fitIterations);
  };

  virtual bool iterate(bool justPick) override {
    Vector p;
    int sz = workSpace->trajectories.size();
    if(sz < 2)
      throw RuntimeError("Cannot iterate the shuffle explorer with no trajectories");
    p = workSpace->trajectories[0].initialParameters;

    int id1 = ::rand() % sz;
    int id2 = ::rand() % sz;
    if(id2 == id1)
      --id2;
    if(id2 < 0)
      id2 = sz-1;               // This guarantees id1 != id2
    if(id1 == id2)
      throw InternalError("Identical indices, that really should not happen: %1 %2").
        arg(id1).arg(id2);

    double res = std::min(workSpace->trajectories[id1].residuals,
                          workSpace->trajectories[id2].residuals);
    Vector p1,p2;
    if(currentIteration % 2 == 0) {
      Terminal::out << "shuffle explorer picking from final parameters"
                    << endl;
      p1 = workSpace->trajectories[id1].finalParameters;
      p2 = workSpace->trajectories[id2].finalParameters;
    }
    else {
      Terminal::out << "shuffle explorer picking from initial parameters"
                    << endl;
      p1 = workSpace->trajectories[id1].initialParameters;
      p2 = workSpace->trajectories[id2].initialParameters;
    }
    Terminal::out << "shuffle starting residuals:"
                  << workSpace->trajectories[id1].residuals 
                  << " (#" << id1 << "), "
                  << workSpace->trajectories[id2].residuals
                  << " (#" << id2 << ")" << endl;
    
    for(int i = 0; i < p.size(); i++) {
      int w = ::rand() % 2;
      p[i] = (w ? p1[i] : p2[i]);
    }
    workSpace->restoreParameterValues(p);
    if(! runHooks())
      return false;

    if(! justPick) {
      ++currentIteration;
      workSpace->runFit(fitIterations);
      double final = workSpace->lastTrajectory().residuals;
      if(final < res)
        Terminal::out << "shuffle succeeded in improving the residuals: "
                      << workSpace->trajectories[id1].residuals 
                      << ", "
                      << workSpace->trajectories[id2].residuals
                      << " => " << final
                      << endl;
    }
    return currentIteration < iterations;
  };

  virtual QString progressText() const override {
    return QString("%1/%2").
      arg(currentIteration+1).arg(iterations);
  };
};


ArgumentList
ShuffleExplorer::opts(QList<Argument*>() 
                      << new IntegerArgument("iterations",
                                             "Iterations",
                                             "Number of monte-carlo iterations")
                      << new IntegerArgument("fit-iterations",
                                             "Fit iterations",
                                             "Maximum number of fit iterations")
                      );


ParameterSpaceExplorerFactoryItem 
shuffle("shuffle", "Shuffle",
        ArgumentList(),
        ShuffleExplorer::opts,
        [](FitWorkspace *ws) -> ParameterSpaceExplorer * {
          return new ShuffleExplorer(ws);
        });

//////////////////////////////////////////////////////////////////////

/// This explorer just linearly varies the given parameters from a
/// starting point to an ending point. 
class LinearExplorer : public ParameterSpaceExplorer {

  int iterations;

  int currentIteration;

  int fitIterations;

  /// The parameter specification
  class ParameterSpec {
  public:
    /// The target, like what is returned by
    /// FitWorkspace::parseParameterList()
    QPair<int, int> parameter;

    /// The starting point
    double begin;

    /// The ending point
    double end;

    /// Whether the range is logarithmic or not
    bool log;
  };

  QList<ParameterSpec> parameterSpecs;

public:
  static ArgumentList args;
  static ArgumentList opts;

  LinearExplorer(FitWorkspace * ws) :
    ParameterSpaceExplorer(ws), iterations(20),
    currentIteration(0), fitIterations(50) {
  };

  // virtual ArgumentList * explorerArguments() const override {
  //   return &args;
  // };

  // virtual ArgumentList * explorerOptions() const override {
  //   return &opts;
  // };

  virtual void setup(const CommandArguments & args,
                     const CommandOptions & opts) override {

    QStringList specs = args[0]->value<QStringList>();

    parameterSpecs.clear();
    QRegExp re("^\\s*(.*):([^:]*)\\.\\.([^:,]*)(,log)?\\s*$");
    for(const QString & s : specs) {
      if(re.indexIn(s) != 0)
        throw RuntimeError("Invalid parameter specification: '%1'").
          arg(s);

      

      QString pa = re.cap(1);
      
      QString l = re.cap(2);
      QString h = re.cap(3);

      bool log = ! re.cap(4).isEmpty();

      QList<QPair<int, int> > params = workSpace->parseParameterList(pa);
      for(const QPair<int, int> & p : params) {
        double val = workSpace->getValue(p.first, p.second);
        auto fn = [](const QString spec, double val) -> double {
          if(spec.isEmpty())
            return val;
          QRegExp p("^\\s*([+-]?)(.*)%\\s*$");
          if(p.indexIn(spec) >= 0) {
            // Relative
            double v = p.cap(2).toDouble()*1e-2;
            if(p.cap(1) == "+")
              v += 1;
            if(p.cap(1) == "-")
              v = 1 - v;
            return v * val;
          }
          else
            return spec.toDouble();
        };
        double b = fn(l, val);
        double e = fn(h, val);
        if(log) {
          b = ::log(b);
          e = ::log(e);
        }
          
        ParameterSpec sp = {p, b, e, log};
        parameterSpecs << sp;
      }
    }

    updateFromOptions(opts, "iterations", iterations);
    updateFromOptions(opts, "fit-iterations", fitIterations);

    Terminal::out << "Setting up linear explorator with: "
                  << iterations << " iterations and "
                  << fitIterations << " fit iterations" << endl;

    QStringList names = workSpace->parameterNames();
    for(const ParameterSpec & s : parameterSpecs)
      Terminal::out << " * " << names[s.parameter.first]
                    << "[#" << s.parameter.second << "]: from "
                    << s.begin << " to " << s.end
                    << (s.log ? " log" : " lin") << endl;
  };

  virtual bool iterate(bool justPick) override {
    QStringList names = workSpace->parameterNames();
    Terminal::out << "Setting initial parameters: " << endl;
    
    for(const ParameterSpec & s : parameterSpecs) {
      double fact = currentIteration/(iterations - 1.0);
      double v = s.begin + (s.end - s.begin) * fact;
      if(s.log)
        v = exp(v);
      workSpace->setValue(s.parameter.first, s.parameter.second, v);
      Terminal::out << " -> " << names[s.parameter.first]
                    << "[#" << s.parameter.second << "] =  " << v << endl;
    }

    if(! runHooks())
      return false;

    if(! justPick) {
      workSpace->runFit(fitIterations);
      currentIteration++;
    }
    return currentIteration < iterations;
  };

  virtual QString progressText() const override {
    return QString("%1/%2").
      arg(currentIteration+1).arg(iterations);
  };


};

ArgumentList
LinearExplorer::args(QList<Argument*>() 
                         << new SeveralStringsArgument("parameters",
                                                       "Parameters",
                                                       "Parameter specification"));

ArgumentList
LinearExplorer::opts(QList<Argument*>() 
                         << new IntegerArgument("iterations",
                                                "Iterations",
                                                "Number of monte-carlo iterations")
                         << new IntegerArgument("fit-iterations",
                                                "Fit iterations",
                                                "Maximum number of fit iterations")
                         );

ParameterSpaceExplorerFactoryItem 
linear("linear", "Linear ramp",
       LinearExplorer::args,
       LinearExplorer::opts,
       [](FitWorkspace *ws) -> ParameterSpaceExplorer * {
         return new LinearExplorer(ws);
       });

//////////////////////////////////////////////////////////////////////


/// This class represents a set of parameter variations, i.e a range
/// around parameters.
class ParameterVariations {

  class OneVariation {
  public:
    typedef enum {
      Absolute,                 // Absolute variation
      Relative,                
      Sigmas
    } Kind;

    /// What kind of variation
    Kind kind;
    
    double value;

    /// Whether or not we're using a log scale
    bool isLog;

    void parseSpec(const QString & spec) {
      QRegExp res("([^S%,]+)([S%])?(,log)?$");
      if(res.indexIn(spec) != 0) 
        throw RuntimeError("Invalid variation specification: '%1'").
          arg(spec);

      value = Utils::stringToDouble(res.cap(1));
      if(res.cap(2).isEmpty())
        kind = OneVariation::Absolute;
      else {
        if(res.cap(2) == "S") 
          kind = OneVariation::Sigmas;
        else if(res.cap(2) == "%") {
          kind = OneVariation::Relative;
          value *= 0.01;
        }
      }
      isLog = ! res.cap(3).isEmpty();
    };
      

    /// Returns true if target is within the range given by param and
    /// sigma.
    bool isWithin(double param, double sigma, double target,
                  double factor = 1) const {
      bool rv = false;
      switch(kind) {
      case Absolute:
        if(isLog) {
          param = log(param);
          target = log(target);
        }
        rv = fabs(param - target) < fabs(value*factor);
        break;
      case Relative:
        if(isLog) {               // Unsure it means something to have
                                // relative log ?
          param = log(param);
          target = log(target);
        }
        rv = fabs((param - target)/param) < fabs(value*factor);
        break;
      case Sigmas:
        if(isLog) {               // Unsure it means something to have
                                // relative log ?
          sigma = ::log1p(sigma/param);
          param = log(param);
          target = log(target);
        }
        rv = fabs(param - target) < fabs(sigma*factor);
        break;
      }
      // Works ?
      // QTextStream o(stdout);
      // o << "Comparing " << param << " (" << sigma << ") to " << target
      //   << " using " << textRepresentation()
      //   << "\t: " << rv << endl;
      
      return rv;
    };

    /// Returns a random value
    double random(double param, double sigma, double factor = 1) const {
      double range = factor;
      switch(kind) {
      case Absolute:
        range *= value;
        break;
      case Relative:
        range *= value * param;
        break;
      case Sigmas:
        range *= value * sigma;
        break;
      }
      if(kind != Absolute && isLog)
        range = ::log1p(range/param);
      double rnd = Utils::random(-range, range);
      if(isLog)
        return param * exp(rnd);
      else
        return param + rnd;
    };

    QString textRepresentation() const {
      QString rep;
      switch(kind) {
      case Absolute:
        rep = "absolute";
        break;
      case Relative:
        rep = "relative";
        break;
      case Sigmas:
        rep = "sigmas";
      };
      if(isLog)
        rep += ", log";

      return QString("%1, %2").arg(value).arg(rep);
    };
    
  };

  /// A hash index in FitWorkSpace::saveParameterValues() ->
  /// corresponding variation.
  QHash<int, OneVariation> variations;

  OneVariation defaultVariation;

  FitWorkspace * workSpace;

public:
  explicit ParameterVariations(FitWorkspace * ws) : workSpace(ws)
  {
    defaultVariation.kind = OneVariation::Relative;
    defaultVariation.value = 0.15; // 15 % variation by default
    defaultVariation.isLog = false;
  };

  /// Parses the specs in string list, and stores them in the
  /// variations hash. If @a wrongParams is provided, is filled with
  /// unknonwn parameters. If not, the function will raised exceptions.
  void parseSpecs(const QStringList & specs,
                  QStringList * wrongParams = NULL) {
    int nb_per_ds = workSpace->data()->parametersPerDataset();
    QRegExp re("^\\s*(.+):([^,:]+(,log)?)\\s*$");

    for(const QString & s : specs) {
      if(re.indexIn(s) != 0)
        throw RuntimeError("Invalid parameter specification: '%1'").
          arg(s);


      QStringList pars = Utils::nestedSplit(re.cap(1), ',', "[", "]");
      QString spec = re.cap(2);

      OneVariation var;
      var.parseSpec(spec);

      for(const QString & pa : pars) {
        if(pa == "*")           // default
          defaultVariation = var;
        else {
          QList<QPair<int, int> > params =
            workSpace->parseParameterList(pa, wrongParams);
          for(const QPair<int, int> & p : params) {
            /// @todo Should this be a FitWorkspace function ?
            int idx = p.first + (p.second < 0 ? 0 : p.second) * nb_per_ds;
            variations[idx] = var;
          }
        }
      }
    }
  };

  /// Returns a text representation of the parameters variation.
  QString textRepresentation() const {
    int nb_per_ds = workSpace->data()->parametersPerDataset();
    QString rv;
    QList<int> k = variations.keys();
    std::sort(k.begin(), k.end());
    for(int i : k) {
      int pidx = i % nb_per_ds;
      int ds = i / nb_per_ds;
      QString name = workSpace->parameterName(pidx);
      if(! workSpace->isGlobal(pidx))
        name += QString("[#%1]").arg(ds);
      rv += QString(" * %1: %2\n").arg(name).
        arg(variations[i].textRepresentation());
    }
    rv += QString(" * all others: %2\n").
        arg(defaultVariation.textRepresentation());
    return rv;
  };
                                      


  /// Returns true if within range according to the specification.
  bool parametersWithinRange(const Vector & sourceParameters,
                             const Vector & sourceSigmas,
                             const Vector & target,
                             double factor = 1) const
  {
    for(int i = 0; i < sourceParameters.size(); i++) {
      OneVariation var = variations.value(i, defaultVariation);
      if(! var.isWithin(sourceParameters[i], sourceSigmas[i],
                        target[i], factor))
        return false;
    }
    return true;
  };

  /// Returns true if within range according to the specification.
  bool parametersWithinRange(const FitTrajectory & source,
                             const FitTrajectory & target,
                             double factor = 1) const
  {
    return parametersWithinRange(source.finalParameters,
                                 source.parameterErrors,
                                 target.finalParameters, factor);
  };

  Vector randomParameters(const Vector & baseParameters,
                          const Vector & baseSigmas,
                          double factor = 1, bool ignoreFixed = false) const {
    Vector rv = baseParameters;
    int nb_per_ds = workSpace->data()->parametersPerDataset();
    for(int i = 0; i < rv.size(); i++) {
      if(! ignoreFixed && workSpace->isFixed(i % nb_per_ds, i/nb_per_ds))
        continue;

      OneVariation var = variations.value(i, defaultVariation);
      rv[i] = var.random(baseParameters[i], baseSigmas[i], factor);
    }
    return rv;
  };

  Vector randomParameters(const FitTrajectory & base,
                          double factor = 1, bool ignoreFixed = false) const {
    return randomParameters(base.finalParameters, base.parameterErrors,
                            factor, ignoreFixed);
  };

  /// Clusters the given trajectories according to the distance rules
  /// given by this object. Each cluster is a list of trajectories.
  /// The most representative element is the lowest residuals. This is
  /// also the element against which we compare all of them.

  QList<FitTrajectories> clusterTrajectories(const FitTrajectories & trajs,
                                             double factor = 1) const {
    
    QList<FitTrajectories> clusters;
    for(const FitTrajectory & t : trajs) {
      bool found = false;
      for(FitTrajectories & cl : clusters) {
        if(parametersWithinRange(cl.best(), t, factor)) {
          cl << t;
          found = true;
          break;
        }
      }
      if(! found) {
        clusters << FitTrajectories(workSpace);
        clusters.last() << t;
      }
    }
    std::sort(clusters.begin(), clusters.end(), [](const FitTrajectories & a,
                                                   const FitTrajectories & b) -> double {
                return a.best().residuals < b.best().residuals;
              });
    return clusters;
  }


  
};

/// This explorer starts from the current parameters, and "warms them
/// up" smoothly, that is draws parameters with increasing errors on
/// the values (scaled according to the covariance matrix)
class SimulatedAnnealingExplorer : public ParameterSpaceExplorer {

  int iterations;

  int currentIteration;

  double minTemperature;
  
  double maxTemperature;

  int fitIterations;

  /// The base parameters
  Vector baseParameters;

  /// The sigmas -- the base unit for variation
  Vector sigmas;


  ParameterVariations variations;

  QList<FitTrajectory> clusters;

  int currentCluster;

public:

  static ArgumentList args;
  static ArgumentList opts;

  SimulatedAnnealingExplorer(FitWorkspace * ws) :
    ParameterSpaceExplorer(ws), iterations(50),
    currentIteration(0), minTemperature(0.1),
    maxTemperature(4), fitIterations(50), variations(ws),
    currentCluster(-1) {
  };

  // virtual ArgumentList * explorerArguments() const override {
  //   return &args;
  // };

  // virtual ArgumentList * explorerOptions() const override {
  //   return &opts;
  // };

  virtual void setup(const CommandArguments & args,
                     const CommandOptions & opts) override {

    QStringList specs = args[0]->value<QStringList>();
    QStringList wrongParams;
    variations.parseSpecs(specs, &wrongParams);
    if(wrongParams.size() > 0)
      Terminal::out << "WARNING: could not understand the following parameters: "
                    << wrongParams.join(", ")
                    << endl;

    updateFromOptions(opts, "iterations", iterations);
    updateFromOptions(opts, "fit-iterations", fitIterations);

    updateFromOptions(opts, "start-temperature", minTemperature);
    updateFromOptions(opts, "end-temperature", maxTemperature);

    Terminal::out << "Simulated annealing -- warming the parameters from "
                  << minTemperature << " to "
                  << maxTemperature << " in "
                  << iterations << " steps:" << endl;
    Terminal::out << variations.textRepresentation() << endl;

    int clusters = -1;
    updateFromOptions(opts, "clusters", clusters);
    if(clusters == 0) {
      baseParameters = workSpace->saveParameterValues();
      GSLMatrix cov(baseParameters.size(), baseParameters.size());
      workSpace->data()->computeCovarianceMatrix(cov, baseParameters.data());
      sigmas = baseParameters;
      for(int i = 0; i < baseParameters.size(); i++)
        sigmas[i] = sqrt(cov.value(i,i));
      Terminal::out << "Using current parameters" << endl;
    }
    else {
      QList<FitTrajectories> cls = variations.
        clusterTrajectories(workSpace->trajectories);
      for(int i = 0; i < cls.size(); i++) {
        if(clusters > 0 && i >= clusters)
          break;
        // Stupid name :-(...
        this->clusters << cls[i].best();
      }
      QStringList cl;
      for(const FitTrajectory & t : this->clusters)
        cl << QString::number(t.residuals);
      Terminal::out << "Using " << cl.size()
                    << " clusters with best residuals " << cl.join(", ")
                    << endl;
    }
  };

  virtual bool iterate(bool justPick) override {
    double curTemperature = minTemperature + (maxTemperature - minTemperature)/(iterations - 1) * currentIteration;

    Terminal::out << "Choosing at temperature: "
                  << curTemperature << endl;

    Vector choice;
      
    if(clusters.size() > 0 && (currentCluster < 0 || currentIteration >= iterations)) {
      ++currentCluster;
      if(clusters.size() <= currentCluster)
        return false;
      Terminal::out << "Starting to work on cluster "
                    << currentCluster << endl;
      currentIteration = 0;
    }

    if(clusters.size() == 0)
      choice = variations.randomParameters(baseParameters, sigmas,
                                           curTemperature);
    else
      choice = variations.randomParameters(clusters[currentCluster],
                                           curTemperature);

    Vector base = clusters.size() == 0 ? baseParameters :
      clusters[currentCluster].finalParameters;
    

    
    // Write out the parameters:
    Terminal::out << "Picking out the following parameters:" << endl;
    int nb_per_ds = workSpace->data()->parametersPerDataset();
    for(int i = 0; i < nb_per_ds; i++) {
      if(workSpace->isGlobal(i))
        Terminal::out << " * " << workSpace->fullParameterName(i)
                      << ":\t" << base[i] << "\t-> "
                      << choice[i] << endl;
    }

    for(int i = 0; i < choice.size(); i++) {
      int idx = i % nb_per_ds;
      if(! workSpace->isGlobal(idx))
        Terminal::out << " * " << workSpace->fullParameterName(i)
                      << ":\t" << base[i] << "\t-> "
                      << choice[i] << endl;
    }


    workSpace->restoreParameterValues(choice);

    if(! runHooks())
      return false;

    if(! justPick) {
      workSpace->runFit(fitIterations);
      currentIteration++;
    }
    return (clusters.size() > 0 && currentCluster+1 < clusters.size()) ||
      currentIteration < iterations;
  };

  virtual QString progressText() const override {
    if(clusters.size() > 0)
      return QString("%1/%2, cluster %3/%4").
        arg(currentIteration+1).arg(iterations).
        arg(currentCluster+1).arg(clusters.size());
    else
      return QString("%1/%2").
        arg(currentIteration+1).arg(iterations);
  };


};

ArgumentList
SimulatedAnnealingExplorer::args(QList<Argument*>() 
                                 << new SeveralStringsArgument("parameters",
                                                               "Parameters",
                                                               "Parameter specification", true)
                                 );
 
ArgumentList
SimulatedAnnealingExplorer::opts(QList<Argument*>() 
                                 << new IntegerArgument("clusters",
                                                        "Clusters",
                                                        "Number of parameter clusters to anneal, -1 for all clusters, 0 to use current parameters")
                                 << new IntegerArgument("iterations",
                                                        "Iterations",
                                                        "Number of monte-carlo iterations")
                                 << new IntegerArgument("fit-iterations",
                                                        "Fit iterations",
                                                        "Maximum number of fit iterations")
                                 << new NumberArgument("start-temperature",
                                                       "Starting temperature",
                                                       "The starting 'temperature' for the random choices")
                                 << new NumberArgument("end-temperature",
                                                       "Ending temperature",
                                                       "The ending 'temperature' for the random choices")
                                 );

ParameterSpaceExplorerFactoryItem 
sa("simulated-annealing", "Simulated annealing",
   SimulatedAnnealingExplorer::args,
   SimulatedAnnealingExplorer::opts,
   [](FitWorkspace *ws) -> ParameterSpaceExplorer * {
     return new SimulatedAnnealingExplorer(ws);
   });

//////////////////////////////////////////////////////////////////////


class OrderClassifyExplorer : public ParameterSpaceExplorer {
  int fitIterations;

  class ParameterValues {
  public:

    /// the parameter index
    int paramIndex;

    /// the current index in the parameter values
    int cur;

    /// The various values of the given parameter.
    QList<Vector> values;
  };

  QList<ParameterValues> parameters;

  int total;
  int current;

  /// Saved initial parameters
  Vector initialParameters;

public:

  static ArgumentList args;
  static ArgumentList opts;

  OrderClassifyExplorer(FitWorkspace * ws) :
    ParameterSpaceExplorer(ws)
  {
  };

  virtual void setup(const CommandArguments & args,
                     const CommandOptions & opts) override {
    QStringList specs = args[0]->value<QStringList>();
    updateFromOptions(opts, "fit-iterations", fitIterations);

    int trends = 3;
    updateFromOptions(opts, "trends", trends);

    double magnitude = 3;
    updateFromOptions(opts, "magnitude", magnitude);

    int nb_per_ds = workSpace->data()->parametersPerDataset();
    int nbds = workSpace->data()->datasets.size();

    initialParameters = workSpace->saveParameterValues();

    total = 1;
    current = 0;

    for(const QString & s : specs) {
      int idx = workSpace->parameterIndex(s);
      if(idx < 0) {
        Terminal::out << "Unkown parameter: '" << s
                      << "', ignoring" << endl;
        continue;
      }
      Vector yv;
      Vector xv = workSpace->perpendicularCoordinates;
      for(int i = 0; i < nbds; i++) {
        if(xv.size() == i)
          xv << i;
        yv << initialParameters[i*nb_per_ds + idx];
      }

      QList< QList<Vector> > trds =
        Vector::orderOfMagnitudeClassify(xv, yv, magnitude/2);

      // Classifies the returns according to size
      auto fn =  [](const QList<Vector> & a, const QList<Vector> & b) -> bool {
        if(a.first().size() > b.first().size())
          return true;
        if(a.first().size() == b.first().size()) {
          double da = a.first().max() - a.first().min();
          double db = b.first().max() - b.first().min();
          return da > db;
        }
        return false;
      };

      std::sort(trds.begin(), trds.end(), fn);

      QStringList t;
      for(const QList<Vector> & v : trds)
        t << QString("[%1,%2] (%3 points)").
          arg(v[1].min()).arg(v[1].max()).arg(v[1].size());

      Terminal::out << "Found " << trds.size() << " trends for parameter '"
                    << s << "': " << t.join(", ") << endl;
      if(trds.size() == 1)
        continue;               // No need to bother

      parameters << ParameterValues();
      ParameterValues & v = parameters.last();
      v.paramIndex = idx;
      v.cur = 0;
      
      for(int i = 0; i < trds.size() && i < trends; i++) {
        DataSet ds(trds[i]);
        Vector nv = xv;
        for(int j = 0; j < nv.size(); j++)
          nv[j] = ds.yValueAt(xv[j], true);
        v.values << nv;
      }
      total *= v.values.size();
    }
    // Ok now
    Terminal::out << " -> " << total << " total iterations" << endl;

  };

  virtual bool iterate(bool justPick) override {
    Vector nv = initialParameters;

    int nb_per_ds = workSpace->data()->parametersPerDataset();
    int nbds = workSpace->data()->datasets.size();

    for(const ParameterValues & v : parameters) {
      for(int i = 0; i < nbds; i++) {
        double val = v.values[v.cur][i];
        nv[i * nb_per_ds + v.paramIndex] = val;
        Terminal::out << " * " << workSpace->parameterName(v.paramIndex)
                      << "[#" << i << "] = " << val << endl;
      }
    }

    workSpace->restoreParameterValues(nv);

    /// @todo Shouldn't that be a common function ?
    if(! runHooks())
      return false;
    if(! justPick) {
      workSpace->runFit(fitIterations);
    }
    
    // Now iterate
    bool found = false;
    for(int i = 0; i < parameters.size(); i++) {
      parameters[i].cur += 1;
      if(parameters[i].cur >= parameters[i].values.size()) {
        parameters[i].cur = 0;
      }
      else {
        found = true;
        break;
      }
    }
    current += 1;
    
    return found;
  };

  virtual QString progressText() const override {
    return QString("%1/%2").arg(current).arg(total);
  };


};

ArgumentList
OrderClassifyExplorer::args(QList<Argument*>() 
                                 << new SeveralStringsArgument("parameters",
                                                               "Parameters",
                                                               "Parameter specification", true)
                                 );
 
ArgumentList
OrderClassifyExplorer::opts(QList<Argument*>() 
                            << new IntegerArgument("fit-iterations",
                                                   "Fit iterations",
                                                   "Maximum number of fit iterations")
                            << new IntegerArgument("trends",
                                                   "Number of trends",
                                                   "Number of most significant trends to keep")
                            << new NumberArgument("magnitude",
                                                  "Magnitude window",
                                                  "Size of the acceptable difference in magnitude")
                            );

ParameterSpaceExplorerFactoryItem 
om("order-of-magnitude", "Order of magnitude",
   OrderClassifyExplorer::args,
   OrderClassifyExplorer::opts,
   [](FitWorkspace *ws) -> ParameterSpaceExplorer * {
     return new OrderClassifyExplorer(ws);
   });

//////////////////////////////////////////////////////////////////////

#include <gsl/gsl_permute.h>

class PermutationExplorer : public ParameterSpaceExplorer {
  int fitIterations;

  /// List of the parameters that get permuted.w
  ///
  /// Only the parameter names in the end
  QList<QList<int> > parameterSets;

  QList<gsl_permutation*> permutations;

  /// Saved initial parameters
  Vector initialParameters;

  int current;

  int total;

public:

  static ArgumentList args;
  static ArgumentList opts;

  PermutationExplorer(FitWorkspace * ws) :
    ParameterSpaceExplorer(ws),
    fitIterations(50)
  {
  };

  virtual void setup(const CommandArguments & args,
                     const CommandOptions & opts) override {

    QStringList specs = args[0]->value<QStringList>();
    updateFromOptions(opts, "fit-iterations", fitIterations);


    initialParameters = workSpace->saveParameterValues();

    total = 1;
    current = 1;

    for(const QString & s : specs) {
      QStringList param = s.split(",");
      if(param.size() == 1)
        throw RuntimeError("Only one parameter in '%1'").arg(s);
      QList<int> indx;
      for(const QString & p: param) {
        int idx = workSpace->parameterIndex(p);
        if(idx < 0)
          throw RuntimeError("Unkown parameter: '%1'").arg(p);
        indx << idx;
      }
      parameterSets << indx;

      gsl_permutation * p = gsl_permutation_alloc(indx.size());
      gsl_permutation_init(p);

      permutations << p;

      int nb = indx.size();
      while(nb > 1) {
        total *= nb;
        --nb;
      }
    }
    // Ok now
    Terminal::out << " -> " << total << " total iterations" << endl;

  };

  virtual bool iterate(bool justPick) override {
    Vector nv = initialParameters;

    int nb_per_ds = workSpace->data()->parametersPerDataset();
    int nbds = workSpace->data()->datasets.size();

    // First apply the permutations, then step them

    for(int i = 0; i < parameterSets.size(); i++) {
      for(int j = 0; j < nbds; j++) {
        double prms[parameterSets[i].size()];
        for(int k = 0; k < parameterSets[i].size(); k++) {
          prms[k] = nv[j*nb_per_ds + parameterSets[i][k]];
        }
        // Hmmm. This isn't so nice in fact, that the API forces one
        // to look into the details of the permutation.
        // Or I should make a vetor view ?
        gsl_permute(permutations[i]->data, prms, 1, parameterSets[i].size());
        for(int k = 0; k < parameterSets[i].size(); k++) {
          nv[j*nb_per_ds + parameterSets[i][k]] = prms[k];
        }
      }
    }
    workSpace->restoreParameterValues(nv);

    /// @todo Shouldn't that be a common function ?
    if(! runHooks())
      return false;
    if(! justPick) {
      workSpace->runFit(fitIterations);
    }

    // bool found = false;
    int k = parameterSets.size()-1;
    current += 1;
    while(k >= 0) {
      int r = gsl_permutation_next(permutations[k]);
      if(r == GSL_SUCCESS)
        return true;
      else {
        gsl_permutation_init(permutations[k]);
        --k;
      }
    }


    
    return false;
  };

  virtual QString progressText() const override {
    return QString("%1/%2").arg(current).arg(total);
  };

  ~PermutationExplorer() {
    for(gsl_permutation * p : permutations)
      gsl_permutation_free(p);
  };


};

ArgumentList
PermutationExplorer::args(QList<Argument*>() 
                          << (new SeveralStringsArgument("parameters",
                                                        "Parameters",
                                                         "Parameter specification", true))->describe("A set of parameters to permute, separated by ','; use several times to specify several sets", "parameter-names")
                                 );
 
ArgumentList
PermutationExplorer::opts(QList<Argument*>() 
                          << new IntegerArgument("fit-iterations",
                                                 "Fit iterations",
                                                 "Maximum number of fit iterations")
                            );

ParameterSpaceExplorerFactoryItem 
perm("permutation", "Permutation",
     PermutationExplorer::args,
     PermutationExplorer::opts,
     [](FitWorkspace *ws) -> ParameterSpaceExplorer * {
       return new PermutationExplorer(ws);
     });




//////////////////////////////////////////////////////////////////////


#include <command.hh>
#include <commandcontext.hh>
#include <commandeffector-templates.hh>
#include <file-arguments.hh>


/// @todo This command has nothing to do here.

static void clusterTrajectoriesCommand(const QString & /*name*/,
                                       QStringList specs,
                                       const CommandOptions & opts)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();

  ParameterVariations vars(ws);
  QStringList missing;
  vars.parseSpecs(specs, &missing);
  if(missing.size() > 0)
    Terminal::out << "WARNING: could not understand the following parameters: "
                  << missing.join(", ")
                  << endl;


  
  double factor = 1;
  updateFromOptions(opts, "factor", factor);

  Terminal::out << "Clustering trajectories with the following parameters:\n"
                << vars.textRepresentation() << endl;

  QList<FitTrajectories> clusters = vars.clusterTrajectories(ws->trajectories);
  Terminal::out << " -> found " << clusters.size() << " clusters" << endl;
  QString expt;
  updateFromOptions(opts, "export", expt);
  int exportOnly = -1;
  updateFromOptions(opts, "export-only", exportOnly);
  int idx = 0;
  for(FitTrajectories & c : clusters) {
    Terminal:: out << "Cluster with " << c.size() << " elements, best: "
                   << c.best().residuals << endl;
    
    if(! expt.isEmpty()) {
      if(exportOnly < 0 || idx < exportOnly) {
        QString suff = QString::asprintf("-%03d.dat", idx);
        QString fn = expt + suff;
        Terminal::out << "-> writing cluster to " << fn << endl;

        File f(fn, File::TextWrite, opts);
        QTextStream o(f);
        c.exportToFile(o);
      }
    }
    ++idx;
  }
}

ArgumentList ctArgs(QList<Argument*>()
                   << new SeveralStringsArgument("parameters",
                                                 "Parameters",
                                                 "Parameter specification", true, true)
                   );

ArgumentList ctOpts(QList<Argument*>()
                    << new NumberArgument("factor",
                                          "Scaling factor",
                                          "Scaling factor for the clustering")
                    << new FileArgument("export",
                                        "Export clusters",
                                        "prefix to export the clusters as trajectory files")
                    << new IntegerArgument("export-only",
                                           "Number of trajectories",
                                           "only export that many of the best trajectories")
                    << File::fileOptions(File::OverwriteOption)
                    );

static Command 
ct("cluster-trajectories", // command name
    effector(clusterTrajectoriesCommand), // action
    "fits",  // group name
    &ctArgs, // arguments
    &ctOpts, // options
    "Cluster trajectories",
    "Cluster the trajectories according to the specifications",
    "", CommandContext::fitContext());

 
