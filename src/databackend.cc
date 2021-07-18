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

#include <file.hh>
#include <fileinfo.hh>

#include <datastackhelper.hh>
#include <datasetoptions.hh>
#include <metadataprovider.hh>
#include <metadatafile.hh>

QList<DataBackend*> * DataBackend::availableBackends = NULL;

QList<Command*> * DataBackend::backendCommands = NULL;

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

  int number() const {
    return datasets.size();
  };

  int byteSize() const {
    int sz = 0;
    for(const DataSet * ds : datasets)
      sz += ds->byteSize();
    return sz;
  };

};


static SettingsValue<int> cacheSize("backends/cache-size", 1000);

// A cache for 1000 datasets
/// @todo Use real size ?
QCache<QString, CachedDataSets> * DataBackend::cachedDatasets = NULL;


CachedDataSets * DataBackend::cacheForFile(const QString & file)
{
  if(! cachedDatasets)
    return NULL;
  return cachedDatasets->object(file);
}

void DataBackend::addToCache(const QString & file, const QList<DataSet*> & dss)
{
  if(! cachedDatasets)
    cachedDatasets = new QCache<QString, CachedDataSets>(::cacheSize);
  cachedDatasets->insert(file, new CachedDataSets(dss));
}

void DataBackend::setCacheSize(int sz)
{
  cacheSize = sz;
  if(cachedDatasets)
    cachedDatasets->setMaxCost(sz);
}

void DataBackend::cacheStats(int * nbFiles, int * nbDatasets,
                         int * size, int * maxFiles)
{
  *nbFiles = 0;
  *nbDatasets = 0;
  *size = 0;
  *maxFiles = 0;
  if(! cachedDatasets)
    return;
  *nbFiles = cachedDatasets->totalCost();
  *maxFiles = cachedDatasets->maxCost();

  QStringList lst = cachedDatasets->keys();
  for(const QString & f : lst) {
    const CachedDataSets * cds = (*cachedDatasets)[f];
    *nbDatasets += cds->number();
    *size += cds->byteSize();
  }
}






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
  File file(fileName, File::BinaryRead);

  bool ignoreCache = false;
  updateFromOptions(opts, "ignore-cache", ignoreCache);

  for(CommandOptions::const_iterator i = opts.begin(); i != opts.end(); ++i) {
    if(! nonBackendOptions.contains(i.key()))
       ignoreCache = true;
  }

  /// @todo Switch to using a File-based class...
  FileInfo info = file.info();
  QString key = info.canonicalFilePath();
  QDateTime lastModified = info.lastModified();
  QDateTime metaModified = MetaDataFile::metaDataLastModified(fileName);
  if(metaModified.isValid() && metaModified > lastModified)
    lastModified = metaModified; // so we reload if the meta is too young.

  QList<DataSet *> datasets;

  if(verbose)
    Terminal::out << "Loading file: '" << fileName << "' ";
  
  CachedDataSets * cached = cacheForFile(key);

  if(! cached || cached->date < lastModified || ignoreCache) {
    // Utils::open(&file, QIODevice::ReadOnly);

    DataBackend * b = backendForStream(file, fileName);
    if(! b)
      throw RuntimeError(QObject::tr("No backend found to load '%1'").
                         arg(fileName));
    datasets = b->readFromStream(file, fileName, opts);

    for(DataSet * d : datasets)
      d->setMetaData("backend", b->name);


    if(verbose)
      Terminal::out << "using backend " << b->name << endl;

    // Now we update the cache
    if(! ignoreCache)
      addToCache(key, datasets);
  }
  else {
    datasets = cached->cachedDataSets();
    if(verbose)
      Terminal::out << "(cached)" << endl;
  }

  return datasets;
}

ArgumentList DataBackend::loadOptions() const
{
  return ArgumentList();
}

QList<DataSet *> DataBackend::readFile(const QString & fileName, 
                                       const CommandOptions & opts) const
{
  File file(fileName, File::BinaryRead);
  QList<DataSet *> datasets = readFromStream(file, fileName, opts);
  for(DataSet * d : datasets)
    d->setMetaData("backend", name);
  return datasets;
}



void DataBackend::loadFilesAndDisplay(bool update, QStringList files, 
                                      const CommandOptions & opts,
                                      DataBackend * backend)
{
  soas().view().disableUpdates();

  // First load
  QList<DataSet *> datasets;

  QString frm;
  updateFromOptions(opts, "for-which", frm);

  bool ignoreEmpty = true;
  updateFromOptions(opts, "ignore-empty", ignoreEmpty);

  int expected = -1;
  updateFromOptions(opts, "expected", expected);

  for(int i = 0; i < files.size(); i++) {
    try {
      QList<DataSet *> dss = 
        (backend ? backend->readFile(files[i], opts) : 
         DataBackend::loadFile(files[i], opts));

      if(dss.size() > 1)
        Terminal::out << " -> got " << dss.size() << " datasets" << endl;
      else
        Terminal::out << " -> OK" << endl;
      FileInfo info(files[i]);
      for(DataSet * s : dss) {
        s->setMetaData("original_file", info.canonicalFilePath());
        
        if(ignoreEmpty && (s->nbRows() == 0 || s->nbColumns() == 0)) {
          Terminal::out << " -> ignoring empty dataset '"
                        << s->name
                        << "', use /ignore-empty=false if you really want it" 
                        << endl;

          continue;
        }
        if(! frm.isEmpty()) {
          try {
            if(! s->matches(frm)) {
              Terminal::out << "File '"
                            << s->name
                            << "' does not match the selection rule" 
                            << endl;
            }
            else
              datasets << s;
          }
          catch(const RuntimeError & re) {
            Terminal::out << "Error evaluating expression file '"
                          << s->name
                          << "': " << re.message()
                          << endl;
          }
        }
        else
          datasets << s;
      }
    }
    catch (const RuntimeError & e) {
      Terminal::out << "\n" << e.message() << endl;
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  if(expected >= 0 && datasets.size() != expected)
    throw RuntimeError("Expected %1 datasets but got %2").
      arg(expected).arg(datasets.size());

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
  dataset->setMetaData("original_file", fp);
  FileInfo info(fp);
  dataset->setMetaData("file_date", info.lastModified());

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
  if(backendCommands)
    throw InternalError("Probably calling registerBackendCommands twice");
  backendCommands = new QList<Command *>;
  ArgumentList lst = 
    ArgumentList(QList<Argument *>() 
                     << new SeveralFilesArgument("file", 
                                                 "Files",
                                                 "the files to load", true
                                                 ));

  ArgumentList overallOptions = 
    ArgumentList(QList<Argument *>() 
                 << DataStackHelper::helperOptions()
                 << new BoolArgument("ignore-cache", 
                                     "Ignores cache",
                                     "if on, ignores cache (default off)")
                 );

  ArgumentList oo = DatasetOptions::optionList();
  oo << DataStackHelper::helperOptions()
     << new BoolArgument("ignore-empty", 
                         "Ignores empty files",
                         "if on, skips empty files (default on)")
     << new CodeArgument("for-which", 
                         "For which",
                         "Select on formula")
     << new IntegerArgument("expected", 
                            "Expected number",
                            "Expected number of loaded datasets");

  overallOptions.mergeOptions(oo);
  nonBackendOptions = QSet<QString>::fromList(overallOptions.argumentNames());

  if(allBackendsOptions)
    throw InternalError("Registering backends commands several times");
  allBackendsOptions = new ArgumentList;


  for(int i = 0; i < availableBackends->size(); i++) {
    DataBackend * b = availableBackends->value(i);
    ArgumentList opts = b->loadOptions();
    allBackendsOptions->mergeOptions(opts);
    opts.mergeOptions(oo);

    QString name = "load-as-" + b->name;

    QString d1 = QString("Load files with backend '%1'").arg(b->name);
    Command * cmd =
      new Command(name.toLocal8Bit(),
                  effector(b, &DataBackend::loadDatasetCommand),
                  "load", lst, opts, (const char*) d1.toLocal8Bit(), 
                  (const char*) d1.toLocal8Bit());
    *backendCommands << cmd;
  }

  overallOptions.mergeOptions(*allBackendsOptions);

  // Now, we create the load and overlay commands.

  Command * cmd = 
    new Command("load", // command name
                effector(loadCommand), // action
                "load",  // group name
                lst, // arguments
                overallOptions, // options
                "Load",
                "Loads one or several files",
                "l");
  *backendCommands << cmd;

  cmd = 
    new Command("overlay", // command name
                effector(overlayFilesCommand), // action
                "stack",  // group name
                lst, // arguments
                overallOptions, // options
                "Overlay",
                "Loads files and overlay them",
                "v");
  *backendCommands << cmd;
}

void DataBackend::cleanupBackends()
{
  if(! backendCommands)
    return;
  for(Command * cmd : *backendCommands)
    delete cmd;
}
