/**
   \file fitparametersfile.hh
   The FitParametersFile class, handling "saved parameters" files
   Copyright 2012, 2013, 2014, 2017 by CNRS/AMU

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
#ifndef __FITPARAMETERSFILE_HH
#define __FITPARAMETERSFILE_HH

class FitParameter;
class DataSet;

/// A storage class for parameters as read from a file
///
/// @todo Maybe this class could handle the writing of the parameters
/// file as well.
class FitParametersFile {
public:

  /// The names of the datasets, as guessed from the comments
  QHash<int, QString> datasetNames;

  /// The original name of the fit
  QString fitName;

  /// Comments (all of them)
  QStringList comments;
  
  class Parameter {
  public:
    /// Name of the parameter
    QString name;

    /// Index of the dataset (-1 if unspecified)
    int datasetIndex;

    /// String value of the parameter.
    QString value;

    Parameter(const QString & n, int ds, const QString & v) :
      name(n), datasetIndex(ds), value(v) {;};

    /// Returns the double value of the first '!'-separated part
    double toDouble(bool * ok = NULL) const {
      return value.split('!').first().toDouble(ok);
    };

    void replaceParameter(FitParameter * & parameter, double * tg, 
                          int idx, int ds);
  };

  /// Parameters (or pseudo-parameters) defined in the file
  QSet<QString> definedParameters;

  /// Parameter names in the order in which they are found in the file
  QStringList parametersOrder;


  bool hasPerpendicularCoordinates() const {
    return definedParameters.contains("Z");
  };

  /// All the parameters read from the file.
  QList<Parameter> parameters;

  /// Reads a stream and parses the contents into this structure
  void readFromStream(QTextStream & in);

  /// Returns the values of the parameters as a function of Z.
  ///
  QHash<QString,DataSet> parameterValuesAsfZ(bool makeupZ = false) const;  
};

#endif
