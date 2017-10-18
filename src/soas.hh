/**
   \file soas.hh
   The Soas meta-container class
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2014 by CNRS/AMU

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
#ifndef __SOAS_HH
#define __SOAS_HH

class MainWin;
class CurveView;
class DataSet;
class DataStack;
class CommandWidget;
class GraphicsSettings;
class ValueHash;

/// The class holding all information/actor related to Soas.
///
/// This class wraps all the information and the effectors that make
/// up a Soas session (but it does not own them). Most of the time,
/// you'll access the application-wide instance using the soas()
/// convenience function.
class Soas {

  MainWin * mw;
  DataStack * ds;
  GraphicsSettings * gs;

  static Soas * theSoasInstance;

  /// The date of QSoas's startup
  QDateTime startup;

  
public:

  Soas();
  ~Soas();

  void setMainWindow(MainWin * m) {
    mw = m;
  };

  static Soas * soasInstance() {
    return theSoasInstance;
  };

  /// Shows a message in the main window status bar.
  void showMessage(const QString &str, int ms = 3000);

  /// Returns the current data set.
  DataSet * currentDataSet(bool silent = false);

  /// Returns the properly quoted command-line
  QString currentCommandLine() const;

  /// Pushes a new dataset onto the stack
  void pushDataSet(DataSet * ds, bool silent = false);

  /// @name Element accessors
  ///
  /// Accessors to the main elements
  /// @{
  MainWin & mainWin() { return *mw; };
  DataStack & stack() { return *ds; };
  CurveView & view();
  CommandWidget & prompt(); 
  GraphicsSettings & graphicsSettings() { return *gs;};
  /// @}

  
  /// The current temperature
  double temperature() const;

  /// The current temperature
  void setTemperature(double t);

  /// Returns the startup time of QSoas
  const QDateTime & startupTime() const {
    return startup;
  };

  /// @name Flags
  ///
  /// These are flags that are used to implement "external"
  /// interruptions.  I'm unsure that's the best way to implement
  /// them.
  ///
  /// @{

  /// If the flag is on, the fit engine should stop.
  bool shouldStopFit;

  /// @}

  /// Write overall specs
  static void writeSpecFile(QTextStream & out, bool full);

  /// Returns a long, descriptive version string
  static QString versionString();

  /// Returns a hash containing a number of lists of "features".
  static ValueHash versionInfo();
};

/// Returns the application-wide Soas instance
inline Soas & soas() {
  return *Soas::soasInstance();
};

#endif
