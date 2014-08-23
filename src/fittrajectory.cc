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
