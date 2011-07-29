/*
  databackend.cc: implementation of the DataBackend factory class
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
#include <databackend.hh>

QList<DataBackend*> * DataBackend::availableBackends = NULL;

void DataBackend::registerBackend(DataBackend * backend)
{
  if(! availableBackends)
    availableBackends = new QList<DataBackend *>;
  availableBackends->append(backend);
}



DataBackend * DataBackend::backendForStream(QIODevice * stream,
                                            const QString & fileName)
{
  /// @todo Error handling for peek ?
  QByteArray head = stream->peek(4096); // Should be enough to get
                                        // past most headers ?
  DataBackend * backend = NULL;
  int priority = 0;
  QTextStream o(stdout);
  if(! availableBackends)
    return NULL;
  for(int i = 0; i < availableBackends->size(); i++) {
    DataBackend * b = availableBackends->value(i);
    int p = b->couldBeMine(head, fileName);
    /// @todo Maybe find a way to document which backend wins
    /// add a command for just checking which backend does ?
    // o << "Backend: " << b->name << " -> " << p << endl;
    if(p >= 1000)
      return b;
    if(p > priority) {
      priority = p;
      backend = b;
    }
  }
  return backend;
}

DataSet * DataBackend::loadFile(const QString & fileName, 
                                const QString & options, 
                                const QString & backend)
{
  /// @todo implement backend manual selection.
  QFile file(fileName);
  if(! file.open(QIODevice::ReadOnly)) {
    QString str = QObject::tr("Failed to load file %1: %2").
      arg(fileName).arg(file.errorString());
    throw std::runtime_error(str.toStdString());
  }
  DataBackend * b = backendForStream(&file, fileName);
  if(! b) {
    QString str = QObject::tr("No backend found to load %1").
      arg(fileName);
    throw std::runtime_error(str.toStdString());
  }
  return b->readFromStream(&file, fileName, options);
}
