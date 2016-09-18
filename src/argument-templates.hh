/**
   \file argument-templates.hh
   Some templated argument classes
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
#ifndef __ARGUMENT_TEMPLATES_HH
#define __ARGUMENT_TEMPLATES_HH

#include <argument.hh>
#include <exceptions.hh>
#include <utils.hh>


/// A choice between several fixed strings
template <class T> 
class TemplateChoiceArgument : public Argument {
  QHash<QString, T> fixedChoices;

  QStringList order;

  QString choiceName;
public:

  TemplateChoiceArgument(const QHash<QString, T> & c,
                         const char * cn, const char * pn,
                         const char * d = "", bool def = false,
                         const char * chN = "") : 
    Argument(cn, pn, d, false, def), 
    fixedChoices(c), choiceName(chN) {
    order = fixedChoices.keys();
    qSort(order);
  }; 

  TemplateChoiceArgument(const QList<QString> & c1,
                         const QList<T> & c2,
                         const char * cn, const char * pn,
                         const char * d = "", bool def = false,
                         const char * chN = "") : 
    Argument(cn, pn, d, false, def), choiceName(chN) {
    order = c1;
    // Using exceptions in constructors, but constructors are called
    // mostly statically, so that shouldn't be a problem.
    if(c1.size() != c2.size())
      throw InternalError("Mismatch in sizes: %1 vs %2").
        arg(c1.size()).arg(c2.size());
    for(int i = 0; i < c2.size(); i++) {
      fixedChoices[c1[i]] = c2[i];
    }
  }; 

  virtual QString typeName() const {
    if(choiceName.isEmpty())
      return "choice";
    return choiceName;
  };
  virtual QString typeDescription() const {
    return QString("One of: `%1`").arg(order.join("`, `"));
  };

  /// Returns a wrapped T
  virtual ArgumentMarshaller * fromString(const QString & str) const {
    if(! fixedChoices.contains(str))
      throw RuntimeError("%1 is not a valid choice").arg(str);
    return new ArgumentMarshallerChild<T>(fixedChoices[str]);
  };

  /// Prompting uses QInputDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const {
    bool ok = false;
    QString str = 
      QInputDialog::getItem(base, argumentName(), description(),
                            order, 0, false, &ok);
    if(! ok)
      throw RuntimeError("Aborted");
    return fromString(str);
  };

  /// a rather easy one.
  virtual QStringList proposeCompletion(const QString & starter) const {
    return Utils::stringsStartingWith(fixedChoices.keys(), starter);
  };

  
  virtual QWidget * createEditor(QWidget * parent = NULL) const {
    QComboBox * cb = new QComboBox(parent);

    QStringList keys = fixedChoices.keys();
    qSort(keys);
    for(int i = 0; i < keys.size(); i++)
      cb->addItem(keys[i]);
    return cb;
  };

  virtual void setEditorValue(QWidget * editor, 
                              ArgumentMarshaller * value) const {
    QComboBox * cb = dynamic_cast<QComboBox*>(editor);
    if(! cb)
      throw InternalError("Not a combo box");
    T val = value->value<T>();
    for(int i = 0; i < cb->count();i++) {
      if(fixedChoices[cb->itemText(i)] == val) {
        cb->setCurrentIndex(i);
        break;
      }
    }
  };

  virtual ArgumentMarshaller * fromRuby(RUBY_VALUE value) const {
    return Argument::convertRubyString(value);
  };

};

#endif
