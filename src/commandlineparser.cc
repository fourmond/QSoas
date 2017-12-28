/*
    commandlineparser.cc: implementation of command-line parsing
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

#include <headers.hh>
#include <commandlineparser.hh>
#include <exceptions.hh>

void CommandLineOption::handle(QStringList & args)
{
  args.takeFirst();		// Remove the first argument, which
				// should be --longKey
  int nbreq = abs(numberNeeded);
  if(nbreq > args.size())
    throw RuntimeError("Missing arguments for command-line option %1: %2 for %3").arg(longKey).arg(args.size()).arg(abs(numberNeeded));

  if(numberNeeded < 0) {
    handler(args);
    args.clear();
  }
  else {
    QStringList arglist;
    for(int i = 0; i < numberNeeded; i++)
      arglist.append(args.takeFirst());
    handler(arglist);
  }
}

CommandLineOption::CommandLineOption(const QString & k,
                                     CommandLineOption::Handler h,
                                     int nb, const QString & help,
                                     bool registerSelf, bool def) :
    longKey(k), numberNeeded(nb), helpText(help),
    handler(h), isDefault(def)
{
  if(registerSelf)
    CommandLineParser::globalParser()->addOption(this);
}


//////////////////////////////////////////////////////////////////////

CommandLineParser * CommandLineParser::parser = NULL;

CommandLineParser::CommandLineParser() : defaultOption(NULL)
{
}

CommandLineParser * CommandLineParser::globalParser()
{
  if(! parser)
    parser = new CommandLineParser;
  return parser;
}

void CommandLineParser::addOption(CommandLineOption * option)
{
  QString key;
  if(option->longKey.startsWith("--"))
    key = option->longKey;
  else
    key = QString("--") + option->longKey;
  options[key] = option;
  if(option->isDefault) {
    if(defaultOption)
      throw InternalError("Two default options: %1 and %2").
        arg(defaultOption->longKey).arg(option->longKey);
    defaultOption = option;
  }
}

void CommandLineParser::doParsing(const QStringList & args)
{
  QStringList a = args;
  while(a.size() > 0) {
    CommandLineOption * opt = options.value(a.first(), NULL);
    if(! opt) {
      if(defaultOption) {
        opt = defaultOption;
        a.insert(0, "(def)");
      }
      else 
        throw RuntimeError("Unkown command-line option: %1").
          arg(a.first());
    }   
    opt->handle(a);
  }
}

void CommandLineParser::parseCommandLine()
{
  QStringList a = QCoreApplication::arguments();
  a.takeFirst();		// That's the program name
  globalParser()->doParsing(a);
}

void CommandLineParser::showHelpText(QTextStream & s)
{
  QStringList lst = options.keys();
  lst.sort();
  for(int i = 0; i < lst.size(); i++) {
    CommandLineOption * opt = options[lst[i]];
    s << "\t" << opt->longKey << "\t" << opt->helpText << endl;
  }
}


void CommandLineParser::writeSpecFile(QTextStream & s)
{
  QStringList lst = options.keys();
  lst.sort();
  for(int i = 0; i < lst.size(); i++) {
    CommandLineOption * opt = options[lst[i]];
    s << " " << opt->longKey << "/" << opt->numberNeeded << endl;
  }
}

//////////////////////////////////////////////////////////////////////

static CommandLineOption hlp("--help", [](const QStringList & /*args*/) {
    QTextStream o(stdout);
    CommandLineParser::globalParser()->showHelpText(o);
    ::exit(0);
  }, 0, "show this help text");
