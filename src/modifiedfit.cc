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

  /// A list of extra conditions
  QStringList conditions;

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

    /// The conditions
    QList<Expression *> conditions;

    /// Storage for the underlying fit
    FitInternalStorage * underlyingStorage;

    /// The indices skipped from copying from full to final
    QSet<int> skippedIndices;
    
    virtual ~Storage()
    {
      for(QHash<int, Expression *>::iterator i = expressions.begin();
          i != expressions.end(); ++i)
        delete i.value();

      for(int i = 0; i < conditions.size(); i++)
        delete conditions[i];
      delete underlyingStorage;
    };

    Storage() {
    };
    
    Storage(const Storage & o) :
      originalParameters(o.originalParameters),
      finalParameters(o.finalParameters),
      totalSize(o.totalSize),
      underlyingStorage(NULL),
      skippedIndices(o.skippedIndices)
    {
      for(QHash<int, Expression *>::const_iterator i = o.expressions.begin();
          i != o.expressions.end(); ++i) {
        expressions[i.key()] = new Expression(*i.value());
      }
      for(int i = 0; i < o.conditions.size(); i++)
        conditions << new Expression(*o.conditions[i]);
    };
  };
  
  virtual FitInternalStorage * allocateStorage(FitData * data) const override {
    Storage * s = new Storage;
    s->underlyingStorage = underlyingFit->allocateStorage(data);
    return s;
  };
  
  virtual FitInternalStorage * copyStorage(FitData * data, FitInternalStorage * source, int ds) const override {
    Storage * s = static_cast<Storage *>(source);
    Storage * ns = deepCopy<Storage>(s);

    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->underlyingStorage);
      ns->underlyingStorage = underlyingFit->copyStorage(data, s->underlyingStorage, ds);
    }
    
    return ns;
  };

  void prepareExpressions(FitData * data) const
  {
    Storage * s = storage<Storage>(data);
    // QTextStream o(stdout);
    // o << "Called for fit " << this << " with data: " << s << endl;

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

    s->totalSize = fullP.size();

    QList<int> strippedIndices;
    // OK, so far, so good.
    for(QHash<QString, QString>::const_iterator i = redefinitions.begin();
        i != redefinitions.end(); ++i) {
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

    // Now prepare the condition expressions
    for(int i = 0; i < conditions.size(); i++) {
      QStringList varNames = fullP;
      QString nex = Expression::rubyIzeExpression(conditions[i], varNames);
      s->conditions << new Expression(nex, varNames);
    }

    // Now, prepare the parameters list
    qSort(strippedIndices);
    for(int i = strippedIndices.size() - 1; i >= 0; i--)
      s->finalParameters.takeAt(strippedIndices[i]);
    
  }

  virtual void processOptions(const CommandOptions & opts, FitData * data) const override
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

  virtual bool hasSubFunctions (FitData * data) const override
  {
    Storage * s = storage<Storage>(data);
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->underlyingStorage);
    return underlyingFit->hasSubFunctions(data);
  };

  virtual bool displaySubFunctions (FitData * data) const override
  {
    Storage * s = storage<Storage>(data);
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                      s->underlyingStorage);
    return underlyingFit->displaySubFunctions(data);
  };


  ArgumentList * fitSoftOptions() const override
  {
    return Fit::fitSoftOptions(underlyingFit);
  }

  ArgumentList * fitHardOptions() const override
  {
    return Fit::fitHardOptions(underlyingFit);
  }

  CommandOptions currentSoftOptions(FitData * data) const override
  {
    Storage * s = storage<Storage>(data);
    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      return Fit::currentSoftOptions(underlyingFit, data);
    }
  }

  void processSoftOptions(const CommandOptions & opts, FitData * data) const override
  {
    Storage * s = storage<Storage>(data);
    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      Fit::processSoftOptions(underlyingFit, opts, data);
    }
  }

  /// computes the parameters of the fit for the given buffer
  void computeParameters(FitData * data, 
                         const DataSet * /*ds*/,
                         const double * src,
                         double * dest, bool skipChecks = false) const
  {
    Storage * s = storage<Storage>(data);
    double buffer[s->totalSize];
    // QTextStream o(stdout);
    // o << s << " -- totl: " << s->totalSize << " -- " 
    //   << s->originalParameters.size() << endl;
    
    // for(int i : s->expressions.keys())
    //   o << "#" << i << " -> " << s->expressions[i]->formula()
    //     << " -- " << s->expressions[i]->currentVariables().join(", ")
    //     << endl;
    
    Utils::skippingCopy(src, buffer, s->totalSize, s->skippedIndices);

    // o << "Before:";
    // for(int i = 0; i < s->totalSize; i++)
    //   o << "\t" << buffer[i];
    // o << endl;

    // As many evaluations as formulas to ensure that all depths are
    // resolved.
    for(int j = 0; j < s->expressions.size(); j++) {
      for(QHash<int, Expression *>::const_iterator i = s->expressions.begin();
          i != s->expressions.end(); ++i)
        buffer[i.key()] = i.value()->evaluate(buffer);
    }

    // o << "After:";
    // for(int i = 0; i < s->totalSize; i++)
    //   o << "\t" << buffer[i];
    // o << endl;

    memcpy(dest, buffer, sizeof(double) * s->originalParameters.size());

    // Now, run the checks
    if(! skipChecks)
      for(int i = 0; i < s->conditions.size(); i++) {
        if(! s->conditions[i]->evaluateAsBoolean(buffer))
          throw RangeError("Condition #%1 -- %2 is not fulfilled").
            arg(i).arg(conditions[i]);
      }

  }
  
public:

  virtual QString optionsString(FitData * data) const override
  {
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

  virtual QList<ParameterDefinition> parameters(FitData * data) const override
  {
    Storage * s = storage<Storage>(data);
    return s->finalParameters;
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet * ds,
                            double * a) const override
  {
    Storage * s = storage<Storage>(data);
    double ii[s->totalSize];

    for(int i = 0; i < s->totalSize; i++)
      ii[i] = 1;                // As good as can be
    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      underlyingFit->initialGuess(data, ds, ii);
    }
    Utils::skippingCopy(ii, a, s->totalSize, s->skippedIndices, true);
  }

  virtual void function(const double * parameters,
                        FitData * data, 
                        const DataSet * ds,
                        gsl_vector * target) const override
  {
    Storage * s = storage<Storage>(data);
    double buf[s->originalParameters.size()];

    computeParameters(data, ds, parameters, buf);
    
    {
      TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
      underlyingFit->function(buf, data, ds, target);
    }
  };

  virtual void computeSubFunctions(const double * parameters,
                                   FitData * data, 
                                   const DataSet * ds,
                                   QList<Vector> * targetData,
                                   QStringList * targetAnnotations) const override
  {
    Storage * s = storage<Storage>(data);
    double buf[s->originalParameters.size()];

    computeParameters(data, ds, parameters, buf);
    
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                                        s->underlyingStorage);
    underlyingFit->computeSubFunctions(buf, data, ds,
                                       targetData, targetAnnotations);
  };


  ModifiedFit(const QString & name,
              const QStringList & newParams,
              const QHash<QString, QString> & redefs,
              PerDatasetFit * under, const QStringList & conds) :
  PerDatasetFit(name.toLocal8Bit(), 
                "Modified fit",
                "Modified fit", 1, -1, false),
  newParameters(newParams),
  redefinitions(redefs),
  underlyingFit(under),
  conditions(conds)
{

  ArgumentList * opts = new ArgumentList(*underlyingFit->fitHardOptions());
  ArgumentList * o2 = underlyingFit->fitSoftOptions();
  if(o2)
    (*opts) += *o2;
  makeCommands(NULL, NULL, NULL, opts);
}

  
};

//////////////////////////////////////////////////////////////////////
// Now, the command !

static void reparametrizeFit(const QString &, QString newName,
                             QString fitN,
                             QString extra,
                             QStringList redefs,
                             const CommandOptions & opts)
{
  
  QList<PerDatasetFit *> fts;
  bool overwrite  = false;
  updateFromOptions(opts, "redefine", overwrite);
  Fit::safelyRedefineFit(newName, overwrite);

  QStringList  conds;
  updateFromOptions(opts, "conditions", conds);

  PerDatasetFit * fit = dynamic_cast<PerDatasetFit *>(Fit::namedFit(fitN));
  if(! fit)
    throw RuntimeError("Cannot find fit %1").arg(fitN);

  QStringList newParams = extra.split(QRegExp("\\s*,\\s*"));

  QHash<QString, QString> redefinitions;
  for(int i = 0; i < redefs.size(); i++) {
    QStringList el = redefs[i].split(QRegExp("\\s*=\\s*"));
    if(el.size() < 2)
      throw RuntimeError("Argument '%1' is not a parameter redefinition").
        arg(redefs[i]);
    QString a = el.takeAt(0);
    redefinitions[a] = el.join("=");
  }

  new ModifiedFit(newName, newParams, redefinitions, fit, conds);
}


static ArgumentList 
rfA(QList<Argument *>() 
    << new StringArgument("name", "Name",
                          "name of the new fit")
    << new FitNameArgument("fit", "Fit",
                           "the fit to modify")
    << new StringArgument("new-parameters", "New parameters",
                          "Comma-separated list of new parameters")
    << new SeveralStringsArgument(QRegExp("\\s*;;\\s*"),
                                  "redefinitions", 
                                  "Redefinitions",
                                  "a list of redefinitions, separated by `;;`"));



static ArgumentList 
rfO(QList<Argument *>() 
    << new BoolArgument("redefine", 
                        "Redefine",
                        "If the new fit already exists, redefines it")
    << new SeveralStringsArgument("conditions", 
                                  "Conditions",
                                  "Additional conditions that must be fullfilled by the parameters (Ruby code)")
    );

static Command 
rf("reparametrize-fit",         // command name
   effector(reparametrizeFit),  // action
   "fits",                      // group name
   &rfA,                        // arguments
   &rfO,                        // options
   "Reparametrize fit",
   "Reparametrize a fit");
