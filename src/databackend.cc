/*
  databackend.cc: implementation of the DataBackend factory class
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

#include <datastackhelper.hh>
#include <datasetoptions.hh>
#include <metadataprovider.hh>

QList<DataBackend*> * DataBackend::availableBackends = NULL;

ArgumentList * DataBackend::allBackendsOptions = NULL;

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

QList<DataSet *> DataBackend::loadFile(const QString & fileName, 
                                       const CommandOptions & opts, 
                                       bool verbose)
{
  QString fn = Utils::expandTilde(fileName);
  QFile file(fn);

  bool ignoreCache = false;
  updateFromOptions(opts, "ignore-cache", ignoreCache);

  for(CommandOptions::const_iterator i = opts.begin(); i != opts.end(); ++i) {
    if(! nonBackendOptions.contains(i.key()))
       ignoreCache = true;
  }

  QFileInfo info(fn);
  QString key = info.canonicalFilePath();
  QDateTime lastModified = info.lastModified();

  QList<DataSet *> datasets;

  if(verbose)
    Terminal::out << "Loading file: '" << fileName << "' ";
  
  CachedDataSets * cached = cachedDatasets.object(key);

  if(! cached || cached->date < lastModified || ignoreCache) {
    Utils::open(&file, QIODevice::ReadOnly);

    DataBackend * b = backendForStream(&file, fileName);
    if(! b)
      throw RuntimeError(QObject::tr("No backend found to load '%1'").
                         arg(fileName));
    datasets = b->readFromStream(&file, fileName, opts);

    if(verbose)
      Terminal::out << "using backend " << b->name << endl;

    // Now we update the cache
    if(! ignoreCache)
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



void DataBackend::loadFilesAndDisplay(bool update, QStringList files, 
                                      const CommandOptions & opts,
                                      DataBackend * backend)
{
  soas().view().disableUpdates();

  // First load
  QList<DataSet *> datasets;

  for(int i = 0; i < files.size(); i++) {
    try {
      QList<DataSet *> dss = 
        (backend ? backend->readFile(files[i], opts) : 
         DataBackend::loadFile(files[i], opts));

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

  DataStackHelper pusher(opts, update);
  for(int i = 0; i < datasets.size(); i++) {
    DataSet * s = datasets[i];
    // Set the options:
    DatasetOptions::setDatasetOptions(s, opts);
    pusher << s;
  }

  soas().view().enableUpdates();
}

void DataBackend::loadDatasetCommand(const QString & /*cmdname*/, 
                                     QStringList files,
                                     const CommandOptions & opts)
{
  loadFilesAndDisplay(false, files, opts, this);
}

void DataBackend::setMetaDataForFile(DataSet * dataset, 
                                     const QString& filename)
{
  /// @todo Should the new meta-data stuff go here or somewhere else ?
  /// Here is fine.
  QDir dir = QDir::current();
  QString fp = QDir::cleanPath(dir.absoluteFilePath(filename));
  dataset->setMetaData("original-file", fp);
  QFileInfo info(fp);
  dataset->setMetaData("file-date", info.lastModified());

  dataset->setMetaData("age", info.lastModified().
                       msecsTo(soas().startupTime()) * 1e-3);

  
  ValueHash md = MetaDataProvider::allMetaDataForFile(fp);
  dataset->addMetaData(md);
}

static void loadCommand(const QString &, QStringList files, 
                        const CommandOptions & opts)
{
  DataBackend::loadFilesAndDisplay(false, files, opts);
}

static void overlayFilesCommand(const QString &, QStringList files, 
                                const CommandOptions & opts)
{
  DataBackend::loadFilesAndDisplay(true, files, opts);
}



QSet<QString> DataBackend::nonBackendOptions;

void DataBackend::registerBackendCommands()
{
  ArgumentList * lst = 
    new ArgumentList(QList<Argument *>() 
                     << new SeveralFilesArgument("file", 
                                                 "Files",
                                                 "the files to load", true
                                                 ));

  ArgumentList * overallOptions = 
    new ArgumentList(QList<Argument *>() 
                     << DataStackHelper::helperOptions()
                     << new BoolArgument("ignore-cache", 
                                         "Ignores cache",
                                         "if on, ignores cache (default off)")
                     );

  ArgumentList * oo = DatasetOptions::optionList();
  overallOptions->mergeOptions(*oo);
  nonBackendOptions = QSet<QString>::fromList(overallOptions->argumentNames());

  if(allBackendsOptions)
    throw InternalError("Registering backends commands several times");
  allBackendsOptions = new ArgumentList;


  for(int i = 0; i < availableBackends->size(); i++) {
    DataBackend * b = availableBackends->value(i);
    ArgumentList * opts = b->loadOptions();
    if(opts) {
      allBackendsOptions->mergeOptions(*opts);
      opts = new ArgumentList(*opts);
    }
    else
      opts = new ArgumentList;
    /// @todo Try to find a way to share that with options for the
    /// load and overlay commands.
    *opts << DataStackHelper::helperOptions();
    
    opts->mergeOptions(*oo);
      
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
                "load", lst, opts, (const char*) d1.toLocal8Bit(), 
                (const char*) d1.toLocal8Bit(), 
                (const char*) d2.toLocal8Bit());
  }

  overallOptions->mergeOptions(*allBackendsOptions);

  // Now, we create the load and overlay commands.

  Command * cmd = 
    new Command("load", // command name
                effector(loadCommand), // action
                "load",  // group name
                lst, // arguments
                overallOptions, // options
                "Load",
                "Loads one or several files",
                "Loads the given files and push them onto the data stack",
                "l");

  cmd = 
    new Command("overlay", // command name
                effector(overlayFilesCommand), // action
                "stack",  // group name
                lst, // arguments
                overallOptions, // options
                "Overlay",
                "Loads files and overlay them",
                "Loads the given files and push them onto the data "
                "stack, adding them to the display at the same time",
                "v");
  
}
