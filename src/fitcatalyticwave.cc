/*
  fitcatalyticwave.cc: a fit for catalytic waves
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
#include <kineticsystemsteadystate.hh>

#include <kineticsystem.hh>
#include <exceptions.hh>


#include <soas.hh>
#include <terminal.hh>
#include <command.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>
#include <vector.hh>
#include <dataset.hh>
#include <datastack.hh>

#include <expression.hh>

// Here come fits !
#include <perdatasetfit.hh>
#include <fitdata.hh>


/// Fits a full kinetic system.
class CatalyticWaveFit : public PerDatasetFit {

  /// System we're working on - deleted and recreated for each fit.
  KineticSystem * system;

  /// The evolver... Created and delete at the same time as each fit.
  KineticSystemSteadyState * solver;

  bool dispersion;

  mutable int potIdx;

protected:

  virtual void processSoftOptions(const CommandOptions & opts) {
    solver->forceNonLinear = false;
    updateFromOptions(opts, "force-nonlinear", solver->forceNonLinear);

    // @todo Add integration options
  }

  virtual void processOptions(const CommandOptions & opts)
  {
    dispersion = false;
    updateFromOptions(opts, "dispersion", dispersion);

    QStringList extra;
    if(dispersion)
      extra << "bd0";

    system->prepareForSteadyState(extra);
    
    solver = new KineticSystemSteadyState(system);

    processSoftOptions(opts);
  }

  void runFit(const QString & name, QString fileName, 
              QList<const DataSet *> datasets,
              const CommandOptions & opts)
  {
    delete system;
    system = NULL;
    delete solver;
    solver = NULL;             // Prevent segfault upon destruction
                                // if next step fails
    system = new KineticSystem;
    system->parseFile(fileName);

    Fit::runFit(name, datasets, opts);
  }

  void runFitCurrentDataSet(const QString & n, 
                            QString fileName, const CommandOptions & opts)
  {
    QList<const DataSet *> ds;
    ds << soas().currentDataSet();
    runFit(n, fileName, ds, opts);
  }


public:

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> defs;

    QStringList parameters = system->allParameters();

    bool notSeenKinetic = true; // First kinetic parameter is fixed by
                                // default
    for(int i = 0; i < parameters.size(); i++) {
      QString p = parameters[i];
      bool fixed = false;
      if(p == "temperature")
        fixed = true;
      else if(p == "e") {
        potIdx = i;
        continue;
      }
      else if(p == "c_tot") {
        p = "nFSGamma";
      }
      else if(p.startsWith("k") && notSeenKinetic) {
        fixed = true;
        notSeenKinetic = false;
      }
      defs << ParameterDefinition(p, fixed); 
    }
    return defs;
  };

  virtual void function(const double * a, FitData * data, 
                        const DataSet * ds , gsl_vector * target)
  {
    int nb = system->allParameters().size();
    double params[nb];
    
    for(int i = 0, j = 0; i < nb; i++) {
      if(i == potIdx)
        continue;
      solver->setParameter(i, a[j]);
      ++j;
    }
    solver->computeVoltammogram(ds->x(), target, NULL);
  };

  virtual void initialGuess(FitData * data, 
                            const DataSet *ds,
                            double * a)
  {
    const QList<ParameterDefinition> &defs = data->parameterDefinitions;
    for(int i = 0; i < defs.size(); i++) {
      const QString & p = defs[i].name;
      if(p == "temperature")
        a[i] = soas().temperature();
      else if(p == "nFSGamma") {
        a[i] = fabs(ds->y().magnitude());
      }
      else if(p.startsWith("e"))
        a[i] = 0.5 * (ds->x().min() + ds->x().max());
      else
        a[i] = 1;
    }
  };

  virtual ArgumentList * fitArguments() const {
    if(system)
      return NULL;
    ArgumentList * al = new 
      ArgumentList(QList<Argument *>()
                   << new FileArgument("system", 
                                       "System",
                                       "Path to the file describing the "
                                       "system"));
    return al;
  };

  virtual ArgumentList * fitHardOptions() const {
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new BoolArgument("dispersion", 
                                       "Dispersion",
                                       "If on, handles the dispersion of k_0 values")
                   );
    return opts;
  };

  virtual ArgumentList * fitSoftOptions() const {
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new BoolArgument("force-nonlinear", 
                                       "Force non-linear",
                                       "Force the use of the non-linear solver, even for a linear system")
                   );
    return opts;
  };

  CatalyticWaveFit() :
    PerDatasetFit("catalytic-wave", 
                  "Catalytic wave",
                  "", 1, -1, false), system(NULL), solver(NULL)
  { 



    makeCommands(NULL, 
                 effector(this, &CatalyticWaveFit::runFitCurrentDataSet, true),
                 effector(this, &CatalyticWaveFit::runFit, true),
                 NULL);
  };

  CatalyticWaveFit(const QString & name, 
                   KineticSystem * sys,
                   const QString & file
                   ) : 
    PerDatasetFit(name.toLocal8Bit(), 
                  QString("Catalytic wave of %1").arg(file).toLocal8Bit(),
                  "", 1, -1, false), system(sys)
  {
    makeCommands();
  }

};

static CatalyticWaveFit fit_catalytic_wave;

//////////////////////////////////////////////////////////////////////

static ArgumentList 
dLWFArgs(QList<Argument *>() 
         << new FileArgument("file", 
                             "System",
                              "System to load")
         << new StringArgument("name", 
                               "Name")
         );


static void defineCWFitCommand(const QString &, QString file, 
                               QString name)
{
  /// @todo exception safe (ie guarded pointer detached in the end)
  KineticSystem * ks = new KineticSystem;
  ks->parseFile(file);

  new CatalyticWaveFit(name, ks, file);
}


static Command 
dlwf("define-catalytic-wave-fit", // command name
     optionLessEffector(defineCWFitCommand), // action
     "fits",                                  // group name
     &dLWFArgs,                              // arguments
     NULL,                                   // options
     "Define a fit based on a kinetic mode",
     "Defines a new fit based on kinetic model",
     "...");
