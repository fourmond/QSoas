/**
   \file fit.hh
   Implementation of fits.
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


#ifndef __FIT_HH
#define __FIT_HH

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


/// This abstract class defines the interface for handling fits.
///
/// @todo handle validity range checking
class Fit {
protected:

  /// Fit name, used to make up command names
  QString name;

  /// Fit description
  const char * shortDesc;

  /// Long description
  const char * longDesc;

  /// Prepares and registers the commands corresponding to the fit.
  ///
  /// If provided, the commands will have \a args as argument list
  /// (with the additional buffer arguments for the multiFit command)
  void makeCommands(ArgumentList * args = NULL, 
                    CommandEffector * singleFit = NULL,
                    CommandEffector * multiFit = NULL,
                    ArgumentList * options = NULL);

  /// The minimum number of datasets the fit should take.
  int minDataSets;

  /// The maximum number of datasets the fit can take (-1 for
  /// boundless)
  int maxDataSets;
public:

  /// The fit name
  QString fitName() const {
    return name;
  };  

  /// @name Public interface
  ///
  /// Most of these functions need to be reimplemented by subclasses.
  ///
  /// @{

  /// The parameters
  virtual QList<ParameterDefinition> parameters() const = 0;
  

  /// Computes the function (ie in the absence of the residuals)
  ///
  /// \a parameters are given in the fully expanded form, ie
  /// parameters().size() times number of datasets
  ///
  /// Keep in mind that in multidataset mode, \a target contains \b
  /// all datasets !
  virtual void function(const double * parameters,
                        FitData * data, gsl_vector * target) = 0;

  /// Does the same thing as function, but only updates the part
  /// corresponding to the given dataset.
  ///
  /// \warning It is not guaranteed that the function will not be
  /// computed elsewhere. What is guaranteed, though, is that the
  /// values for the given dataset are updated, and that whatever else
  /// is updated has the correct value.
  virtual void functionForDataset(const double * parameters,
                                  FitData * data, gsl_vector * target, 
                                  int dataset);

  /// This function is called at the beginning of each function
  /// evaluation. If it returns a non-zero value, the computation is
  /// cancelled and the given value is returned.
  ///
  /// Default implementation returns 0.
  virtual int parametersCheck(const double * parameters,
                              FitData * data);


  /// Returns a string meant for the user to understand what is the
  /// specific role of the given dataset.
  ///
  /// Of course, this function only make sense for fits that treat
  /// buffers differently.
  virtual QString annotateDataSet(int idx) const;
  /// @}

  /// Prepares an initial guess for all parameters. The \a guess array
  /// is a fully expanded version of the parameters.
  ///
  /// \see packParameters()
  /// \see unpackParameters()
  virtual void initialGuess(FitData * data, double * guess) = 0;

  /// Short description.
  QString shortDescription() const {
    return QObject::tr(shortDesc);
  };

  /// Long description
  QString description() const {
    return QObject::tr(longDesc);
  };

  /// Constructs a Fit object with the given parameters and registers
  /// the corresponding commands.
  Fit(const char * n, const char * sd, const char * desc,
      int min = 1, int max = -1, bool mkCmds = true) :
    name(n), shortDesc(sd), longDesc(desc),
    minDataSets(min), maxDataSets(max) { 
    if(mkCmds)
      makeCommands();
  };

  virtual ~Fit();

  /// Runs the fit on the current dataset, that is, pops up the dialog
  /// box prompting for fit parameters and for user interaction in
  /// general.
  void runFitCurrentDataSet(const QString & name);

  /// Runs the fit on the given datasets. \sa runFitCurrentDataSet
  void runFit(const QString & name, QList<const DataSet *> datasets);
};


#endif
