/**
   \file datastack.hh
   The DataStack class, that contains all data Soas knows about.
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
#ifndef __DATASTACK_HH
#define __DATASTACK_HH

#include <dataset.hh>

/// The data stack, ie all DataSet objects known to Soas.
///
/// Commands missing here:
/// @li save/load stack (in a binary format containing
/// exactly everyting, see QDataStream)
class DataStack : public QObject {

  Q_OBJECT;

  /// When this flag is on, no cleanup is performed at destruction.
  bool notOwner;

  /// The DataSet objects
  QList<DataSet *> dataSets;

  /// The datasets produced by the last command -- currently acquiring
  QList< QList<GuardedPointer<DataSet> > > latest;

  /// The redo stack.
  QList<DataSet *> redoStack;

  // /// A chache DataSet name -> DataSet.
  // Not used, so more obnoxious than anything else...
  // QHash<QString, DataSet *> dataSetByName;

  /// Converts the dataset number given into a real list index, along
  /// with setting the list pointer to the right one.
  int dsNumber2Index(int nb, const QList<DataSet *> * * target) const;
  int dsNumber2Index(int nb, QList<DataSet *> ** target);

  /// @name Stack-serialization related functions
  ///
  /// @{

  friend QDataStream & operator<<(QDataStream & out, const DataStack & stack);
  friend QDataStream & operator>>(QDataStream & in, DataStack & stack);


  /// The version of the current serialization load. Stored as a
  /// negative number in the binary storage.
  static qint32 serializationVersion;

  friend QDataStream & operator<<(QDataStream & out, const DataSet & ds);
  friend QDataStream & operator>>(QDataStream & in, DataSet & ds);

  friend QDataStream & operator<<(QDataStream & out, const DatasetOptions & ds);
  friend QDataStream & operator>>(QDataStream & in, DatasetOptions & ds);

  /// Writes the serialization header
  static void writeSerializationHeader(QDataStream & out);

  /// Reads the serialization header. Sets serializationVersion
  static void readSerializationHeader(QDataStream & in);

  /// Serializes the stack to the binary output
  void writeStack(QDataStream & out) const;

  /// Reads the stack
  void readStack(QDataStream & in);

  /// @}
  
  /// Total size of the stack.
  quint64 cachedByteSize;

  /// Auto-flags. When this set isn't empty, the corresponding flags
  /// are added automatically to each dataset pushed onto the stack.
  QSet<QString> autoFlags;

  /// The current accumulator
  DataSet * accumulator;


  /// This list stores the datasets produced by "context". Outer
  /// contexts have at least as many datasets as the nested ones.
  QList<QList<GuardedPointer<DataSet> > > spies;
  
public:

  /// Push a spy, returns the previous nesting level
  int pushSpy();

  /// Pop spies back to the given nesting level. Returns the
  /// datasets in the last nesting level popped.
  QList<DataSet *> popSpy(int targetLevel);


  /// Constructs a DataStack object.
  explicit DataStack(bool notOwner = false);

  ~DataStack();


  /// Push the given DataSet object to the stack. It becoms the
  /// currentDataSet().
  ///
  /// @warning DataStack takes ownership of the DataSet.
  ///
  /// if \a silent is true, the stack won't emit the
  /// currentBufferChanged() signal (useful for the functions that
  /// already take care of the display)
  void pushDataSet(DataSet * dataset, bool silent = false);


  /// This hook is called at the start of each new command.
  void startNewCommand();

  /// Displays to terminal a small text description of the contents of
  /// the stack.
  ///
  /// The @a meta arguments lists meta that will be displayed in
  /// addition to the buffer presentation, 
  void showStackContents(int limit = 0,
                         const QStringList & meta = QStringList()) const;

  /// @name Dataset selection
  ///
  /// Dataset selection functions, with flags or other mechanisms
  ///
  /// @{
  
  /// Returns the list of datasets flagged (or not) with the given
  /// flag (or with any flag if the second argument is empty)
  QList<DataSet *> flaggedDataSets(bool flagged = true, 
                                   const QString &  flag = QString());

  /// Hmmm...
  QList<const DataSet *> flaggedDataSets(bool flagged = true, 
                                   const QString &  flag = QString()) const;

  /// Returns the list of flags defined.
  QSet<QString> definedFlags() const;

  /// Sets the auto-flags.
  void setAutoFlags(const QSet<QString> & flags);

  /// Parses a single specification of datasets, including flags and
  /// numbered lists, and returns the corresponding list of datasets.
  QList<const DataSet *> datasetsFromSpec(const QString & spec) const;

  /// Returns the list of dataset names
  QSet<QString> datasetNames() const;

  typedef enum {
    Strict,
    Glob,
    Regex
  } NameMatchingRule;

  /// Returns all the datasets having this name.
  ///
  /// If matcher is Strict, only datasets of the right name are given
  /// else, a search is performed using name either as a glob or a
  /// regexp pattern
  QList<const DataSet *> namedDataSets(const QString & name,
                                       NameMatchingRule matcher = Glob) const;


  /// Returns the numbered data set.
  /// \li 0 is the most recent
  /// \li -1 is the most recently pushed onto the redo stack.
  ///
  /// Returns NULL if no dataset could be found.
  DataSet * numberedDataSet(int nb) const;

  /// Returns the current dataset, ie the one that should be displayed
  /// currently.
  ///
  /// If silent is false, an exception will be raised if the buffer is
  /// NULL.
  DataSet * currentDataSet(bool silent = true) const;

  /// @}

  /// Undo (ie, buffer 0 becomes buffer -1)
  void undo(int nbtimes = 1);

  /// Clears the whole stack, freeing the memory held
  void clear();

  /// Does the reverse of undo();
  void redo(int nbtimes = 1);

  /// Returns true if the given DataSet is in the stack and sets \a
  /// idx accordingly.
  bool indexOf(const DataSet * ds, int * idx) const;

  /// Drops the given dataset
  void dropDataSet(int idx);

  /// Drops the given dataset
  void dropDataSet(const DataSet * ds);


  /// Returns all the datasets in the numeric order
  QList<const DataSet *> allDataSets() const;

  /// Returns the size of the undo stack, not counting the redo
  /// buffers.
  int stackSize() const {
    return dataSets.size();
  };

  /// Returns the size of the redo stack
  int redoStackSize() const {
    return redoStack.size();
  };

  /// Returns the total number of datasets
  int totalSize() const;

  /// Returns the total byte size of the stack
  quint64 byteSize() const;

  /// Returns a textual summary of the stack: current dataset, total
  /// size in kB, number of datasets
  QString textSummary() const;

  /// Accumulate the given ValueHash to the accumulator.
  ///
  /// The string corresponds to a new row name if it isn't empty.
  void accumulateValues(const ValueHash & data,
                        const QString & rowName = QString());

  /// Returns the current accumulator and release its ownership
  DataSet * popAccumulator();

  /// Returns the current state of the accumulator.
  const DataSet * peekAccumulator();

  // /// Returns the current accumulator without releasing ownership
  // DataSet * getAccumulator();

  /// Inserts the given stack into the current one.
  void insertStack(const DataStack & s);

  /// Reorders the datasets given so they are in the order given.
  /// Will raise exceptions upon missing datasets
  void reorderDatasets(const QList<const DataSet *> newOrder);


signals:
  /// Emitted whenever the current dataset changed.
  void currentDataSetChanged();

};

QDataStream & operator<<(QDataStream & out, const DataStack & stack);
QDataStream & operator>>(QDataStream & in, DataStack & stack);


#endif
