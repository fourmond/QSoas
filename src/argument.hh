/**
   \file argument.hh
   Argument handling for QSoas.
   Copyright 2011 by Vincent Fourmond
             2012, 2013, 2017 by CNRS/AMU

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
class Argument : public QSharedData {
  Argument(const Argument & o);
protected:

  QString name;

  QString pubName;

  QString desc;

  /// Converts a plain Ruby string using the type's fromString() method.
  ArgumentMarshaller * convertRubyString(mrb_value value) const;

  /// Converts from a Ruby Array using fromString() and
  /// concatenateArguments() 
  ArgumentMarshaller * convertRubyArray(mrb_value value) const;

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
    return pubName;
  };

  /// A description, used for tool tips
  virtual QString description() const {
    return desc;
  };

  /// Sets the description for the argument
  void setDescription(const QString & s);


  /// The type name is single word (ie no spaces) used to describe the
  /// nature of the argument
  virtual QString typeName() const;

  /// More detailed description of the type, intended to be used as a
  /// pop-up or something like that.
  virtual QString typeDescription() const;

  /// @name Type-conversion interface
  ///
  /// These functions are used to convert from input (string, ruby) to
  /// the correctly typed ArgumentMarshaller and back.
  ///
  /// @{

  /// Converts from string to the argument with the correct type.
  virtual ArgumentMarshaller * fromString(const QString & str) const = 0;

  /// Converts a from a Ruby object
  virtual ArgumentMarshaller * fromRuby(mrb_value value) const = 0;

  /// Converts back to a list of strings.  The idea is to cram as many
  /// arguments into a single string, but sometimes it is not
  /// possible, in which case the function returns several strings.
  virtual QStringList toString(const ArgumentMarshaller * arg) const = 0;

  /// @}

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

  // This function is not used...
  // /// If this function returns an non-empty list, then it corresponds
  // /// to all the possible choices (ie no free-form). In particular,
  // /// prompting will be done using a non-editable QComboBox, and no
  // /// prompting dialog will show up.
  // ///
  // /// @todo Maybe have a way to provide a "real" string and a
  // /// "display" string ?
  // virtual QStringList allChoices() const;


  /// Handling of automatic completion. Provided with a beginning of
  /// the string, propose a QStringList with potential candidates.
  virtual QStringList proposeCompletion(const QString & starter) const;

  /// Appends the value of \p b to \p a. Must be reimplemented by
  /// arguments that can be greedy.
  virtual void concatenateArguments(ArgumentMarshaller * a, 
                                    const ArgumentMarshaller * b) const;


  
  /// @name Custom widgets for edition of arguments
  ///
  /// @{

  /// Create a widget for the edition of the parameter.
  ///
  /// The default implementation returns a grayed-out label.
  virtual QWidget * createEditor(QWidget * parent = NULL) const = 0;

  /// Sets the value of the widget from an ArgumentMarshaller stuff.
  ///
  /// The QWidget passed will be one that has been previously created
  /// using createEditor();
  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const = 0;

  /// Returns the current value of the given editor.
  ///
  /// It never returns NULL but fails with an exception
  ///
  /// This implementation does the right thing (TM) if the widget is a
  /// line edit.
  virtual ArgumentMarshaller * getEditorValue(QWidget * editor) const;

  /// @}

protected:

  /// A helper function to handle the case when using a text editor is
  /// satisfactory.
  QWidget * createTextEditor(QWidget * parent = NULL) const;

  /// Sets the value of the target text editor from the given value.
  ///
  /// Internally uses toString();
  void setTextEditorValue(QWidget * editor,
                          const ArgumentMarshaller * value) const;
};

#endif
