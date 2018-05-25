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

#include <terminal.hh>
#include <utils.hh>

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
    for(const QString & s : specs) {
      if(re.indexIn(s) != 0)
        throw RuntimeError("Invalid parameter specification: '%1'").
          arg(s);

      QString pa = re.cap(1);
      double l = re.cap(2).toDouble();
      double h = re.cap(3).toDouble();
      bool log = ! re.cap(4).isEmpty();

      QList<QPair<int, int> > params = workSpace->parseParameterList(pa);
      for(const QPair<int, int> & p : params) {
        ParameterSpec sp = {p, l, h, log};
        parameterSpecs << sp;
      }
    }

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

  virtual bool iterate() override {
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

    workSpace->runFit(fitIterations);
    currentIteration++;
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


/// This explorer just randomly shuffles the parameters among the fit
/// trajectories:
/// * random selection of the trajectory
/// * 50 % chance of selecting either the starting of the final values.
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

  virtual bool iterate() override {
    Vector p;
    int sz = workSpace->trajectories.size();
    if(sz < 1)
      throw RuntimeError("Cannot iterate the shuffle explorer with no trajectories");
    p = workSpace->trajectories[0].initialParameters;
    for(int i = 0; i < p.size(); i++) {
      int idx = ::rand() % sz;
      int w = ::rand() % 2;
      QString which;
      if(w) 
        p[i] = workSpace->trajectories[idx].initialParameters[i];
      else
        p[i] = workSpace->trajectories[idx].finalParameters[i];
    }
    workSpace->restoreParameterValues(p);
    workSpace->runFit(fitIterations);
    currentIteration++;
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

  virtual bool iterate() override {
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

    workSpace->runFit(fitIterations);
    currentIteration++;
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
