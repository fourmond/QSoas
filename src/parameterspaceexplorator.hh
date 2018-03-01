/**
   \file parameterspaceexplorator.hh
   Parameter space explorators
   Copyright 2018 by Vincent Fourmond

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
#ifndef __PARAMETERSPACEEXPLORATOR_HH
#define __PARAMETERSPACEEXPLORATOR_HH

#include <argumentmarshaller.hh>

class ParameterSpaceExplorator;
class FitWorkspace;
class ArgumentList;
class Command;

class ParameterSpaceExploratorFactoryItem {
public:
  /// A generator function, taking the overall number of styles to
  /// give and an optional string argument.

  typedef std::function<ParameterSpaceExplorator * (FitWorkspace *) > Creator;

protected:
  friend class ParameterSpaceExplorator;
  /// The creation function
  Creator creator;

public:
  /// The creation name
  QString name;

  /// The public name
  QString publicName;

  /// Creates and register a factory item.
  ParameterSpaceExploratorFactoryItem(const QString & n, 
                                      const QString & pn, Creator c);
};

/// Generates a style for the next curve.
class ParameterSpaceExplorator {

  /// The application-wide ParameterSpaceExplorator factory
  static QHash<QString, ParameterSpaceExploratorFactoryItem*> * factory;

  friend class ParameterSpaceExploratorFactoryItem;

  static void registerFactoryItem(ParameterSpaceExploratorFactoryItem * item);

  static ParameterSpaceExploratorFactoryItem * namedFactoryItem(const QString & name);


protected:
  /// The underlying workspac
  FitWorkspace * workSpace;

public:

  /// Creates the parameter space exploration
  ParameterSpaceExplorator(FitWorkspace * workSpace);
  virtual ~ParameterSpaceExplorator();

  /// A hash public name -> real name for the explorators.
  static QHash<QString, QString> availableExplorators();

  /// Creates the named parameter space explorator
  static ParameterSpaceExplorator * createExplorator(const QString & name,
                                                     FitWorkspace * ws);
  

  /// Creates the commands for the explorators for the given workspace.
  ///
  /// These commands should be destroyed upon destruction of the
  /// workspace.
  static QList<Command *> createCommands(FitWorkspace * workspace);
  
  /// @name Public interface
  ///
  /// @{

  /// Returns the arguments for the explorator command
  virtual ArgumentList * exploratorArguments() const = 0;

  /// Returns the options for the explorator
  virtual ArgumentList * exploratorOptions() const = 0;

  /// Setup the explorator with the given arguments and options.
  virtual void setup(const CommandArguments & args,
                     const CommandOptions & options) = 0;

  /// Runs the next explorator iteration. Returns true if there are
  /// still iteratons available, and false if not.
  virtual bool iterate() = 0;

  /// Returns the current progress
  virtual QString progressText() const = 0;

  /// @}
};


#endif
