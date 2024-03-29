/**
   \file alias.cc aliases
   Copyright 2013 by CNRS/AMU

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
#include <command.hh>
#include <commandcontext.hh>
#include <soas.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>

#include <terminal.hh>

#include <possessive-containers.hh>


/// This class wraps another command along with a few options, and
/// possibly some time later arguments too.
class Alias : public CommandEffector {

protected:

  CommandOptions spliceOptions(const CommandOptions & args,
                               const CommandOptions & options)
  {
    CommandOptions args2 = args;
    for(CommandOptions::const_iterator i 
          = options.begin(); i != options.end(); ++i) {
      if(args2.contains(i.key())) {
          const Argument * arg = 
            targetCommand->commandOptions()->namedArgument(i.key());
          if(! arg)
            throw InternalError("Somehow still got an unparsed "
                                "argument after parsing !");
          if(arg->greedy)
            arg->concatenateArguments(args2[i.key()], i.value());
          else
            args2[i.key()] = i.value();
      }
      else
        args2[i.key()] = i.value();
    }
    return args2;
  };

public:

  /// The wrapped command
  Command * targetCommand;

  /// The default options. They are not parsed yet.
  QMultiHash<QString, QString> defaultOptions;

  virtual bool needsLoop() const override {
    return targetCommand->effector->needsLoop();
  };

  /// Arguments to come later ?


  explicit Alias(Command * tg) : CommandEffector(tg->effector->isInteractive()),
                                 targetCommand(tg) {
  };

  virtual void runCommand(const QString & commandName, 
                          const CommandArguments & arguments,
                          const CommandOptions & options) override {

    PossessiveHash<QString, ArgumentMarshaller> args(targetCommand->parseOptions(defaultOptions));
    CommandOptions args2 = spliceOptions(args, options);

    targetCommand->effector->runCommand(commandName, arguments, args2);
  };

  virtual void runWithLoop(CurveEventLoop & loop,
                           const QString & commandName, 
                           const CommandArguments & arguments,
                           const CommandOptions & options) override 
  {
    PossessiveHash<QString, ArgumentMarshaller> args(targetCommand->parseOptions(defaultOptions));
    CommandOptions args2 = spliceOptions(args, options);

    targetCommand->effector->runWithLoop(loop,commandName, arguments, args2);
  }

  
};


static QHash<QString, Alias *> definedAliases;

//////////////////////////////////////////////////////////////////////

static void defineAliasCommand(const QString &, QString alias,
                               Command * cmd, 
                               const CommandOptions & opts)
{
  // 
  // if(definedAliases.contains(alias))
  //   throw RuntimeError("An alias named %1 already exists !").arg(alias);
  if(soas().commandContext().namedCommand(alias))
    throw RuntimeError("A command named '%1' already exists !").
      arg(alias);

  // First convert the options hash to a QMultiHash:
  QMultiHash<QString, QString> options;

  /// @todo Don't forget to remove alias-specific options first !
  for(CommandOptions::const_iterator i = opts.begin();
      i != opts.end(); ++i)
    options.replace(i.key(), i.value()->value<QString>());


  Alias * a = new Alias(cmd);
  definedAliases[alias] = a;
  a->defaultOptions = options;

  QString sh = QString("Alias for %1").arg(cmd->commandName());

  QString groupName = (cmd->group ? cmd->group->groupName() : "");
  new Command(alias, a, 
              groupName,
              const_cast<ArgumentList*>(cmd->commandArguments()),
              const_cast<ArgumentList*>(cmd->commandOptions()),
              sh, sh, NULL,
              &soas().commandContext());
}

static ArgumentList 
daA(QList<Argument *>() 
      << new StringArgument("alias", "Alias",
                             "The name to give to the new alias")
      << new CommandArgument("command", "Command",
                             "The command to give an alias for"));

static ArgumentList 
daO(QList<Argument *>() 
      << new StringArgument("*", "Options",
                            "All options"));


static Command 
hlpc("define-alias", // command name
     effector(defineAliasCommand), // action
     "file",  // group name
     &daA, // arguments
     &daO, // options
     "Define alias",
     "Make an alias for another command");


//////////////////////////////////////////////////////////////////////

static void displayAliasesCommand(const QString &)
{
  QStringList aliases = definedAliases.keys();
  std::sort(aliases.begin(), aliases.end());

  Terminal::out << "Defined aliases:" << endl;
  for(int i = 0; i < aliases.size(); i++) {
    const QString & name = aliases[i];
    Alias * alias = definedAliases[name];
    QString optString;
    for(QMultiHash<QString, QString>::const_iterator i = 
          alias->defaultOptions.begin(); 
        i != alias->defaultOptions.end(); ++i)
      optString += QString("/%1=%2 ").arg(i.key()).arg(i.value());
    Terminal::out << " * " << name << ": " 
                  << alias->targetCommand->commandName()
                  << " " << optString << endl;
  }
}

static Command 
display("display-aliases", // command name
     optionLessEffector(displayAliasesCommand), // action
     "file",  // group name
     NULL, // arguments
     NULL, // options
     "Display aliases",
     "Displays all defined aliases");
