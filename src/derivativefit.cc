/*
  derivativefit.cc: fit with automatic fitting of the derivative
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
#include <derivativefit.hh>

#include <fitdata.hh>
#include <dataset.hh>
#include <perdatasetfit.hh>
#include <command.hh>
#include <argumentlist.hh>
#include <general-arguments.hh>
#include <commandeffector-templates.hh>

void DerivativeFit::processOptions(const CommandOptions & opts)
{
  Fit::processOptions(underlyingFit, opts);
}


QString DerivativeFit::optionsString() const
{
  return Fit::optionsString(underlyingFit) + " -- derivative" + 
    (alsoOriginal ? "" : " only");
}

void DerivativeFit::checkDatasets(const FitData * data) const
{
  if(alsoOriginal) {
    if(data->datasets.size() != 2)
      throw RuntimeError("Fit " + name + " needs exactly two buffers");
  }
  // No restriction in the other case.
}

QList<ParameterDefinition> DerivativeFit::parameters() const
{
  QList<ParameterDefinition> params = underlyingFit->parameters(); 
  for(int i = 0; i < params.size(); i++)
    params[i].canBeBufferSpecific = false;
  return params;
}

void DerivativeFit::initialGuess(FitData * data, double * guess)
{
  underlyingFit->initialGuess(data, guess);
}

QString DerivativeFit::annotateDataSet(int idx) const
{
  if(! alsoOriginal)
    return QString();
  if(idx == 0)
    return "function";
  return "derivative";
}

DerivativeFit::~DerivativeFit()
{
}

void DerivativeFit::reserveBuffers(const FitData * data)
{
  for(int i = 0; i < data->datasets.size(); i++) {
    if(i >= buffers.size())
      buffers << Vector();
    buffers[i].resize(data->datasets[i]->x().size());
  }
  /// @todo Potentially, this is a limited memory leak.
  
}

void DerivativeFit::function(const double * parameters,
                             FitData * data, gsl_vector * target)
{
  reserveBuffers(data);
  int i = 0;
  if(alsoOriginal) {
    gsl_vector_view fnView = data->viewForDataset(0, target);
    const DataSet * baseDS = data->datasets[0];

    underlyingFit->function(parameters, data,
                            baseDS, &fnView.vector);
    ++i;
  }                      
  for(; i < data->datasets.size(); ++i) {
    gsl_vector_view derView = data->viewForDataset(i, target);
    gsl_vector_view bufView = buffers[i].vectorView(); 

    const DataSet * derDS = data->datasets[i];

    underlyingFit->function(parameters, data,
                            derDS, &bufView.vector);
    DataSet::firstDerivative(derDS->x().data(), 1, 
                             bufView.vector.data, bufView.vector.stride,
                             derView.vector.data, derView.vector.stride,
                             derDS->x().size());

  }
}

DerivativeFit::DerivativeFit(PerDatasetFit * source, bool as) :
  Fit(QString((as ? "deriv-" :"deriv-only-") +  
              source->fitName(false)).toLocal8Bit(), 
      "Derived fit",
      "(derived fit)",
      (as ? 2: 1) , (as ? 2: -1) , false), 
  underlyingFit(source), alsoOriginal(as)

{
  // How to remove the "parameters" argument ?
  Command * cmd = Command::namedCommand("fit-" + source->fitName(false));
  ArgumentList * opts = new ArgumentList(*cmd->commandOptions());

  /// @todo Add own options.

  makeCommands(NULL, NULL, NULL, opts);
}


//////////////////////////////////////////////////////////////////////

// Now, the command !


static void defineDerivedFit(const QString &, QString fitName, 
                             const CommandOptions & opts)
{
  PerDatasetFit * fit = dynamic_cast<PerDatasetFit *>(Fit::namedFit(fitName));

  if(! fit)
    throw RuntimeError("The fit " + fitName + " isn't working buffer-by-buffer: impossible to make a derived fit");

  bool derivOnly = false;
  updateFromOptions(opts, "deriv-only", derivOnly);
  
  new DerivativeFit(fit, !derivOnly);
}

static ArgumentList 
ddfA(QList<Argument *>() 
      << new ChoiceArgument(&Fit::availableFits,
                            "fit", "Fit",
                            "The fit to make a derived fit of"));

static ArgumentList 
ddfO(QList<Argument *>() 
     << new BoolArgument("deriv-only", "Derivative only",
                         "If true, one only wants to fit the derivatives, without fitting the original function at the same time"));


static Command 
ddf("define-derived-fit", // command name
    effector(defineDerivedFit), // action
    "fits",  // group name
    &ddfA, // arguments
    &ddfO, // arguments
    "Create a derived fit",
    "Create a derived fit to fit both the data and its derivative",
    "(...)");
