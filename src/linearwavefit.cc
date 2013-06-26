/**
   \file linearwavefit.cc: implementation of fits using LinearWave
   Copyright 2012 by Vincent Fourmond

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
#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>
#include <terminal.hh>
#include <soas.hh>

#include <datastack.hh>
#include <linearwave.hh>

#include <perdatasetfit.hh>
#include <dataset.hh>
#include <vector.hh>
#include <fitdata.hh>
#include <exceptions.hh>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_math.h>

class LinearWaveFit : public FunctionFit {
private:

  /// The system description (owned)
  LinearWave * system;

  int referenceRate;

protected:

  virtual QString optionsString() const {
    return "(linear wave)";
  };

  virtual void processOptions(const CommandOptions & opts) {
    QString ref;
    updateFromOptions(opts, "reference-rate", ref);
    if(! ref.isEmpty()) {
      QStringList params = system->parameterNames();
      int idx = params.indexOf(ref);
      if(idx >= 0 && system->parameterType(idx) == LinearWave::Rate)
        referenceRate = idx;
      else
        Terminal::out << "Unkown rate: " << ref << endl;
    }
  };


    
public:

  virtual void initialGuess(FitData *data, const DataSet *ds, 
                            double *a)
  {
    a[0] = soas().temperature();
    for(int i = 0; i < system->parametersNumber(); i++) {
      if(i == referenceRate) {
        a[i + 1] = fabs(ds->y().magnitude());
        continue;
      }
      switch(system->parameterType(i)) {
      case LinearWave::Rate:
        a[i + 1] = 1;
        break;
      case LinearWave::Alpha:
        a[i + 1] = 0.5;
        break;
      case LinearWave::Potential:
        a[i + 1] = 0.5 * (ds->x().min() + ds->x().max());
        break;
      }
    }
  };

  virtual QList<ParameterDefinition> parameters() const {
    QList<ParameterDefinition> p;
    p << ParameterDefinition("temperature", true);

    QStringList params = system->parameterNames();
    QString refName = params[referenceRate];
    for(int i = 0; i < params.size(); i++) {
      QString name = params[i];
      if(system->parameterType(i) == LinearWave::Rate) {
        if(i == referenceRate)
          name = "iref_"+name;
        else
          name += "/" + refName;
      }
      p << ParameterDefinition(name, system->parameterType(i) == 
                               LinearWave::Alpha);
    }

    return p;
  };

  virtual double function(const double *a, FitData *data, double x)
  {
    QVarLengthArray<double, 100> params(system->parametersNumber());

    for(int i = 0; i < system->parametersNumber(); i++) {
      if(system->parameterType(i) == LinearWave::Rate && i != referenceRate)
        params[i] = a[i+1] * a[referenceRate + 1];
      else
        params[i] = a[i+1];
      
      // Now, parameter range check
      switch(system->parameterType(i)) {
      case LinearWave::Rate:
        if(params[i] < 0)
          throw RangeError("Negative rate constant");
        break;
      default:
        ;
      }
    }
    return system->computeCurrent(x, a[0], params.data());
  }


  LinearWaveFit(const char * name, LinearWave * sys) : 
    FunctionFit(name, 
                "Arbitrary fit",
                "Arbitrary fit with user-supplied formula "
                "(possibly spanning multiple buffers)\n"
                "Special parameters: x_0, y_0, temperature, fara.\n"
                "Already defined constants: f, pi",
                1, -1, false), system(sys)
  { 
    ArgumentList * opts = new 
      ArgumentList(QList<Argument *>()
                   << new StringArgument("reference-rate",
                                         "Reference rate",
                                         "The reference rate, ie the one "
                                         "used for real current determination; "
                                         "all the other rates are relative to this one" )
                   );
    makeCommands(NULL, NULL, NULL, opts);

    // We need to pick up a reference rate;
    referenceRate = 0;
    while(referenceRate < system->parametersNumber() && 
          system->parameterType(referenceRate) != LinearWave::Rate)
      referenceRate++;
  };

};

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////

static ArgumentList 
dLWFArgs(QList<Argument *>() 
         << new FileArgument("file", 
                             "System",
                              "System to load")
         << new StringArgument("name", 
                               "Name")
         );


static void defineLWFitCommand(const QString &, QString file, 
                               QString name)
{
  QFile f(file);
  Utils::open(&f, QIODevice::ReadOnly);
  LinearWave * lw = new LinearWave;
  lw->parseSystem(&f);

  new LinearWaveFit(name.toLocal8Bit(), lw);
}


static Command 
dlwf("define-lw-fit", // command name
     optionLessEffector(defineLWFitCommand), // action
     "fits",                                  // group name
     &dLWFArgs,                              // arguments
     NULL,                                   // options
     "Define a fit based on a kinetic mode",
     "Defines a new fit based on kinetic model",
     "...");
