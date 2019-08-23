/*
  argument.cc: implementation of default functions for Argument
  Copyright 2011 by Vincent Fourmond
            2013 by CNRS/AMU

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
#include <argument.hh>
#include <mruby.hh>
#include <debug.hh>

ArgumentMarshaller * Argument::convertRubyString(mrb_value value) const
{
  MRuby * mr = MRuby::ruby();
  return fromString(mr->toQString(value));
}



ArgumentMarshaller * Argument::convertRubyArray(mrb_value value) const
{
  MRuby * mr = MRuby::ruby();
  ArgumentMarshaller * ret = NULL;
  if(!mr->isArray(value))
    throw InternalError("Calling convertRubyArray with a non array argument, you are probably one step from infinite recursion");
  mr->arrayIterate(value, [this, &ret] (mrb_value v) {
      ArgumentMarshaller * cur = fromRuby(v);
      if(ret) {
        concatenateArguments(ret, cur);
        delete cur;
      }
      else
        ret = cur;
    });
  return ret;
}

ArgumentMarshaller * Argument::fromRuby(mrb_value value) const
{
  return convertRubyString(value);
}


QStringList Argument::toString(const ArgumentMarshaller * arg) const
{
  // default implementation: assume we have one string
  QStringList lst;
  lst << arg->value<QString>();
  return lst;
}


ArgumentMarshaller * Argument::promptForValue(QWidget * base) const
{
  bool ok = false;
  QString str = 
    QInputDialog::getText(base, argumentName(), description(),
                          QLineEdit::Normal, QString(), &ok);
  if(! ok)
    throw RuntimeError("Aborted"); 
  return fromString(str);
}

QString Argument::promptAsString(QWidget * ) const
{
  throw RuntimeError(QObject::tr("Argument %1 does not support prompting").
                     arg(name));
  return QString();
}

QStringList Argument::proposeCompletion(const QString & ) const
{
  return QStringList();
}

void Argument::concatenateArguments(ArgumentMarshaller *,
                                    const ArgumentMarshaller *) const
{
  throw InternalError("Trying to use a greedy argument with "
                      "a type that does not support it");
}

void Argument::setDescription(const QString & s)
{
  desc = s;
}

QWidget * Argument::createEditor(QWidget * parent) const
{
  return new QLabel("<b>(not modifiable)</b>", parent);
}

void Argument::setEditorValue(QWidget * editor, 
                              ArgumentMarshaller * value) const
{
  Debug::debug()
    << "Warning: missing setEditorValue implementation for type "
    << typeName() << endl;
}

ArgumentMarshaller * Argument::getEditorValue(QWidget * editor) const
{
  QLineEdit * le = dynamic_cast<QLineEdit *>(editor);
  if(le)
    return fromString(le->text());
  QComboBox * cb = dynamic_cast<QComboBox *>(editor);
  if(cb)
    return fromString(cb->currentText());
  return NULL;
}

QString Argument::typeName() const
{
  return "NAME-MISSING";
}

QString Argument::typeDescription() const
{
  return QString();
}
