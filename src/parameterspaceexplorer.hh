/**
   \file parameterspaceexplorer.hh
   Parameter space explorers
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
#ifndef __PARAMETERSPACEEXPLORER_HH
#define __PARAMETERSPACEEXPLORER_HH

#include <argumentmarshaller.hh>
#include <factory.hh>

class ParameterSpaceExplorer;
class FitWorkspace;
class ArgumentList;
class Command;
class CommandEffector;

class ParameterSpaceExplorerFactoryItem :
  public Factory<ParameterSpaceExplorer, FitWorkspace *> {
public:

  /// An alias for the description field
  QString & publicName;
  
  /// Creates and register a factory item.
  ParameterSpaceExplorerFactoryItem(const QString & n, 
                                    const QString & pn, Creator c);
};

/// Generates a style for the next curve.
class ParameterSpaceExplorer {

  friend class ParameterSpaceExplorerFactoryItem;

  static ParameterSpaceExplorerFactoryItem * namedFactoryItem(const QString & name);

protected:
  /// The underlying workspac
  FitWorkspace * workSpace;


  static CommandEffector * explorerEffector(const QString & n);
public:

  /// The item used to create the 
  ParameterSpaceExplorerFactoryItem * createdFrom;

  /// Creates the parameter space exploration
  ParameterSpaceExplorer(FitWorkspace * workSpace);
  virtual ~ParameterSpaceExplorer();

  /// A hash public name -> real name for the explorers.
  static QHash<QString, QString> availableExplorers();

  /// Creates the named parameter space explorer
  static ParameterSpaceExplorer * createExplorer(const QString & name,
                                                 FitWorkspace * ws);
  

  /// Creates the commands for the explorers for the given workspace.
  ///
  /// These commands should be destroyed upon destruction of the
  /// workspace.
  static QList<Command *> createCommands(FitWorkspace * workspace);
  
  /// @name Public interface
  ///
  /// @{

  /// Returns the arguments for the explorer command
  virtual ArgumentList * explorerArguments() const = 0;

  /// Returns the options for the explorer
  virtual ArgumentList * explorerOptions() const = 0;

  /// Setup the explorer with the given arguments and options.
  virtual void setup(const CommandArguments & args,
                     const CommandOptions & options) = 0;

  /// Runs the next explorer iteration. Returns true if there are
  /// still iteratons available, and false if not.
  ///
  /// What is meant by running the next iteration is the following:
  /// @li preparing the initial parameters
  /// @li running a fit (or several, or many !, or none, if the
  /// explorer does not do that, but that would be weird)
  ///
  /// The user will have the opportunity to execute a script right @b
  /// after each iteration.
  virtual bool iterate() = 0;

  /// Returns the current progress
  virtual QString progressText() const = 0;

  /// @}
};


#endif
