/**
   \file argument.hh
   Argument handling for QSoas.
   Copyright 2011 by Vincent Fourmond

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
#ifndef __ARGUMENT_HH
#define __ARGUMENT_HH

#include <argumentmarshaller.hh>
class Command;

/// An argument. This is an abstract base class that must be reimplemented.
///
/// It represents an argument to a command, including the methods to
/// convert from string and to prompt for a value. Possibly include
/// range checking facilities too for integers, floats...
///
/// It relies on the ArgumentMarshaller framework to carry arguments
/// in a type-safe fashion.
///
/// All commands take a list of arguments.
class Argument {
protected:

  const char * name;

  const char * pubName;

  const char * desc;

public:

  /// The argument is greedy if it can accumulate more than one
  /// argument. There can't be more than one greedy argument in an
  /// ArgumentList. 
  bool greedy;

  /// There can be a default option, ie the one used
  bool defaultOption;

  /// Specifies the various elements linked to the Argument.
  ///
  /// \warning Argument doesn't take ownership of any string; they
  /// should therefore point to locations that will not move, ideally
  /// constant strings.
  Argument(const char * cn, const char * pn,
           const char * d = "", bool g = false, bool def = false) : 
    name(cn), pubName(pn), 
    desc(d), greedy(g), defaultOption(def) {
  }; 
  

  /// The argument name, for displaying purposes (not the same as
  /// public name)
  virtual QString argumentName() const {
    return name;
  };


  /// The public name, the one to be used in the menus. 
  virtual QString publicName() const {
    return QObject::tr(pubName);
  };

  /// A description, used for tool tips
  virtual QString description() const {
    return QObject::tr(desc);
  };

  /// Converts from string to the argument with the correct type.
  virtual ArgumentMarshaller * fromString(const QString & str) const = 0;

  /// Prompts for a value for the argument, using something of a
  /// dialog box or the like. Default implementation prompts for a
  /// standard string.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const;

  
  /// Prompts for a value for the argument, but returns the value as a
  /// QString.
  ///
  /// @todo This should be rewritten as a QString-returning function
  /// for prompting all arguments/options (in a grid-like widget with
  /// addition of options + line edit + automatic completion +
  /// prompting when applicable)
  ///
  /// Ideas: how to decide whether we want to use a full dialog box or
  /// just prompt for the missing arguments ?
  ///
  /// If there is more than one argument missing, always display the
  /// dialog box.
  ///
  /// If there is only one missing, maybe CTRL+Enter would allow the
  /// display of the fully-blown dialog box ?
  virtual QString promptAsString(QWidget * base) const;

  /// If this function returns an non-empty list, then it corresponds
  /// to all the possible choices (ie no free-form). In particular,
  /// prompting will be done using a non-editable QComboBox, and no
  /// prompting dialog will show up.
  ///
  /// @todo Maybe have a way to provide a "real" string and a
  /// "display" string ?
  virtual QStringList allChoices() const;


  /// Handling of automatic completion. Provided with a beginning of
  /// the string, propose a QStringList with potential candidates.
  virtual QStringList proposeCompletion(const QString & starter) const;

  /// Appends the value of \p b to \p a. Must be reimplemented by
  /// arguments that can be greedy.
  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const;
  
};

#endif
