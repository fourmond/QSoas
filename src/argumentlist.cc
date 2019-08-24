/*
  argumentlist.cc: implementation of the ArgumentList class
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
#include <commandeffector.hh>
#include <argumentlist.hh>

#include <terminal.hh>
#include <mruby.hh>


ArgumentList::ArgumentList(const QList<Argument *> & lst)
  : QList<Argument *>(lst), greedyArg(-1), defaultOptionIndex(-1)
{
}

ArgumentList::ArgumentList() : greedyArg(-1), defaultOptionIndex(-1)
{
}

void ArgumentList::regenerateCache() const
{
  cache.clear();
  for(int i = 0; i < size(); i++) {
    cache[value(i)->argumentName()] = value(i);
    /// @todo detect when more than one are greedy and/or default
    if(value(i)->greedy)
      greedyArg = i;
    if(value(i)->defaultOption)
      defaultOptionIndex = i;
  }
}

bool ArgumentList::contains(const QString & name) const
{
  if(cache.size() != size())
    regenerateCache();
  return cache.contains(name);
}

Argument * ArgumentList::namedArgument(const QString & name) const
{
  if(cache.size() != size())
    regenerateCache();
  Argument * arg = cache.value(name, NULL);
  if(! arg)
    return cache.value("*", NULL); // we return "*" if it exists
  return arg;                   
}

void ArgumentList::setArgumentDescription(const QString & name, const QString & desc)
{
  Argument * arg = namedArgument(name);
  if(! arg)
    throw InternalError("Trying to modify unexisting argument: %1").arg(name);
  arg->setDescription(desc);
}


QStringList ArgumentList::argumentNames() const
{
  if(cache.size() != size())
    regenerateCache();
  return cache.keys();
}

inline int ArgumentList::assignArg(int i, int total) const
{
  if(total <= size() || greedyArg < 0 || i < greedyArg )
    return i;
  int excess = total - size();
  if(i < greedyArg + excess)
    return greedyArg;
  return i - excess;
}

int ArgumentList::assignArgument(int arg, int total) const
{
  // Regenerate the cache if necessary.
  if(cache.size() != size())
    regenerateCache();
  return assignArg(arg, total);
}

/// @bug This function will leak ArgumentMarshaller objects when an
/// exception arises.
CommandArguments ArgumentList::parseArguments(const QStringList & args,
                                              QString * defaultOpt,
                                              QWidget * base,
                                              bool * prompted) const
{

  CommandArguments ret;  
  if(cache.size() != size())
    regenerateCache();

  int nb = args.size();
  int sz = std::max(size(), nb);
  for(int i = 0; i < sz; i++) {
    int idx = assignArg(i, nb);
    if(idx >= size()) {
      if(idx == size() && defaultOpt) {
        *defaultOpt = args[i];
        continue;
      }
      else {
        /// @todo Go on with the "default" options ? That doesn't seem
        /// so easy...
        /// 
        /// @todo add a warning function to Terminal ?
        Terminal::out << Terminal::bold(QObject::tr("Warning: ")) 
                      << QObject::tr("too many arguments: %1 for %2").
          arg(nb).arg(size())
                      << endl;
        break;
      }
    }
    const Argument * arg = value(idx, NULL);
    if(! arg)
      throw InternalError("Missing argument description");
    ArgumentMarshaller * value;
    if(i < nb)
      value = arg->fromString(args[i]);
    else {
      if(! base)
        throw RuntimeError("Not enough arguments and no "
                           "prompting possible");
      value = arg->promptForValue(base);
      if(prompted)
        *prompted = true;
    }
    if(idx == greedyArg && idx != i) {
      arg->concatenateArguments(ret.last(), value);
      delete value;
    }
    else
      ret << value;
  }
  return ret;
}

CommandOptions ArgumentList::parseRubyOptions(mrb_value hsh) const
{
  CommandOptions opts;
  MRuby * mr = MRuby::ruby();
    
  for(int i = 0; i < size(); i++) {
    mrb_value key = mr->symbolFromQString(value(i)->argumentName());
    mrb_value v = mr->hashRef(hsh, key);
    if(mrb_nil_p(v)) {
      key = mr->fromQString(value(i)->argumentName());
      v = mr->hashRef(hsh, key);
    }

    if(! mrb_nil_p(v))
      opts[value(i)->argumentName()] = value(i)->fromRuby(v);
  }
  return opts;
}


CommandArguments ArgumentList::parseRubyArguments(int nb, mrb_value * values) const
{
  CommandArguments rv;
  if(nb < size())
    throw RuntimeError("Not enough arguments: %1 for %2").arg(nb).arg(size());

  for(int i = 0; i < nb; i++) {
    int idx = assignArg(i, nb);
    if(idx >= size())
      throw RuntimeError("Too many arguments: %1 for %2").arg(nb).arg(size());
    const Argument * arg = value(idx, NULL);
    if(! arg)
      throw InternalError("Missing argument description");
    ArgumentMarshaller * value = arg->fromRuby(values[i]);
    if(idx == greedyArg && idx != i) {
      arg->concatenateArguments(rv.last(), value);
      delete value;
    }
    else
      rv << value;
  }
  return rv;
}

bool ArgumentList::hasDefaultOption() const
{
  if(cache.size() != size())
    regenerateCache();
  return defaultOptionIndex >= 0;
}

Argument * ArgumentList::defaultOption() const
{
  if(! hasDefaultOption())
    return NULL;
  return value(defaultOptionIndex);
}

void ArgumentList::mergeOptions(const ArgumentList & other)
{
  regenerateCache();
  for(int i = 0; i < other.size();i++) {
    Argument * arg = other[i];
    if(! cache.contains(arg->argumentName())) {
      *this << arg;
      cache[arg->argumentName()] = NULL; // good enough until the end.
    }
    else  {
      ;                         // do something ?
    }
    
  }
  regenerateCache();
}

