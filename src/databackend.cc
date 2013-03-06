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
#include <utils.hh>

#include <dataset.hh>
#include <terminal.hh>

#include <commandeffector-templates.hh>
#include <file-arguments.hh>
#include <command.hh>

#include <exceptions.hh>

#include <soas.hh>
#include <curveview.hh>
#include <datastack.hh>

QList<DataBackend*> * DataBackend::availableBackends = NULL;

// A 30 MB cache
QCache<QString, DataSet> DataBackend::cachedDatasets(30*1024*1024);

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

DataSet * DataBackend::loadFile(const QString & fileName, bool verbose)
{
  /// @todo implement backend manual selection.
  QFile file(fileName);

  QFileInfo info(fileName);
  QString key = info.canonicalFilePath();
  QDateTime lastModified = info.lastModified();

  DataSet * ds = NULL;

  
  if(cachedDatasets.contains(key)) {
    ds = cachedDatasets.take(key);
    if(ds->date < lastModified) {
      delete ds;
      ds = NULL;
    }
  }

  if(verbose)
    Terminal::out << "Loading file: '" << fileName << "' ";

  if(! ds) {
    Utils::open(&file, QIODevice::ReadOnly);

    DataBackend * b = backendForStream(&file, fileName);
    if(! b)
      throw RuntimeError(QObject::tr("No backend found to load '%1'").
                         arg(fileName));
    ds = b->readFromStream(&file, fileName, CommandOptions());

    if(verbose)
      Terminal::out << "using backend " << b->name << endl;

  }
  else 
    if(verbose)
      Terminal::out << "(cached)" << endl;
  
  if(!ds->date.isValid())
    ds->date = lastModified;

  // We insert a copy in the cache 
  DataSet * dscopy = new DataSet(*ds);
  if(! cachedDatasets.insert(key, dscopy, ds->byteSize()))
    delete dscopy;              // Delete if add failed
  return ds;
}

ArgumentList * DataBackend::loadOptions() const
{
  return NULL;
}

void DataBackend::registerBackendCommands()
{
  ArgumentList * lst = 
    new ArgumentList(QList<Argument *>() 
                     << new SeveralFilesArgument("file", 
                                                 "File",
                                                 "Files to load !", true
                                                 ));

  for(int i = 0; i < availableBackends->size(); i++) {
    DataBackend * b = availableBackends->value(i);
    ArgumentList * opts = b->loadOptions();

    /// @todo Add general options processing.
    QString name = "load-as-" + b->name;

    QString d1 = QString("Load files with backend '%1'").arg(b->name);
    QString d2 = QString("Load any number of files directly using the backend "
                         "'%1', bypassing cache andautomatic backend "
                         "detection, and "
                         "giving more fine-tuning in the loading via the "
                         "use of dedicated options").arg(b->name);

    new Command(name.toLocal8Bit(),
                effector(b, &DataBackend::loadDatasetCommand),
                "stack", lst, opts, (const char*) d1.toLocal8Bit(), 
                (const char*) d1.toLocal8Bit(), 
                (const char*) d2.toLocal8Bit());
  }
}

void DataBackend::loadDatasetCommand(const QString & /*cmdname*/, 
                                     QStringList files,
                                     const CommandOptions & opts)
{

  /// @todo There is some code shared with the loadAndDisplay
  /// function. That should be factored out as far as possible.

  soas().view().disableUpdates();
  int nb = 0;
  for(int i = 0; i < files.size(); i++) {
    QString fileName = files[i];
    Terminal::out << "Loading file '" << fileName << "'";
    try {
      QFile file(fileName);
      Utils::open(&file, QIODevice::ReadOnly);
      DataSet * ds = readFromStream(&file, fileName, opts);
      soas().stack().pushDataSet(ds, true); // use the silent version
      // as we display ourselves
      if(nb++ > 0)
        soas().view().addDataSet(ds);
      else
        soas().view().showDataSet(ds);
      Terminal::out << " -- " << ds->nbColumns() << " columns" << endl;
    }
    catch (const RuntimeError & e) {
      Terminal::out << "\n" << e.message() << endl;
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  soas().view().enableUpdates();
}

void DataBackend::setMetaDataForFile(DataSet * dataset, 
                                     const QString& filename)
{
  QDir dir = QDir::current();
  dataset->setMetaData("original-file",
                       QDir::cleanPath(dir.absoluteFilePath(filename)));
}
