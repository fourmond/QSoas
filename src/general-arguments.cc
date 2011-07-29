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

ArgumentMarshaller * FileArgument::fromString(const QString & str) const
{
  return new ArgumentMarshallerChild<QString>(str);
}

ArgumentMarshaller * FileArgument::promptForValue(QWidget * base) const
{
  QString file = QFileDialog::getOpenFileName(base, publicName());
  if(file.isEmpty())
    throw std::runtime_error("Aborted"); 
  /// @todo Maybe use a specific exception to signal abortion ?
  return fromString(file);
}


QStringList FileArgument::proposeCompletion(const QString & starter) const
{
  return Utils::glob(starter + "*");
}

////////////////////////////////////////////////////////////

ArgumentMarshaller * SeveralFilesArgument::fromString(const QString & str) const
{
  return new ArgumentMarshallerChild<QStringList>(Utils::glob(str, false));
}

ArgumentMarshaller * SeveralFilesArgument::promptForValue(QWidget * base) const
{
  QStringList files = QFileDialog::getOpenFileNames(base, publicName());
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
  return Utils::glob(starter + "*");
}
