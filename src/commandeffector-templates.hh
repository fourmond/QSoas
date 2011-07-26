/**
   \file commandeffector-templates.hh
   Various templates to be used with CommandEffector
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

#ifndef __COMMANDEFFECTOR_TEMPLATES_HH
#define __COMMANDEFFECTOR_TEMPLATES_HH

#include <commandeffector.hh>
#include <argumentmarshaller.hh>
#include <utils.hh>

template<class C, typename A1> void 
CommandEffector::runFunction(void (C::*f)(const QString &, A1), 
                             const QString & name, 
                             const QList<ArgumentMarshaller *> & arguments) {
  if(arguments.size() != 1) {
    QString str;
    str = QObject::tr("Expected 1 argument, but got %1").
      arg(arguments.size());
  }
  A1 a1 = arguments[0]->value<A1>();
  CALL_MEMBER_FN(*dynamic_cast<C>(this), f)(a1);
};

/// Argumentless callback to static function
///
/// Rather than using this class directly, use
/// CommandEffector::functionEffector().
class CommandEffectorCallback0OptionLess : public CommandEffector {

  typedef void (*Callback)(const QString &);
  Callback callback;

public:

  CommandEffectorCallback0OptionLess(Callback c) : callback(c) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments &,
                                 const CommandOptions &) {
    callback(commandName);
  };

};

inline CommandEffector * 
CommandEffector::functionEffectorOptionLess(void (*f)(const QString &)) {
  return new CommandEffectorCallback0OptionLess(f);
};

#endif
