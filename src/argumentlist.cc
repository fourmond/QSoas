/*
  argumentlist.cc: implementation of the ArgumentList class
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
#include <commandeffector.hh>
#include <argumentlist.hh>

#include <terminal.hh>


ArgumentList::ArgumentList(const QList<Argument *> & lst)
  : QList<Argument *>(lst), greedyArg(-1)
{
}

ArgumentList::ArgumentList() : greedyArg(-1)
{
}

void ArgumentList::regenerateCache() const
{
  cache.clear();
  for(int i = 0; i < size(); i++) {
    cache[value(i)->argumentName()] = value(i);
    if(value(i)->greedy)
      greedyArg = i;
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
  return cache.value(name, NULL);
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


CommandArguments ArgumentList::parseArguments(const QStringList & args,
                                              QWidget * base) const
{

  CommandArguments ret;  
  if(cache.size() != size())
    regenerateCache();

  int nb = args.size();
  int sz = std::max(size(), nb);
  for(int i = 0; i < sz; i++) {
    int idx = assignArg(i, nb);
    if(idx >= size()) {
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
