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

void FitTrajectories::exportToFile(QTextStream & out)
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
    trajectories << FitTrajectory();
    trajectories.last().loadFromColumns(ls, sz);
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
  std::sort(trajectories.begin(), trajectories.end(),
            [](FitTrajectory a, FitTrajectory b) -> bool {
              return a.startTime < b.startTime;
            });
  clearCache();
}

void FitTrajectories::sortByResiduals()
{
  std::sort(trajectories.begin(), trajectories.end());
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
    if(trajectories[i].relativeResiduals > factor * res) {
      trajectories.takeAt(i);
      ++nb;
    }
    else
      ++i;
  }
  return nb;
}

FitTrajectory & FitTrajectories::last()
{
  return trajectories.last();
}

void FitTrajectories::clear()
{
  trajectories.clear();
}

QList<FitTrajectory>::const_iterator FitTrajectories::begin() const
{
  return trajectories.begin();
}

QList<FitTrajectory>::const_iterator FitTrajectories::end() const
{
  return trajectories.end();
}
