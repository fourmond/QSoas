/**
   \file parameterspaceexplorator.hh
   Automatic generation of styles for display purposes
   Copyright 2013 by Vincent Fourmond

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
#ifndef __PARAMETERSPACEEXPLORATOR_HH
#define __PARAMETERSPACEEXPLORATOR_HH

class ParameterSpaceExplorator;

class ParameterRangeEditor;
class FitData;
class FitTrajectory;

class ParameterSpaceExploratorFactoryItem {
public:
  /// A generator function, taking the overall number of styles to
  /// give and an optional string argument.

  typedef ParameterSpaceExplorator * (*Creator) (FitData *);

protected:
  friend class ParameterSpaceExplorator;
  /// The creation function
  Creator creator;

public:
  /// The creation name
  QString name;

  /// The public name
  QString publicName;

  /// Creates and register a factory item.
  ParameterSpaceExploratorFactoryItem(const QString & n, 
                            const QString & pn, Creator c);
};

/// Generates a style for the next curve.
class ParameterSpaceExplorator {

  /// The application-wide ParameterSpaceExplorator factory
  static QHash<QString, ParameterSpaceExploratorFactoryItem*> * factory;

  friend class ParameterSpaceExploratorFactoryItem;

  static void registerFactoryItem(ParameterSpaceExploratorFactoryItem * item);

  static ParameterSpaceExploratorFactoryItem * namedFactoryItem(const QString & name);


protected:
  /// Underlying data
  FitData * data;

  /// a storage space for the editors
  QList<ParameterRangeEditor*> * editors;

public:

  /// Creates the parameter space exploration
  ParameterSpaceExplorator(FitData * data);

  virtual ~ParameterSpaceExplorator();

  /// A hash public name -> real name for the explorators.
  static QHash<QString, QString> availableExplorators();

  /// Creates the named parameter space explorator
  static ParameterSpaceExplorator * createExplorator(const QString & name,
                                                     FitData * data);
  

  /// Prepares the iterations.
  ///
  /// Returns the number of iterations.
  virtual int prepareIterations(QWidget * parameters,
                                QList<ParameterRangeEditor*> * editors,
                                QString * tg) = 0;


  /// Creates a widget suitable for editing the various parameters.
  virtual QWidget * createParametersWidget(QWidget * parent) = 0;

  /// Stores parameters for the next iteration within the
  /// editors. Returns a negative number when no further iterations
  /// are under way.
  ///
  /// The string is used to store a message to display on the progress
  /// dialog
  virtual int sendNextParameters(QString * tg) = 0;

  /// After the fit is performed, the trajectory is send to the
  /// explorator through this function. (a no-op in general)
  virtual void resultingTrajectory(const FitTrajectory & trj);
};


#endif
