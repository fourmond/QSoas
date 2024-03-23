/*
  distribfit.cc: fit with automatic fitting of the distrib
  Copyright 2015,2016 by CNRS/AMU

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
#include <distribfit.hh>

#include <fitdata.hh>
#include <dataset.hh>
#include <perdatasetfit.hh>
#include <command.hh>
#include <argumentlist.hh>
#include <general-arguments.hh>
#include <commandeffector-templates.hh>

#include <multiintegrator.hh>

#include <idioms.hh>

#include <gsl/gsl_randist.h>
#include <gsl/gsl_cdf.h>


QHash<QString, Distribution *> * Distribution::distributions = NULL;

Distribution::Distribution(const QString & n) : name(n)
{
  if(! distributions)
    distributions = new QHash<QString, Distribution *>;
  (*distributions)[n] = this;
}

Distribution::~Distribution()
{
}

QStringList Distribution::availableDistributions()
{
  if(distributions)
    return distributions->keys();
  return QStringList();
}

const Distribution * Distribution::namedDistribution(const QString & n)
{
  if(! distributions)
    return NULL;
  return distributions->value(n, NULL);
}

double Distribution::convertParameter(const double *, double value) const
{
  return value;
}



//////////////////////////////////////////////////////////////////////
// Now, several distributions

// Gaussian distribution
class GaussianDistribution : public Distribution {
public:
  virtual QList<ParameterDefinition> parameters(const QString & param) const override {
    QList<ParameterDefinition> ret;
    ret << ParameterDefinition(QString("%1_avg").arg(param))
        << ParameterDefinition(QString("%1_sigma").arg(param))
        << ParameterDefinition(QString("%1_extent").arg(param), true);
    return ret;
  };
  
  void range(const double * parameters, double * first,
             double * last) const override {
    double center = parameters[0];
    double sigm = parameters[1];
    if(sigm < 0)
      throw RangeError("sigma must be positive");
    double ext = parameters[2];
    *first = center - fabs(sigm * ext);
    *last = center + fabs(sigm * ext);
  };
  
  virtual double weight(const double * parameters, double x) const override {
    double center = parameters[0];
    double sigm = parameters[1];
    return gsl_ran_gaussian_pdf(x - center, sigm);
  };
  
  virtual double rangeWeight(const double * parameters) const override {
    double ext = parameters[2];
    return 2 * gsl_cdf_gaussian_P(ext, 1) - 1;
  }
  
  virtual void initialGuess(double * parameters, double value) const override {
    parameters[0] = value;
    parameters[1] = fabs(value * 0.1);
    if(parameters[1] == 0)
      parameters[1] = 0.5;      // as good as anything
    parameters[2] = 5;
  };


  GaussianDistribution() : Distribution("gaussian") {
  };
};

static GaussianDistribution gaussianDistribution;

// Lorentzian distribution
class LorentzianDistribution : public Distribution {
public:
  virtual QList<ParameterDefinition> parameters(const QString & param) const override {
    QList<ParameterDefinition> ret;
    ret << ParameterDefinition(QString("%1_avg").arg(param))
        << ParameterDefinition(QString("%1_gamma").arg(param))
        << ParameterDefinition(QString("%1_extent").arg(param), true);
    return ret;
  };
  
  void range(const double * parameters, double * first,
             double * last) const override {
    double center = parameters[0];
    double sigm = parameters[1];
    if(sigm < 0)
      throw RangeError("gamma must be positive");
    double ext = parameters[2];
    *first = center - fabs(sigm * ext);
    *last = center + fabs(sigm * ext);
  };
  
  virtual double weight(const double * parameters, double x) const override {
    double center = parameters[0];
    double sigm = parameters[1];
    return gsl_ran_cauchy_pdf(x - center, sigm);
  };
  
  virtual double rangeWeight(const double * parameters) const override {
    double ext = parameters[2];
    return 2 * gsl_cdf_cauchy_P(ext, 1) - 1;
  }
  
  virtual void initialGuess(double * parameters, double value) const override {
    parameters[0] = value;
    parameters[1] = fabs(value * 0.1);
    if(parameters[1] == 0)
      parameters[1] = 0.5;      // as good as anything
    parameters[2] = 30;
  };


  LorentzianDistribution() : Distribution("lorentzian") {
  };
};

static LorentzianDistribution lorentzianDistribution;

// so called k0 distribution from Leger's 2002 J Phys Chem B paper,
// doi: 10.1021/jp0265687
//
// In short, the integrated parameter follows the depency:
// k0 = k0m * exp(-beta * d)
//
// Integrated between d = 0 and dmax (uniform distribution).
class K0Distribution : public Distribution {
public:
  virtual QList<ParameterDefinition> parameters(const QString & param) const override {
    QList<ParameterDefinition> ret;
    ret << ParameterDefinition(QString("%1_max").arg(param))
        << ParameterDefinition(QString("%1_betadmax").arg(param));
    return ret;
  };
  
  void range(const double * parameters, double * first,
             double * last) const override {
    *first = 0; 
    *last = parameters[1];
    if(parameters[1] < 0)
      throw RangeError("betadmax must be positive");

  };
  
  virtual double weight(const double * parameters, double ) const override {
    return 1/parameters[1];
  };
  
  virtual double rangeWeight(const double * parameters) const override {
    return 1;
  }
  
  virtual void initialGuess(double * parameters, double value) const override {
    parameters[0] = value;
    parameters[1] = 10;
  };

  virtual double convertParameter(const double * parameters, double value) const override {
    double k0 = parameters[0];
    return k0 * exp(-value);
  };


  K0Distribution() : Distribution("log-uniform") {
  };
};

static K0Distribution k0Distribution;

// Uniform distribution between two values
class UniformDistribution : public Distribution {
public:
  virtual QList<ParameterDefinition> parameters(const QString & param) const override {
    QList<ParameterDefinition> ret;
    ret << ParameterDefinition(QString("%1_min").arg(param))
        << ParameterDefinition(QString("%1_max").arg(param));
    return ret;
  };
  
  void range(const double * parameters, double * first,
             double * last) const override {
    *first = parameters[0]; 
    *last = parameters[1];
  };
  
  virtual double weight(const double * parameters, double ) const override {
    return 1/(parameters[1] - parameters[0]);
  };
  
  virtual double rangeWeight(const double * parameters) const override {
    return 1;
  }
  
  virtual void initialGuess(double * parameters, double value) const override {
    if(value == 0) {
      parameters[0] = -0.1;
      parameters[1] = 0.1;
    }
    else {
      parameters[0] = value * 0.8;
      parameters[1] = value * 1.2;
      if(value < 0)
        std::swap(parameters[0], parameters[1]);
    }
  };

  UniformDistribution() : Distribution("uniform") {
  };
};

static UniformDistribution uniformDistribution;





//////////////////////////////////////////////////////////////////////


DistribFit::Storage::~Storage()
{
  delete sub;
}


FitInternalStorage * DistribFit::allocateStorage(FitData * data) const
{
  Storage * s = new Storage;
  s->sub = underlyingFit->allocateStorage(data);
  return s;
}

FitInternalStorage * DistribFit::copyStorage(FitData * data,
                                             FitInternalStorage * source,
                                             int ds) const
{
  Storage * s = static_cast<Storage *>(source);
  Storage * s2 = new Storage(*s);
  s2->sub = NULL;
  {
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                           s->sub);
    s2->sub = underlyingFit->copyStorage(data, s->sub, ds);
  }
  
  if(s->integrator)
    s2->integrator = s->integrator->dup();
  return s2;
}


//////////////////////////////////////////////////////////////////////

void DistribFit::processOptions(const CommandOptions & opts, FitData * data) const
{
  Storage * s = storage<Storage>(data);
  
  {
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                           s->sub);
    Fit::processOptions(underlyingFit, opts, data);
    s->parametersCache = underlyingFit->parameters(data);

    s->parameterIndex = -1;
    for(int i = 0; i < s->parametersCache.size(); i++) {
      if(s->parametersCache[i].name == parameterName) {
        s->parameterIndex = i;
        break;
      }
    }
    if(s->parameterIndex < 0)
      throw RuntimeError("Parameter %1 is missing from the fit %2").
        arg(parameterName).arg(underlyingFit->fitName(true, data));

  }

  s->parametersCache.takeAt(s->parameterIndex);
  s->distribIndex = s->parametersCache.size();

  /// @todo Process own options

  QString n;
  updateFromOptions(opts, "distribution", n);
  s->dist = Distribution::namedDistribution(n);
  if(! s->dist)
    s->dist = defaultDistribution;

  s->parametersCache << s->dist->parameters(parameterName);

  processSoftOptions(opts, data);
}

void DistribFit::processSoftOptions(const CommandOptions & opts, FitData * data) const
{
  Storage * s = storage<Storage>(data);
  {
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                           s->sub);
    Fit::processSoftOptions(underlyingFit, opts, data);
  }
  if(s->integrator)
    delete s->integrator;
  s->integrator = MultiIntegrator::fromOptions(opts,
                                               MultiIntegrator::Function(), 0);
}

void DistribFit::initialGuess(FitData * data, 
                              const DataSet * ds,
                              double * guess) const
{
  Storage * s = storage<Storage>(data);
  {
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage,
                                           s->sub);
    underlyingFit->initialGuess(data, ds, guess);
  }
  double value = guess[s->parameterIndex];
  for(int i = s->parameterIndex; i < s->distribIndex; i++)
    guess[i] = guess[i+1];
  s->dist->initialGuess(guess + s->distribIndex, value);
}

ArgumentList DistribFit::fitHardOptions() const
{
  ArgumentList rv = underlyingFit->fitHardOptions();
  rv << new ChoiceArgument(Distribution::availableDistributions,
                           "distribution",
                           "Distribution", "Distribution for the argument");
  return rv;
}


QString DistribFit::optionsString(FitData * data) const
{
  Storage * s = storage<Storage>(data);
  QString sub;
  {
    TemporaryThreadLocalChange<FitInternalStorage*> d(data->fitStorage, s->sub);
    sub = underlyingFit->fitName(true, data);
  }
  return QString("%1 with %2 %3").arg(sub).arg(s->dist->name).
    arg(parameterName);
}

QList<ParameterDefinition> DistribFit::parameters(FitData * data) const
{
  Storage * s = storage<Storage>(data);
  return s->parametersCache;
}


DistribFit::~DistribFit()
{
  
}


void DistribFit::function(const double * parameters,
                            FitData * data, 
                            const DataSet * ds,
                            gsl_vector * target) const
{
  Storage * s = storage<Storage>(data);
  TemporaryThreadLocalChange<FitInternalStorage*> dd(data->fitStorage,
                                          s->sub);

  // Splice parameters
  double ddd[s->distribIndex + 1];
  double * d = ddd;
  const double * distParams = parameters + s->distribIndex;
  for(int i = 0; i < s->parameterIndex; i++)
    d[i] = parameters[i];
  for(int i = s->parameterIndex; i < s->distribIndex; i++)
    d[i+1] = parameters[i];
  MultiIntegrator::Function fcn = [data, s, d, ds, distParams, this](double x, gsl_vector * tgt) {
    d[s->parameterIndex] = s->dist->convertParameter(distParams, x);
    underlyingFit->function(d, data, ds, tgt);
    gsl_vector_scale(tgt, s->dist->weight(distParams, x));
  };

  s->integrator->reset(fcn, target->size);
  double left, right;
  s->dist->range(distParams, &left, &right);
  s->integrator->integrate(target, left, right);
  gsl_vector_scale(target, 1/s->dist->rangeWeight(distParams));
}


ArgumentList DistribFit::fitSoftOptions() const
{
  ArgumentList rv = underlyingFit->fitSoftOptions();
  rv << MultiIntegrator::integratorOptions();
  return rv;
}

DistribFit::DistribFit(const QString & name, const QString & param, 
                       PerDatasetFit *fit, const Distribution * defdist) :
  PerDatasetFit(name, 
                "Distrib fit",
                "Distrib fit", 1, -1, false),
  parameterName(param), underlyingFit(fit)
{
  defaultDistribution = defdist ? defdist : &gaussianDistribution; 
  makeCommands();
}

//////////////////////////////////////////////////////////////////////
// Now, the command !

static void defineDistribFit(const QString &, QString newName,
                             QString fitName,
                             QString parameter,
                             const CommandOptions & opts)
{


  PerDatasetFit * fit = 
    dynamic_cast<PerDatasetFit *>(Fit::namedFit(fitName));
  QString n;
  updateFromOptions(opts, "distribution", n);
  const Distribution * dist = Distribution::namedDistribution(n);
  
  if(! fit)
    throw RuntimeError("The fit " + fitName + " isn't working "
                       "buffer-by-buffer: not possible to use for distribution fits");
  bool overwrite  = false;
  updateFromOptions(opts, "redefine", overwrite);
  Fit::safelyRedefineFit(newName, overwrite);

  new DistribFit(newName, parameter, fit, dist);
}

static ArgumentList 
dfA(QList<Argument *>() 
    << new StringArgument("name", "Name",
                          "name of the new fit")
    << new FitNameArgument("existing-fit", "Fit",
                           "name of fit to make a derived fit from")
    << new StringArgument("parameter", "Parameter",
                          "the parameter over which to integrate"));

static ArgumentList 
dfO(QList<Argument *>() 
    << new BoolArgument("redefine", 
                        "Redefine",
                        "If the new fit already exists, redefines it")
    << new ChoiceArgument(&Distribution::availableDistributions,
                          "distribution", "Distribution",
                          "The default distribution"));

static Command 
df("define-distribution-fit", // command name
    effector(defineDistribFit), // action
    "fits",  // group name
    &dfA, // arguments
    &dfO, // options
    "Define fit with distribution",
    "Defines a fit with a distribution of values");
