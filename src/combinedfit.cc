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

// void CombinedFit::reserveBuffers(const FitData * data)
// {
//   for(int i = 0; i < data->datasets.size(); i++) {
//     if(i >= buffers.size())
//       buffers << Vector();
//     buffers[i].resize(data->datasets[i]->x().size());
//   }
//   /// @todo Potentially, this is a limited memory leak.
  
// }

void CombinedFit::function(const double * parameters,
                        FitData * data, 
                        const DataSet * ds,
                        gsl_vector * target)
{
}

CombinedFit::CombinedFit(const QString & name, const QString & f, 
                         QList<PerDatasetFit *> fits) :
  PerDatasetFit(name.toLocal8Bit(), 
                "Combined fit",
                "Combined fit", 1, -1, false), 
  underlyingFits(fits), formula(f)

{
  ArgumentList * opts = new ArgumentList();
  // How to remove the "parameters" argument ?
  for(int i = 0; i < underlyingFits.size(); i++) {
    PerDatasetFit *f = underlyingFits[i];
    Command * cmd = Command::namedCommand("fit-" + f->fitName(false));
    *opts << *cmd->commandOptions();

    QList<ParameterDefinition> params = f->parameters();
    firstParameterIndex << overallParameters.size();
    for(int j = 0; j < params.size(); j++) {
      ParameterDefinition def = params[j];
      def.name = QString("%1_f#%2").arg(def.name).arg(i+1);
      overallParameters << def;
    }
  }

  /// @todo implement

  /// @todo Global register of options for fits...

  makeCommands(NULL, NULL, NULL, opts);
}


// //////////////////////////////////////////////////////////////////////
// // Now, the command !

// static void defineDerivedFit(const QString &, QString fitName, 
//                              const CommandOptions & opts)
// {
//   PerDatasetFit * fit = dynamic_cast<PerDatasetFit *>(Fit::namedFit(fitName));

//   if(! fit)
//     throw RuntimeError("The fit " + fitName + " isn't working buffer-by-buffer: impossible to make a derived fit");

//   bool derivOnly = false;
//   updateFromOptions(opts, "deriv-only", derivOnly);
  
//   new CombinedFit(fit, !derivOnly);
// }

// static ArgumentList 
// ddfA(QList<Argument *>() 
//       << new ChoiceArgument(&Fit::availableFits,
//                             "fit", "Fit",
//                             "The fit to make a derived fit of"));

// static ArgumentList 
// ddfO(QList<Argument *>() 
//      << new BoolArgument("deriv-only", "Combined only",
//                          "If true, one only wants to fit the combineds, without fitting the original function at the same time"));


// static Command 
// ddf("define-derived-fit", // command name
//     effector(defineDerivedFit), // action
//     "fits",  // group name
//     &ddfA, // arguments
//     &ddfO, // arguments
//     "Create a derived fit",
//     "Create a derived fit to fit both the data and its combined",
//     "(...)");
