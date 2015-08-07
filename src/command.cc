/*
  command.cc: implementation of the Command class
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014 by CNRS/AMU

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
#include <curveeventloop.hh>

#include <eventhandler.hh>
#include <commandlineparser.hh>


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
      if(! grp) {
        QTextStream o(stdout);
        o << "Missing group: " << i.value()->groupName << endl;
      }
      else {
        i.value()->group = grp;
        grp->commands.append(i.value());
      }
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

CommandOptions Command::parseOptions(const QMultiHash<QString, QString> & opts,
                                     QString * defaultOption) const
{
  CommandOptions ret;  
  if(! options)
    return ret;
  if(defaultOption && !defaultOption->isEmpty()) {
    Argument * opt = options->defaultOption();
    ret[opt->argumentName()] = opt->fromString(*defaultOption);
  }

  // We need to make a copy of the initial MultiHash so that the
  // inserted keys come in the right order !
  QMultiHash<QString, QString> newopts;
  for(QMultiHash<QString, QString>::const_iterator i = opts.begin();
      i != opts.end(); i++)
    newopts.insert(i.key(), i.value());

  for(QMultiHash<QString, QString>::const_iterator i = newopts.begin();
      i != newopts.end(); i++) {
    Argument * opt = options->namedArgument(i.key());
    if(! opt)
      throw RuntimeError(QObject::tr("Unknown option '%1' for command %2").
                         arg(i.key()).arg(commandName()));
    /// @bug Memory leak in case of an exception thrown: ret is not
    /// freed (and that happens often !)
    ArgumentMarshaller * newv = opt->fromString(i.value());

    /// @todo Probably, instead of setting the target hash based on
    /// the key, one should ask options or opt where to put it ? This
    /// would allow abbreviation of option names ?
    if(ret.contains(i.key())) {
      if(opt->greedy) {
        opt->concatenateArguments(ret[i.key()],
                                  newv);
        delete newv;
        continue;
      }
      delete ret[i.key()];      // Make sure we don't forget this pointer.
    }
    ret[i.key()] = newv;
  }
  return ret;
}

void Command::runCommand(const QString & commandName, 
                         const QStringList & arguments,
                         QWidget * base)
{
  // First, arguments conversion
  QPair<QStringList, QMultiHash<QString, QString> > split = 
    splitArgumentsAndOptions(arguments);

  bool hasDefault = (this->options ? this->options->hasDefaultOption() : false);
  QString def;
  PossessiveList<ArgumentMarshaller> 
    args(parseArguments(split.first, (hasDefault ? &def : NULL), base));
  PossessiveHash<QString, ArgumentMarshaller> 
    options(parseOptions(split.second, (hasDefault ? &def : NULL)));
  // Then the call !

  if(effector->needsLoop()) {
    CurveEventLoop loop; /// @todo Find a way to provide context ?
    effector->runWithLoop(loop, commandName, args, options);
  }
  else 
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


QPair<QStringList, QMultiHash<QString, QString> > 
Command::splitArgumentsAndOptions(const QStringList & rawArgs,
                                  QList<int> * annotate,
                                  bool * pendingOption)
{
  QPair<QStringList, QMultiHash<QString, QString> > ret;
  QStringList & args = ret.first;
  QMultiHash<QString, QString> & opts = ret.second;
  int size = rawArgs.size();
  if(pendingOption)
    *pendingOption = false;

  if(annotate) {
    annotate->clear();
    for(int i = 0; i < rawArgs.size(); i++)
      *annotate << -1;
  }
  
  QRegExp optionRE("^\\s*/([a-zA-Z0-9-]+)\\s*(?:=?\\s*|=\\s*(.*))$");
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
        opts.insert(optionName, optionRE.cap(2));
      }
      else {                    // Looking at next words
        i++;
        QString next = rawArgs.value(i, "");
        if(equalRE.indexIn(next) == 0) {
          if(! equalRE.cap(1).isEmpty())
            opts.insert(optionName, equalRE.cap(1));
          else {
            i++;
            next = rawArgs.value(i, "");
            opts.insert(optionName, next);
          }
        }
        else
          opts.insert(optionName, next);
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

QStringList Command::interactiveCommands()
{
  if(! availableCommands)
    return QStringList();

  QStringList ret;
  
  for(QHash<QString, Command*>::const_iterator i = availableCommands->begin();
      i != availableCommands->end(); i++)
    if(i.value()->isInteractive())
      ret << i.value()->cmdName;
  return ret;
}

QStringList Command::nonInteractiveCommands()
{
  if(! availableCommands)
    return QStringList();

  QStringList ret;
  
  for(QHash<QString, Command*>::const_iterator i = availableCommands->begin();
      i != availableCommands->end(); i++)
    if(! i.value()->isInteractive())
      ret << i.value()->cmdName;
  return ret;
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

/// @todo Probably should join Utils some day.
QString wrapIf(const QString & str, const QString & left, bool cond, 
               const QString & right = QString())
{
  if(cond) 
    return QString("%1%2%3").arg(left).arg(str).
      arg(right.isEmpty() ? left : right);
  return str;
}

QString Command::synopsis(bool markup) const 
{
  QStringList synopsis;
  QString descs;

  if(commandArguments()) {
    const ArgumentList & args = *commandArguments();
    for(int i = 0; i < args.size(); i++) {
      QString n = args[i]->argumentName();
      if(args[i]->greedy)
        n += "...";
      QString a = wrapIf(n, "_", markup);
      if(markup) {
        QString td = args[i]->typeDescription();
        if(! td.isEmpty())
          a += QString("{:title=\"%1\"}").arg(td);
      }
      synopsis << a;
      descs += QString("  * %1: %2\n").
        arg(a).
        arg(args[i]->description());
    }
  }

  if(commandOptions()) {
    const ArgumentList & args = *commandOptions();
    for(int i = 0; i < args.size(); i++) {
      QString a = args[i]->argumentName();
      QString td = args[i]->typeDescription();
      QString b = wrapIf("/" + a + "=", "`", markup) + 
        wrapIf(args[i]->typeName(), "_", markup) + 
        ((markup && !(td.isEmpty())) ? 
         QString("{:title=\"%1\"}").arg(td) : "");
      synopsis << b;
      descs += QString("  * %1%3: %2\n").
        arg(b).
        arg(args[i]->description()).
        arg(args[i]->defaultOption ? " (default)" : "");

    }
  }

  if(isInteractive())
    synopsis << wrapIf("(interactive)", "**", markup);

  if(! shortCmdName.isEmpty())
    descs = "Alias: " + wrapIf(shortCmdName, "`", markup) + "\n\n" + descs;

  // We need to escape |, which comes in too many times
  if(markup)
    descs.replace(QString("|"), QString("\\|"));

  return wrapIf(cmdName,"`", markup) + " " + 
    synopsis.join(" ") + "\n\n" + descs + "\n";
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

QStringList Command::loadDocumentation(const QString & str)
{
  QRegExp re("\\{::comment\\} description-(start|end):\\s*([0-9a-z-]+)\\s*\\{:/\\}\\s*");

  QHash<QString, Command *> cmds = *availableCommands;

  QTextStream o(stdout);

  int idx = 0;
  int nx = 0;

  int beg = -1;
  QString cur;
  while(nx = re.indexIn(str, idx), nx >= 0) {
    if(re.cap(1) == "start") {
      beg = nx + re.matchedLength();
      cur = re.cap(2);
    } else {
      if(beg >= 0 && cmds.contains(cur)) {
        cmds[cur]->longDesc = str.mid(beg, nx - beg);
        cmds.remove(cmds[cur]->shortCommandName());
        cmds.remove(cur);
      }
    }
    idx = nx + re.matchedLength();
  }
  return cmds.keys();
}

void Command::setDocumentation(const QString & str)
{
  longDesc = str;
}


bool Command::isInteractive() const
{
  return effector->isInteractive();
}

QString Command::commandSpec() const
{
  QString ret;
  ret = cmdName;
  if(isInteractive()) {
    ret += " (interactive)";
    if(effector->needsLoop()) {
      ret += "\n  handler: ";
      EventHandler * handler = EventHandler::handlerForCommand(cmdName);
      if(handler)
        ret += handler->buildSpec();
      else
        ret += " (none)";
    }
  }
  ret += "\n";
  if(! shortCmdName.isEmpty())
    ret += QString("  -> aliased as %1\n").arg(shortCmdName);
  if(arguments)
    for(int i = 0; i < arguments->size(); i++) {
      Argument * arg = (*arguments)[i];
      ret += " - " + arg->argumentName() + 
        (arg->greedy ? "..." : "") + "\n";
    }
  if(options) {
    QStringList an = options->argumentNames();
    qSort(an);
    for(int i = 0; i < an.size(); i++) {
      Argument * arg = options->namedArgument(an[i]);
      ret += " - /" + arg->argumentName() + 
        (arg->defaultOption ? "*" : "") + "\n";
    }
  }
  return ret;
}


QSet<Command *> Command::uniqueCommands()
{
  if(! availableCommands)
    return QSet<Command * >();
  return QSet<Command * >::fromList(availableCommands->values());
}

static bool cmpCommands(const Command * a, const Command * b)
{
  return a->commandName() < b->commandName();
}

void Command::writeSpecFile(QTextStream & out)
{
  QList<Command *> lst = uniqueCommands().toList();
  qSort(lst.begin(), lst.end(), ::cmpCommands);

  for(int i = 0; i < lst.size(); i++)
    out << lst[i]->commandSpec();

}

//////////////////////////////////////////////////////////////////////
CommandLineOption sp("--spec", [](const QStringList & /*args*/) {
    {
      QTextStream o(stdout);
      Command::writeSpecFile(o);
    }
    ::exit(0);
  }, 0, "write command specs");
