/**
   \file help-commands.cc get help!
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
#include <command.hh>
#include <commandcontext.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>
#include <terminal.hh>

#include <file.hh>

#include <soas.hh>
#include <exceptions.hh>
#include <gslfunction.hh>
#include <statistics.hh>

#include <commandlineparser.hh>
#include <helpbrowser.hh>

static Group help("help", 1000,
                  "Help",
                  "Help");

//////////////////////////////////////////////////////////////////////


static void commandsCommand(const QString &)
{
  QStringList cmds = soas().commandContext().allCommands();
  cmds.sort();
  int len = 0;
  for(int i = 0; i < cmds.size(); i++)
    if(cmds[i].size() > len)
      len = cmds[i].size();

  QHash<Command *, int> t;
  Terminal::out << "All commands:" << endl;
  for(int i = 0; i < cmds.size(); i++) {
    Command * cmd = soas().commandContext().namedCommand(cmds[i]);
    Terminal::out << QString("%1 %2\n").
      arg(cmds[i], -len).arg(cmd->shortDescription());
    t[cmd] = 1;
  }
  Terminal::out << "\nIn total " << t.size() << " commands "
                << "(and " << cmds.size() - t.size() << " aliases)"
                << endl;
}

static Command 
cmds("commands", // command name
     optionLessEffector(commandsCommand), // action
     "help",  // group name
     NULL, // arguments
     NULL, // options
     "Commands",
     "List commands");

static Command 
cmd2("commands", // command name
     optionLessEffector(commandsCommand), // action
     "help",  // group name
     NULL, // arguments
     NULL, // options
     "Commands",
     "List commands", "", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////


QString docUrl("http://www.qsoas.org/manual.html");


static void helpCommand(const QString & /*name*/,
                        const CommandOptions & opts)
{
  Command * cmd = NULL;
  updateFromOptions(opts, "command", cmd);

  if(! cmd) {
    HelpBrowser::browseLocation("doc/tutorial.html");
    return;
  }
  bool showSyn = false;
  updateFromOptions(opts, "synopsis", showSyn);

  if(! showSyn) {
    HelpBrowser::browseCommand(cmd);
    return;
  }
  QStringList synopsis;
  QString descs;

  if(cmd->commandArguments()) {
    const ArgumentList & args = *cmd->commandArguments();
    for(int i = 0; i < args.size(); i++) {
      QString a = args[i]->argumentName();
      if(args[i]->greedy)
        a += "...";
      synopsis << a;
      descs += QString("  * %1: %2\n").
        arg(args[i]->argumentName()).
        arg(args[i]->description());
    }
  }

  if(cmd->commandOptions()) {
    const ArgumentList & args = *cmd->commandOptions();
    
    QStringList names = args.argumentNames();
    std::sort(names.begin(), names.end());
    
    for(const QString & a : names) {
      const Argument * arg = args.namedArgument(a);
      synopsis << "/" + a + "=" ;
      descs += QString("  * /%1%3: %2\n").
        arg(arg->argumentName()).
        arg(arg->description()).
        arg(arg->defaultOption ? " (default)" : "");
    }
  }

  Terminal::out << "Command: " << cmd->commandName() << " -- "
                << cmd->publicName() << "\n\n"
                << "  " << cmd->commandName() << " " 
                << synopsis.join(" ") << "\n" 
                << descs << "\n" << endl;
}

static ArgumentList 
helpO(QList<Argument *>()
      << new CommandArgument("command", "Command",
                             "The command on which to give help", true)
      << new BoolArgument("synopsis", "Synopsis",
                          "Does not show the help, but print a brief synopsis"));


static Command 
hlpc("help", // command name
     effector(helpCommand), // action
     "help",  // group name
     NULL, // arguments
     &helpO, // options
     "Help on...",
     "Give help on command",
     "?");

static Command 
hlp2("help", // command name
     effector(helpCommand), // action
     "help",  // group name
     NULL, // arguments
     &helpO, // options
     "Help on...",
     "Give help on command",
     "?", CommandContext::fitContext());

//////////////////////////////////////////////////////////////////////



void updateDocumentationFile(const QString &, QString file)
{
  QString str;
  try {
    str = File::readFile(file);
    QFile::remove(file + ".old");
    QFile::rename(file, file + ".old");
  }
  catch (RuntimeError &re) {
    // Nothing to do !
  }


  // Now perform updates

  
  {                             // Commands first
    QSet<Command * > cmds = CommandContext::allAvailableCommands();
    QTextStream o(stdout);

    for(Command * cmd : cmds) {
      if(! cmd->isCustom())
        cmd->updateDocumentation(str);
      else
        o << "Skipping custom command: " << cmd->commandName() << endl;
    }
  }

  {                             // Then functions
    Utils::updateWithin(str, "{::comment} special-functions-start {:/}",
                        "{::comment} special-functions-end {:/}\n",
                        "\n\n" + GSLFunction::functionDocumentation() + "\n");

    Utils::updateWithin(str, "{::comment} constants-start {:/}",
                        "{::comment} constants-end {:/}\n",
                        "\n\n" + GSLConstant::constantsDocumentation() + "\n");

    QStringList nonInt = CommandContext::globalContext()->
      nonInteractiveCommands();
    qSort(nonInt);
    Utils::makeUnique(nonInt);
    for(int i = 0; i < nonInt.size(); i++) {
      // Command * cmd = Command::namedCommand(nonInt[i]);
      // if(cmd->isCustom()) {
      //   nonInt.takeAt(i);
      //   i--;
      // }
      // else
        nonInt[i] = QString(" * [`%1`](#cmd-%2)").arg(nonInt[i]).
          arg(nonInt[i]);
    }

    Utils::updateWithin(str, "{::comment} non-interactive-start {:/}",
                        "{::comment} non-interacive-end {:/}\n",
                        "\n\n" + nonInt.join("\n") + "\n");

    // Statistics
    Utils::updateWithin(str, "{::comment} statistics-start {:/}",
                        "{::comment} statistics-end {:/}\n",
                        "\n\n" + StatisticsValue::docString() + "\n");

    // // Now dealing with non-interactive fit commands ?
    // QStringList nonInt = CommandContext::globalContext()->
    //   nonInteractiveCommands();
    // qSort(nonInt);
    // Utils::makeUnique(nonInt);
  }


  File o(file, File::TextOverwrite);
  o.ioDevice()->write(str.toLocal8Bit());
  
}

static ArgumentList 
udfA(QList<Argument *>() 
     << new FileArgument("file", "File",
                         "The file to update"));

static Command 
udCmd("update-documentation", // command name
     optionLessEffector(updateDocumentationFile), // action
     "help",  // group name
     &udfA, // arguments
     NULL, // options
     "Update documentation",
     "Update documentation file");

//////////////////////////////////////////////////////////////////////

static CommandLineOption hlp("--update-documentation", [](const QStringList & args) {
    updateDocumentationFile("ud", args[0]);
    ::exit(0);
  }, 1, "updates the given documentation file");

