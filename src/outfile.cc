/*
  outfile.cc: implementation of the interface to the outfile
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
#include <outfile.hh>
#include <terminal.hh>

#include <utils.hh>
#include <dataset.hh>
#include <valuehash.hh>

OutFile OutFile::out("out.dat");

OutFile::OutFile(const QString &n) :
  internalStream(NULL), name(n), output(NULL)
{
}

void OutFile::ensureOpened()
{
  if(! output) {
    name = Utils::expandTilde(name);
    output = new QFile(name);

    QIODevice::OpenMode mode = QIODevice::ReadWrite | QIODevice::Text;
    if(truncate) {
      truncate = false;
      mode |= QIODevice::Truncate;
    }
    if(! output->open(mode))
      Terminal::out << "Failed to open output file '" 
                    << name << "'" << endl;
    else
      output->seek(output->size());

    fullFilePath = QDir::current().absoluteFilePath(name);

    Terminal::out << "Opening output file '" 
                  << fullFilePath << "'" << endl;

    internalStream = new QTextStream(output);
  }
  if(! internalStream)
    internalStream = new QTextStream(output);
}

void OutFile::setHeader(const QString & head)
{
  if(head != currentHeader || (! isOpened()))
    (*this) << "\n" << head << "\n";
  currentHeader = head;
}

OutFile::~OutFile()
{
  delete internalStream;
  delete output;
}

void OutFile::writeValueHash(const ValueHash & h, const DataSet * ds,
                             const QString & comment)
{
  ValueHash hsh(h);
  hsh.keyOrder += desiredMeta;
  if(ds) {
    ValueHash meta = ds->getMetaData();
    if(desiredMeta.contains("Z") && ds->perpendicularCoordinates().size() > 0) {
      // Z always means perpendicular coordinate
      meta["Z"] = ds->perpendicularCoordinates()[0]; // That of the
                                                     // first Y column
    }
    meta.keyOrder.clear();
    hsh.merge(meta, false);
  }
  QString hd = QString("## %1").arg(hsh.keyOrder.join("\t"));
  if(! comment.isEmpty())
    hd = QString("# %1\n").arg(comment) + hd;
  setHeader(hd);
  (*this) << hsh.toString("\t", "x", true) << "\n" << flush;
}

bool OutFile::isOpened() const
{
  return internalStream && output;
}

void OutFile::setFileName(const QString & nn)
{
  if(internalStream && output) {
    delete internalStream;
    internalStream = NULL;
    delete output;
    output = NULL;
    currentHeader = QString();  // else we lose the headers upon file change
  }
  name = nn;
}
