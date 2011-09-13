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

/// A utility function for a clean file completion.
static QStringList proposeFileCompletion(const QString & str)
{
  QStringList candidates = Utils::glob(str + "*");
  for(int i = 0; i < candidates.size(); i++)
    candidates[i] = QDir::cleanPath(candidates[i]);
  if(candidates.size() == 1) {
    QFileInfo info(candidates.first());
    if(info.isDir())
      candidates.first().append("/");
    candidates << candidates.first();
  }
  return candidates;
}

ArgumentMarshaller * FileArgument::fromString(const QString & str) const
{
  return new ArgumentMarshallerChild<QString>(str);
}

ArgumentMarshaller * FileArgument::promptForValue(QWidget * base) const
{
  QString file = 
    QFileDialog::getOpenFileName(base, publicName(),
                                 QDir::currentPath());
  if(file.isEmpty())
    throw std::runtime_error("Aborted"); 
  /// @todo Maybe use a specific exception to signal abortion ?
  return fromString(file);
}


QStringList FileArgument::proposeCompletion(const QString & starter) const
{
  return proposeFileCompletion(starter);
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * FileSaveArgument::fromString(const QString & str) const
{
  if(askOverwrite && QFile::exists(str)) {
    QString s = QObject::tr("Overwrite file '%1' ?").
      arg(str);
    if(! Utils::askConfirmation(s))
      throw std::runtime_error("Aborted");
  }
  return new ArgumentMarshallerChild<QString>(str);
}


ArgumentMarshaller * FileSaveArgument::promptForValue(QWidget * base) const
{
  QString def = defaultName;
  if(provider)
    def = provider();

  QFileDialog fd(base, publicName(), QDir::currentPath());
  fd.setAcceptMode(QFileDialog::AcceptSave);
  if(! def.isEmpty())
    fd.selectFile(def);

  if(fd.exec() != QDialog::Accepted)
    throw std::runtime_error("Aborted"); 

  return fromString(fd.selectedFiles().value(0, QString("")));
}


////////////////////////////////////////////////////////////

ArgumentMarshaller * SeveralFilesArgument::fromString(const QString & str) const
{
  return new ArgumentMarshallerChild<QStringList>(Utils::glob(str, false));
}

ArgumentMarshaller * SeveralFilesArgument::promptForValue(QWidget * base) const
{
  QStringList files = 
    QFileDialog::getOpenFileNames(base, publicName(),
                                  QDir::currentPath());
  if(! files.size())
    throw std::runtime_error("Aborted"); 
  return new ArgumentMarshallerChild<QStringList>(files);
}

void SeveralFilesArgument::concatenateArguments(ArgumentMarshaller * a, 
                                                const ArgumentMarshaller * b) const
{
  a->value<QStringList>() += b->value<QStringList>();
}

QStringList SeveralFilesArgument::proposeCompletion(const QString & starter) const
{
  return proposeFileCompletion(starter);
}

////////////////////////////////////////////////////////////


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
    throw std::runtime_error("Aborted"); 
  return fromString(str);
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * DataSetArgument::fromString(const QString & str) const
{
  DataSet * ds = soas().stack().fromText(str);

  if(! ds) {
    QString s = QObject::tr("Not a buffer: '%1'").
      arg(str);
    throw std::runtime_error(s.toStdString());
  }
  return new ArgumentMarshallerChild<DataSet *>(ds);
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * SeveralDataSetArgument::fromString(const QString & s) const
{
  QStringList splitted = s.split(QRegExp("\\s*,\\s*"));
  QList<const DataSet *> dsets;
  QRegExp multi("^\\s*(-?[0-9]+)\\s*..\\s*(-?[0-9]+)\\s*$");

  for(int i = 0; i < splitted.size(); i++) {
    const QString & str = splitted[i];
    
    if(multi.indexIn(str) == 0) {
      int first = multi.cap(1).toInt();
      int last = multi.cap(2).toInt();
      int delta = (first < last ? 1 : -1);
      do {
        DataSet * ds = soas().stack().numberedDataSet(first);
        if(! ds)
          Terminal::out << "No such buffer number : " << first << endl;
        else
          dsets << ds;
        first += delta;
      }
      while(first != (last + delta));

      if(dsets.size() == 0) {
        QString s = QObject::tr("Buffer range '%1' corresponds to no buffers").
          arg(str);
        throw std::runtime_error(s.toStdString());
      }
    }
    else {
      DataSet * ds = soas().stack().fromText(str);
      if(! ds) {
        QString s = QObject::tr("Not a buffer: '%1'").
          arg(str);
        throw std::runtime_error(s.toStdString());
      }
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
  if(! ok) {
    QString s = QObject::tr("Not a number: '%1'").
      arg(str);
    throw std::runtime_error(s.toStdString());
  }
  return new ArgumentMarshallerChild<double>(v);
}

ArgumentMarshaller * NumberArgument::promptForValue(QWidget * base) const
{
  bool ok = false;
  QString str = 
    QInputDialog::getText(base, argumentName(), description(),
                          QLineEdit::Normal, QString(), &ok);
  if(! ok)
    throw std::runtime_error("Aborted"); 
  return fromString(str);
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * SeveralNumbersArgument::fromString(const QString & str) const
{
  bool ok;
  double v = str.toDouble(&ok);
  if(! ok) {
    QString s = QObject::tr("Not a number: '%1'").
      arg(str);
    throw std::runtime_error(s.toStdString());
  }
  QList<double> l;
  l << v;
  return new ArgumentMarshallerChild< QList<double> >(l);
}

void SeveralNumbersArgument::concatenateArguments(ArgumentMarshaller * a, 
                                                  const ArgumentMarshaller * b) const
{
  a->value<QList<double> >() += 
    b->value<QList<double> >();
}
