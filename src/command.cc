/*
  command.cc: implementation of the Command class
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014, 2018 by CNRS/AMU

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
#include <argument.hh>
#include <commandeffector.hh>

#include <possessive-containers.hh>
#include <exceptions.hh>
#include <utils.hh>
#include <curveeventloop.hh>

#include <eventhandler.hh>

#include <general-arguments.hh>

#include <mruby.hh>
#include <terminal.hh>


Command::Command(const char * cn, 
                 CommandEffector * eff,
                 const char * gn, 
                 ArgumentList * ar,
                 ArgumentList * op,
                 const char * pn,
                 const char * sd, 
                 const char * sc,
                 CommandContext * cxt,
                 bool autoRegister) : 
  cmdName(cn), shortCmdName(sc), pubName(pn), 
  shortDesc(sd), groupName(gn), 
  arguments(ar), options(op), custom(CommandContext::finishedLoading),
  context(cxt),
  effector(eff), 
  group(NULL)
{
  if(autoRegister)
    registerMe();
}; 

Command::Command(const char * cn, 
                 CommandEffector * eff,
                 const char * gn, 
                 ArgumentList * ar,
                 ArgumentList * op,
                 const QByteArray & pn,
                 const QByteArray & sd, 
                 const QByteArray & sc, 
                 CommandContext * cxt,
                 bool autoRegister) : 
  cmdName(cn), shortCmdName(sc), pubName(pn), 
  shortDesc(sd), groupName(gn), 
  arguments(ar), options(op), custom(CommandContext::finishedLoading),
  context(cxt),
  effector(eff), 
  group(NULL)
{
  if(autoRegister)
    registerMe();
}; 

Command::~Command()
{
  context->unregisterCommand(this);
}

void Command::registerMe()
{
  if(! context)
    context = CommandContext::globalContext();
  context->registerCommand(this);
}


/// Dummy ArgumentList. @todo move somewhere else ?
static ArgumentList emptyList;

/// @todo I think this whole function should move to ArgumentList,
/// probably with much better checking of argument size and slurping
/// arguments.
CommandArguments Command::parseArguments(const QStringList & args,
                                         QStringList * defaultOption,
                                         QWidget * base, bool * prompted) const
{
  ArgumentList * a = arguments ? arguments : &emptyList;
  return a->parseArguments(args, defaultOption, base, prompted);
}

CommandOptions Command::parseOptions(const QMultiHash<QString, QString> & opts,
                                     QStringList * defaultOption) const
{
  CommandOptions ret;  
  if(! options)
    return ret;
  if(defaultOption) {
    const Argument * opt = options->defaultOption();
    ArgumentMarshaller * arg = NULL;
    for(const QString a : *defaultOption) {
      ArgumentMarshaller * na = opt->fromString(a);
      if(arg) {
        if(! opt->greedy)
          Terminal::out << "Ignoring supplementary argument: '"
                        << a << "'" << endl;
        else
          opt->concatenateArguments(arg, na);
        delete na;
      }
      else
        arg = na;
    }
    if(arg)
      ret[opt->argumentName()] = arg;
  }

  // We need to make a copy of the initial MultiHash so that the
  // inserted keys come in the right order !
  QMultiHash<QString, QString> newopts;
  for(QMultiHash<QString, QString>::const_iterator i = opts.begin();
      i != opts.end(); ++i)
    newopts.insert(i.key(), i.value());

  for(QMultiHash<QString, QString>::const_iterator i = newopts.begin();
      i != newopts.end(); ++i) {
    const Argument * opt = options->namedArgument(i.key());
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

bool Command::parseArgumentsAndOptions(const QStringList & arguments,
                                       CommandArguments * args,
                                       CommandOptions * opts,
                                       QWidget * base)
{
  // First, arguments conversion
  QPair<QStringList, QMultiHash<QString, QString> > split = 
    splitArgumentsAndOptions(arguments);

  bool hasDefault = (this->options ? this->options->hasDefaultOption() : false);
  QStringList def;
  bool prompted = false;
  *args = parseArguments(split.first, (hasDefault ? &def : NULL),
                         base, &prompted);
  *opts = parseOptions(split.second, (hasDefault ? &def : NULL));
  return prompted;
}

QStringList Command::rebuildCommandLine(const CommandArguments & args,
                                        const CommandOptions & opts) const
{
  QStringList rv;
  // Dealing first with arguments
  ArgumentList * argList = arguments ? arguments : &emptyList;
  if(args.size() != argList->size())
    throw InternalError("Mismatch in the number of arguments: %1 vs %2").
      arg(args.size()).arg(argList->size());
  for(int i = 0; i < argList->size(); i++) {
    const Argument * a = argList->value(i);
    rv += a->toString(args[i]);
  }

  argList = options ? options : &emptyList;
  for(const QString & n : opts.keys()) {
    const Argument * o = argList->namedArgument(n);
    if(! o)
      throw InternalError("Unknown option: '%1'").arg(n);
    QStringList vals = o->toString(opts[n]);
    for(const QString & v : vals)
      rv += QString("/%1=%2").arg(n).arg(v);
  }

  return rv;
  
}


void Command::runCommand(const QString & commandName, 
                         const QStringList & arguments,
                         QWidget * base)
{
  // First, arguments conversion
  QPair<QStringList, QMultiHash<QString, QString> > split = 
    splitArgumentsAndOptions(arguments);

  bool hasDefault = (this->options ? this->options->hasDefaultOption() : false);
  QStringList def;
  PossessiveList<ArgumentMarshaller> 
    args(parseArguments(split.first, (hasDefault ? &def : NULL), base));
  PossessiveHash<QString, ArgumentMarshaller> 
    options(parseOptions(split.second, (hasDefault ? &def : NULL)));
  // Then the call !

  runCommand(commandName, args, options);
}

void Command::runCommand(const QString & commandName,
                         const CommandArguments & args,
                         const CommandOptions & options)
{
  if(effector->needsLoop()) {
    CurveEventLoop loop; /// @todo Find a way to provide context ?
    effector->runWithLoop(loop, commandName, args, options);
  }
  else 
    effector->runCommand(commandName, args, options);
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
      QString td = Utils::uncapitalize(args[i]->typeDescription());
      if(markup) {
        if(! td.isEmpty())
          a += QString("{:title=\"%1\"}").arg(td);
      }
      synopsis << a;
      descs += QString("  * %1: %2 -- values: %3\n").
        arg(a).
        arg(args[i]->description()).
        arg(td);
    }
  }

  if(commandOptions()) {
    const ArgumentList &args = *commandOptions();
    QStringList names = args.argumentNames();
    
    for(const QString & a : names) {
      const Argument * arg = args.namedArgument(a);
      QString td = Utils::uncapitalize(arg->typeDescription());
      QString b = wrapIf("/" + a + "=", "`", markup) + 
        wrapIf(arg->typeName(), "_", markup) + 
        ((markup && !(td.isEmpty())) ? 
         QString("{:title=\"%1\"}").arg(td) : "");
      synopsis << b;
      descs += QString("  * %1%3: %2 -- values: %4\n").
        arg(b).
        arg(arg->description()).
        arg(arg->defaultOption ? " (default option)" : "").
        arg(td);

    }
  }

  if(isInteractive())
    synopsis << wrapIf("(interactive)", "**", markup);
  if(context != CommandContext::globalContext())
    synopsis << wrapIf("(fit command)", "**", markup);

  if(! shortCmdName.isEmpty())
    descs = "Short name: " + wrapIf(shortCmdName, "`", markup) + "\n\n" + descs;

  // We need to escape |, which comes in too many times
  if(markup)
    descs.replace(QString("|"), QString("\\|"));

  return wrapIf(cmdName,"`", markup) + " " + 
    synopsis.join(" ") + "\n\n" + descs + "\n";
}

QString & Command::updateDocumentation(QString & str, int level) const
{
  QString pref;
  if(context != CommandContext::globalContext())
    pref = "fit-";

  QString beg = QString("{::comment} synopsis-start: %1%2 {:/}").
    arg(pref).arg(cmdName);

  QString end = QString("{::comment} synopsis-end: %1%2 {:/}\n").
    arg(pref).arg(cmdName);

  QString headings(level, '#');
  QString syn = "\n\n" +
    headings + " `" + cmdName + "` - " + pubName + 
    " {#" + pref + "cmd-" + cmdName + "}\n\n" + 
    synopsis(true);

  Utils::updateWithin(str, beg, end, syn);
  return str;
}


void Command::setDocumentation(const QString & str)
{
  longDesc = str;
}


bool Command::isInteractive() const
{
  return effector->isInteractive();
}

bool Command::hasNoArgsNorOpts() const
{
  if(arguments && arguments->size() > 0)
    return false;
  if(options && options->size() > 0)
    return false;
  return true;
}

QString Command::commandSpec(bool full) const
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
      const Argument * arg = (*arguments)[i];
      ret += " - " + arg->argumentName() + 
        (arg->greedy ? "..." : "");
      if(full)
        ret += " (" + arg->typeName() + ")"; 
      ret += "\n";
    }
  if(options) {
    QStringList an = options->argumentNames();
    qSort(an);
    for(int i = 0; i < an.size(); i++) {
      const Argument * arg = options->namedArgument(an[i]);
      ret += " - /" + arg->argumentName() + 
        (arg->defaultOption ? "*" : "");
      if(full)
        ret += " (" + arg->typeName() + ")"; 
      ret += "\n";
    }
  }
  return ret;
}



// Ruby interface commands:
void Command::runCommand(int nb, mrb_value * args)
{
  MRuby * mr = MRuby::ruby();

  CommandOptions op;
  if(args && nb > 0 &&
     mr->isHash(args[nb-1])) { // There are options, we parse them
    mrb_value hsh = args[nb-1];
    nb--;
    if(commandOptions())
      op = commandOptions()->parseRubyOptions(hsh);
  }

  
  // Now, parse arguments. No prompting.
  CommandArguments a;
  if(arguments)
    a = arguments->parseRubyArguments(nb, args);
  PossessiveHash<QString, ArgumentMarshaller> t(op);
  PossessiveList<ArgumentMarshaller> t2(a);
  runCommand(cmdName, a, op);
}

