/*
  combinedfit.cc: fit with automatic fitting of the combined
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
#include <combinedfit.hh>

#include <fitdata.hh>
#include <dataset.hh>
#include <perdatasetfit.hh>
#include <command.hh>
#include <argumentlist.hh>
#include <general-arguments.hh>
#include <commandeffector-templates.hh>

void CombinedFit::processOptions(const CommandOptions & opts)
{
  for(int i = 0; i < underlyingFits.size(); i++)
    Fit::processOptions(underlyingFits[i], opts);

  /// @todo any own options ?

  overallParameters.clear();
  firstParameterIndex.clear();
  // Now, handle the parameters
  for(int i = 0; i < ownParameters.size(); i++)
    overallParameters << ParameterDefinition(ownParameters[i]);
  
  for(int i = 0; i < underlyingFits.size(); i++) {
    PerDatasetFit * f = underlyingFits[i];
    QList<ParameterDefinition> params = f->parameters();
    firstParameterIndex << overallParameters.size();
    for(int j = 0; j < params.size(); j++) {
      ParameterDefinition def = params[j];
      def.name = QString("%1_f#%2").arg(def.name).arg(i+1);
      overallParameters << def;
    }
  }
}

void CombinedFit::initialGuess(FitData * data, 
                               const DataSet * ds,
                               double * guess)
{
  for(int i = 0; i < underlyingFits.size(); i++)
    underlyingFits[i]->initialGuess(data, ds, 
                                    guess + firstParameterIndex[i]);
}


QString CombinedFit::optionsString() const
{
  QStringList strs;
  for(int i = 0; i < underlyingFits.size(); i++)
    strs << QString("y%1: %2").arg(i+1).
      arg(underlyingFits[i]->fitName(true));
  return QString("%1 with %2").arg(formula.formula()).
    arg(strs.join(", "));
}

QList<ParameterDefinition> CombinedFit::parameters() const
{
  return overallParameters;
}


CombinedFit::~CombinedFit()
{
  
}

void CombinedFit::reserveBuffers(const DataSet * ds)
{
  for(int i = 0; i < underlyingFits.size(); i++) {
    if(i >= buffers.size())
      buffers << Vector();
    buffers[i].resize(ds->x().size());
  }
  /// @todo Potentially, this is a limited memory leak.
  
}

void CombinedFit::function(const double * parameters,
                           FitData * data, 
                           const DataSet * ds,
                           gsl_vector * target)
{
  reserveBuffers(ds);

  for(int i = 0; i < underlyingFits.size(); i++) {
    gsl_vector_view v = 
      gsl_vector_view_array(buffers[i].data(), ds->x().size());
    underlyingFits[i]->function(parameters + firstParameterIndex[i], data, 
                                ds, &v.vector);
  }

  // Now, combine everything...

  QVarLengthArray<double, 100> args(1 + underlyingFits.size() + 
                                    overallParameters.size());
  for(int i = 0; i < ds->x().size(); i++) {
    args[0] = ds->x()[i];
    int base = 1;
    for(int j = 0; j < underlyingFits.size(); j++, base++)
      args[base] = buffers[j][i];
    for(int j = 0; j < firstParameterIndex[0]; j++, base++)
      args[base] = parameters[j];
    gsl_vector_set(target, i, formula.evaluate(args.data()));
  }
  
}

CombinedFit::CombinedFit(const QString & name, const QString & f, 
                         QList<PerDatasetFit *> fits) :
  PerDatasetFit(name.toLocal8Bit(), 
                "Combined fit",
                "Combined fit", 1, -1, false), 
  underlyingFits(fits), formula(f)

{

  QStringList params;
  params << "x";
  for(int i = 0; i < fits.size(); i++)
    params << QString("y%1").arg(i+1);

  params << formula.naturalVariables();
  Utils::makeUnique(params);

  formula.setVariables(params);
  ownParameters = params.mid(1 + fits.size());

  ArgumentList * opts = new ArgumentList();

  for(int i = 0; i < underlyingFits.size(); i++) {
    PerDatasetFit *f = underlyingFits[i];
    Command * cmd = Command::namedCommand("fit-" + f->fitName(false));
    *opts << *cmd->commandOptions();
  }

  /// @todo Global register of options for fits...

  makeCommands(NULL, NULL, NULL, opts);
}


//////////////////////////////////////////////////////////////////////
// Now, the command !

static void combineFits(const QString &, QString newName,
                        QString formula,
                        QStringList fits,
                        const CommandOptions & opts)
{
  
  QList<PerDatasetFit *> fts;
  for(int i = 0; i < fits.size(); i++) {
      QString fitName = fits[i];
      PerDatasetFit * fit = 
        dynamic_cast<PerDatasetFit *>(Fit::namedFit(fitName));
      
      if(! fit)
        throw RuntimeError("The fit " + fitName + " isn't working "
                           "buffer-by-buffer: impossible to combine "
                           "it with others");
      fts << fit;
      }
  new CombinedFit(newName, formula, fts);
}

static ArgumentList 
cfA(QList<Argument *>() 
    << new StringArgument("name", "Name",
                          "The name of the new fit")
    << new StringArgument("formula", "Formula",
                          "How to combine the various fits")
    << new SeveralChoicesArgument(&Fit::availableFits,
                                  "fits", "Fits",
                                  "The fit to combine together"));

static Command 
cf("combine-fits", // command name
    effector(combineFits), // action
    "fits",  // group name
    &cfA, // arguments
    NULL, // options
    "Combine fits",
    "Combine different fits together",
    "(...)");
