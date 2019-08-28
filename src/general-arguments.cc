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

#include <cmath>

#include <soas.hh>
#include <datastack.hh>
#include <terminal.hh>

#include <exceptions.hh>
#include <commandcontext.hh>
#include <curveview.hh>

#include <stylegenerator.hh>
#include <regex.hh>

#include <fit.hh>
#include <mruby.hh>

#include <command.hh>

ArgumentMarshaller * StringArgument::fromString(const QString & str) const
{
  return new ArgumentMarshallerChild<QString>(str);
}

QStringList StringArgument::toString(const ArgumentMarshaller * arg) const
{
  return Argument::toString(arg);
}

QWidget * StringArgument::createEditor(QWidget * parent) const
{
  return createTextEditor(parent);
}

void StringArgument::setEditorValue(QWidget * editor, 
                                    const ArgumentMarshaller * value) const
{
  return setTextEditorValue(editor, value);
}

QString StringArgument::typeDescription() const
{
  return "Arbitrary text. If you need spaces, do not forget to quote them with ' or \"";
}

ArgumentMarshaller * StringArgument::fromRuby(mrb_value value) const
{
  return Argument::convertRubyString(value);
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

QStringList SeveralStringsArgument::toString(const ArgumentMarshaller * arg) const
{
  if(greedy)
    return arg->value<QStringList>();
  else {
    QString sep = separator.pattern();
    sep.replace("\\s*", "");
    if(sep.isEmpty())
      sep = " ";
    else {
      QRegExp pt("^\\[(.*)\\]$");
      if(pt.indexIn(sep) == 0) {
        QStringList seps = pt.cap(1).split("", QString::SkipEmptyParts);
        sep = seps[0];
      }
    }
    QStringList lst;
    lst << arg->value<QStringList>().join(sep);
    return lst;
  }
}

QString SeveralStringsArgument::typeName() const {
  return "words";
}

QString SeveralStringsArgument::typeDescription() const
{
  QString sep;
  if(separator.pattern() == " " || separator.pattern() == "\\s+")
    sep = "spaces";
  else {
    sep = separator.pattern();
    sep.replace("\\s*", "");
    QRegExp pt("^\\[(.*)\\]$");
    if(pt.indexIn(sep) == 0) {
      QStringList seps = pt.cap(1).split("", QString::SkipEmptyParts);
      sep = QString("'%1'").arg(seps.join("' or '"));
    }
    else
      sep = QString("'%1'").arg(sep);
  }
  return QString("several words, separated by %1").arg(sep);
}

ArgumentMarshaller * SeveralStringsArgument::fromRuby(mrb_value value) const
{
  MRuby * mr = MRuby::ruby();
  if(!mr->isArray(value))
    return convertRubyString(value);
  else
    return convertRubyArray(value);
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * MetaHashArgument::fromString(const QString & str) const
{
  QHash<QString, QVariant> r;
  int idx = str.indexOf('=');
  if(idx < 0)
    throw RuntimeError("Need an equal sign in '%1'").arg(str);
  QString n = str.left(idx);
  QString v = str.mid(idx+1);
  bool ok;
  double val = v.toDouble(&ok);
  if(ok)
    r[n] = val;
  else
    r[n] = v;

  return new ArgumentMarshallerChild<QHash<QString, QVariant> >(r);
}

QStringList MetaHashArgument::toString(const ArgumentMarshaller * arg) const
{
  /// @todo Shouldn't this use ValueHash conversion functions ?
  QStringList rv;
  QHash<QString, QVariant> r = arg->value<QHash<QString, QVariant> >();
  for(const QString & n : r.keys())
    rv << QString("%1=%2").arg(n).arg(r[n].toString());
  return rv;
}

void MetaHashArgument::concatenateArguments(ArgumentMarshaller * a, 
                                            const ArgumentMarshaller * b) const
{
  a->value<QHash<QString, QVariant> >().
    unite(b->value<QHash<QString, QVariant> >());
}

QString MetaHashArgument::typeName() const {
  return "meta-data";
}

QString MetaHashArgument::typeDescription() const
{
  return QString("one or more meta=value assignements");
}

ArgumentMarshaller * MetaHashArgument::fromRuby(mrb_value value) const
{
  throw InternalError("Not implemented");
  return NULL;
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

QStringList BoolArgument::toString(const ArgumentMarshaller * arg) const
{
  QStringList rv;
  if(arg->value<bool>())
    rv << "true";
  else
    rv << "false";
  return rv;
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
                                  const ArgumentMarshaller * value) const {
  QComboBox * cb = dynamic_cast<QComboBox*>(editor);
  if(! cb)
    throw InternalError("Not a combo box");

  bool val = value->value<bool>();
  if(val)
    cb->setCurrentIndex(0);
  else
    cb->setCurrentIndex(1);
}

ArgumentMarshaller * BoolArgument::fromRuby(mrb_value value) const
{
  bool val = mrb_test(value);
  return new ArgumentMarshallerChild<bool>(val);
}


static QStringList yesno = (QStringList() 
                            << "yes" << "no" 
                            << "true" << "false" 
                            << "on" << "off");

QStringList BoolArgument::proposeCompletion(const QString & starter) const
{
  return Utils::stringsStartingWith(yesno, starter);
}

QString BoolArgument::typeDescription() const
{
  return "A boolean: `yes`, `on`, `true` or `no`, `off`, `false`";
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

QStringList ChoiceArgument::toString(const ArgumentMarshaller * arg) const
{
  return Argument::toString(arg);
}

ArgumentMarshaller * ChoiceArgument::fromString(const QString & str) const
{
  QStringList c = choices();
  if(! c.contains(str))
    throw 
      RuntimeError(QObject::tr("Invalid argument: '%1'\nValid choices: `%2`").
                   arg(str).arg(c.join("`, `")));
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
  if(cs.size() > 12)
    return "(list too long)";
  return QString("One of: `%1`").arg(cs.join("`, `"));
}

ArgumentMarshaller * ChoiceArgument::fromRuby(mrb_value value) const
{
  return Argument::convertRubyString(value);
}

////////////////////////////////////////////////////////////

QStringList SeveralChoicesArgument::choices() const
{
  QStringList c = fixedChoices;
  if(provider)
    c = provider();
  qSort(c); // smart ?
  return c;
}

QStringList SeveralChoicesArgument::toString(const ArgumentMarshaller * arg) const
{
  QStringList val = arg->value<QStringList>(), lst;
  lst << val.join(sep);
  return lst;
}


ArgumentMarshaller * SeveralChoicesArgument::fromString(const QString & str) const
{
  QStringList c = choices();

  QStringList els = str.split(sep);
  for(int i = 0; i < els.size(); i++) {
    const QString & s = els[i];
    if(! c.contains(s))
      throw 
        RuntimeError(QObject::tr("Invalid argument: '%1'\nValid choices: %2").
                     arg(s).arg(c.join(", ")));
  }
  return new ArgumentMarshallerChild<QStringList>(els);
}

QStringList SeveralChoicesArgument::proposeCompletion(const QString & starter) const
{
  int idx = starter.lastIndexOf(sep);
  QString lft = starter.left(idx> 0 ? idx+1 : 0);
  QString cur = starter.mid(idx+1);
  QStringList inter = Utils::stringsStartingWith(choices(), cur);
  inter.replaceInStrings(QRegExp("^"), lft);
  return inter;
}

void SeveralChoicesArgument::concatenateArguments(ArgumentMarshaller * a, 
                                                  const ArgumentMarshaller * b) const
{
  a->value<QStringList>() += 
    b->value<QStringList>();
  
}

ArgumentMarshaller * SeveralChoicesArgument::fromRuby(mrb_value value) const
{
  MRuby * mr = MRuby::ruby();
  if(!mr->isArray(value))
    return convertRubyString(value);
  else
    return convertRubyArray(value);
}

////////////////////////////////////////////////////////////

FitNameArgument::FitNameArgument(const char * cn, const char * pn,
                                 const char * d, bool def,
                                 const char * chN) :
  ChoiceArgument(&Fit::availableFits, cn, pn, d, def, chN)
{
}

QString FitNameArgument::typeDescription() const
{
  return "The name of a fit (without the fit- prefix)";
}

////////////////////////////////////////////////////////////

SeveralFitNamesArgument::SeveralFitNamesArgument(const char * cn,
                                                 const char * pn,
                                                 const char * d, bool g,
                                                 bool def,
                                                 const char * chN) :
  SeveralChoicesArgument(&Fit::availableFits, ' ', cn, pn, d, g, def)
{
}

QString SeveralFitNamesArgument::typeDescription() const
{
  return "A series of names of a fit (without the fit- prefix), separated by spaces";
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

QString DataSetArgument::dataSetName(const DataSet * ds)
{
  int idx;
  if(! soas().stack().indexOf(ds, &idx))
    throw RuntimeError("Could not find the dataset '%1' in the stack").
      arg(ds->name);
  return QString::number(idx);
}

QStringList DataSetArgument::toString(const ArgumentMarshaller * arg) const
{
  QStringList lst;
  lst << dataSetName(arg->value<DataSet * >());
  return lst;
}

/// @todo integrate with ruby values...
ArgumentMarshaller * DataSetArgument::fromRuby(mrb_value value) const
{
  return Argument::convertRubyString(value);
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * SeveralDataSetArgument::fromString(const QString & s) const
{
  return new
    ArgumentMarshallerChild<QList<const DataSet *> >
    (soas().stack().datasetsFromSpec(s));
}

void SeveralDataSetArgument::concatenateArguments(ArgumentMarshaller * a, 
                                                const ArgumentMarshaller * b) const
{
  a->value<QList<const DataSet *> >() += 
    b->value<QList<const DataSet *> >();
}

QStringList SeveralDataSetArgument::proposeCompletion(const QString & starter) const
{
  QStringList all;
  all << "displayed" << "all" << "latest";
  QSet<QString> st = soas().stack().definedFlags();
  for(auto i = st.begin(); i != st.end(); i++) {
    const QString &s = *i;
    all << QString("flagged:%1").arg(s)
        << QString("unflagged:%1").arg(s)
        << QString("flagged-:%1").arg(s)
        << QString("unflagged-:%1").arg(s);
  }
  st = soas().stack().datasetNames();
  for(const QString & n : st)
    all << QString("named:%1").arg(n);
  return Utils::stringsStartingWith(all, starter);
}

ArgumentMarshaller * SeveralDataSetArgument::fromRuby(mrb_value value) const
{
  MRuby * mr = MRuby::ruby();
  if(!mr->isArray(value))
    return convertRubyString(value);
  else
    return convertRubyArray(value);

}

QStringList SeveralDataSetArgument::toString(const ArgumentMarshaller * arg) const
{
  QList<const DataSet *> lst;
  QStringList t;
  for(const DataSet * ds : lst)
    t << DataSetArgument::dataSetName(ds);
  
  QStringList rv;
  rv << t.join(",");
  return rv;
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * NumberArgument::fromString(const QString & str) const
{
  double v;
  if(special && str == "*")
    v = NAN;
  else if(special && str == "=")
    v = INFINITY;
  else
    v = Utils::stringToDouble(str);
  return new ArgumentMarshallerChild<double>(v);
}

QStringList NumberArgument::toString(const ArgumentMarshaller * arg) const
{
  QStringList lst;
  lst << QString::number(arg->value<double>(), 'g', 12);
  return lst;
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
                                    const ArgumentMarshaller * value) const
{
  QLineEdit * le = dynamic_cast<QLineEdit*>(editor);
  if(! le)
    throw InternalError("Wrong editor given to setEditorValue");

  le->setText(QString::number(value->value<double>()));
}

ArgumentMarshaller * NumberArgument::fromRuby(mrb_value value) const
{
  MRuby * mr = MRuby::ruby();
  return new ArgumentMarshallerChild<double>(mr->floatValue(value));
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

ArgumentMarshaller * SeveralNumbersArgument::fromRuby(mrb_value value) const
{
  QList<double> rv;
  MRuby * mr = MRuby::ruby();
  mr->arrayIterate(value, [&rv, mr](mrb_value val) {
      rv << mr->floatValue(val);
    });
  
  return new ArgumentMarshallerChild< QList<double> >(rv);
}

QStringList SeveralNumbersArgument::toString(const ArgumentMarshaller * arg) const
{
  QList<double> values = arg->value<QList<double> >();
  QStringList lst;
  for(const double & d : values)
    lst << QString::number(d, 'g', 12);
  QStringList rv;
  rv << lst.join(delim);
  return rv;
}

////////////////////////////////////////////////////////////


ArgumentMarshaller * IntegerArgument::fromString(const QString & str) const
{
  return new ArgumentMarshallerChild<int>(Utils::stringToInt(str));
}

QStringList IntegerArgument::toString(const ArgumentMarshaller * arg) const
{
  QStringList lst;
  lst << QString::number(arg->value<int>());
  return lst;
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
                                     const ArgumentMarshaller * value) const
{
  QLineEdit * le = dynamic_cast<QLineEdit*>(editor);
  if(! le)
    throw InternalError("Wrong editor given to setEditorValue");

  le->setText(QString::number(value->value<int>()));
}

ArgumentMarshaller * IntegerArgument::fromRuby(mrb_value value) const
{
  int v = mrb_fixnum(value);
  return new ArgumentMarshallerChild<int>(v);
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

ArgumentMarshaller * SeveralIntegersArgument::fromRuby(mrb_value value) const
{
  QList<int> rv;
  MRuby * mr = MRuby::ruby();
  mr->arrayIterate(value, [&rv](mrb_value val) {
      rv << mrb_fixnum(val);
    });
  return new ArgumentMarshallerChild< QList<int> >(rv);
}

QStringList SeveralIntegersArgument::toString(const ArgumentMarshaller * arg) const
{
  QList<int> vals = arg->value<QList<int> >();
  QStringList lst, rv;
  for(int i : vals)
    lst << QString::number(i);
  rv << lst.join(",");
  return rv;
}

//////////////////////////////////////////////////////////////////////

// ArgumentMarshaller * ParametersHashArgument::fromString(const QString & str) const
// {
//   QHash<QString, double> v;

//   // We split on ; by default

//   QStringList elems = str.split(QRegExp("\\s*;\\s*"));
  
//   for(int i = 0; i < elems.size(); i++) {
//     QStringList lst = elems[i].split(delims);
//     if(lst.size() != 2)
//       throw RuntimeError(QString("Invalid parameter specification: '%1'").
//                          arg(str));
//     v[lst[0]] = Utils::stringToDouble(lst[1]);
//   }
//   return new ArgumentMarshallerChild< QHash<QString, double> >(v);
// }

// void ParametersHashArgument::concatenateArguments(ArgumentMarshaller * a, 
//                                                   const ArgumentMarshaller * b) const
// {
//   a->value< QHash<QString, double> >().
//     unite(b->value<QHash<QString, double> >());
// }

// ArgumentMarshaller * ParametersHashArgument::fromRuby(mrb_value value) const
// {
//   MRuby * mr = MRuby::ruby();
//   if(!mr->isArray(value))
//     return convertRubyString(value);
//   else
//     return convertRubyArray(value);
// }

//////////////////////////////////////////////////////////////////////

CommandArgument::CommandArgument(const char * cn, const char * pn,
                                 const char * d, bool def)
  : ChoiceArgument([]() -> QStringList {
      return soas().commandContext().allCommands();
    }, cn, pn, d, def) {
}; 

ArgumentMarshaller * CommandArgument::fromString(const QString & str) const
{
  Command * cmd = soas().commandContext().namedCommand(str);
  if(! cmd)
    throw RuntimeError("Invalid command: %1").arg(str);
  return new ArgumentMarshallerChild<Command *>(cmd);
}

QStringList CommandArgument::toString(const ArgumentMarshaller * arg) const
{
  QStringList rv;
  rv << arg->value<Command *>()->commandName();
  return rv;
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

QString RegexArgument::typeName() const
{
  return "pattern";
}

QString RegexArgument::typeDescription() const
{
  return "plain text, or [regular expressions](#regexps) enclosed within / / delimiters";
}

ArgumentMarshaller * RegexArgument::fromRuby(mrb_value value) const
{
  return convertRubyString(value);
}

QStringList RegexArgument::toString(const ArgumentMarshaller * arg) const
{
  QStringList rv;
  rv << arg->value<Regex>().patternString();
  return rv;
}


//////////////////////////////////////////////////////////////////////

int ColumnArgument::parseFromText(const QString & str)
{
  QRegExp num1("^\\s*\\d+\\s*$");
  QRegExp num2("^\\s*#(\\d+)\\s*$");

  QRegExp name("^\\s*((x)|(y)|(z)|(y(\\d+))|(non?e?))\\s*$", Qt::CaseInsensitive);
  
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
    else if(! name.cap(7).isEmpty())
      return -1;                // None
  }
  throw RuntimeError("Invalid buffer column specification '%1'").arg(str);
}

ArgumentMarshaller * ColumnArgument::fromString(const QString & str) const
{
  return new ArgumentMarshallerChild<int>(parseFromText(str));
}

QStringList ColumnArgument::toString(const ArgumentMarshaller * arg) const
{
  QStringList rv;
  rv << QString("#%1").arg(arg->value<int>());
  return rv;
}

QString ColumnArgument::typeDescription() const
{
  return "The [number/name of a column](#column-names) in a buffer";
}

ArgumentMarshaller * ColumnArgument::fromRuby(mrb_value value) const
{
  return Argument::convertRubyString(value);
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

QStringList SeveralColumnsArgument::toString(const ArgumentMarshaller * arg) const
{
  QStringList lst, rv;
  QList<int> vals = arg->value<QList<int> >();
  for(int i : vals)
    lst << QString("#%1").arg(i);
  rv << lst.join(",");
  return rv;
}

void SeveralColumnsArgument::concatenateArguments(ArgumentMarshaller * a, 
                                                  const ArgumentMarshaller * b) const
{
  a->value<QList<int> >() += 
    b->value<QList<int> >();
}

QString SeveralColumnsArgument::typeDescription() const
{
  return "A comma-separated list of [columns names](#column-names)";
}

ArgumentMarshaller * SeveralColumnsArgument::fromRuby(mrb_value value) const
{
  MRuby * mr = MRuby::ruby();
  if(!mr->isArray(value))
    return convertRubyString(value);
  else
    return convertRubyArray(value);
}
