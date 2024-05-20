/*
  parameterrangespec.cc: implementation of ParameterRangeSpec (and associated)
  Copyright 2024 by Vincent Fourmond

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
#include <parameterrangespec.hh>
#include <vector.hh>

#include <exceptions.hh>

#include <fitworkspace.hh>
#include <fittrajectory.hh>

#include <utils.hh>

#include <terminal.hh>



double ParameterRangeSpec::center() const
{
  if(log) {
    return sqrt(low * high);
  }
  else
    return 0.5*(low + high);
}

double ParameterRangeSpec::sigma() const
{
  if(log)
    return 0.5 * (log10(high) - log10(low));
  else
    return 0.5 * (high - low);
}

double ParameterRangeSpec::trim(double val) const
{
  if(std::isnan(val))
    return center();          // Safety catch
  return std::max(std::min(val, high), low);
}


double ParameterRangeSpec::distance(double x1, double x2) const
{
  if(log)
    return fabs(log10(x2/x1)/log10(high/low));
  return fabs((x1 - x2)/(high - low));
}




/// Parses the parameter list
QList<ParameterRangeSpec> ParameterRangeSpec::parseSpecs(const QStringList & specs,
                                               FitWorkspace * workSpace,
                                               QStringList * unknowns) {
  QList<ParameterRangeSpec> parameterSpecs;
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
      ParameterRangeSpec sp = {p, l, h, log, uniform, 0};
      parameterSpecs << sp;
    }
  }
  return parameterSpecs;
};


double ParameterRangeSpec::trajectoryDistance(const QList<ParameterRangeSpec> & specs,
                                              const FitTrajectory & a,
                                              const FitTrajectory & b,
                                              const FitWorkspace * workspace,
                                              bool useFinal)
{
  const Vector & ap = useFinal ? a.finalParameters : a.initialParameters;
  const Vector & bp = useFinal ? b.finalParameters : b.initialParameters;
  return trajectoryDistance(specs, ap, bp, workspace);
}

double ParameterRangeSpec::trajectoryDistance(const QList<ParameterRangeSpec> & specs,
                                              const Vector & ap,
                                              const Vector & bp,
                                              const FitWorkspace * workspace)
{
  double rv = 0;
  int nbParams = workspace->parametersPerDataset();
  int nbDatasets = workspace->datasetNumber();

  for(const ParameterRangeSpec & spec : specs) {
    if(spec.parameter.second == -1) {
      for(int i = 0; i < nbDatasets; i++)
        rv += pow(spec.distance(ap[i*nbParams + spec.parameter.first],
                                bp[i*nbParams + spec.parameter.first]), 2);
    }
    else
      rv += pow(spec.distance(ap[spec.parameter.second*nbParams +
                                 spec.parameter.first],
                              bp[spec.parameter.second*nbParams +
                                 spec.parameter.first]), 2);
  }
  return pow(rv, 0.5);
}

Vector ParameterRangeSpec::averageParameters(const QList<ParameterRangeSpec> & specs,
                                             const QList<Vector> & parameters,
                                             const FitWorkspace * workspace)
{
  if(parameters.size() == 0)
    throw InternalError("Should have at least one element");

  Vector rv(parameters.first().size(), 0);
  int nbParams = workspace->parametersPerDataset();
  int nbDatasets = workspace->datasetNumber();
  int nb = parameters.size();

  for(const ParameterRangeSpec & spec : specs) {
    if(spec.parameter.second == -1) {
      for(int i = 0; i < nbDatasets; i++) {
        int idx = i*nbParams + spec.parameter.first;
        for(const Vector & s: parameters) {
          if(spec.log)
            rv[idx] += log10(s[idx]);
          else
            rv[idx] += s[idx];
        }
        rv[idx] /= nb;
        if(spec.log)
          rv[idx] = pow(10.0, rv[idx]);
      }
    }
    else {
      int idx = spec.parameter.second*nbParams + spec.parameter.first;
      for(const Vector & s: parameters) {
        if(spec.log)
          rv[idx] += log10(s[idx]);
        else
          rv[idx] += s[idx];
      }
      rv[idx] /= nb;
      if(spec.log)
        rv[idx] = pow(10.0, rv[idx]);
    }
  }
  return rv;
}


//////////////////////////////////////////////////////////////////////

ParameterRangeSpecs::ParameterRangeSpecs(const QList<ParameterRangeSpec> & lst,
                                         FitWorkspace * ws) :
  QList<ParameterRangeSpec>(lst), workspace(ws)
{
}

ParameterRangeSpecs ParameterRangeSpecs::parseSpecs(const QStringList & specs,
                                                    FitWorkspace * workSpace,
                                                    QStringList * unknowns)
{
  return ParameterRangeSpecs(ParameterRangeSpec::parseSpecs(specs, workSpace,
                                                            unknowns),
                             workSpace);
}

Vector ParameterRangeSpecs::averageParameters(const QList<Vector> & parameters) const
{
  return ParameterRangeSpec::averageParameters(*this, parameters, workspace);
}

double ParameterRangeSpecs::trajectoryDistance(const Vector & a,
                                               const Vector & b) const
{
  return ParameterRangeSpec::trajectoryDistance(*this, a, b, workspace);
}


//////////////////////////////////////////////////////////////////////

#include <command.hh>
#include <commandcontext.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>


// This command attemps to do K-means clustering.
// Just saves the results as tags


static void clusterTrajectoriesCommand(const QString & /*name*/,
                                       int clusters,
                                       QStringList specs,
                                       const CommandOptions & opts)
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  QStringList unknowns;
  QList<ParameterRangeSpec> parameterSpecs =
    ParameterRangeSpec::parseSpecs(specs, ws, &unknowns);

  // max number of iterations
  int iterations = 10;

  QString flag = "k-means";
  updateFromOptions(opts, "flag", flag);

  if(unknowns.size() > 0)
    Terminal::out << "Did not find the following parameters: "
                  << unknowns.join(", ") << ", ignored them" << endl;

  if(clusters <= 2)
    throw RuntimeError("Needs at least two clusters (not %1)").
      arg(clusters);

  if(ws->trajectories.size() <= clusters)
    throw RuntimeError("Needs more trajectories than clusters !");

  class Cluster {
  public:
    Vector centroid;

    Vector oldCentroid;

    /// The index of the trajectories in the workspace
    QSet<int> currentTrajectories;

    /// The trajectories of the previous iteration
    QSet<int> previousTrajectories;

    /// Distance of the given trajectory to the previous centroid
    QHash<int, double> distances;
  };

  // First, bootstrap:
  QSet<int> starting;
  while(starting.size() < clusters)
    starting.insert(rand() % ws->trajectories.size());

  QList<Cluster> clusterList;
  for(int idx : starting) {
    Cluster c;
    c.centroid = ws->trajectories[idx].finalParameters;
    c.currentTrajectories.insert(idx);
    clusterList << c;
  }

  int it = 0;
  bool over = false;

  while(it < iterations && (! over)) {
    // First, dumping the states
    Terminal::out << "Clustering iteration " << it << endl;
    for(int i = 0; i < clusters; i++) {
      Terminal::out << "Cluster #" << i << " -> "
                    << clusterList[i].currentTrajectories.size() << endl;
      clusterList[i].previousTrajectories = clusterList[i].currentTrajectories;
      clusterList[i].currentTrajectories.clear();
    }

    // Now we measure the distance for each trajectory, find the best cluster
    for(int idx = 0; idx < ws->trajectories.size(); idx++) {
      double mnd = -1;
      int cm = -1;
      const Vector & params = ws->trajectories[idx].finalParameters;
      for(int c = 0; c < clusterList.size(); c++) {
        double dst =
          ParameterRangeSpec::trajectoryDistance(parameterSpecs,
                                                 clusterList[c].centroid,
                                                 params, ws);
        if(cm < 0 || dst < mnd) {
          mnd = dst;
          cm = c;
        }
      }
      clusterList[cm].currentTrajectories.insert(idx);
      clusterList[cm].distances[idx] = mnd;
    }

    // recenter the clusters
    int nb = 0;
    int unmoved = 0;
    for(Cluster & cl : clusterList) {
      QList<Vector> vects;
      for(int trj : cl.currentTrajectories)
        vects << ws->trajectories[trj].finalParameters;
      cl.oldCentroid = cl.centroid;
      cl.centroid = ParameterRangeSpec::averageParameters(parameterSpecs,
                                                          vects, ws);
      double dst = ParameterRangeSpec::trajectoryDistance(parameterSpecs,
                                                          cl.oldCentroid,
                                                          cl.centroid,
                                                          ws);
      Terminal::out << "Cluster #" << nb++ <<  " now has " << vects.size()
                    << " trajectories, center has moved by "
                    << dst
                    << endl;
      if(dst == 0)
        unmoved++;
    }
    if(unmoved == clusterList.size())
      over = true;
  }

  // OK, so now tagging the trajectories
  int clust = 0;
  for(Cluster & cl : clusterList) {
    QString flg = flag + "-%2-%1";
    flg = flg.arg(clust).arg(clusters);
    for(int idx: cl.currentTrajectories)
      ws->trajectories[idx].flags.insert(flg);

    clust++;
  }

}



ArgumentList kctArgs(QList<Argument*>()
                     << new IntegerArgument("clusters",
                                            "Clusters",
                                            "The number of clusters")
                     << new SeveralStringsArgument("parameters",
                                                   "Parameters",
                                                   "Parameter specification")
                   );

ArgumentList kctOpts(QList<Argument*>()
                    << new StringArgument("flag",
                                          "Flag",
                                          "Flag for the clusters")
                    );

static Command 
kct("cluster-trajectories", // command name
    effector(clusterTrajectoriesCommand), // action
    "fits",  // group name
    &kctArgs, // arguments
    &kctOpts, // options
    "Cluster trajectories",
    "Cluster the trajectories using K-means clustering",
    "", CommandContext::fitContext());
