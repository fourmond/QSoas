/**
   \file fit.hh
   Implementation of fits.
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
#ifndef __FIT_HH
#define __FIT_HH

#include <argumentmarshaller.hh>

/// The definition of a parameter.
class ParameterDefinition {
public:
  /// Parameter name
  QString name;

  /// Whether or not the parameter is fixed by default. Parameters
  /// that are fixed by default could be external things (say, an
  /// offset), rather than real parameters, although in some cases it
  /// may make sense to have them variable.
  bool defaultsToFixed;

  /// If true, then this parameter can be specific to one dataset
  /// (one buffer) instead of being global to all datasets fitted at
  /// the same time.
  bool canBeBufferSpecific;

  ParameterDefinition(const QString & n, bool fixed = false,
                      bool cbs = true) :
    name(n), defaultsToFixed(fixed), canBeBufferSpecific(cbs)
  {
  };
};

class ArgumentList;
class CommandEffector;
class FitData;
class DataSet;
class Vector;

/// This is the base class of all fit-related storage
///
/// All interanl fit data must belong to that 
class FitInternalStorage {
public:

  virtual ~FitInternalStorage() {
  };
};

/// This abstract class defines the interface for handling fits.
///
/// @todo handle validity range checking
class Fit {
protected:

  /// Fit name, used to make up command names
  QString name;

  /// Fit description
  QString shortDesc;

  /// Long description
  QString longDesc;

  /// Prepares and registers the commands corresponding to the fit.
  ///
  /// If provided, the commands will have \a args as argument list
  /// (with the additional buffer arguments for the multiFit command)
  void makeCommands(ArgumentList * args = NULL, 
                    CommandEffector * singleFit = NULL,
                    CommandEffector * multiFit = NULL,
                    ArgumentList * options = NULL,
                    CommandEffector * sim = NULL
                    );

  /// The minimum number of datasets the fit should take.
  int minDataSets;

  /// The maximum number of datasets the fit can take (-1 for
  /// boundless)
  int maxDataSets;

  /// Process the fit-specific options.
  ///
  /// This function can also be used to perfom per-fit initialization,
  /// even if you don't care about options.
  virtual void processOptions(const CommandOptions & opts, FitData * data) const;

  /// Very cumbersome, but it seems to be necessary anyway...
  static void processOptions(const Fit * f, const CommandOptions & opts, FitData * data) {
    f->processOptions(opts, data);
  };

  /// Returns a string describing the options used, when applicable
  virtual QString optionsString(FitData * /*data*/) const {
    return QString();
  };

  /// Same cumbersome hack as for processOptions()
  static QString optionsString(const Fit * f, FitData * data) {
    return f->optionsString(data);
  };

  /// Checks that the datasets provided to the fits are correct. Raise
  /// an appropriate runtime exception if that isn't the case.
  ///
  /// In particular, this checks that the number of datasets is
  /// correct (but other checks may be implemented based on the
  /// contents of the datasets too).
  virtual void checkDatasets(const FitData * data) const;

  /// A static correspondance fit name -> Fit*
  static QHash<QString, Fit*> * fitsByName;

  /// Registers the given fit
  static void registerFit(Fit * fit);


  /// A small helper for copying FitInternalStorage
  template <class T> static T * deepCopy(FitInternalStorage * src) {
    return new T(*static_cast<T*>(src));
  };


  /// This is the way to get the internal storage space from a given
  /// FitData.
  FitInternalStorage * getStorage(FitData * d) const;

  /// A helper function to get the right class of storage
  template <class T> T * storage(FitData * d) const {
    return static_cast<T*>(getStorage(d));
  };

public:

  /// Returns the name of all the available fits.
  static QStringList availableFits();

  /// Returns the named fit. (or null)
  static Fit * namedFit(const QString & name);



  /// The fit name
  QString fitName(bool includeOptions = true, FitData * data = NULL) const {
    if(includeOptions) {
      QString str = optionsString(data);
      if(str.size() > 0) 
        return name + " (" + str + ")";
    }
    return name;
  };  

  /// @name Public interface
  ///
  /// Most of these functions need to be reimplemented by subclasses.
  ///
  /// @{

  /// The parameters
  virtual QList<ParameterDefinition> parameters(FitData * data) const = 0;
  

  /// Computes the function (ie in the absence of the residuals)
  ///
  /// \a parameters are given in the fully expanded form, ie
  /// parameters().size() times number of datasets
  ///
  /// Keep in mind that in multidataset mode, \a target contains \b
  /// all datasets !
  virtual void function(const double * parameters,
                        FitData * data, gsl_vector * target) const = 0;

  /// Does the same thing as function, but only updates the part
  /// corresponding to the given dataset.
  ///
  /// \warning It is not guaranteed that the function will not be
  /// computed elsewhere. What is guaranteed, though, is that the
  /// values for the given dataset are updated, and that whatever else
  /// is updated has the correct value.
  virtual void functionForDataset(const double * parameters,
                                  FitData * data, gsl_vector * target, 
                                  int dataset) const;

  /// Returns a string meant for the user to understand what is the
  /// specific role of the given dataset.
  ///
  /// Of course, this function only make sense for fits that treat
  /// buffers differently.
  virtual QString annotateDataSet(int idx, FitData * data) const;

  /// Prepares an initial guess for all parameters. The \a guess array
  /// is a fully expanded version of the parameters.
  ///
  /// \see packParameters()
  /// \see unpackParameters()
  virtual void initialGuess(FitData * data, double * guess) const = 0;


  /// Allocate an internal storage for the fit.
  /// Can be NULL if the fit doesn't need any storage.
  virtual FitInternalStorage * allocateStorage(FitData * data) const;

  /// Copy the internal storage, for the target numbered dataset (-1
  /// to copy everything)
  virtual FitInternalStorage * copyStorage(FitData * data, FitInternalStorage * source, int ds = -1) const;

  /// This function returns the arguments to the fit, ie the stuff
  /// that have to be passed to the fit function.
  ///
  /// Mostly empty
  virtual ArgumentList * fitArguments() const;

  /// The options to the fit, ie the options whose value may affect
  /// the number/interepretation of the fit parameters.
  ///
  /// Any change to these options must happen before the start of the
  /// fit.
  virtual ArgumentList * fitHardOptions() const;

  /// These options can be provided before the beginning of the fit,
  /// but they can be changed from within the dialog box without
  /// adverse effects.
  virtual ArgumentList * fitSoftOptions() const;


  /// This function returns the current values of the soft options.
  virtual CommandOptions currentSoftOptions(FitData * data) const;


  /// This function processes the soft options, ie the ones returned
  /// by fitSoftOptions(). They MUST NOT change the number of
  /// parameters in the fit, else it will be a big mess.
  virtual void processSoftOptions(const CommandOptions & opts, FitData * data) const;

  /// @}

  /// Short description.
  QString shortDescription() const {
    return shortDesc;
  };

  /// Long description
  QString description() const {
    return longDesc;
  };

  /// Constructs a Fit object with the given parameters and registers
  /// the corresponding commands.
  Fit(const QString & n, const QString & sd, 
      const QString & desc,
      int min = 1, int max = -1, bool mkCmds = true) :
    name(n), shortDesc(sd), longDesc(desc),
    minDataSets(min), maxDataSets(max) { 
    registerFit(this);
    if(mkCmds)
      makeCommands();
  };

  virtual ~Fit();

  /// Runs the fit on the current dataset, that is, pops up the dialog
  /// box prompting for fit parameters and for user interaction in
  /// general.
  void runFitCurrentDataSet(const QString & name, const CommandOptions & opts);

  /// Runs the fit on the given datasets. \sa runFitCurrentDataSet
  void runFit(const QString & name, QList<const DataSet *> datasets,
              const CommandOptions & opts);

protected:
  /// This is the actual function running the fit. It gives the
  /// control to a hook that is called when the FitData has been setup
  /// but the fit options have not been processed yet. This hook can
  /// be used to process fit arguments.
  void runFit(std::function<void (FitData *)> hook, const QString & name, QList<const DataSet *> datasets,
              const CommandOptions & opts);

public:
  /// Use the given data file to compute the simulated data, without
  /// prompting
  void computeFit(const QString & name, QString file,
                  QList<const DataSet *> datasets,
                  const CommandOptions & opts);

protected:
  /// Is to the other computeFit like runFit()'s hooks @sa runFit
  void computeFit(std::function<void (FitData *)> hook, const QString & name,
                  QString file, QList<const DataSet *> datasets,
                  const CommandOptions & opts);
public:
  

  /// @name Subfunctions-related functions
  ///
  /// For some fits, the fit function naturally splits into
  /// subcomponents (mostly additive, but there's no reason to
  /// restrict ourselves to only that). We can think of that for fit
  /// for adsorbed species (ie display redox couples individually),
  /// but there's many more potential applications.
  ///
  /// These are functions for advertising and handling that.
  ///
  /// @{

  /// Whether or not the fit has subfunctions. Defaults to false.
  virtual bool hasSubFunctions(FitData * data) const;

  /// Whether or not to display the subfunctions by default when
  /// hasSubFunctions() returns true. Defaults to true.
  virtual bool displaySubFunctions(FitData * data) const;

  /// Computes the subfunctions. This function places the results
  /// inside the two target lists:
  /// \li \a targetData receives the raw data for all datasets;
  /// \li \a targetAnnotations receives a small text describing each of the
  /// data vectors
  ///
  /// All other arguments as in function().
  ///
  /// Default implementation raises an exception.
  ///
  /// @todo Having a single vector for the whole set of data is
  /// cumbersome. It would probably be nice to have it available on a
  /// per-dataset basis ? (but then it would mean chacheing ?)
  virtual void computeSubFunctions(const double * parameters,
                                   FitData * data, 
                                   QList<Vector> * targetData,
                                   QStringList * targetAnnotations) const;

  /// @}
};


#endif
