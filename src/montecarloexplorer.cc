/*
  montecarloexplorer.cc: a basic monte-carlo parameter space explorer
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

class MonteCarloExplorer : public ParameterSpaceExplorer {

  int iterations;

  int currentIteration;

  int fitIterations;

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
public:

  MonteCarloExplorer(FitWorkspace * ws) :
    ParameterSpaceExplorer(ws), iterations(20),
    currentIteration(0), fitIterations(50) {
  };

  virtual ArgumentList * explorerArguments() const override {
    return new ArgumentList(QList<Argument*>() 
                            << new SeveralStringsArgument("parameters",
                                                          "Parameters",
                                                          "Parameter specification"));
  };

  virtual ArgumentList * explorerOptions() const override {
    return new ArgumentList(QList<Argument*>() 
                            << new IntegerArgument("iterations",
                                                   "Iterations",
                                                   "Number of monte-carlo iterations")
                            << new IntegerArgument("fit-iterations",
                                                   "Fit iterations",
                                                   "Maximum number of fit iterations")
                            );
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

    Terminal::out << "Setting up monte-carlo explorator with: "
                  << iterations << " iterations and"
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
    return QString("Iteration %1/%2").
      arg(currentIteration).arg(iterations);
  };


};

ParameterSpaceExplorerFactoryItem 
montecarlo("monte-carlo", "Monte Carlo", 
           [](FitWorkspace *ws) -> ParameterSpaceExplorer * {
             return new MonteCarloExplorer(ws);
           });
