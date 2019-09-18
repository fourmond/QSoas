/*
  parameterspaceexplorers.cc: several simple parameter space explorers
  Copyright 2013, 2018 by Vincent Fourmond

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

#include <fitworkspace.hh>

#include <argumentlist.hh>
#include <general-arguments.hh>

#include <gsl-types.hh>

#include <terminal.hh>
#include <utils.hh>

#include <fitdata.hh>
#include <fittrajectory.hh>

class MonteCarloExplorer : public ParameterSpaceExplorer {

  int iterations;

  int currentIteration;

  int fitIterations;

  int resetFrequency;

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
  };

  QList<ParameterSpec> parameterSpecs;

  static ArgumentList args;
  static ArgumentList opts;
public:

  MonteCarloExplorer(FitWorkspace * ws) :
    ParameterSpaceExplorer(ws), iterations(20),
    currentIteration(0), fitIterations(50), resetFrequency(0) {
  };

  virtual ArgumentList * explorerArguments() const override {
    return &args;
  };

  virtual ArgumentList * explorerOptions() const override {
    return &opts;
  };

  virtual void setup(const CommandArguments & args,
                     const CommandOptions & opts) override {

    QStringList specs = args[0]->value<QStringList>();

    parameterSpecs.clear();
    QRegExp re("^\\s*(.*):([^:]+)\\.\\.([^:,]+)(,log)?\\s*$");
    QStringList unknowns;
    for(const QString & s : specs) {
      if(re.indexIn(s) != 0)
        throw RuntimeError("Invalid parameter specification: '%1'").
          arg(s);


      // Now handling: 
      // monte-carlo-explorer tau_1[#0,#1],tau_2[#1]:1e-2..1e2,log
      
      QStringList pars = Utils::nestedSplit(re.cap(1), ',', "[", "]");
      double l = re.cap(2).toDouble();
      double h = re.cap(3).toDouble();
      bool log = ! re.cap(4).isEmpty();
      QList<QPair<int, int> > params;
      for(const QString & pa : pars)
        params << workSpace->parseParameterList(pa, &unknowns);
      for(const QPair<int, int> & p : params) {
        ParameterSpec sp = {p, l, h, log};
        parameterSpecs << sp;
      }
    }

    if(unknowns.size() > 0)
      Terminal::out << "WARNING: could not understand the following parameters: "
                    << unknowns.join(", ")
                    << endl;
    if(parameterSpecs.size() == 0)
      throw RuntimeError("Could understand no parameters at all");
       

    updateFromOptions(opts, "iterations", iterations);
    updateFromOptions(opts, "fit-iterations", fitIterations);
    updateFromOptions(opts, "reset-frequency", resetFrequency);

    Terminal::out << "Setting up monte-carlo explorator with: "
                  << iterations << " iterations and "
                  << fitIterations << " fit iterations" << endl;

    QStringList names = workSpace->parameterNames();
    for(const ParameterSpec & s : parameterSpecs)
      Terminal::out << " * " << names[s.parameter.first]
                    << "[#" << s.parameter.second << "]: "
                    << s.low << " to " << s.high
                    << (s.log ? " log" : " lin") << endl;
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
    
    for(const ParameterSpec & s : parameterSpecs) {
      double v = Utils::random(s.low, s.high, s.log);
      workSpace->setValue(s.parameter.first, s.parameter.second, v);
      Terminal::out << " -> " << names[s.parameter.first]
                    << "[#" << s.parameter.second << "] =  " << v << endl;
    }
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
                         );

ParameterSpaceExplorerFactoryItem 
montecarlo("monte-carlo", "Monte Carlo", 
           [](FitWorkspace *ws) -> ParameterSpaceExplorer * {
             return new MonteCarloExplorer(ws);
           });


//////////////////////////////////////////////////////////////////////


/// This explorer takes two random trajectories from the trajectory
/// list and mixes them randomly, using either the starting parameters
/// or the final parameters (one after the other).
class ShuffleExplorer : public ParameterSpaceExplorer {

  int iterations;

  int currentIteration;

  int fitIterations;

  static ArgumentList opts;
public:

  ShuffleExplorer(FitWorkspace * ws) :
    ParameterSpaceExplorer(ws), iterations(20),
    currentIteration(0), fitIterations(50) {
  };

  virtual ArgumentList * explorerArguments() const override {
    return NULL;
  };

  virtual ArgumentList * explorerOptions() const override {
    return &opts;
  };

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
    if(! justPick) {
      ++currentIteration;
      workSpace->runFit(fitIterations);
      double final = workSpace->trajectories.last().residuals;
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

  static ArgumentList args;
  static ArgumentList opts;
public:

  LinearExplorer(FitWorkspace * ws) :
    ParameterSpaceExplorer(ws), iterations(20),
    currentIteration(0), fitIterations(50) {
  };

  virtual ArgumentList * explorerArguments() const override {
    return &args;
  };

  virtual ArgumentList * explorerOptions() const override {
    return &opts;
  };

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
           [](FitWorkspace *ws) -> ParameterSpaceExplorer * {
             return new LinearExplorer(ws);
           });

//////////////////////////////////////////////////////////////////////

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

  /// Whether or not the things are log
  QList<bool> paramIsLog;

  static ArgumentList opts;

public:

  SimulatedAnnealingExplorer(FitWorkspace * ws) :
    ParameterSpaceExplorer(ws), iterations(50),
    currentIteration(0), minTemperature(0.3),
    maxTemperature(4), fitIterations(50) {
  };

  virtual ArgumentList * explorerArguments() const override {
    return NULL;
  };

  virtual ArgumentList * explorerOptions() const override {
    return &opts;
  };

  virtual void setup(const CommandArguments & /*args*/,
                     const CommandOptions & opts) override {

    baseParameters = workSpace->saveParameterValues();
    GSLMatrix cov(baseParameters.size(), baseParameters.size());
    workSpace->data()->computeCovarianceMatrix(cov, baseParameters.data());
    sigmas = baseParameters;
    paramIsLog.clear();

    int nb_per_ds = workSpace->data()->parametersPerDataset();

    for(int i = 0; i < baseParameters.size(); i++) {
      sigmas[i] = sqrt(cov.value(i,i));
      paramIsLog << false;
      if(workSpace->isFixed(i % nb_per_ds, i/nb_per_ds))
        sigmas[i] = 0;
      
    }

    QStringList specs;
    updateFromOptions(opts, "parameters", specs);
    QStringList wrongParams;

    QRegExp re("^\\s*(.*):([^,:]+)(,log)?\\s*$");
    for(const QString & s : specs) {
      if(re.indexIn(s) != 0)
        throw RuntimeError("Invalid parameter specification: '%1'").
          arg(s);


      // Now handling: 
      // monte-carlo-explorer tau_1[#0,#1],tau_2[#1]:1e-2..1e2,log
      
      QStringList pars = Utils::nestedSplit(re.cap(1), ',', "[", "]");
      QString spec = re.cap(2);
      bool log = ! re.cap(3).isEmpty();

      QList<QPair<int, int> > params;
      for(const QString & pa : pars)
        params << workSpace->parseParameterList(pa, &wrongParams);

      for(const QPair<int, int> & p : params) {
        /// @todo Should this be a FitWorkspace function ?
        int idx = p.first + (p.second < 0 ? 0 : p.second) * nb_per_ds;
        if(workSpace->isFixed(p.first, p.second)) {
          sigmas[idx] = 0;
          continue;
        }
        
        
        paramIsLog[idx] = log;

        QRegExp res("([^S%]+)([S%])?$");
        if(res.indexIn(spec) != 0)
          throw RuntimeError("Invalid sigma specification: '%1'").
            arg(spec);

        bool ok = true;
        double v = Utils::stringToDouble(res.cap(1));
        if(res.cap(2).isEmpty())
          sigmas[idx] = v;
        else {
          if(res.cap(2) == "S") {
            sigmas[idx] *= v;
          }
          else if(res.cap(2) == "%") {
            sigmas[idx] = baseParameters[idx]*v*0.01;
          }
          if(log) {
            sigmas[idx] = ::log(1 + sigmas[idx]/baseParameters[idx]);
          }
        }
      }
    }
    if(wrongParams.size() > 0)
      Terminal::out << "WARNING: could not understand the following parameters: "
                    << wrongParams.join(", ")
                    << endl;

    
    Terminal::out << "Simulated annealing -- warming the parameters from "
                  << minTemperature << " to "
                  << maxTemperature << " in "
                  << iterations << " steps:" << endl;
    for(int i = 0; i < baseParameters.size(); i++) {
      if(sigmas[i] == 0)        // fixed
        continue;
      int idp = i % nb_per_ds;
      int ds = i / nb_per_ds;
      QString pn = workSpace->parameterName(idp);
      if(workSpace->isGlobal(idp)) {
        if(ds > 0)
          continue;
      }
      else
        pn = pn + QString("[#%1]").arg(ds);
      QString lg;
      if(paramIsLog[i])
        lg = " (log)";
      Terminal::out << " * " << pn << " = " << baseParameters[i]
                    << " +- " << sigmas[i] << lg << endl;
    }

    updateFromOptions(opts, "iterations", iterations);
    updateFromOptions(opts, "fit-iterations", fitIterations);

    updateFromOptions(opts, "start-temperature", minTemperature);
    updateFromOptions(opts, "end-temperature", maxTemperature);

      
  };

  virtual bool iterate(bool justPick) override {
    double curTemperature = minTemperature + (maxTemperature - minTemperature)/(iterations - 1) * currentIteration;

    Vector choice = baseParameters;

    Terminal::out << "Choosing at temperature: "
                  << curTemperature << endl;

    
    for(int i = 0; i < choice.size(); i++) {
      double rng = sigmas[i] * curTemperature;
      double rnd = Utils::random(-rng, rng);
      if(paramIsLog[i])
        choice[i] *= exp(rnd);
      else
        choice[i] += rnd;
    }

    workSpace->restoreParameterValues(choice);

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
SimulatedAnnealingExplorer::opts(QList<Argument*>() 
                                 << new SeveralStringsArgument("parameters",
                                                               "Parameters",
                                                               "Parameter specification", true, true)
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
   [](FitWorkspace *ws) -> ParameterSpaceExplorer * {
     return new SimulatedAnnealingExplorer(ws);
   });
