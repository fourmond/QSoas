/*
  timer.cc: implementation of the Timer class
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
#include <timer.hh>

#include <utils.hh>
#include <exceptions.hh>

TimerData TimerData::getCurrent()
{
  TimerData rv;
  rv.mSecs = QDateTime::currentDateTime().toMSecsSinceEpoch();
  rv.memoryUsed = Utils::memoryUsed();
  Utils::processorUsed(&rv.userTime, &rv.systemTime,
                       &rv.voluntaryCS, &rv.involuntaryCS);
  return rv;
}

TimerData & TimerData::operator +=(const TimerData & other)
{
  mSecs         += other.mSecs;
  memoryUsed    += other.memoryUsed;
  userTime      += other.userTime;
  systemTime    += other.systemTime;
  voluntaryCS   += other.voluntaryCS;
  involuntaryCS += other.involuntaryCS;
  return *this;
}

TimerData & TimerData::operator -=(const TimerData & other)
{
  mSecs         -= other.mSecs;
  memoryUsed    -= other.memoryUsed;
  userTime      -= other.userTime;
  systemTime    -= other.systemTime;
  voluntaryCS   -= other.voluntaryCS;
  involuntaryCS -= other.involuntaryCS;
  return *this;
}

TimerData & TimerData::operator *=(double value)
{
  mSecs         *= value;
  memoryUsed    *= value;
  userTime      *= value;
  systemTime    *= value;
  voluntaryCS   *= value;
  involuntaryCS *= value;
  return *this;
}

QString TimerData::toString(bool compact) const
{
  QString base = compact ?
    "%1 tot, %2 proc, %3 us, %4 sys, %5 MiB, %6 vcs, %7 ics" :
    "%1 seconds elapsed since timer start, (%2 total processor time, "
    "%3 user, %4 system, %5 MiB, %6 voluntary CS, %7 involuntary CS)";
  return base.
    arg(mSecs*0.001).
    arg((userTime  + systemTime)*0.001).
    arg(userTime * 0.001).
    arg(systemTime * 0.001).
    arg(memoryUsed * 0.001).
    arg(voluntaryCS).arg(involuntaryCS);
}


//////////////////////////////////////////////////////////////////////

Timer::Timer() : ticks(-1)
{
}

void Timer::start()
{
  ticks = 0;
  starting = TimerData::getCurrent();
  previous = starting;
  last = starting;
}

void Timer::reset()
{
  ticks = -1;
}

bool Timer::isRunning() const
{
  return ticks >= 0;
}

TimerData Timer::tick()
{
  if(ticks < 0)
    throw InternalError("Running tick on an unstarted timer");
  previous = last;
  ticks += 1;
  last = TimerData::getCurrent();
  TimerData rv = last;
  rv -= previous;
  return rv;
}



//////////////////////////////////////////////////////////////////////
// A rudimentary timer command

#include <command.hh>
#include <general-arguments.hh>
#include <commandeffector-templates.hh>

#include <terminal.hh>
#include <debug.hh>


void timerCommand(const QString &, const CommandOptions & opts)
{
  static Timer timer;
  QString name;
  updateFromOptions(opts, "name", name);
  if(timer.isRunning()) {
    TimerData delta = timer.tick();
    timer.reset();

    QString message;
    if(! name.isEmpty())
      message = name + ": " + delta.toString();
    else
      message = delta.toString(false);
    Terminal::out << message << endl;
    Debug::debug() << message << endl;
  }
  else {
    timer.start();
    Terminal::out << "Starting timer " << name <<  endl;
    if(name.isEmpty())
      Debug::debug() << "Starting timer" << endl;
  }
}

static ArgumentList 
tmO(QList<Argument *>() 
    << new StringArgument("name",
                          "Name",
                          "name for the timer")
    );


static Command 
tm("timer", // command name
   effector(timerCommand), // action
   "file",  // group name
   NULL, // arguments
   &tmO, // options
   "Timer",
   "Start/stop timer");
