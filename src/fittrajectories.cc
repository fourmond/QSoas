/*
  fittrajectories.cc: implementation of fit trajectory lists
  Copyright 2018 by CNRS/AMU

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
#include <fittrajectories.hh>
#include <fittrajectory.hh>
#include <exceptions.hh>

#include <fitdata.hh>
#include <utils.hh>


#include <linereader.hh>
#include <terminal.hh>

FitTrajectories::FitTrajectories(const FitWorkspace * ws) :
  workSpace(ws)
{
}

void FitTrajectories::exportToFile(QTextStream & out) const
{
  QStringList pns = workSpace->parameterNames();
  QStringList hds =
    FitTrajectory::exportHeaders(pns, workSpace->datasetNumber());
  
  out << "## " << hds.join("\t") << endl;
  int sz = trajectories.size();
  for(int i = 0; i < sz; i++) {
    QStringList lst = trajectories[i].exportColumns(pns);
    out << lst.join("\t") << endl;
  }
}

void FitTrajectories::clearCache() const
{
  residualsOrder.clear();
}

void FitTrajectories::updateCache() const
{
  clearCache();
  int sz = size();
  for(int i = 0; i < sz; i++)
    residualsOrder << i;
  std::sort(residualsOrder.begin(), residualsOrder.end(),
            [this](int a, int b) -> bool {
              if(! std::isfinite(trajectories[a].residuals))
                return false;
              if(! std::isfinite(trajectories[b].residuals))
                return true;
              return trajectories[a] < trajectories[b];
            });
  
}

int FitTrajectories::importFromFile(QTextStream & in)
{
  LineReader lr(&in);
  int nb = 0;
  int line = 0;
  QStringList names = workSpace->parameterNames();

  QStringList headers;
  while(!lr.atEnd()) {
    line += 1;
    QString ln = lr.readLine();
    if(ln.startsWith("## ")) {
      // QTextStream o(stdout);
      
      headers = ln.mid(3).split("\t");
      // o << "Found headers: \n * " << headers.join("\n * ") << endl;
      continue;
    }
    if(ln.startsWith("#"))
      continue;
    
    QStringList ls = ln.split("\t");
    QHash<QString, QString> vals;
    bool missing = false;
    // QTextStream o(stdout);
    for(int i = 0; i < headers.size(); i++) {
      if(ls.size() <= i) { /// @todo Should signal missing stuff ?
        missing = true;
        break;
      }
      vals[headers[i]] = ls[i];
      // o << " * " << headers[i] << " -> " << ls[i] << endl;
    }
    if(missing)
      continue;


    
    /// @todo Should signal extra bits ?
    
    FitTrajectory t;
    t.loadFromColumns(vals, names, workSpace->datasetNumber());
    trajectories << t;
    nb++;
  }
  clearCache();
  return nb;
}

FitTrajectories & FitTrajectories::operator<<(const FitTrajectory & trj)
{
  trajectories << trj;
  clearCache();
  return *this;
}

void FitTrajectories::sort()
{
  sort([](const FitTrajectory &a, const FitTrajectory &b) -> bool {
         return a.startTime < b.startTime;
       });
}

void FitTrajectories::sortByResiduals()
{
  std::sort(trajectories.begin(), trajectories.end());
  clearCache();
}

void FitTrajectories::sort(std::function<bool (const FitTrajectory &a,
                                               const FitTrajectory & b)> cmp)
{
  std::sort(trajectories.begin(), trajectories.end(), cmp);
  clearCache();
}




const FitTrajectory & FitTrajectories::operator[](int idx) const
{
  return trajectories[idx];
}

FitTrajectory & FitTrajectories::operator[](int idx)
{
  return trajectories[idx];
}

const FitTrajectory & FitTrajectories::best(int idx) const
{
  if(residualsOrder.size() != trajectories.size())
    updateCache();
  return trajectories[residualsOrder[idx]];
}


int FitTrajectories::size() const
{
  return trajectories.size();
}

void FitTrajectories::merge(const FitTrajectories & other)
{
  trajectories += other.trajectories;
  sort();
  int i = 1;
  while(i < trajectories.size()) {
    while(i <  trajectories.size() && trajectories[i] == trajectories[i-1]) {
      trajectories.takeAt(i);
    }
    i++;
  }
}

int FitTrajectories::trim(double factor)
{
  if(trajectories.size() == 0)
    return 0;
  double res = best().relativeResiduals;
  int i = 0;
  int nb = 0;
  while(i < size()) {
    // strip also NaNs
    if(trajectories[i].relativeResiduals > factor * res
       || std::isnan(trajectories[i].relativeResiduals)) {
      trajectories.takeAt(i);
      ++nb;
    }
    else
      ++i;
  }
  return nb;
}

int FitTrajectories::keepBestTrajectories(int max)
{
  int i = 0;
  int nb = 0;
  typedef QPair<double, int> Pair;
  QList<Pair > res;
  while(i < size()) {
    // strip also NaNs
    if(std::isnan(trajectories[i].relativeResiduals)) {
      trajectories.takeAt(i);
      ++nb;
    }
    else {
      res << Pair(trajectories[i].relativeResiduals, i);
      ++i;
    }
  }
  std::sort(res.begin(), res.end(), [](const Pair & a, const Pair & b) -> bool {
      return a.first < b.first;
    });
  QList<int> toTrim;
  for(int i = max; i < res.size(); i++)
    toTrim << res[i].second;
  std::sort(toTrim.begin(), toTrim.end());
  while(toTrim.size() > 0) {
    int idx = toTrim.takeLast();
    trajectories.takeAt(idx);
    ++nb;
  }
  return nb;
}

FitTrajectory & FitTrajectories::last()
{
  return trajectories.last();
}

const FitTrajectory & FitTrajectories::last() const
{
  return trajectories.last();
}

void FitTrajectories::clear()
{
  trajectories.clear();
}

void FitTrajectories::remove(int idx)
{
  clearCache();
  trajectories.takeAt(idx);
}

FitTrajectories FitTrajectories::flaggedTrajectories(const QString & flag,
                                                     bool flagged)
  const
{
  FitTrajectories rv(workSpace);
  for(const FitTrajectory & t : *this) {
    bool tf;
    // QTextStream o(stdout);
    if(flag.isEmpty()) {
      QStringList flgs = t.flags.toList();
      // o << "Flags: " << flgs.size() << " '" << flgs.join("', '")
      //   << "'" << endl; 
      tf = (t.flags.size() > 0);
    }
    else
      tf = t.flagged(flag);
    if((flagged && tf) || (!flagged && !tf))
      rv << t;
  }
  return rv;
}

QSet<QString> FitTrajectories::allFlags() const
{
  QSet<QString> rv;
  for(const FitTrajectory & t : *this)
    rv += t.flags;
  return rv;
}

QList<Vector> FitTrajectories::summarizeTrajectories(double * weight) const
{
  if(trajectories.size() == 0)
    throw RuntimeError("No trajectory yet");

  const FitTrajectory & bst = best();
  double scale = bst.residuals;
  Vector params(bst.finalParameters.size(), 0);
  Vector stddev = params;
  Vector errors = params;

  double wt = 0;
  for(const FitTrajectory & traj : trajectories) {
    double w = exp(-(traj.residuals/scale - 1));
    Vector a = traj.finalParameters;
    Vector b = a;
    a *= w;
    params += a;
    b *= b;
    b *= w;
    stddev += b;

    a = traj.parameterErrors;
    a *= w;
    errors += a;

    wt += w;

  }

  params /= wt;
  errors /= wt;
  stddev /= wt;
  Vector tmp = params;
  tmp *= params;
  stddev -= tmp;
  for(int i = 0; i < stddev.size(); i++)
    stddev[i] = sqrt(stddev[i]);
  if(weight)
    *weight = wt;
  QList<Vector> lst;
  lst << params << errors << stddev;
  return lst;
}


QList<FitTrajectory>::const_iterator FitTrajectories::begin() const
{
  return trajectories.begin();
}

QList<FitTrajectory>::const_iterator FitTrajectories::end() const
{
  return trajectories.end();
}

QList<FitTrajectory>::iterator FitTrajectories::begin()
{
  clearCache();
  return trajectories.begin();
}

QList<FitTrajectory>::iterator FitTrajectories::end()
{
  return trajectories.end();
}
