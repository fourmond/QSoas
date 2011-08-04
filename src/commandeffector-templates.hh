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

// template<class C, typename A1> void 
// CommandEffector::runFunction(void (C::*f)(const QString &, A1), 
//                              const QString & name, 
//                              const QList<ArgumentMarshaller *> & arguments) {
//   if(arguments.size() != 1) {
//     QString str;
//     str = QObject::tr("Expected 1 argument, but got %1").
//       arg(arguments.size());
//   }
//   A1 a1 = arguments[0]->value<A1>();
//   CALL_MEMBER_FN(*dynamic_cast<C>(this), f)(a1);
// };

/// Argumentless and optionless callback to static function
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


/// Effector for an argumentless and optionless command
inline CommandEffector * optionLessEffector(void (*f)(const QString &)) {
  return new CommandEffectorCallback0OptionLess(f);
};

/// Optionless callback to a static function with one argument
///
/// Rather than using this class directly, use
/// CommandEffector::functionEffector().
template <class A1>
class CommandEffectorCallback1OptionLess : public CommandEffector {

  typedef void (*Callback)(const QString &, A1);
  Callback callback;

public:

  CommandEffectorCallback1OptionLess(Callback c) : callback(c) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions &) {
    if(args.size() != 1) {
      QString str = QString("1 argument expected, but got %2").
        arg(args.size());
      throw std::logic_error(str.toStdString());
    }
    A1 a1 = args[0]->value<A1>();
    callback(commandName, a1);
  };

};



/// Effector for an optionless command that takes one argument.
template<class A1> CommandEffector * optionLessEffector(void (*f)(const QString &, A1)) {
  return new CommandEffectorCallback1OptionLess<A1>(f);
};

/// Optionless callback to a static function with two arguments
///
/// Rather than using this class directly, use
/// CommandEffector::functionEffector().
template <class A1, class A2>
class CommandEffectorCallback2OptionLess : public CommandEffector {

  typedef void (*Callback)(const QString &, A1, A2);
  Callback callback;

public:

  CommandEffectorCallback2OptionLess(Callback c) : callback(c) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions &) {
    if(args.size() != 2) {
      QString str = QString("2 arguments expected, but got %2").
        arg(args.size());
      throw std::logic_error(str.toStdString());
    }
    A1 a1 = args[0]->value<A1>();
    A2 a2 = args[1]->value<A2>();
    callback(commandName, a1, a2);
  };

};

/// Effector for an optionless command that takes two arguments.
template<class A1, class A2> CommandEffector * optionLessEffector(void (*f)(const QString &, A1, A2)) {
  return new CommandEffectorCallback2OptionLess<A1, A2>(f);
};


/// Callback to a static function with only options.
///
/// Rather than using this class directly, use
/// CommandEffector::functionEffector().
class CommandEffectorCallback0 : public CommandEffector {

  typedef void (*Callback)(const QString &, const CommandOptions &);
  Callback callback;

public:

  CommandEffectorCallback0(Callback c) : callback(c) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions & options) {
    if(args.size() > 0) {
      QString str = QString("0 argument expected, but got %2").
        arg(args.size());
      throw std::logic_error(str.toStdString());
    }
    callback(commandName, options);
  };

};

inline CommandEffector * 
effector(void (*f)(const QString &, 
                   const CommandOptions &)) {
  return new CommandEffectorCallback0(f);
};

/// Callback to a static function with one argument + options.
///
/// Rather than using this class directly, use
/// CommandEffector::functionEffector().
template <class A1>
class CommandEffectorCallback1 : public CommandEffector {

  typedef void (*Callback)(const QString &, A1, const CommandOptions &);
  Callback callback;

public:

  CommandEffectorCallback1(Callback c) : callback(c) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions & options) {
    if(args.size() != 1) {
      QString str = QString("1 argument expected, but got %2").
        arg(args.size());
      throw std::logic_error(str.toStdString());
    }
    A1 a1 = args[0]->value<A1>();
    callback(commandName, a1, options);
  };

};

template<class A1> CommandEffector * 
effector(void (*f)(const QString &, A1, 
                   const CommandOptions &)) {
  return new CommandEffectorCallback1<A1>(f);
};

#endif
