/*
  general-arguments.cc:
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
#include <general-arguments.hh>
#include <argumentmarshaller.hh>
#include <utils.hh>

#include <soas.hh>
#include <datastack.hh>
#include <terminal.hh>

#include <exceptions.hh>
#include <command.hh>
#include <curveview.hh>

#include <stylegenerator.hh>
#include <regex.hh>

ArgumentMarshaller * StringArgument::fromString(const QString & str) const
{
  return new ArgumentMarshallerChild<QString>(str);
}

QWidget * StringArgument::createEditor(QWidget * parent) const
{
  return new QLineEdit(parent);
}

void StringArgument::setEditorValue(QWidget * editor, 
                                    ArgumentMarshaller * value) const
{
  QLineEdit * le = dynamic_cast<QLineEdit*>(editor);
  if(! le)
    throw InternalError("Wrong editor given to setEditorValue");

  le->setText(value->value<QString>());
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * SeveralStringsArgument::fromString(const QString & str) const
{
  QStringList r;
  if(separator.isEmpty())
    r << str;
  else
    r = str.split(separator);
  return new ArgumentMarshallerChild<QStringList>(r);
}

void SeveralStringsArgument::concatenateArguments(ArgumentMarshaller * a, 
                                                  const ArgumentMarshaller * b) const
{
  a->value<QStringList>() += 
    b->value<QStringList>();
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

QWidget * BoolArgument::createEditor(QWidget * parent) const {
  QComboBox * cb = new QComboBox(parent);
  cb->addItem("yes");
  cb->addItem("no");
  return cb;
}

void BoolArgument::setEditorValue(QWidget * editor, 
                                  ArgumentMarshaller * value) const {
  QComboBox * cb = dynamic_cast<QComboBox*>(editor);
  if(! cb)
    throw InternalError("Not a combo box");

  bool val = value->value<bool>();
  if(val)
    cb->setCurrentIndex(0);
  else
    cb->setCurrentIndex(1);
}


static QStringList yesno = (QStringList() 
                            << "yes" << "no" 
                            << "true" << "false" 
                            << "on" << "off");

QStringList BoolArgument::proposeCompletion(const QString & starter) const
{
  return Utils::stringsStartingWith(yesno, starter);
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

QString ChoiceArgument::typeName() const
{
  if(choiceName.isEmpty())
    return "choice";
  return choiceName;
}

QString ChoiceArgument::typeDescription() const
{
  QStringList cs = choices();
  if(cs.size() > 7)
    return "";
  return QString("One of: %1").arg(cs.join(", "));
}


////////////////////////////////////////////////////////////

QStringList SeveralChoicesArgument::choices() const
{
  QStringList c = fixedChoices;
  if(provider)
    c = provider();
  qSort(c);
  return c;
}

ArgumentMarshaller * SeveralChoicesArgument::fromString(const QString & str) const
{
  QStringList c = choices();
  if(! c.contains(str))
    throw 
      RuntimeError(QObject::tr("Invalid argument: '%1'\nValid choices: %2").
                   arg(str).arg(c.join(", ")));
  QStringList r;
  r << str;
  return new ArgumentMarshallerChild<QStringList>(r);
}

QStringList SeveralChoicesArgument::proposeCompletion(const QString & starter) const
{
  return Utils::stringsStartingWith(choices(), starter);
}

void SeveralChoicesArgument::concatenateArguments(ArgumentMarshaller * a, 
                                                  const ArgumentMarshaller * b) const
{
  a->value<QStringList>() += 
    b->value<QStringList>();
  
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

/// @todo This function belongs in DataStack and not here !
ArgumentMarshaller * SeveralDataSetArgument::fromString(const QString & s) const
{
  QStringList splitted = s.split(QRegExp("\\s*,\\s*"));
  QList<const DataSet *> dsets;
  QRegExp multi("^\\s*(-?[0-9]+)\\s*..\\s*(-?[0-9]+|end)\\s*(?::(\\d+)\\s*)?");

  QRegExp flgs("^\\s*(un)?flagged(-)?(:(.*))?\\s*$");
  flgs.setMinimal(true);        // to catch the last - if applicable

  for(int i = 0; i < splitted.size(); i++) {
    const QString & str = splitted[i];
    
    if(multi.indexIn(str) == 0 || str == "all") {
      int first;
      int last;
      if(str == "all") {
        first = -soas().stack().redoStackSize();
        last = soas().stack().stackSize()-1;
      }
      else {
        first = multi.cap(1).toInt();
        last = (multi.cap(2) == "end" ? soas().stack().stackSize()-1 : 
                multi.cap(2).toInt());
      }
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
    else if(flgs.indexIn(str) == 0) {
      bool flg = (flgs.cap(1).size() == 0);
      bool dec = (flgs.cap(2).size() > 0); // the - sign at the end
      
      QString flagName = flgs.cap(4);
      QList<DataSet *> mkd;
      if(flagName.isEmpty())
        mkd = soas().stack().flaggedDataSets(flg);
      else
        mkd = soas().stack().flaggedDataSets(flg, flagName);

      if(dec)
        Utils::reverseList(mkd);

      for(int i = 0; i < mkd.size(); i++)
        dsets << mkd[i];
    }
      
    else if(str == "displayed")  {
      QList<DataSet *> mkd = soas().view().displayedDataSets();
      for(int i = 0; i < mkd.size(); i++)
        dsets << mkd[i];
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
  return new ArgumentMarshallerChild<double>(Utils::stringToDouble(str));
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

QWidget * NumberArgument::createEditor(QWidget * parent) const
{
  return new QLineEdit(parent);
}

void NumberArgument::setEditorValue(QWidget * editor, 
                                    ArgumentMarshaller * value) const
{
  QLineEdit * le = dynamic_cast<QLineEdit*>(editor);
  if(! le)
    throw InternalError("Wrong editor given to setEditorValue");

  le->setText(QString::number(value->value<double>()));
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * SeveralNumbersArgument::fromString(const QString & str) const
{
  QStringList strs = str.split(QRegExp(QString("\\s*%1\\s*").arg(delim)));
  QList<double> l;
  for(int i = 0; i < strs.size(); i++)
    l << Utils::stringToDouble(strs[i]);
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
  return new ArgumentMarshallerChild<int>(Utils::stringToInt(str));
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

QWidget * IntegerArgument::createEditor(QWidget * parent) const
{
  return new QLineEdit(parent);
}

void IntegerArgument::setEditorValue(QWidget * editor, 
                                     ArgumentMarshaller * value) const
{
  QLineEdit * le = dynamic_cast<QLineEdit*>(editor);
  if(! le)
    throw InternalError("Wrong editor given to setEditorValue");

  le->setText(QString::number(value->value<int>()));
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * SeveralIntegersArgument::fromString(const QString & str) const
{
  QStringList strs = str.split(QRegExp("\\s*,\\s*"));
  QList<int> l;
  for(int i = 0; i < strs.size(); i++)
    l << Utils::stringToInt(strs[i]);

  return new ArgumentMarshallerChild< QList<int> >(l);
}

void SeveralIntegersArgument::concatenateArguments(ArgumentMarshaller * a, 
                                                  const ArgumentMarshaller * b) const
{
  a->value<QList<int> >() += 
    b->value<QList<int> >();
}

//////////////////////////////////////////////////////////////////////

ArgumentMarshaller * ParametersHashArgument::fromString(const QString & str) const
{
  QHash<QString, double> v;

  // We split on ; by default

  QStringList elems = str.split(QRegExp("\\s*;\\s*"));
  
  for(int i = 0; i < elems.size(); i++) {
    QStringList lst = elems[i].split(delims);
    if(lst.size() != 2)
      throw RuntimeError(QString("Invalid parameter specification: '%1'").
                         arg(str));
    v[lst[0]] = Utils::stringToDouble(lst[1]);
  }
  return new ArgumentMarshallerChild< QHash<QString, double> >(v);
}

void ParametersHashArgument::concatenateArguments(ArgumentMarshaller * a, 
                                                  const ArgumentMarshaller * b) const
{
  a->value< QHash<QString, double> >().
    unite(b->value<QHash<QString, double> >());
}

//////////////////////////////////////////////////////////////////////

CommandArgument::CommandArgument(const char * cn, const char * pn,
                                 const char * d, bool def)
  : ChoiceArgument(Command::allCommands, cn, pn, d, def) {
}; 

ArgumentMarshaller * CommandArgument::fromString(const QString & str) const
{
  Command * cmd = Command::namedCommand(str);
  if(! cmd)
    throw RuntimeError("Invalid command: %1").arg(str);
  return new ArgumentMarshallerChild<Command *>(cmd);
}

//////////////////////////////////////////////////////////////////////

StyleGeneratorArgument::StyleGeneratorArgument(const char * cn, const char * pn,
                                 const char * d, bool def)
  : ChoiceArgument(StyleGenerator::availableGenerators, cn, pn, d, def) {
}; 

// No verification done here.
ArgumentMarshaller * StyleGeneratorArgument::fromString(const QString & str) const
{
  return new ArgumentMarshallerChild<QString>(str);
}

//////////////////////////////////////////////////////////////////////

ArgumentMarshaller * RegexArgument::fromString(const QString & str) const
{
  return new ArgumentMarshallerChild<Regex>(str);
}

//////////////////////////////////////////////////////////////////////

int ColumnArgument::parseFromText(const QString & str)
{
  QRegExp num1("^\\s*\\d+\\s*$");
  QRegExp num2("^\\s*#(\\d+)\\s*$");

  QRegExp name("^\\s*((x)|(y)|(z)|(y(\\d+)))\\s*$", Qt::CaseInsensitive);
  
  if(num1.indexIn(str, 0) >= 0) {
    int nb = str.toInt() - 1;
    if(nb >= 0)
      return nb;
  }
  if(num2.indexIn(str, 0) >= 0) {
    return num2.cap(1).toInt();
  }

  if(name.indexIn(str, 0) >= 0) {
    if(! name.cap(2).isEmpty())
      return 0;                 // X
    else if(! name.cap(3).isEmpty())
      return 1;                 // Y
    else if(! name.cap(4).isEmpty())
      return 2;                 // Z
    else if(! name.cap(5).isEmpty())
      return name.cap(6).toInt();                 // Yn
  }
  throw RuntimeError("Invalid buffer column specification '%1'").arg(str);
}

ArgumentMarshaller * ColumnArgument::fromString(const QString & str) const
{
  return new ArgumentMarshallerChild<int>(parseFromText(str));
}

//////////////////////////////////////////////////////////////////////

ArgumentMarshaller * SeveralColumnsArgument::fromString(const QString & str) const
{
  QStringList elems = str.split(",");

  QRegExp range("(.*)\\.\\.(.*)");

  QList<int> ret;
  for(int i = 0; i < elems.size(); i++) {
    const QString & s = elems[i];
    if(range.indexIn(s, 0) >= 0) {
      int fst = ColumnArgument::parseFromText(range.cap(1));
      int lst = ColumnArgument::parseFromText(range.cap(2));
      int dx = (fst > lst ? -1 : 1);
      for(int j = fst; j != lst; j+= dx)
        ret << j;
      ret << lst;
    }
    else
      ret << ColumnArgument::parseFromText(s);
  }

  return new ArgumentMarshallerChild<QList<int> >(ret);
}

void SeveralColumnsArgument::concatenateArguments(ArgumentMarshaller * a, 
                                                  const ArgumentMarshaller * b) const
{
  a->value<QList<int> >() += 
    b->value<QList<int> >();
}
