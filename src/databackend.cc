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
#include <general-arguments.hh>
#include <command.hh>

#include <exceptions.hh>

#include <possessive-containers.hh>

#include <soas.hh>
#include <curveview.hh>
#include <datastack.hh>

#include <stylegenerator.hh>

QList<DataBackend*> * DataBackend::availableBackends = NULL;


class CachedDataSets {
  
  static QList<DataSet *> deepCopy(const QList<DataSet*> & dss) {
    QList<DataSet *> ret;
    for(int i = 0; i < dss.size(); i++)
      ret << new DataSet(*dss[i]);
    return ret;
  };

  PossessiveList<DataSet> datasets;
  
public:

  /// The date
  QDateTime date;

  /// We use deep copy
  CachedDataSets(const QList<DataSet*> & dss)  : 
    datasets(deepCopy(dss)) {
    date = QDateTime::currentDateTime();
  };

  QList<DataSet *> cachedDataSets() const {
    return deepCopy(datasets);
  };

};

// A cache for 1000 datasets
/// @todo Use real size ?
QCache<QString, CachedDataSets> DataBackend::cachedDatasets(1000);

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

QList<DataSet *> DataBackend::loadFile(const QString & fileName, bool verbose)
{
  QFile file(Utils::expandTilde(fileName));

  QFileInfo info(fileName);
  QString key = info.canonicalFilePath();
  QDateTime lastModified = info.lastModified();

  QList<DataSet *> datasets;

  if(verbose)
    Terminal::out << "Loading file: '" << fileName << "' ";
  
  CachedDataSets * cached = cachedDatasets.object(key);

  if(! cached || cached->date < lastModified) {
    Utils::open(&file, QIODevice::ReadOnly);

    DataBackend * b = backendForStream(&file, fileName);
    if(! b)
      throw RuntimeError(QObject::tr("No backend found to load '%1'").
                         arg(fileName));
    datasets = b->readFromStream(&file, fileName, CommandOptions());

    if(verbose)
      Terminal::out << "using backend " << b->name << endl;

    // Now we update the cache
    cachedDatasets.insert(key, new CachedDataSets(datasets));
  }
  else {
    datasets = cached->cachedDataSets();
    if(verbose)
      Terminal::out << "(cached)" << endl;
  }
  
  return datasets;
}

ArgumentList * DataBackend::loadOptions() const
{
  return NULL;
}

QList<DataSet *> DataBackend::readFile(const QString & fileName, 
                                       const CommandOptions & opts) const
{
  QFile file(Utils::expandTilde(fileName));
  Utils::open(&file, QIODevice::ReadOnly);
  return readFromStream(&file, fileName, opts);
}



void DataBackend::loadFilesAndDisplay(int nb, QStringList files, 
                                      const CommandOptions & opts,
                                      DataBackend * backend)
{
  soas().view().disableUpdates();
  QString style;
  updateFromOptions(opts, "style", style);

  // First load
  QList<DataSet *> datasets;

  for(int i = 0; i < files.size(); i++) {
    try {
      QList<DataSet *> dss = 
        (backend ? backend->readFile(files[i], opts) : 
         DataBackend::loadFile(files[i]));

      if(dss.size() > 1)
        Terminal::out << " -> got " << dss.size() << " datasets" << endl;
      else
        Terminal::out << " -> OK" << endl;
      datasets << dss;
    }
    catch (const RuntimeError & e) {
      Terminal::out << "\n" << e.message() << endl;
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }

  QScopedPointer<StyleGenerator> 
    gen(StyleGenerator::fromText(style, datasets.size()));
        
  for(int i = 0; i < datasets.size(); i++) {
    DataSet * s = datasets[i];
    soas().stack().pushDataSet(s, true); // use the silent version
    // as we display ourselves
    if(nb > 0)
      soas().view().addDataSet(s, gen.data());
    else
      soas().view().showDataSet(s, gen.data());
    nb++;
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }

  soas().view().enableUpdates();
}

void DataBackend::loadDatasetCommand(const QString & /*cmdname*/, 
                                     QStringList files,
                                     const CommandOptions & opts)
{
  loadFilesAndDisplay(0, files, opts, this);
}

void DataBackend::setMetaDataForFile(DataSet * dataset, 
                                     const QString& filename)
{
  QDir dir = QDir::current();
  dataset->setMetaData("original-file",
                       QDir::cleanPath(dir.absoluteFilePath(filename)));
}


void DataBackend::registerBackendCommands()
{
  ArgumentList * lst = 
    new ArgumentList(QList<Argument *>() 
                     << new SeveralFilesArgument("file", 
                                                 "File",
                                                 "Files to load !", true
                                                 ));

  ArgumentList * overallOptions = 
    new ArgumentList(QList<Argument *>() 
                     << new StyleGeneratorArgument("style", 
                                                   "Style",
                                                   "Style for curves display"));
  

  for(int i = 0; i < availableBackends->size(); i++) {
    DataBackend * b = availableBackends->value(i);
    ArgumentList * opts = b->loadOptions();
    overallOptions->mergeOptions(*opts);
    /// @todo Try to find a way to share that with options for the
    /// load and overlay commands.
    *opts << new StyleGeneratorArgument("style", 
                                        "Style",
                                        "Style for curves display");
      
    /// @todo Add general options processing.
    QString name = "load-as-" + b->name;

    QString d1 = QString("Load files with backend '%1'").arg(b->name);
    QString d2 = QString("Load any number of files directly using the backend "
                         "'%1', bypassing cache and automatic backend "
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
