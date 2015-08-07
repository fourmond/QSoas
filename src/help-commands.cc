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
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>
#include <terminal.hh>

#include <soas.hh>
#include <exceptions.hh>
#include <gslfunction.hh>

#include <commandlineparser.hh>

static Group help("help", 1000,
                  "Help",
                  "Help");

//////////////////////////////////////////////////////////////////////


static void commandsCommand(const QString &)
{
  QStringList cmds = Command::allCommands();
  cmds.sort();
  int len = 0;
  for(int i = 0; i < cmds.size(); i++)
    if(cmds[i].size() > len)
      len = cmds[i].size();

  QHash<Command *, int> t;
  Terminal::out << "All commands:" << endl;
  for(int i = 0; i < cmds.size(); i++) {
    Command * cmd = Command::namedCommand(cmds[i]);
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
     "List commands",
     "List all available commands, along with a little help");

//////////////////////////////////////////////////////////////////////


static void texCommands(const QString &)
{
  QTextStream o(stdout);
  o << Group::latexDocumentationAllGroups() << endl;
}

static Command 
tcmd("tex-commands", // command name
     optionLessEffector(texCommands), // action
     "help",  // group name
     NULL, // arguments
     NULL, // options
     "TeX",
     "List commands",
     "List all available commands, along with a little help");

//////////////////////////////////////////////////////////////////////


QString docUrl("http://www.qsoas.org/manual.html");


static void helpCommand(const QString &, Command * cmd, 
                        const CommandOptions & opts)
{
  QStringList synopsis;
  QString descs;

  
  bool online = true; /// @todo Have that customizable ?

  updateFromOptions(opts, "online", online);
  if(online) {
    QUrl url = docUrl;
    url.setFragment("cmd-" + cmd->commandName());
    QDesktopServices::openUrl(url);
    return;
  }
  /// @todo the documentation-building facilities should join Command
  /// rather than being here.
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
    for(int i = 0; i < args.size(); i++) {
      QString a = args[i]->argumentName();
      synopsis << "/" + a + "=" ;
      descs += QString("  * /%1%3: %2\n").
        arg(args[i]->argumentName()).
        arg(args[i]->description()).
        arg(args[i]->defaultOption ? " (default)" : "");
    }
  }

  Terminal::out << "Command: " << cmd->commandName() << " -- "
                << cmd->publicName() << "\n\n"
                << "  " << cmd->commandName() << " " 
                << synopsis.join(" ") << "\n" 
                << descs << "\n"
                << cmd->longDescription() << endl;
}

static ArgumentList 
helpA(QList<Argument *>() 
      << new CommandArgument("command", "Command",
                             "The command on which to give help"));

static ArgumentList 
helpO(QList<Argument *>() 
      << new BoolArgument("online", "Online version",
                          "Show the online documentation in a browser",
                          true));


static Command 
hlpc("help", // command name
     effector(helpCommand), // action
     "help",  // group name
     &helpA, // arguments
     &helpO, // options
     "Help on...",
     "Give help on command",
     "Gives all help available on the given command",
     "?");

//////////////////////////////////////////////////////////////////////



void updateDocumentationFile(const QString &, QString file)
{
  QString str;
  try {
    QFile f(file);
    Utils::open(&f, QIODevice::ReadOnly|QIODevice::Text);
    str = f.readAll();
    f.close();
    QFile::remove(file + ".old");
    QFile::rename(file, file + ".old");
  }
  catch (RuntimeError &re) {
    // Nothing to do !
  }


  // Now perform updates

  
  {                             // Commands first
    QStringList cmds = Command::allCommands();
    qSort(cmds);

    for(int i = 0; i < cmds.size(); i++) {
      Command * cmd = Command::namedCommand(cmds[i]);
      cmd->updateDocumentation(str);
    }
  }

  {                             // Then functions
    Utils::updateWithin(str, "{::comment} special-functions-start {:/}",
                        "{::comment} special-functions-end {:/}\n",
                        "\n\n" + GSLFunction::availableFunctions() + "\n");

    Utils::updateWithin(str, "{::comment} constants-start {:/}",
                        "{::comment} constants-end {:/}\n",
                        "\n\n" + GSLConstant::availableConstants() + "\n");

    QStringList nonInt = Command::nonInteractiveCommands();
    qSort(nonInt);
    Utils::makeUnique(nonInt);
    for(int i = 0; i < nonInt.size(); i++)
      nonInt[i] = QString(" * [`%1`](#cmd-%2)").arg(nonInt[i]).
        arg(nonInt[i]);

    Utils::updateWithin(str, "{::comment} non-interactive-start {:/}",
                        "{::comment} non-interacive-end {:/}\n",
                        "\n\n" + nonInt.join("\n") + "\n");
  }



  QFile o(file);
  Utils::open(&o, QIODevice::WriteOnly|QIODevice::Text);
  o.write(str.toLocal8Bit());
  
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
     "Update documentation file",
     "...");

//////////////////////////////////////////////////////////////////////

static CommandLineOption hlp("--update-documentation", [](const QStringList & args) {
    updateDocumentationFile("ud", args[0]);
    ::exit(0);
  }, 1, "updates the given documentation file");

//////////////////////////////////////////////////////////////////////



/// This functions loads a documentation file
void loadDocumentationFile(const QString &, 
                           QString file, 
                           const CommandOptions & opts)
{
  bool silent = true;
  updateFromOptions(opts, "silent", silent);
  QString str;
  QFile f(file);
  Utils::open(&f, QIODevice::ReadOnly|QIODevice::Text);
  str = f.readAll();
  f.close();

  /// @todo Setup a standard location for the documentation file and
  /// load at startup.

  Terminal::out << "Loading documentation from file " << file << endl;

  // Now perform updates
  QStringList missing = Command::loadDocumentation(str);
  if(!silent) {
    qSort(missing);
    Terminal::out << "Documentation missing for commands: " << missing.join(", ") << endl;
  }
}

static ArgumentList 
ldfA(QList<Argument *>() 
     << new FileArgument("file", "File",
                         "The document file to load")
     );

static ArgumentList 
ldfO(QList<Argument *>() 
     << new BoolArgument("silent", "Silent",
                         "Whether to say something when "
                         "documentation is missing",
                         true)
     );

static Command 
ldCmd("load-documentation",            // command name
      effector(loadDocumentationFile), // action
      "help",                          // group name
      &ldfA,                           // arguments
      &ldfO,                           // options
      "Load documentation",
      "Load documentation file",
      "...");

//////////////////////////////////////////////////////////////////////
