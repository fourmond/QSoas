/*
  fit-arguments.cc: implementation of many fit commands
  Copyright 2024 by CNRS/AMU

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
#include <fit-arguments.hh>
#include <fitworkspace.hh>
#include <fittrajectory.hh>

#include <utils.hh>



TrajectoriesArgument::TrajectoriesArgument(const char * cn,
                                           const char * pn,
                                           const char * d,
                                           bool def) :
  Argument(cn, pn, d, false, def) {
}

  
ArgumentMarshaller * TrajectoriesArgument::fromString(const QString & str) const
{
  QStringList spl = str.split(":");
  if(spl.size() == 0)
    throw RuntimeError("Invalid trajectories specification: '%1'").
      arg(str);
  QString what = spl.takeFirst();
  QString rest = spl.join(":");
  FitWorkspace * ws = FitWorkspace::currentWorkspace();

  if(what == "flagged") {
    return new ArgumentMarshallerChild<FitTrajectories>
      (ws->trajectories.flaggedTrajectories(rest));
  }
  if(what == "flagged-") {
    return new ArgumentMarshallerChild<FitTrajectories>
      (ws->trajectories.flaggedTrajectories(rest, false));
  }
  if(what == "all")
    return new ArgumentMarshallerChild<FitTrajectories>
      (ws->trajectories);
      
  throw RuntimeError("Invalid trajectories specification: '%1'").
    arg(str);
  return NULL;
}

void TrajectoriesArgument::concatenateArguments(ArgumentMarshaller * a,
                                                const ArgumentMarshaller * b) const
{
  for(const FitTrajectory & t : b->value<FitTrajectories>())
    a->value<FitTrajectories>() << t;
}


QStringList TrajectoriesArgument::proposeCompletion(const QString & starter) const
{
  FitWorkspace * ws = FitWorkspace::currentWorkspace();
  QStringList names;
  for(const QString & s : ws->trajectories.allFlags())
    names << "flagged:" + s << "flagged-:" + s;
  names << "all" << "flagged" << "flagged-";
  return Utils::stringsStartingWith(names, starter);
}


QString TrajectoriesArgument::typeName() const
{
  return "trajectories";
}

QString TrajectoriesArgument::typeDescription() const
{
  return "Fit Trajectories";
}

ArgumentMarshaller * TrajectoriesArgument::fromRuby(mrb_value value) const
{
  return Argument::convertRubyString(value);
}

QStringList TrajectoriesArgument::toString(const ArgumentMarshaller * arg) const
{
  QStringList lst;
  NOT_IMPLEMENTED;
  return lst;
}

QWidget * TrajectoriesArgument::createEditor(QWidget * parent) const
{
  return Argument::createTextEditor(parent);
}

void TrajectoriesArgument::setEditorValue(QWidget * editor,
                                          const ArgumentMarshaller * value) const
{
  Argument::setTextEditorValue(editor, value);
}
