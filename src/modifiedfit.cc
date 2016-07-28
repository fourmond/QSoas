/*
  modifiedfit.cc: fit with redefinition of parameters
  Copyright 2016 by CNRS/AMU

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
#include <combinedfit.hh>

#include <fit.hh>
#include <expression.hh>
#include <fitdata.hh>
#include <dataset.hh>
#include <perdatasetfit.hh>
#include <command.hh>
#include <argumentlist.hh>
#include <general-arguments.hh>
#include <commandeffector-templates.hh>

#include <idioms.hh>

/// This fit permits the modification of a list of fits, a kind of
/// permanent extension of the fixed formula parameter.
/// It takes:
/// * a list of the new parameters to be derived
/// * a list of Param=Value (none of the Param can be
/// in the new parameters)
///
/// All the redefined parameters MUST be present in the fit upon
/// options processing.
///
/// The expressions can involve any parameters (and are evaluated in
/// an array with all the parameters).
/// 
class ModifiedFit : public PerDatasetFit {
protected:

  /// The list of newly-introduced parameters
  QStringList newParameters;

  /// A hash old parameter -> formula
  QHash<QString, QString> redefinitions;

  /// The underlying fit
  PerDatasetFit * underlyingFit;

  class Storage : public FitInternalStorage {
  public:

    /// Parameters of the underlying fit
    QList<ParameterDefinition> originalParameters;

    /// Final parameters
    QList<ParameterDefinition> finalParameters;

    /// Total size of the parameters
    int totalSize;
    
    /// The expressions, indexed on the full parameters list.
    QHash<int, Expression *> expressions;

    /// Storage for the underlying fit
    FitInternalStorage * underlyingStorage;

    /// The indices skipped from copying from full to final
    QSet<int> skippedIndices;
    
    virtual ~Storage()
    {
      for(QHash<int, Expression *>::iterator i = expressions.begin(); i != expressions.end(); i++)
        delete i.value();
      delete underlyingStorage;
    };

    Storage() {
    };
    
    Storage(const Storage & o) :
      originalParameters(o.originalParameters),
      finalParameters(o.finalParameters),
      totalSize(o.totalSize),
      underlyingStorage(NULL)
    {
      for(QHash<int, Expression *>::const_iterator i = o.expressions.begin();
          i != o.expressions.end(); i++) {
        expressions[i.key()] = new Expression(*i.value());
      }
    };
  };
  
  virtual FitInternalStorage * allocateStorage(FitData * /*data*/) const {
    return new Storage;
  };
  
  virtual FitInternalStorage * copyStorage(FitData * data, FitInternalStorage * source, int ds) const {
    Storage * s = static_cast<Storage *>(source);
    Storage * ns = deepCopy<Storage>(s);

    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->underlyingStorage);
      s->underlyingStorage = underlyingFit->copyStorage(data, s->underlyingStorage, ds);
    }
    
    return ns;
  };

  void prepareExpressions(FitData * data) const
  {
    Storage * s = storage<Storage>(data);

    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      s->originalParameters = underlyingFit->parameters(data);
    }

    s->finalParameters = s->originalParameters;

    QStringList orgP;
    QStringList fullP;

    QStringList varNames;
    QStringList finalP;
    QHash<QString, int> indices;

    for(int i = 0; i < s->originalParameters.size(); i++) {
      orgP << s->originalParameters[i].name;
      indices[s->originalParameters[i].name] = i;
    }

    fullP = orgP;
    for(int i = 0; i < newParameters.size(); i++) {
      const QString & n = newParameters[i];
      if(indices.contains(n))
        throw RuntimeError("Trying to add an already existing parameter: %1").
          arg(n);
      indices[n] = fullP.size();
      fullP << n;
      s->finalParameters << ParameterDefinition(n);  
    }

    QList<int> strippedIndices;
    // OK, so far, so good.
    for(QHash<QString, QString>::const_iterator i = redefinitions.begin();
        i != redefinitions.end(); i++) {
      if(! indices.contains(i.key())) {
        throw RuntimeError("Trying to redefine parameter %1, which does not exist").
          arg(i.key());
      }
      int idx = indices[i.key()];
      strippedIndices << idx;
      s->skippedIndices.insert(idx);
      if(s->expressions.contains(idx))
        throw RuntimeError("Trying to redefine parameter %1 twice").
          arg(i.key());

      QStringList varNames = fullP;
      QString nex = Expression::rubyIzeExpression(i.value(), varNames);
      s->expressions[idx] = new Expression(nex, varNames);
    }

    // Now, prepare the parameters list
    qSort(strippedIndices);
    for(int i = strippedIndices.size() - 1; i >= 0; i--)
      s->finalParameters.takeAt(strippedIndices[i]);
    
  }

  virtual void processOptions(const CommandOptions & opts, FitData * data) const
  {
    Storage * s = storage<Storage>(data);

    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      Fit::processOptions(underlyingFit, opts, data);
    }

    // Now, should process the parameters
    prepareExpressions(data);
  }

  virtual QString optionsString(FitData * data) const {
    Storage * s = storage<Storage>(data);

    QString so;
    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      so = underlyingFit->optionsString(data);
    }

    if(! so.isEmpty())
      so = " (options: " + so + ")";

    QStringList defs = redefinitions.keys();
    qSort(defs);

    return QString("fit: %1%2, with %3 redefined").
      arg(underlyingFit->fitName(false)).arg(so).arg(defs.join(", "));
  };
  
public:
  
};
