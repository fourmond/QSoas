/*
  general-arguments.cc:
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
#include <general-arguments.hh>
#include <argumentmarshaller.hh>
#include <utils.hh>

#include <soas.hh>
#include <datastack.hh>
#include <terminal.hh>

#include <exceptions.hh>

ArgumentMarshaller * StringArgument::fromString(const QString & str) const
{
  return new ArgumentMarshallerChild<QString>(str);
}

ArgumentMarshaller * StringArgument::promptForValue(QWidget * base) const
{
  bool ok = false;
  QString str = 
    QInputDialog::getText(base, argumentName(), description(),
                          QLineEdit::Normal, QString(), &ok);
  if(! ok)
    throw RuntimeError("Aborted"); 
  return fromString(str);
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * BoolArgument::fromString(const QString & str) const
{
  QRegExp yesRE("y(es)?|true|on", Qt::CaseInsensitive);
  QRegExp noRE("no?|false|off", Qt::CaseInsensitive);
  bool val;
  if(yesRE.indexIn(str) >= 0)
    val = true;
  else if(noRE.indexIn(str) >= 0)
    val = false;
  else
    throw RuntimeError(QString("'%1' is neither true nor false").arg(str)); 
  return new ArgumentMarshallerChild<bool>(val);
}

ArgumentMarshaller * BoolArgument::promptForValue(QWidget * ) const
{
  bool val = Utils::askConfirmation(description(), argumentName());
  return new ArgumentMarshallerChild<bool>(val);
}

////////////////////////////////////////////////////////////

QStringList ChoiceArgument::choices() const
{
  QStringList c = fixedChoices;
  if(provider)
    c = provider();
  qSort(c);
  return c;
}

ArgumentMarshaller * ChoiceArgument::fromString(const QString & str) const
{
  QStringList c = choices();
  if(! c.contains(str))
    throw 
      RuntimeError(QObject::tr("Invalid argument: '%1'\nValid choices: %2").
                   arg(str).arg(c.join(", ")));
  return new ArgumentMarshallerChild<QString>(str);
}

ArgumentMarshaller * ChoiceArgument::promptForValue(QWidget * base) const
{
  bool ok = false;
  QString str = 
    QInputDialog::getItem(base, argumentName(), description(),
                          choices(), 0, false, &ok);
  if(! ok)
    throw RuntimeError("Aborted");
  return fromString(str);
}

QStringList ChoiceArgument::proposeCompletion(const QString & starter) const
{
  return Utils::stringsStartingWith(choices(), starter);
}


////////////////////////////////////////////////////////////

ArgumentMarshaller * DataSetArgument::fromString(const QString & str) const
{
  DataSet * ds = soas().stack().fromText(str);

  if(! ds)
    throw RuntimeError(QObject::tr("Not a buffer: '%1'").
                       arg(str));
  return new ArgumentMarshallerChild<DataSet *>(ds);
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * SeveralDataSetArgument::fromString(const QString & s) const
{
  QStringList splitted = s.split(QRegExp("\\s*,\\s*"));
  QList<const DataSet *> dsets;
  QRegExp multi("^\\s*(-?[0-9]+)\\s*..\\s*(-?[0-9]+)\\s*(?::(\\d+)\\s*)?");

  for(int i = 0; i < splitted.size(); i++) {
    const QString & str = splitted[i];
    
    if(multi.indexIn(str) == 0) {
      int first = multi.cap(1).toInt();
      int last = multi.cap(2).toInt();
      int sign = (first < last ? 1 : -1);
      int delta = 1;
      if(! multi.cap(3).isEmpty())
        delta = multi.cap(3).toInt();
      do {
        DataSet * ds = soas().stack().numberedDataSet(first);
        if(! ds)
          Terminal::out << "No such buffer number : " << first << endl;
        else
          dsets << ds;
        first += delta * sign;
      }
      while((first - last) * sign <= 0);

      if(dsets.size() == 0)
        throw 
          RuntimeError(QObject::tr("Buffer range '%1' corresponds "
                                   "to no buffers").
                       arg(str)) ;
    }
    else {
      DataSet * ds = soas().stack().fromText(str);
      if(! ds)
        throw RuntimeError(QObject::tr("Not a buffer: '%1'").
                           arg(str));
      dsets << ds;
    }
  }
  return new ArgumentMarshallerChild<QList<const DataSet *> >(dsets);
}

void SeveralDataSetArgument::concatenateArguments(ArgumentMarshaller * a, 
                                                const ArgumentMarshaller * b) const
{
  a->value<QList<const DataSet *> >() += 
    b->value<QList<const DataSet *> >();
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * NumberArgument::fromString(const QString & str) const
{
  bool ok;
  double v = str.toDouble(&ok);
  if(! ok)
    throw RuntimeError(QObject::tr("Not a number: '%1'").
                       arg(str));
  return new ArgumentMarshallerChild<double>(v);
}

ArgumentMarshaller * NumberArgument::promptForValue(QWidget * base) const
{
  bool ok = false;
  QString str = 
    QInputDialog::getText(base, argumentName(), description(),
                          QLineEdit::Normal, QString(), &ok);
  if(! ok)
    throw RuntimeError("Aborted"); 
  return fromString(str);
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * SeveralNumbersArgument::fromString(const QString & str) const
{
  QStringList strs = str.split(QRegExp("\\s*,\\s*"));
  QList<double> l;
  for(int i = 0; i < strs.size(); i++) {
    bool ok;
    double v = strs[i].toDouble(&ok);
    if(! ok)
      throw RuntimeError(QObject::tr("Not a number: '%1'").
                         arg(str));
    l << v;
  }
  return new ArgumentMarshallerChild< QList<double> >(l);
}

void SeveralNumbersArgument::concatenateArguments(ArgumentMarshaller * a, 
                                                  const ArgumentMarshaller * b) const
{
  a->value<QList<double> >() += 
    b->value<QList<double> >();
}

////////////////////////////////////////////////////////////


ArgumentMarshaller * IntegerArgument::fromString(const QString & str) const
{
  bool ok;
  int v = str.toInt(&ok);
  if(! ok)
    throw RuntimeError(QObject::tr("Not a integer: '%1'").
                       arg(str));
  return new ArgumentMarshallerChild<int>(v);
}

ArgumentMarshaller * IntegerArgument::promptForValue(QWidget * base) const
{
  bool ok = false;
  QString str = 
    QInputDialog::getText(base, argumentName(), description(),
                          QLineEdit::Normal, QString(), &ok);
  if(! ok)
    throw RuntimeError("Aborted"); 
  return fromString(str);
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * SeveralIntegersArgument::fromString(const QString & str) const
{
  QStringList strs = str.split(QRegExp("\\s*,\\s*"));
  QList<int> l;
  for(int i = 0; i < strs.size(); i++) {
    bool ok;
    int v = strs[i].toInt(&ok);
    if(! ok)
      throw RuntimeError(QObject::tr("Not a number: '%1'").
                         arg(str));
    l << v;
  }
  return new ArgumentMarshallerChild< QList<int> >(l);
}

void SeveralIntegersArgument::concatenateArguments(ArgumentMarshaller * a, 
                                                  const ArgumentMarshaller * b) const
{
  a->value<QList<int> >() += 
    b->value<QList<int> >();
}
