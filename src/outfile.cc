/*
  outfile.cc: implementation of the interface to the outfile
  Copyright 2010, 2011 by Vincent Fourmond

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
#include <outfile.hh>
#include <terminal.hh>

OutFile OutFile::out("out.dat");

OutFile::OutFile(const QString &n) :
  internalStream(NULL), name(n), output(NULL)
{
}

void OutFile::ensureOpened()
{
  if(! output) {
    output = new QFile(name);
    if(! output->open(QIODevice::WriteOnly | QIODevice::Text))
      Terminal::out << "Failed to open output file '" 
                    << name << "'" << endl;
    output->seek(output->size());
  }
  internalStream = new QTextStream(output);
}

void OutFile::setHeader(const QString & head)
{
  if(head != currentHeader)
    (*this) << "\n" << head << "\n";
  currentHeader = head;
}

OutFile::~OutFile()
{
  delete internalStream;
  delete output;
}

