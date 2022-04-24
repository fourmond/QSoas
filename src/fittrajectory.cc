/*
  fittrajectory.cc: implementation of fit trajectories
  Copyright 2013 by CNRS/AMU

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
#include <fittrajectory.hh>
#include <exceptions.hh>

#include <fitdata.hh>
#include <utils.hh>

#include <columnbasedformat.hh>

FitTrajectory::FitTrajectory(const Vector & init, const Vector & final,
                             const Vector & errors, 
                             double res, double rr,
                             double intr, double d,
                             const Vector & pointRes,
                             const QString & eng,
                             const QDateTime & start,
                             const FitData * data,
                             const QDateTime & end) :
  initialParameters(init), finalParameters(final), 
  parameterErrors(errors), pointResiduals(pointRes),
  ending(FitWorkspace::Converged), residuals(res), relativeResiduals(rr),
  internalResiduals(intr), residualsDelta(d),
  engine(eng), startTime(start)
{
  if(end.isValid())
    endTime = end;
  else
    endTime = QDateTime::currentDateTime();
  if(! final.allFinite())
    ending = FitWorkspace::NonFinite;
  iterations = data->nbIterations;
  evaluations = data->evaluationNumber;

  int total = data->fullParameterNumber();
  int nbp = data->parametersPerDataset();
  for(int i = 0; i < total; i++)
    fixed << data->isFixed(i % nbp, i/nbp);

  pid = QCoreApplication::applicationPid();
}


bool FitTrajectory::isWithinErrorRange(const FitTrajectory & o) const
{
  if(o.finalParameters.size() != finalParameters.size())
    throw InternalError("Comparing fit trajectories of different "
                        "parameter numbers");
  int sz = finalParameters.size();
  for(int i = 0; i < sz; i++) {
    // We assume the errors are positive, but hell they should be !
    double err = parameterErrors[i];
    if(err < 1e-6)
      err = 1e-6;
    err *= fabs(finalParameters[i]);
    if(fabs(o.finalParameters[i] - finalParameters[i]) > err)
      return false;             // out of range
  }
  return true;
}

bool FitTrajectory::flagged(const QString & flag) const
{
  return flags.contains(flag);
}

ColumnBasedFormat * FitTrajectory::formatForTrajectory(const QStringList & s,
                                                       int ds)
{
  if(ds < 0)
    ds = pointResiduals.size();
  
  ColumnBasedFormat * rv = new ColumnBasedFormat;
  // Initial parameters
  for(int i = 0; i < ds; i++) {
    for(int j = 0; j < s.size(); j++)
      rv->addVectorItemColumn(QString("%1[%2]_i").arg(s[j]).arg(i),
                              &initialParameters, i*s.size()+j);
  }

  // Fit status
  rv->addColumn("status",
                [this] (const QString & n) {
                  ending = endingFromName(n);
                },
                [this] () -> QString {
                  return endingName(ending);
                });

  // Final parameters and errors
  for(int i = 0; i < ds; i++) {
    for(int j = 0; j < s.size(); j++) {
      rv->addVectorItemColumn(QString("%1[%2]_f").arg(s[j]).arg(i),
                              &finalParameters, i*s.size()+j);
      rv->addVectorItemColumn(QString("%1[%2]_err").arg(s[j]).arg(i),
                              &parameterErrors, i*s.size()+j);
    }
  }

  // Point residuals
  for(int i = 0; i < ds; i++)
    rv->addVectorItemColumn(QString("point_residuals[%1]").arg(i),
                              &pointResiduals, i);

  // weights
  for(int i = 0; i < ds; i++)
    rv->addVectorItemColumn(QString("buffer_weight[%1]").arg(i),
                            &weights, i, true);

  rv->addNumberColumn("residuals", &residuals);
  rv->addNumberColumn("relative_res", &relativeResiduals);
  rv->addNumberColumn("internal_res", &internalResiduals);
  rv->addStringColumn("engine", &engine);
  
  rv->addTimeColumn("start_time", &startTime, true);
  rv->addTimeColumn("end_time", &endTime, true);
  rv->addIntColumn("iterations", &iterations, true);
  rv->addIntColumn("evals", &evaluations, true);
  rv->addNumberColumn("res_delta", &residualsDelta, true);

  rv->addIntColumn("pid", &pid, true);

  rv->addColumn("fixed",
                [this] (const QString & n) {
                  fixed = Utils::readBooleans(n).toVector();
                },
                [this] () -> QString {
                  return Utils::writeBooleans(fixed.toList());
                }, true);
  rv->addColumn("flags",
                [this] (const QString & n) {
                  flags = n.split(",").toSet();
                  flags.remove("");
                },
                [this] () -> QString {
                  return QStringList(flags.toList()).join(",");
                }, true);
  return rv;
}

QStringList FitTrajectory::exportColumns(const QStringList & names) const
{
  std::unique_ptr<ColumnBasedFormat> c(const_cast<FitTrajectory*>(this)->formatForTrajectory(names));
  return c->writeValues();
}

void FitTrajectory::loadFromColumns(const QHash<QString, QString> & cols,
                                    const QStringList & parameters,
                                    int datasets, bool * ok)
{
  std::unique_ptr<ColumnBasedFormat> c(formatForTrajectory(parameters, datasets));
  QStringList missing;
  // initialize the weights
  weights.resize(datasets);
  for(int i = 0; i < datasets; i++)
    weights[i] = 1;
  
  c->readValues(cols, &missing);

  // Here, sanitize the fixed vector
  int nbtot = datasets*parameters.size();
  if(fixed.size() < nbtot) {
    // make them up all
    for(int i = 0; i < nbtot; i++) {
      bool fx = finalParameters[i] == initialParameters[i];
      if(fixed.size() <= i)
        fixed << fx;
      else
        fixed[i] = fx;
    }
  }
}

QStringList FitTrajectory::exportHeaders(const QStringList & s, int ds)
{
  QStringList ret;
  FitTrajectory t;
  std::unique_ptr<ColumnBasedFormat> c(t.formatForTrajectory(s, ds));
  
  return c->headers();
}

bool FitTrajectory::operator==(const FitTrajectory & o) const
 {
   if(! Vector::withinTolerance(initialParameters,o.initialParameters, 1e-5))
     return false;
   if(! Vector::withinTolerance(finalParameters, o.finalParameters, 1e-5))
     return false;
   if(! Vector::withinTolerance(parameterErrors, o.parameterErrors, 1e-5))
     return false;

   if(startTime != o.startTime)
     return false;
   if(endTime != o.endTime)
     return false;

   // Lets forget the rest for now...

   return true;
 }

QString FitTrajectory::endingName(FitWorkspace::Ending end)
{
  switch(end) {
  case FitWorkspace::Converged:
    return "ok";
  case FitWorkspace::Cancelled:
    return "(cancelled)";
  case FitWorkspace::TimeOut:
    return "(time out)";
  case FitWorkspace::Error:
    return "(fail)";
  case FitWorkspace::Exception:
    return "(exception)";
  case FitWorkspace::NonFinite:
    return "(non finite)";
  default:
    ;
  }
  return "ARGH!";
}

FitWorkspace::Ending FitTrajectory::endingFromName(const QString & n)
{
  if(n == "ok")
    return FitWorkspace::Converged;
  if(n == "(cancelled)")
    return FitWorkspace::Cancelled;
  if(n == "(time out)")
    return FitWorkspace::TimeOut;
  if(n == "(fail)")
    return FitWorkspace::Error;
  if(n == "(exception)")
    return FitWorkspace::Exception;
  if(n == "(non finite)")
    return FitWorkspace::NonFinite;
  return FitWorkspace::Invalid;
}



//////////////////////////////////////////////////////////////////////

FitTrajectoryCluster::FitTrajectoryCluster(const FitTrajectory & trj)
{
  trajectories << trj;
}

/// A sort function to have big clusters come out first.
static bool cmp(const FitTrajectoryCluster & a, const FitTrajectoryCluster & b)
{
  if( (a.trajectories.size() > 0) && 
      (a.trajectories.size() == b.trajectories.size()) )
    return (a.trajectories[0].relativeResiduals < 
            b.trajectories[0].relativeResiduals);
            
  // Hey emacs, you'r not quite right about indentation here
  return a.trajectories.size() > b.trajectories.size();
}

QList<FitTrajectoryCluster> FitTrajectoryCluster::clusterTrajectories(const QList<FitTrajectory> * trajectories)
{
  QList<FitTrajectoryCluster> clusters;
  if(trajectories->size() <= 0)
    return clusters;

  
  for(int i = 0; i < trajectories->size(); i++) {
    int alreadyIn = -1;
    const FitTrajectory & traj = (*trajectories)[i];
    for(int j = 0; j < clusters.size(); j++) {
      if(clusters[j].belongsToCluster(traj)) {
        if(alreadyIn >= 0) {
          clusters[alreadyIn].mergeCluster(clusters[j]);
          j--;
          clusters.removeAt(j);
        }
        else {
          alreadyIn = j;
          clusters[j].trajectories << traj;
        }
      }
    }
    if(alreadyIn < 0) {
      // We form a new cluster
      clusters << FitTrajectoryCluster(traj);
    }
  }

  // Now sort the clusters
  for(int j = 0; j < clusters.size(); j++)
    qSort(clusters[j].trajectories);

  
  qSort(clusters.begin(), clusters.end(), cmp);
  
  return clusters;
}

void FitTrajectoryCluster::mergeCluster(const FitTrajectoryCluster & other)
{
  trajectories << other.trajectories;
}

bool FitTrajectoryCluster::belongsToCluster(const FitTrajectory & traj) const
{
  for(int i = 0; i < trajectories.size(); i++) {
    const FitTrajectory & t = trajectories[i];
    if(t.isWithinErrorRange(traj) || traj.isWithinErrorRange(t))
      return true;
  }
  return false;
}

QString FitTrajectoryCluster::dump() const 
{
  QString str;
  const FitTrajectory & rep = trajectories[0];
  str += QString("Cluster with %1 members\n"
                 "Best residuals: %2\n"
                 "Representative member:\n").
    arg(trajectories.size()).
    arg(rep.relativeResiduals);
  for(int i = 0; i < rep.finalParameters.size(); i++)
    str += QString(" * param[%1] = %2 +- %3%\n").arg(i).
      arg(rep.finalParameters[i]).
      arg(rep.parameterErrors[i] * 100);
  return str;
}
