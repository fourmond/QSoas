/**
   \file namedinstanceargument.hh
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
#ifndef __NAMEDINSTANCEARGUMENT_HH
#define __NAMEDINSTANCEARGUMENT_HH

#include <argument.hh>
#include <exceptions.hh>
#include <utils.hh>


/// A choice from a NamedInstance child.
template <class T> 
class NamedInstanceArgument : public Argument {

  QStringList order;

  QString choiceName;
public:

  NamedInstanceArgument(const char * cn, const char * pn,
                         const char * d = "", bool def = false,
                         const char * chN = "") : 
    Argument(cn, pn, d, false, def) {
  }; 


  virtual QString typeName() const override {
    if(choiceName.isEmpty())
      return "choice";
    return choiceName;
  };
  virtual QString typeDescription() const override {
    return QString("One of: `%1`").arg(order.join("`, `"));
  };

  /// Returns a wrapped T
  virtual ArgumentMarshaller * fromString(const QString & str) const override {
    T * item = T::namedItem(str);
    if(! item)
      throw RuntimeError("%1 is not a valid choice").arg(str);
    return new ArgumentMarshallerChild<T*>(item);
  };

  /// Prompting uses QInputDialog.
  virtual ArgumentMarshaller * promptForValue(QWidget * base) const override {
    bool ok = false;
    QString str = 
      QInputDialog::getItem(base, argumentName(), description(),
                            order, 0, false, &ok);
    if(! ok)
      throw RuntimeError("Aborted");
    return fromString(str);
  };

  virtual QStringList toString(const ArgumentMarshaller * arg) const override {
    QStringList lst;
    for(const QString n : T::availableItems()) {
      if(T::namedItem(n) == arg->value<T*>()) {
        lst << n;
        break;
      }
    }
    return lst;
  };

  /// A rather easy one.
  virtual QStringList proposeCompletion(const QString & starter) const override {
    return Utils::stringsStartingWith(T::availableItems(), starter);
  };

  
  virtual QWidget * createEditor(QWidget * parent = NULL) const override {
    QComboBox * cb = new QComboBox(parent);

    QStringList keys = T::availableItems();
    qSort(keys);
    for(int i = 0; i < keys.size(); i++)
      cb->addItem(keys[i]);
    return cb;
  };

  virtual void setEditorValue(QWidget * editor, 
                              const ArgumentMarshaller * value) const override {
    QComboBox * cb = dynamic_cast<QComboBox*>(editor);
    if(! cb)
      throw InternalError("Not a combo box");
    T * val = value->value<T*>();
    for(int i = 0; i < cb->count();i++) {
      if(T::namedItem(cb->itemText(i)) == val) {
        cb->setCurrentIndex(i);
        break;
      }
    }
  };

  virtual ArgumentMarshaller * fromRuby(mrb_value value) const override {
    return Argument::convertRubyString(value);
  };

};

#endif
