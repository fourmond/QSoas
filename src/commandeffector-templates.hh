/**
   \file commandeffector-templates.hh
   Various templates to be used with CommandEffector
   Copyright 2011 by Vincent Fourmond
             2012, 2013 by CNRS/AMU

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
#ifndef __COMMANDEFFECTOR_TEMPLATES_HH
#define __COMMANDEFFECTOR_TEMPLATES_HH

#include <commandeffector.hh>
#include <argumentmarshaller.hh>
#include <exceptions.hh>
#include <utils.hh>



/// Argumentless and optionless callback to static function
///
/// Rather than using this class directly, use
/// CommandEffector::functionEffector().
class CommandEffectorCallback0OptionLess : public CommandEffector {

  typedef void (*Callback)(const QString &);
  Callback callback;

public:

  CommandEffectorCallback0OptionLess(Callback c, bool i = false) : 
    CommandEffector(i), callback(c) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments &,
                                 const CommandOptions &) {
    callback(commandName);
  };

};


/// Effector for an argumentless and optionless command
inline CommandEffector * optionLessEffector(void (*f)(const QString &), 
                                            bool i = false) {
  return new CommandEffectorCallback0OptionLess(f, i);
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

  CommandEffectorCallback1OptionLess(Callback c, bool i = false) : 
    CommandEffector(i), callback(c) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions &) {
    if(args.size() != 1)
      throw InternalError(QString("1 argument expected, but got %2").
                          arg(args.size()));
    A1 a1 = args[0]->value<A1>();
    callback(commandName, a1);
  };

};



/// Effector for an optionless command that takes one argument.
template<class A1> CommandEffector * optionLessEffector(void (*f)(const QString &, A1), bool i = false) {
  return new CommandEffectorCallback1OptionLess<A1>(f, i);
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

  CommandEffectorCallback2OptionLess(Callback c, bool i = false) : 
    CommandEffector(i), callback(c) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions &) {
    if(args.size() != 2)
      throw InternalError(QString("2 arguments expected, but got %2").
                          arg(args.size()));
    A1 a1 = args[0]->value<A1>();
    A2 a2 = args[1]->value<A2>();
    callback(commandName, a1, a2);
  };

};

/// Effector for an optionless command that takes two arguments.
template<class A1, class A2> CommandEffector * optionLessEffector(void (*f)(const QString &, A1, A2), bool i = false) {
  return new CommandEffectorCallback2OptionLess<A1, A2>(f, i);
};


/// Callback to a static function with only options.
///
/// Rather than using this class directly, use
/// CommandEffector::functionEffector().
class CommandEffectorCallback0 : public CommandEffector {

  typedef void (*Callback)(const QString &, const CommandOptions &);
  Callback callback;

public:

  CommandEffectorCallback0(Callback c, bool i = false) : 
    CommandEffector(i), callback(c) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions & options) {
    if(args.size() > 0)
      throw InternalError(QString("0 arguments expected, but got %2").
                          arg(args.size()));
    callback(commandName, options);
  };

};

inline CommandEffector * 
effector(void (*f)(const QString &, 
                   const CommandOptions &), bool i = false) {
  return new CommandEffectorCallback0(f, i);
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

  CommandEffectorCallback1(Callback c, bool i = false) : 
    CommandEffector(i), callback(c) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions & options) {
    if(args.size() != 1)
      throw InternalError(QString("1 argument expected, but got %2").
                          arg(args.size()));
    A1 a1 = args[0]->value<A1>();
    callback(commandName, a1, options);
  };

};

template<class A1> CommandEffector * 
effector(void (*f)(const QString &, A1, 
                   const CommandOptions &), bool i = false) {
  return new CommandEffectorCallback1<A1>(f, i);
};

/// Callback to a static function with two arguments + options.
///
/// Rather than using this class directly, use
/// effector().
template <class A1, class A2>
class CommandEffectorCallback2 : public CommandEffector {

  typedef void (*Callback)(const QString &, A1, A2, const CommandOptions &);
  Callback callback;

public:

  CommandEffectorCallback2(Callback c, bool i = false) : 
    CommandEffector(i), callback(c) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions & options) {
    if(args.size() != 2)
      throw InternalError(QString("2 arguments expected, but got %2").
                          arg(args.size()));
    A1 a1 = args[0]->value<A1>();
    A2 a2 = args[1]->value<A2>();
    callback(commandName, a1, a2, options);
  };

};

template<class A1, class A2> CommandEffector * 
effector(void (*f)(const QString &, A1, A2,
                   const CommandOptions &), bool i = false) {
  return new CommandEffectorCallback2<A1, A2>(f, i);
};


/// Callback to a static function with three argument + options.
///
/// Rather than using this class directly, use
/// effector().
template <class A1, class A2, class A3>
class CommandEffectorCallback3 : public CommandEffector {

  typedef void (*Callback)(const QString &, A1, A2, A3, const CommandOptions &);
  Callback callback;

public:

  CommandEffectorCallback3(Callback c, bool i = false) : 
    CommandEffector(i), callback(c) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions & options) {
    if(args.size() != 3)
      throw InternalError(QString("3 arguments expected, but got %2").
                          arg(args.size()));
    A1 a1 = args[0]->value<A1>();
    A2 a2 = args[1]->value<A2>();
    A3 a3 = args[2]->value<A3>();
    callback(commandName, a1, a2, a3, options);
  };

};

template<class A1, class A2, class A3> CommandEffector * 
effector(void (*f)(const QString &, A1, A2, A3,
                   const CommandOptions &), bool i = false) {
  return new CommandEffectorCallback3<A1, A2, A3>(f, i);
};

/// Callback to a static function with four argument + options.
///
/// Rather than using this class directly, use
/// effector().
template <class A1, class A2, class A3, class A4>
class CommandEffectorCallback4 : public CommandEffector {

  typedef void (*Callback)(const QString &, A1, A2, A3, A4, 
                           const CommandOptions &);
  Callback callback;

public:

  CommandEffectorCallback4(Callback c, bool i = false) : 
    CommandEffector(i), callback(c) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions & options) {
    if(args.size() != 4)
      throw InternalError(QString("4 arguments expected, but got %2").
                          arg(args.size()));
    A1 a1 = args[0]->value<A1>();
    A2 a2 = args[1]->value<A2>();
    A3 a3 = args[2]->value<A3>();
    A4 a4 = args[3]->value<A4>();
    callback(commandName, a1, a2, a3, a4, options);
  };

};

template<class A1, class A2, class A3, class A4> CommandEffector * 
effector(void (*f)(const QString &, A1, A2, A3, A4,
                   const CommandOptions &), bool i = false) {
  return new CommandEffectorCallback4<A1, A2, A3, A4>(f, i);
};


//////////////////////////////////////////////////////////////////////
// Now, callback to members

/// Argumentless and optionless callback to member function
///
/// Rather than using this class directly, use
/// CommandEffector::functionEffector().
template<class C>
class CommandEffectorMemberCallback0OptionLess : public CommandEffector {

  typedef void (C::*Callback)(const QString &);
  Callback callback;
  C * target;

public:

  CommandEffectorMemberCallback0OptionLess(C * t, Callback c, 
                                           bool i = false) : 
    CommandEffector(i), callback(c), 
    target(t) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments &,
                                 const CommandOptions &) {
    CALL_MEMBER_FN(*target, callback)(commandName);
  };
  
};


/// Effector for an argumentless and optionless command
template<class C> CommandEffector * optionLessEffector(C * cls, void (C::*f)(const QString &), bool i = false) {
  return new CommandEffectorMemberCallback0OptionLess<C>(cls, f, i);
};

/// Argumentless callback to member function with options
///
/// Rather than using this class directly, use
/// CommandEffector::functionEffector().
template<class C>
class CommandEffectorMemberCallback0 : public CommandEffector {

  typedef void (C::*Callback)(const QString &, 
                              const CommandOptions & opts);
  Callback callback;
  C * target;

public:

  CommandEffectorMemberCallback0(C * t,
                                 Callback c, bool i = false) : 
    CommandEffector(i), callback(c), 
    target(t) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments &,
                                 const CommandOptions & opts) {
    CALL_MEMBER_FN(*target, callback)(commandName, opts);
  };
  
};


/// Effector for an argumentless and optionless command
template<class C> CommandEffector * effector(C * cls, void (C::*f)(const QString &, const CommandOptions &), bool i = false) {
  return new CommandEffectorMemberCallback0<C>(cls, f, i);
};


/// Optionless callback to member function with one argument
///
/// Rather than using this class directly, use
/// CommandEffector::functionEffector().
template<class C, class A1>
class CommandEffectorMemberCallback1OptionLess : public CommandEffector {

  typedef void (C::*Callback)(const QString &, A1);
  Callback callback;
  C * target;

public:

  CommandEffectorMemberCallback1OptionLess(C * t,
                                           Callback c, bool i = false) : 
    CommandEffector(i), callback(c), 
    target(t) {;};
  
  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions &) {
    A1 a1 = args[0]->value<A1>();
    CALL_MEMBER_FN(*target, callback)(commandName, a1);
  };
  
};


/// Effector for an optionless command taking one argument
template<class C, class A1> CommandEffector * optionLessEffector(C * cls, void (C::*f)(const QString &, A1), bool i = false) {
  return new CommandEffectorMemberCallback1OptionLess<C, A1>(cls, f, i);
};


/// Callback to member function with options and 1 argument
///
/// Rather than using this class directly, use
/// CommandEffector::functionEffector().
template<class C, class A1>
class CommandEffectorMemberCallback1 : public CommandEffector {

  typedef void (C::*Callback)(const QString &, A1 a1,
                              const CommandOptions & opts);
  Callback callback;
  C * target;

public:

  CommandEffectorMemberCallback1(C * t,
                                 Callback c, bool i = false) : 
    CommandEffector(i), callback(c), 
    target(t) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions & opts) {
    A1 a1 = args[0]->value<A1>();
    CALL_MEMBER_FN(*target, callback)(commandName, a1, opts);
  };
  
};


/// Effector for an argumentless and optionless command
template<class C, class A1> CommandEffector * effector(C * cls, void (C::*f)(const QString &, A1, const CommandOptions &), bool i = false) {
  return new CommandEffectorMemberCallback1<C, A1>(cls, f, i);
};




/// Optionless callback to member function with two arguments
template<class C, class A1, class A2>
class CommandEffectorMemberCallback2OptionLess : public CommandEffector {

  typedef void (C::*Callback)(const QString &, A1, A2);
  Callback callback;
  C * target;

public:

  CommandEffectorMemberCallback2OptionLess(C * t,
                                           Callback c, bool i = false) : 
    CommandEffector(i), callback(c), 
    target(t) {;};
  
  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions &) {
    A1 a1 = args[0]->value<A1>();
    A2 a2 = args[1]->value<A2>();
    CALL_MEMBER_FN(*target, callback)(commandName, a1, a2);
  };
  
};


/// Effector for an argumentless and optionless command
template<class C, class A1, class A2> CommandEffector * optionLessEffector(C * cls, void (C::*f)(const QString &, A1, A2), bool i = false) {
  return new CommandEffectorMemberCallback2OptionLess<C, A1, A2>(cls, f, i);
};


//////////////////////////////////////////////////////////////////////


/// Callback to member function with options and two arguments
///
/// Rather than using this class directly, use
/// CommandEffector::functionEffector().
template<class C, class A1, class A2>
class CommandEffectorMemberCallback2 : public CommandEffector {

  typedef void (C::*Callback)(const QString &, A1 a1, A2 a2,
                              const CommandOptions & opts);
  Callback callback;
  C * target;

public:

  CommandEffectorMemberCallback2(C * t,
                                 Callback c, bool i = false) : 
    CommandEffector(i), callback(c), 
    target(t) {;};

  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions & opts) {
    A1 a1 = args[0]->value<A1>();
    A2 a2 = args[1]->value<A2>();
    CALL_MEMBER_FN(*target, callback)(commandName, a1, a2, opts);
  };
  
};


/// Effector for an argumentless and optionless command
template<class C, class A1, class A2> CommandEffector * effector(C * cls, void (C::*f)(const QString &, A1, A2, const CommandOptions &), bool i = false) {
  return new CommandEffectorMemberCallback2<C, A1, A2>(cls, f, i);
};

//////////////////////////////////////////////////////////////////////

/// Optionless callback to member function with three arguments
template<class C, class A1, class A2, class A3>
class CommandEffectorMemberCallback3OptionLess : public CommandEffector {

  typedef void (C::*Callback)(const QString &, A1, A2, A3);
  Callback callback;
  C * target;

public:

  CommandEffectorMemberCallback3OptionLess(C * t,
                                           Callback c, bool i = false) : 
    CommandEffector(i), callback(c), 
    target(t) {;};
  
  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions &) {
    A1 a1 = args[0]->value<A1>();
    A2 a2 = args[1]->value<A2>();
    A3 a3 = args[2]->value<A3>();
    CALL_MEMBER_FN(*target, callback)(commandName, a1, a2, a3);
  };
  
};


/// Effector for an argumentless and optionless command
template<class C, class A1, class A2, class A3> CommandEffector * optionLessEffector(C * cls, void (C::*f)(const QString &, A1, A2, A3), bool i = false) {
  return new CommandEffectorMemberCallback3OptionLess<C, A1, A2, A3>(cls, f, i);
};

//////////////////////////////////////////////////////////////////////

/// Callback to member function with three arguments
template<class C, class A1, class A2, class A3>
class CommandEffectorMemberCallback3 : public CommandEffector {

  typedef void (C::*Callback)(const QString &, A1, A2, A3, const CommandOptions &);
  Callback callback;
  C * target;

public:

  CommandEffectorMemberCallback3(C * t,
                                 Callback c, bool i = false) : 
    CommandEffector(i), callback(c), 
    target(t) {;};
  
  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions & opts) {
    A1 a1 = args[0]->value<A1>();
    A2 a2 = args[1]->value<A2>();
    A3 a3 = args[2]->value<A3>();
    CALL_MEMBER_FN(*target, callback)(commandName, a1, a2, a3, opts);
  };
  
};


/// Effector for an argumentless and optionless command
template<class C, class A1, class A2, class A3> CommandEffector * effector(C * cls, void (C::*f)(const QString &, A1, A2, A3, const CommandOptions &), bool i = false) {
  return new CommandEffectorMemberCallback3<C, A1, A2, A3>(cls, f, i);
};

//////////////////////////////////////////////////////////////////////

/// Optionless callback to member function with four arguments
template<class C, class A1, class A2, class A3, class A4>
class CommandEffectorMemberCallback4OptionLess : public CommandEffector {

  typedef void (C::*Callback)(const QString &, A1, A2, A3, A4);
  Callback callback;
  C * target;

public:

  CommandEffectorMemberCallback4OptionLess(C * t,
                                           Callback c, bool i = false) : 
    CommandEffector(i), callback(c), 
    target(t) {;};
  
  inline virtual void runCommand(const QString & commandName, 
                                 const CommandArguments & args,
                                 const CommandOptions &) {
    A1 a1 = args[0]->value<A1>();
    A2 a2 = args[1]->value<A2>();
    A3 a3 = args[2]->value<A3>();
    A4 a4 = args[3]->value<A4>();
    CALL_MEMBER_FN(*target, callback)(commandName, a1, a2, a3, a4);
  };
  
};


/// Effector for an argumentless and optionless command
template<class C, class A1, class A2, class A3, class A4> CommandEffector * optionLessEffector(C * cls, void (C::*f)(const QString &, A1, A2, A3, A4), bool i = false) {
  return new CommandEffectorMemberCallback4OptionLess<C, A1, A2, A3, A4>(cls, f, i);
};

/// @todo Maybe we need a generic effector wrapping a call to
/// something getting directly CommandArguments.


//////////////////////////////////////////////////////////////////////

/// Argumentless and optionless callback to static function
///
/// Rather than using this class directly, use
/// CommandEffector::functionEffector().
class InteractiveCommandEffectorCallback0OptionLess : public CommandEffector {

  typedef void (*Callback)(CurveEventLoop &, const QString &);
  Callback callback;

public:

  InteractiveCommandEffectorCallback0OptionLess(Callback c) : 
    CommandEffector(true), callback(c) {;};

  inline virtual void runCommand(const QString & /*commandName*/, 
                                 const CommandArguments &,
                                 const CommandOptions &) {
    throw InternalError("Trying to run an interactive command without a loop");
  };

  inline virtual bool needsLoop() const {
    return true;
  };

  inline virtual void runWithLoop(CurveEventLoop & loop,
                                  const QString & commandName, 
                                  const CommandArguments & /*arguments*/,
                                  const CommandOptions & /*options*/) {
    callback(loop, commandName);
  };
};


/// Effector for an argumentless and optionless command
inline CommandEffector * optionLessEffector(void (*f)(CurveEventLoop &,
                                                      const QString &)) {
  return new InteractiveCommandEffectorCallback0OptionLess(f);
};

//////////////////////////////////////////////////////////////////////

/// Argumentless and optionless callback to static function
///
/// Rather than using this class directly, use
/// CommandEffector::functionEffector().
class InteractiveCommandEffectorCallback0 : public CommandEffector {

  typedef void (*Callback)(CurveEventLoop &, const QString &, 
                           const CommandOptions &);
  Callback callback;

public:

  InteractiveCommandEffectorCallback0(Callback c) : 
    CommandEffector(true), callback(c) {;};

  inline virtual void runCommand(const QString & /*commandName*/, 
                                 const CommandArguments &,
                                 const CommandOptions &) {
    throw InternalError("Trying to run an interactive command without a loop");
  };

  inline virtual bool needsLoop() const {
    return true;
  };

  inline virtual void runWithLoop(CurveEventLoop & loop,
                                  const QString & commandName, 
                                  const CommandArguments & /*arguments*/,
                                  const CommandOptions & options) {
    callback(loop, commandName, options);
  };
};


/// Effector for an argumentless and optionless command
inline CommandEffector * effector(void (*f)(CurveEventLoop &,
                                            const QString &,
                                            const CommandOptions&)) {
  return new InteractiveCommandEffectorCallback0(f);
};


#endif
