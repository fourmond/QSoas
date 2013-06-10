/*
  stylegenerator.cc: implementation of the style generators
  Copyright 2013 by Vincent Fourmond

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
#include <stylegenerator.hh>

#include <graphicssettings.hh>
#include <soas.hh>

QHash<QString, StyleGeneratorFactoryItem*> * StyleGenerator::factory = NULL;


StyleGeneratorFactoryItem::StyleGeneratorFactoryItem(const QString & n,
                                                     const QString & pn,
                                                     StyleGeneratorFactoryItem::Creator c) : creator(c), name(n), publicName(pn)
{
  StyleGenerator::registerFactoryItem(this);
}

//////////////////////////////////////////////////////////////////////

void StyleGenerator::registerFactoryItem(StyleGeneratorFactoryItem * item)
{
  if(! factory)
    factory = new QHash<QString, StyleGeneratorFactoryItem*>;
  (*factory)[item->name] = item;
}

StyleGeneratorFactoryItem * StyleGenerator::namedFactoryItem(const QString & name)
{
  if(! factory)
    return NULL;
  return factory->value(name, NULL);
}

StyleGenerator * StyleGenerator::createNamedGenerator(const QString & name, 
                                                      int nb, const QString & a)
{
  StyleGeneratorFactoryItem * fact = namedFactoryItem(name);
  if(fact)
    return fact->creator(nb, a);
  return NULL;
}

StyleGenerator * StyleGenerator::fromText(const QString & txt, 
                                          int nb)
{
  // simple for now
  return createNamedGenerator(txt, nb);
}

QStringList StyleGenerator::availableGenerators()
{
  if(! factory)
    return QStringList();
  QStringList keys = factory->keys();
  qSort(keys);
  return keys;
}

//////////////////////////////////////////////////////////////////////

// First elements: gradients

/// A gradient, ie something whose "coordinates" vary linearly from
/// beginning to the end.
///
/// @todo Add the possibility to use coordinates other than RGB ? (but
/// that needs some thinking, obviously).
class SimpleGradient : public StyleGenerator {
  Vector beg;
  Vector end;

  int idx;

public:

  SimpleGradient(const QColor & b, const QColor & e, int nb) : 
    StyleGenerator(nb), idx(0) {
    beg = Vector() << b.red() << b.green() << b.blue();
    end = Vector() << e.red() << e.green() << e.blue();
  };

  virtual QPen nextStyle() {
    // Assume RGB color space:
    double d = (totalNumber > 1 ? totalNumber - 1 : 100);
    double alpha = idx++ / d;
    Vector s = beg;
    s *= 1.0 - alpha;
    Vector s2 = end;
    s2 *= alpha;
    s += s2;

    QColor c(s[0], s[1], s[2]);
    QPen pen = soas().graphicsSettings().dataSetPen(0);
    pen.setColor(c);
    return c;
  };
};

static StyleGeneratorFactoryItem 
st("red-blue", "Red-blue gradient",
   [](int n, const QString &) -> StyleGenerator *{
     return new SimpleGradient(QColor("red"),
                               QColor("blue"),
                               n);
   });
