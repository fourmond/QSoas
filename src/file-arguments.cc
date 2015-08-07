/*
  file-arguments.cc:
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
#include <file-arguments.hh>
#include <argumentmarshaller.hh>
#include <utils.hh>

#include <exceptions.hh>

/// A utility function for a clean file completion.
static QStringList proposeFileCompletion(const QString & str, 
                                         bool isDir = false)
{
  QStringList candidates = Utils::glob(str + "*", true, isDir);
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

  QString file;
  if(isDir)
    file = QFileDialog::getExistingDirectory(base, publicName(),
                                             QDir::currentPath());
  else
    file = QFileDialog::getOpenFileName(base, publicName(),
                                        QDir::currentPath());
  if(file.isEmpty())
    throw RuntimeError("Aborted"); 
  /// @todo Maybe use a specific exception to signal abortion ?
  return fromString(file);
}


QStringList FileArgument::proposeCompletion(const QString & starter) const
{
  return proposeFileCompletion(starter, isDir);
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * FileSaveArgument::fromString(const QString & str) const
{
  if(askOverwrite)
    Utils::confirmOverwrite(str);
  return new ArgumentMarshallerChild<QString>(str);
}


ArgumentMarshaller * FileSaveArgument::promptForValue(QWidget * base) const
{
  QString def = defaultName;
  if(provider)
    def = provider();

  QFileDialog fd(base, publicName(), QDir::currentPath());
  fd.setAcceptMode(QFileDialog::AcceptSave);

  // We disable confirmation for overwriting here, as confirmation is
  // systematically asked elsewhere.
  fd.setOption(QFileDialog::DontConfirmOverwrite, true);
  if(! def.isEmpty())
    fd.selectFile(def);

  if(fd.exec() != QDialog::Accepted)
    throw RuntimeError("Aborted"); 

  return fromString(fd.selectedFiles().value(0, QString("")));
}


////////////////////////////////////////////////////////////

ArgumentMarshaller * SeveralFilesArgument::fromString(const QString & str) const
{
  
  QStringList rv;
  if(expandGlobs) {
    rv = Utils::glob(str, false);
    for(int i = 0; i < rv.size(); i++)
      rv[i] = QDir::cleanPath(rv[i]);
  }
  else
    rv << str;
  
  return new ArgumentMarshallerChild<QStringList>(rv);
}

ArgumentMarshaller * SeveralFilesArgument::promptForValue(QWidget * base) const
{
  QStringList files = 
    QFileDialog::getOpenFileNames(base, publicName(),
                                  QDir::currentPath());
  if(! files.size())
    throw RuntimeError("Aborted"); 
  return new ArgumentMarshallerChild<QStringList>(files);
}

void SeveralFilesArgument::concatenateArguments(ArgumentMarshaller * a, 
                                                const ArgumentMarshaller * b) const
{
  a->value<QStringList>() += b->value<QStringList>();
}

QStringList SeveralFilesArgument::proposeCompletion(const QString & starter) const
{
  return proposeFileCompletion(starter, false);
}

