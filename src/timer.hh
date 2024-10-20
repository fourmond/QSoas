/**
   \file timer.hh
   A class for timing and generally displaying resource consumption
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
#ifndef __TIMER_HH
#define __TIMER_HH

/// A data point for the timer.  This class is both used for storage
/// and for the difference
class TimerData {
public:

  /// The number of milliseconds since the epoch
  qint64 mSecs = 0;

  /// The memory used, in KiB
  int memoryUsed = 0;

  /// Processor used, user time
  long userTime = 0;

  /// Processor used, system time
  long systemTime = 0;

  /// Number of voluntary context switches
  long voluntaryCS = 0;

  /// Number of involuntary context switches
  long involuntaryCS = 0;


  /// Returns the current usage
  static TimerData getCurrent();

  TimerData & operator +=(const TimerData & other);
  TimerData & operator -=(const TimerData & other);

  /// This guys rounds things, but, well, given the precision, if you
  /// have rounding issue, then this tool isn't appropriate
  TimerData & operator *=(double value);

  QString toString(bool compact = true) const;

  /// Returns the date of the tick. Only useful if that is an absolute
  /// value, not a difference.
  QDateTime dateTime() const;
};

/// A timer, an object which:
/// @li records the current time and state of resources at creation
/// @li counts the number of times tick() is called
/// @li stores the last, previous and first state
/// @li provides averages and extrapolations
class Timer {
protected:

  TimerData starting;

  TimerData previous;

  TimerData last;

  int ticks;
public:

  /// Constructs a timer. Use reset() to start it.
  Timer();

  /// Clocks a tick in, and returns the difference with the last one
  TimerData tick();

  /// Returns the number of ticks so far
  int count() const;

  /// Returns true if the timer is running
  bool isRunning() const;

  /// Starts the timer, resetting it to 0 if needed.
  void start();

  /// Stops the timer and reset it
  void reset();

  /// Returns the average resources per tick
  TimerData average() const;

  /// Extrapolates the consumed resources to the given number of ticks
  TimerData extrapolate(int total) const;

  /// Returns the time of the last tick
  QDateTime lastTick() const;

  /// Returns some text for the progress.
  /// Only useable if count() > 0
  QString progressText(int total) const;

};

#endif
