/*
  command.cc: implementation of the Command class
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
#include <command.hh>
#include <group.hh>
#include <argument.hh>
#include <commandeffector.hh>

#include <possessive-containers.hh>
#include <exceptions.hh>
#include <utils.hh>


QHash<QString, Command*> * Command::availableCommands = NULL;

void Command::registerCommand(Command * cmd)
{
  if(! availableCommands)
    availableCommands = new QHash<QString, Command*>;

  if(availableCommands->contains(cmd->commandName()))
    throw InternalError(QObject::tr("Duplicate command name : %1").
                        arg(cmd->commandName()));

  (*availableCommands)[cmd->commandName()] = cmd;

  if(cmd->shortCommandName().isEmpty())
    return;
  if(availableCommands->contains(cmd->shortCommandName()))
    throw InternalError(QObject::tr("Duplicate short command name : %1").
                        arg(cmd->shortCommandName()));
  (*availableCommands)[cmd->shortCommandName()] = cmd;
}

void Command::crosslinkCommands()
{
  if(! availableCommands)
    return;
  for(QHash<QString, Command *>::iterator i = availableCommands->begin();
      i != availableCommands->end(); i++) {
    if(! i.value()->group) {
      Group * grp = Group::namedGroup(i.value()->groupName);
      i.value()->group = grp;
      grp->commands.append(i.value());
    }
  }
}

/// Dummy ArgumentList. @todo move somewhere else ?
static ArgumentList emptyList;

/// @todo I think this whole function should move to ArgumentList,
/// probably with much better checking of argument size and slurping
/// arguments.
CommandArguments Command::parseArguments(const QStringList & args,
                                         QString * defaultOption,
                                         QWidget * base) const
{
  ArgumentList * a = arguments ? arguments : &emptyList;
  return a->parseArguments(args, defaultOption, base);
}

CommandOptions Command::parseOptions(const QHash<QString, QString> & opts,
                                     QString * defaultOption) const
{
  /// @todo handle greedy arguments ? That requires a QMultiHash rather
  /// than a QHash, but that should be doable ?
  CommandOptions ret;  
  if(! options)
    return ret;
  if(defaultOption && !defaultOption->isEmpty()) {
    Argument * opt = options->defaultOption();
    ret[opt->argumentName()] = opt->fromString(*defaultOption);
  }
  for(QHash<QString, QString>::const_iterator i = opts.begin();
      i != opts.end(); i++) {
    Argument * opt = options->namedArgument(i.key());
    if(! opt)
      throw RuntimeError(QObject::tr("Unknown option '%1' for command %2").
                         arg(i.key()).arg(commandName()));
    /// @bug Memory leak !
    ret[i.key()] = opt->fromString(i.value());
  }
  return ret;
}

void Command::runCommand(const QString & commandName, 
                         const QStringList & arguments,
                         QWidget * base)
{
  // First, arguments conversion
  QPair<QStringList, QHash<QString, QString> > split = 
    splitArgumentsAndOptions(arguments);

  bool hasDefault = (this->options ? this->options->hasDefaultOption() : false);
  QString def;
  PossessiveList<ArgumentMarshaller> args = 
    parseArguments(split.first, (hasDefault ? &def : NULL), base);
  PossessiveHash<QString, ArgumentMarshaller> options = 
    parseOptions(split.second, (hasDefault ? &def : NULL));
  // Then the call !
  effector->runCommand(commandName, args, options);
}

Command * Command::namedCommand(const QString & cmd)
{
  if(! availableCommands)
    return NULL;
  return availableCommands->value(cmd, NULL);
}

void Command::runCommand(const QStringList & cmd,
                         QWidget * base)
{
  QStringList b = cmd;
  QString name = b.takeFirst();
  Command * command = namedCommand(name);
  if(! command)
    throw RuntimeError(QObject::tr("Unknown command: '%1'").
                       arg(name));
  command->runCommand(name, b, base);
}

QAction * Command::actionForCommand(QObject * parent) const
{
  QAction * action = new QAction(parent);
  QString str = publicName();
  if(! shortCmdName.isEmpty())
    str += QString(" (%1)").arg(shortCmdName);
  action->setText(str);
  action->setStatusTip(QString("%1: %2").
                       arg(commandName()).
                       arg(shortDescription()));
  action->setToolTip(shortDescription()); // probably useless.
  action->setData(QStringList() << commandName());
  return action;
}


QPair<QStringList, QHash<QString, QString> > 
Command::splitArgumentsAndOptions(const QStringList & rawArgs,
                                  QList<int> * annotate,
                                  bool * pendingOption)
{
  QPair<QStringList, QHash<QString, QString> > ret;
  QStringList & args = ret.first;
  QHash<QString, QString> & opts = ret.second;
  int size = rawArgs.size();
  if(pendingOption)
    *pendingOption = false;

  if(annotate) {
    annotate->clear();
    for(int i = 0; i < rawArgs.size(); i++)
      *annotate << -1;
  }
  
  QRegExp optionRE("^\\s*/([a-zA-Z-]+)\\s*(?:=?\\s*|=\\s*(.*))$");
  QRegExp equalRE("^\\s*=\\s*(.*)$");

  for(int i = 0; i < size; i++) {
    if(rawArgs[i].startsWith("/!")) {
      if(annotate)
        (*annotate)[i] = args.size();
      args.append(rawArgs[i].mid(2));
      continue;
    }

    if(optionRE.indexIn(rawArgs[i]) == 0) {
      // we found an option
      QString optionName = optionRE.cap(1);
      if(! optionRE.cap(2).isEmpty()) {
        // The most simple case: an option in a single word
        opts[optionName] = optionRE.cap(2);
      }
      else {                    // Looking at next words
        i++;
        QString next = rawArgs.value(i, "");
        if(equalRE.indexIn(next) == 0) {
          if(! equalRE.cap(1).isEmpty())
            opts[optionName] = equalRE.cap(1);
          else {
            i++;
            next = rawArgs.value(i, "");
            opts[optionName] = next;
          }
        }
        else
          opts[optionName] = next;
        // We're left with an unfinished option.
        if(pendingOption && i >= rawArgs.size())
          *pendingOption = true;
      }
    }
    else {
      if(annotate)
        (*annotate)[i] = args.size();
      args.append(rawArgs[i]);
    }
  }

  return ret;
}

QStringList Command::wordSplit(const QString & str, 
                               QList<int> * wordBegin,
                               QList<int> * wordEnd)
{
  int sw = -1;
  QChar quote = 0;
  int size = str.size();
  QString curString;
  QStringList retVal;

  if(wordBegin)
    wordBegin->clear();
  if(wordEnd)
    wordEnd->clear();

  int cur;
  for(cur = 0; cur < size; cur++) {
    QChar c = str[cur];
    if(quote != 0) {
      if(c != quote)
        curString.append(c);
      else
        quote = 0;
      continue;
    }
    if(c == ' ' || c == '\t') {
      // We flush the last word, if there is one
      if(sw >= 0) {
        retVal << curString;
        if(wordBegin)
          *wordBegin << sw;
        if(wordEnd)
          *wordEnd << cur;
        sw = -1;
        curString.clear();
      }
      continue;
    }
    if(c == '"' || c == '\'') {
      quote = c;
      // We start a word
      if(sw < 0)
        sw = cur;
      continue;
    }
    if(sw < 0)
      sw = cur;
    curString.append(c);
  }
  if(sw >= 0) {
    retVal << curString;
    if(wordBegin)
      *wordBegin << sw;
    if(wordEnd)
      *wordEnd << cur;
  }
  return retVal;
}

QString Command::quoteString(const QString & str)
{
  bool sp = str.contains(' ');
  bool sq = str.contains('\'');
  bool dq = str.contains('"');
  if(! (sp || sq || dq)) {
    return str;
  }
  if(sq && dq) {
    QString s = str;            // We arbitrarily use single quotes.
    s.replace("'", "'\"'\"'");
    return "'" + s + "'";
  }
  if(sq) {
    return "\"" + str + "\"";
  }
  return "'" + str + "'";
}

QString Command::unsplitWords(const QStringList & cmdline)
{
  QStringList s;
  for(int i = 0; i < cmdline.size(); i++)
    s << quoteString(cmdline[i]);
  return s.join(" ");
}

QStringList Command::allCommands()
{
  if(! availableCommands)
    return QStringList();
  return availableCommands->keys();
}

QString Command::latexDocumentation() const
{
  QString ret;
  ret = QString("\\subsection{\\texttt{%1}: %2}\n").
    arg(commandName()).arg(shortDescription());

  QString synopsis = QString("\\textbf{Synopsis:}\n\n\\texttt{%1}").
    arg(commandName());

  QString desc;

  if(commandArguments()) {
    desc += "\\textbf{Arguments:}\n\n\\begin{itemize}\n";
    const ArgumentList & args = *commandArguments();
    for(int i = 0; i < args.size(); i++) {
      QString a = QString("\\emph{%1}").
        arg(args[i]->argumentName());
      if(args[i]->greedy)
        a += "\\emph{...}";
      synopsis += " " + a;
      desc += QString("\\item \\emph{%1}: %2\n").
        arg(args[i]->argumentName()).
        arg(args[i]->description());
    }
    desc += "\\end{itemize}\n\n";
  }

  if(commandOptions()) {
    desc += "\\textbf{Options:}\n\n\\begin{itemize}\n";
    const ArgumentList & args = *commandOptions();
    for(int i = 0; i < args.size(); i++) {
      QString a = QString("\\texttt{%1}").
        arg(args[i]->argumentName());
      synopsis += " /" + a + "= \\emph{...}" ;
      desc += QString("\\item \\texttt{/%1}: %2\n").
        arg(args[i]->argumentName()).
        arg(args[i]->description());
    }
    desc += "\\end{itemize}\n\n";
  }
  return ret + synopsis + "\n\n" + desc + longDescription();
}

QString Command::synopsis(bool markup) const 
{
  QStringList synopsis;
  QString descs;

  if(commandArguments()) {
    const ArgumentList & args = *commandArguments();
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

  if(commandOptions()) {
    const ArgumentList & args = *commandOptions();
    for(int i = 0; i < args.size(); i++) {
      QString a = args[i]->argumentName();
      synopsis << "/" + a + "=" ;
      descs += QString("  * /%1%3: %2\n").
        arg(args[i]->argumentName()).
        arg(args[i]->description()).
        arg(args[i]->defaultOption ? " (default)" : "");
    }
  }

  // We need to escape |, which comes in too many times
  if(markup)
    descs.replace(QString("|"), QString("\\|"));

  return cmdName +  " " + synopsis.join(" ") + "\n\n" + descs + "\n";
}

QString & Command::updateDocumentation(QString & str, int level) const
{
  QString beg = QString("{::comment} synopsis-start: %1 {:/}").
    arg(cmdName);

  QString end = QString("{::comment} synopsis-end: %1 {:/}\n").
    arg(cmdName);

  QString headings(level, '#');
  QString syn = "\n\n" +
    headings + " " + cmdName + " - " + pubName + 
    " {#cmd-" + cmdName + "}\n\n" + 
    synopsis(true);

  Utils::updateWithin(str, beg, end, syn);
  return str;
}

void Command::loadDocumentation(const QString & str)
{
  QString ret = "\\{::comment\\} description-start: " + cmdName + 
    " \\{:/\\}\\s*(.*)" + 
    "\\{::comment\\} description-end: " + cmdName + " \\{:/\\}\\s*";

  QRegExp re(ret);
  if(re.indexIn(str, 0) >= 0)
    longDesc = re.cap(1);
}
