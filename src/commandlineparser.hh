/**
    \file commandlineparser.hh
    Command-line parsing
    Copyright 2011 by Vincent Fourmond
              2015 by CNRS/AMU

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


#ifndef __COMMANDLINEPARSER_HH
#define __COMMANDLINEPARSER_HH

/// A command-line option
///
/// This class represents a command-line option such as --help
///
/// As of now short keys are not supported.
class CommandLineOption {
public:

  /// The handler for a given option
  typedef std::function<void (const QStringList & args)> Handler;

  /// The long key, including --
  QString longKey;

  /// The number of arguments needed. If negative, it means at least
  /// -numberNeeded but it slurps up all anyway 
  int numberNeeded;

  /// Help text
  QString helpText;


  /// The function to run to process the option
  Handler handler;

  CommandLineOption(const QString & k, Handler h,
		    int nb, const QString & help = "",
                    bool registerSelf = true);

  /// Handles the given arguments. It modifies the string list to
  /// leave only what it hasn't touched yet.
  ///
  /// It expects args[0] to be the key.
  void handle(QStringList & args);

};

/// The actual parser for the command line
class CommandLineParser {

  /// The --key -indexed options.
  QHash<QString, CommandLineOption *> options;

  /// The global parser
  static CommandLineParser * parser;

  CommandLineParser();

  CommandLineParser & operator<<(CommandLineOption * option) {
    addOption(option);
    return *this;
  };

  /// Parses the given command-line (only arguments should be given)
  void doParsing(const QStringList & args);

public:

  /// Returns the global parser
  static CommandLineParser * globalParser();

  /// Parsers the application-wide command-line.
  static void parseCommandLine();

  /// Sends help text to the given stream
  void showHelpText(QTextStream & s);

  /// Adds an option to the parser.
  void addOption(CommandLineOption * option);

  /// Writes specs to the given file 
  void writeSpecFile(QTextStream & s);
 
};

#endif
