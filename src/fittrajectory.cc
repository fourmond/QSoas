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

QStringList FitTrajectory::exportColumns() const
{
  QStringList ret;
  
  for(int i = 0; i < initialParameters.size(); i++)
    ret << QString::number(initialParameters[i]);

  ret << endingName(ending);

  for(int i = 0; i < finalParameters.size(); i++)
    ret << QString::number(finalParameters[i])
        << QString::number(parameterErrors[i]);

  ret << QString::number(residuals)
      << QString::number(relativeResiduals)
      << QString::number(internalResiduals)
      << engine;

  return ret;
}

void FitTrajectory::loadFromColumns(const QStringList & cls, int nb)
{
  QStringList cols(cls);

  initialParameters.resize(nb);
  for(int i = 0; i < initialParameters.size(); i++)
    initialParameters[i] = cols.takeFirst().toDouble(); // No validation ?

  ending = endingFromName(cols.takeFirst());


  finalParameters.resize(nb);
  parameterErrors.resize(nb);
  for(int i = 0; i < finalParameters.size(); i++) {
    finalParameters[i] = cols.takeFirst().toDouble();
    parameterErrors[i] = cols.takeFirst().toDouble();
  }

  residuals = cols.takeFirst().toDouble();
  relativeResiduals = cols.takeFirst().toDouble();
  internalResiduals = cols.takeFirst().toDouble();
  engine = cols.takeFirst();
}

QStringList FitTrajectory::exportHeaders(const QStringList & s, int ds)
{
  QStringList ret;

  for(int i = 0; i < ds; i++)
    for(int j = 0; j < s.size(); j++)
      ret << QString("%1[%2]_i").arg(s[j]).arg(i);

  ret << "status";
  for(int i = 0; i < ds; i++)
    for(int j = 0; j < s.size(); j++)
      ret << QString("%1[%2]_f").arg(s[j]).arg(i)
          << QString("%1[%2]_err").arg(s[j]).arg(i);
  ret << "residuals" << "relative_res" << "internal_res" << "engine";
  return ret;
}

QString FitTrajectory::endingName(FitTrajectory::Ending end)
{
  switch(end) {
  case Converged:
    return "ok";
  case Cancelled:
    return "(cancelled)";
  case TimeOut:
    return "(time out)";
  case Error:
    return "(fail)";
  case NonFinite:
    return "(non finite)";
  default:
    ;
  }
  return "ARGH!";
}

FitTrajectory::Ending FitTrajectory::endingFromName(const QString & n)
{
  if(n == "ok")
    return Converged;
  if(n == "(cancelled)")
    return Cancelled;
  if(n == "(time out)")
    return TimeOut;
  if(n == "(fail)")
    return Error;
  if(n == "(non finite)")
    return NonFinite;
  return Invalid;
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
