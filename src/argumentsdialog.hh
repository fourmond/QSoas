/**
   \file argumentsdialog.hh
   A dialog box for prompting all the arguments/options of a command.
   Copyright 2019 by CNRS/AMU

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
#ifndef __ARGUMENTSDIALOG_HH
#define __ARGUMENTSDIALOG_HH

#include <argumentlist.hh>

class Command;
class Argument;


class ArgumentEditor : public QObject {
  Q_OBJECT;

  const Argument * argument;

  bool isOption;

  /// Only used when it is not an option
  QLabel * argName;

  /// Only used when it is an option
  QCheckBox * optName;

  /// The editor.
  QWidget * editor;
  
public:
  ArgumentEditor(const Argument * arg, bool isOption);
  ~ArgumentEditor();
  
  /// Adds the elements of the object to the target layout.
  void addToGrid(QGridLayout * target, int row);

  void setValue(const ArgumentMarshaller * value);
  ArgumentMarshaller * getValue() const;

  /// Returns true if it is an argument or a present option
  bool isPresent() const;

  /// Returns the name of the argument -- useful for options mostly.
  QString argumentName() const;

protected slots:
  void enable(bool enabled);
};


/// A dialog box for prompting values of 
class ArgumentsDialog : public QDialog {
  Q_OBJECT;

  /// The underlying command.
  const Command * command;

  QList<ArgumentEditor *> arguments;
  QList<ArgumentEditor *> options;
  
public:

  ArgumentsDialog(const Command * command);
  ~ArgumentsDialog();

  /// Runs a full prompt for the given command, and returns true if
  /// the user wants to proceed with the command.
  ///
  /// If the user cancels, then the contents of args and opts are
  /// cleared.
  static bool doFullPrompt(const Command * cmd,
                           CommandArguments * args,
                           CommandOptions * opts);

  void retrieveArgumentsAndOptions(CommandArguments * args,
                                   CommandOptions * opts) const;
};



#endif
