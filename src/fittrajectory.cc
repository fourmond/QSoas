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

FitTrajectory::FitTrajectory(const Vector & init, const Vector & final,
                             const Vector & errors, 
                             double res, double rr,
                             double intr, double d,
                             const QString & eng,
                             const QDateTime & start,
                             const FitData * data,
                             const QDateTime & end) :
    initialParameters(init), finalParameters(final), 
    parameterErrors(errors),
    ending(FitWorkspace::Converged), residuals(res), relativeResiduals(rr),
    internalResiduals(intr), residualsDelta(d),
    engine(eng), startTime(start) {
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
  };


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

QStringList FitTrajectory::exportColumns() const
{
  QStringList ret;
  
  for(int i = 0; i < initialParameters.size(); i++)
    ret << QString::number(initialParameters[i], 'g', 8);

  ret << endingName(ending);

  for(int i = 0; i < finalParameters.size(); i++)
    ret << QString::number(finalParameters[i], 'g', 8)
        << QString::number(parameterErrors[i], 'g', 8);

  ret << QString::number(residuals)
      << QString::number(relativeResiduals)
      << QString::number(internalResiduals)
      << engine
      << QString::number(startTime.toMSecsSinceEpoch())
      << QString::number(endTime.toMSecsSinceEpoch())
      << QString::number(iterations)
      << QString::number(evaluations)
      << QString::number(residualsDelta)
      << Utils::writeBooleans(fixed.toList())
      << QStringList(flags.toList()).join(",");

  return ret;
}

void FitTrajectory::loadFromColumns(const QStringList & cls, int nb, bool * ok)
{
  QStringList cols(cls);

  class Spec {};
  if(ok)
    *ok = true;

  auto next = [&cols]() -> QString {
    if(cols.size() > 0)
      return cols.takeFirst();
    throw Spec();
    return QString();
  };

  initialParameters.resize(nb);

  try {
  
    for(int i = 0; i < initialParameters.size(); i++)
      initialParameters[i] = next().toDouble(); // No validation ?

    ending = endingFromName(next());


    finalParameters.resize(nb);
    parameterErrors.resize(nb);
    for(int i = 0; i < finalParameters.size(); i++) {
      finalParameters[i] = next().toDouble();
      parameterErrors[i] = next().toDouble();
    }

    residuals = next().toDouble();
    relativeResiduals = next().toDouble();
    internalResiduals = next().toDouble();
    engine = next();
    if(cols.size() == 0)
      return;
      
    qint64 t = next().toLongLong();
    startTime = QDateTime::fromMSecsSinceEpoch(t);
    t = next().toLongLong();
    endTime = QDateTime::fromMSecsSinceEpoch(t);
    
    if(cols.size() == 0)
      return;

    iterations = next().toInt();
    evaluations = next().toInt();

    if(cols.size() == 0)
      return;
    residualsDelta = next().toDouble();
    fixed =  Utils::readBooleans(next()).toVector();

    flags = next().split(",").toSet();
  }
  catch(Spec & s) {
    if(ok)
      *ok = false;
  }
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
  ret << "residuals" << "relative_res" << "internal_res"
      << "engine"  << "flags";
  return ret;
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
