/*
  stylegenerator.cc: implementation of the style generators
  Copyright 2013 by CNRS/AMU

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

#include <exceptions.hh>

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

/// @todo Maybe move to Utils ?
static Vector colorToVector(const QColor & color)
{
  Vector rv;
  rv << color.red() << color.green() << color.blue();
  return rv;
}

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
    beg = ::colorToVector(b);
    end = ::colorToVector(e);
  };

  virtual QPen nextStyle() override {
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
    return pen;
  };
};

static StyleGeneratorFactoryItem 
grd1("red-blue", "Red-blue gradient",
     [](int n, const QString &) -> StyleGenerator *{
       return new SimpleGradient(QColor("red"),
                                 QColor("blue"),
                                 n);
     });

static StyleGeneratorFactoryItem 
grd2("red-green", "Red-green gradient",
     [](int n, const QString &) -> StyleGenerator *{
       return new SimpleGradient(QColor("red"),
                                 QColor("green"),
                                 n);
     });


//////////////////////////////////////////////////////////////////////

/// Same as SimpleGradient, but with as many as one wants...
class MultiGradient : public StyleGenerator {
  QList<Vector> colors;

  int idx;

public:

  MultiGradient(const QStringList & cols, int nb) : 
    StyleGenerator(nb), idx(0) {
    for(int i = 0; i < cols.size(); i++)
      colors << ::colorToVector(QColor(cols[i]));
    if(colors.size() < 2)
      throw InternalError("Need 2 colors or more to make a gradient");
  };

  virtual QPen nextStyle() override {
    // Assume RGB color space:
    double d = (totalNumber > 1 ? totalNumber - 1 : 100);
    double alpha = idx++ / d * (colors.size() - 1);
    if(alpha > colors.size() - 1)
      alpha = colors.size() - 1.1;
    int base = floor(alpha);
    Vector s = colors[base];
    if(base < colors.size() - 1) {
      alpha = alpha - base;
      s *= 1.0 - alpha;
      Vector s2 = colors[base+1];
      s2 *= alpha;
      s += s2;
    }

    QColor c(s[0], s[1], s[2]);
    QPen pen = soas().graphicsSettings().dataSetPen(0);
    pen.setColor(c);
    return pen;
  };
};

// Some of the colors here come from http://colorbrewer2.org/

static StyleGeneratorFactoryItem 
grd3("red-to-blue", "More advanced red-to-blue gradient",
     [](int n, const QString &) -> StyleGenerator *{
       QStringList cls;
       cls << "#b2182b" << "#ef8a62"
           << "#fddbc7" << "#d1e5f0"
           << "#67a9cf" << "#2166ac";
       return new MultiGradient(cls, n);
     });

static StyleGeneratorFactoryItem 
grd4("red-yellow-green", "Red-to-yellow-to-green",
     [](int n, const QString &) -> StyleGenerator *{
       QStringList cls;
       cls << "#d7191c" << "#fdae61"
           << "#ffffbf" << "#a6d96a" << "#1a9641";
       return new MultiGradient(cls, n);
     });

static StyleGeneratorFactoryItem 
grd5("brown-green", "Brown to dark green",
     [](int n, const QString &) -> StyleGenerator *{
       QStringList cls;
       cls << "#a6611a" << "#dfc27d"
           << "#80cdc1" << "#018571";
       return new MultiGradient(cls, n);
     });

