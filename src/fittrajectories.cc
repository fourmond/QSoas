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
  QStringList hds =
    FitTrajectory::exportHeaders(workSpace->parameterNames(),
                                 workSpace->datasetNumber());
  
  out << "## " << hds.join("\t") << endl;
  int sz = trajectories.size();
  for(int i = 0; i < sz; i++) {
    QStringList lst = trajectories[i].exportColumns();
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
              return trajectories[a] < trajectories[b];
            });
  
}

int FitTrajectories::importFromFile(QTextStream & in)
{
  LineReader lr(&in);
  int nb = 0;
  int line = 0;
  int sz = workSpace->parameterNames().size() * workSpace->datasetNumber();
  while(!lr.atEnd()) {
    line += 1;
    QString ln = lr.readLine();
    if(ln.startsWith("#"))
      continue;
    QStringList ls = ln.split("\t");
    if(ls.size() < sz) {
      Terminal::out << "Line " << line << " has only " << ls.size()
                    << " entries, need at least " << sz << endl;
      continue;
    }
    FitTrajectory t;
    bool ok = true;
    t.loadFromColumns(ls, sz, workSpace->datasetNumber(), &ok);
    if(ok) {
      trajectories << t;
      nb++;
    }
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
  for(int i = max-1; i < res.size(); i++)
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

FitTrajectories FitTrajectories::flaggedTrajectories(const QString & flag)
  const
{
  FitTrajectories rv(workSpace);
  for(const FitTrajectory & t : *this) {
    if(t.flagged(flag))
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
