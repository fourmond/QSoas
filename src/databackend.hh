/**
   \file databackend.hh
   Backends for reading data files.
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
#ifndef __DATABACKEND_HH
#define __DATABACKEND_HH

#include <argumentmarshaller.hh>
#include <argumentlist.hh>
class DataSet;


/// This is an element of the cache. Implementation internal.
class CachedDataSets;


/// The base class of a series that reads data from files.
///
/// It supports auto-detection...
///
/// @todo I should have a way to have backends accept real
/// CommandOption arguments and register them as real commands too, so
/// that one could force using a particular backend.
///
/// Idea: load-as-text / load-as-binary / load-as-GPES, etc...
///
/// @todo Setup a cache. It needs a time stamp; should it be held by
/// the cache, or by the DataSet ? Most probably the latter.
class DataBackend {
  /// A global hash holding a correspondance name->databackend
  static QList<DataBackend*> * availableBackends;

  static QList<Command*> * backendCommands;

  /// Registers the given backend to the static registry
  static void registerBackend(DataBackend * backend);


  /// A cache for datasets
  ///
  /// @todo Should be a pointer ?
  static QCache<QString, CachedDataSets> * cachedDatasets;

  /// A cache for the merged options of all backends
  static ArgumentList * allBackendsOptions;

  /// A cache for the list of options that are not backend-related
  static QSet<QString> nonBackendOptions;

  /// Returns the cache for the given file
  static CachedDataSets * cacheForFile(const QString & file);

  /// Adds the given results for the given file
  static void addToCache(const QString & file,
                         const QList<DataSet *> & datasets);
  

protected:
  /// A short code-like name
  QString name;

  /// A public name
  QString pubName;

  /// A description
  QString desc;

  DataBackend(const char * n, const char * pn, const char * d = "", 
              bool autoRegister = true) :
    name(n), pubName(pn), desc(d) {
    if(autoRegister)
      registerBackend(this);
  };
public:

  /// The public name of the backend
  QString publicName() const; 

  /// A description of what the backend loads
  QString description() const;

  /// The code name of the backend, the one we're using in the end
  QString codeName() const;

protected:
  /// Returns a value signalling whether the given file could be read
  /// by the DataBackend.
  ///
  /// Return value:
  /// \li 0: no go
  /// \li 1 to 999: could be, but if some other DataBackend wants to take it
  /// fine
  /// \li 1000 and more: definitely yes.
  ///
  /// The search for a DataBackend will stop at the first value over
  /// 1000
  ///
  /// \warning \p fileName does not necessarily correspond to a real
  /// file, do not rely on it.
  virtual int couldBeMine(const QByteArray & peek, 
                          const QString & fileName) const = 0;


  /// Returns the list of options the backend can take when called
  /// directly.
  virtual ArgumentList loadOptions() const;




  /// The command for loading several datasets in a dataset-specific
  /// fashion
  void loadDatasetCommand(const QString & cmdname, 
                          QStringList files,
                          const CommandOptions & opts);

  /// Decorates a \a dataset's metadata with file information about
  /// the file name.
  static void setMetaDataForFile(DataSet * dataset, 
                                 const QString& fileName);

  /// Reads a DataSet from the given stream. The \p fileName parameter
  /// does not necessarily point to a real file.
  ///
  /// \b Note The backend is responsible for setting as much meta-data
  /// as reasonably possible based on the given file, but also
  /// potentially on connected files.
  ///
  /// <b>In particular</b> all backends should set a meta-data telling
  /// which backend read the dataset !
  ///
  /// @todo I should find a way to also gather information from the
  /// so-called conditions.dat files I used so often now. I'll have to
  /// decide how the interface will go.
  virtual QList<DataSet *> readFromStream(QIODevice * stream,
                                          const QString & fileName,
                                          const CommandOptions & opts) const = 0;

public:

  QList<DataSet *> readFile(const QString & fileName, 
                            const CommandOptions & opts) const;

  
  /// Find which DataBackend is best suited to load the given stream.
  ///
  /// It relies on QIODevice::peek().
  static DataBackend * backendForStream(QIODevice * stream,
                                        const QString & fileName);

  /// Loads the given file. Returns a valid DataSet pointer or raise
  /// an appropriate exception.
  ///
  /// This function caches the result !
  static QList<DataSet *> loadFile(const QString & fileName, 
                                   const CommandOptions & opts = 
                                   CommandOptions(), 
                                   bool verbose = true);

  /// Register all the individual backend load-as commands. Then,
  /// registers the overall load functions
  static void registerBackendCommands();

  /// Cleans up all the backend commands.
  static void cleanupBackends();

  /// Load files using the given backend (or with backend detection
  /// should the backend pointer be NULL) and display them.
  static void loadFilesAndDisplay(bool update, QStringList files, 
                                  const CommandOptions & opts,
                                  DataBackend * backend = NULL);



  /// Returns statistics about the cache:
  /// @li the number of files
  /// @li the number of datasets
  /// @li the total size of the cache, in bytes
  /// @li the maximum number of files that can be cached
  static void cacheStats(int * nbFiles, int * nbDatasets,
                         int * size, int * maxFiles);

  /// Sets the cache size, in terms of number of files.
  static void setCacheSize(int number);

};

#endif
