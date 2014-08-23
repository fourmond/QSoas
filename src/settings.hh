/**
   \file settings.hh
   Implementation of application-wide settings.
   Copyright 2011 by Vincent Fourmond
             2012 by CNRS/AMU

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
#ifndef __SETTINGS_HH
#define __SETTINGS_HH

/// Settings
class Settings {
protected:

  /// Settings name (ie key name)
  QString name;

  /// Save the settings
  virtual void save(QSettings * target) = 0;

  /// Load the settings
  virtual void load(QSettings * source) = 0;

  /// The overall list of settings
  static QList<Settings *> * settings;
  
  /// Register a Settings object globally.
  static void registerSettings(Settings * settings);
public:
  
  Settings(const QString & k);

  static void loadSettings(const QString & org, const QString & app);
  static void saveSettings(const QString & org, const QString & app);
  
};


#endif
