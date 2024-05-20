/*
  parametervariations.cc: a class for grouping parameters and its uses
  Copyright 2013, 2018, 2019, 2020, 2024 by Vincent Fourmond

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

#include <fit-arguments.hh>

#include <file.hh>

// random generators
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>


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

//////////////////////////////////////////////////////////////////////

/// This explorer starts from the current parameters, and "warms them
/// up" smoothly, that is draws parameters with increasing errors on
/// the values (scaled according to the covariance matrix)
class OldSimulatedAnnealingExplorer : public ParameterSpaceExplorer {

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

  OldSimulatedAnnealingExplorer(FitWorkspace * ws) :
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
OldSimulatedAnnealingExplorer::args(QList<Argument*>()
                                    << new SeveralStringsArgument("parameters",
                                                                  "Parameters",
                                                                  "Parameter specification", true)
                                    );
 
ArgumentList
OldSimulatedAnnealingExplorer::opts(QList<Argument*>()
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
sa("old-simulated-annealing", "Simulated annealing",
   OldSimulatedAnnealingExplorer::args,
   OldSimulatedAnnealingExplorer::opts,
   [](FitWorkspace *ws) -> ParameterSpaceExplorer * {
     return new OldSimulatedAnnealingExplorer(ws);
   });




//////////////////////////////////////////////////////////////////////


#include <command.hh>
#include <commandcontext.hh>
#include <commandeffector-templates.hh>
#include <file-arguments.hh>


static void varClusterTrajectoriesCommand(const QString & /*name*/,
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
ct("var-cluster-trajectories", // command name
    effector(varClusterTrajectoriesCommand), // action
    "fits",  // group name
    &ctArgs, // arguments
    &ctOpts, // options
    "Cluster trajectories (old)",
    "Cluster the trajectories according to the specifications",
    "", CommandContext::fitContext());


