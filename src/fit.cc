/*
  fit.cc: implementation of the fits interface
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014 by CNRS/AMU

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
#include <fit.hh>
#include <dataset.hh>

#include <soas.hh>
#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>

#include <exceptions.hh>

#include <fitdata.hh>
#include <fitdialog.hh>
#include <fitworkspace.hh>

#include <fitengine.hh>
#include <factoryargument.hh>

#include <terminal.hh>
#include <debug.hh>

static Group fits("fits", 0,
                  "Fits",
                  "Fitting of data");

QHash<QString, Fit*> * Fit::fitsByName = NULL;

bool Fit::threadSafe() const {
  return false;
}

void Fit::registerFit(Fit * fit)
{
  if(! fit)
    return;
  if(! fitsByName)
    fitsByName = new QHash<QString, Fit*>;
  /// @todo check double registration ?
  (*fitsByName)[fit->name] = fit;
}

QStringList Fit::availableFits()
{
  if(! fitsByName)
    return QStringList();
  return fitsByName->keys();
}

Fit * Fit::namedFit(const QString & name)
{
  if(! fitsByName)
    return NULL;
  return fitsByName->value(name, NULL);
}

void Fit::functionForDataset(const double * parameters,
                             FitData * data, gsl_vector * target, 
                             int /*dataset*/) const
{
  /// Defaults to all datasets at once
  function(parameters, data, target);
}


ArgumentList * Fit::fitArguments() const
{
  return NULL;
}

ArgumentList * Fit::fitHardOptions() const
{
  return NULL;
}

ArgumentList * Fit::fitSoftOptions() const
{
  return NULL;
}

CommandOptions Fit::currentSoftOptions(FitData * /*data*/) const
{
  return CommandOptions();
}

void Fit::processSoftOptions(const CommandOptions &, FitData * ) const
{
}

void Fit::processOptions(const CommandOptions & /*opts*/, FitData * /*data*/) const
{
  // No specific option to process by default.
}

FitInternalStorage * Fit::allocateStorage(FitData * ) const
{
  return NULL;
}

FitInternalStorage * Fit::copyStorage(FitData *, FitInternalStorage *, int) const
{
  return NULL;
}

FitInternalStorage * Fit::getStorage(FitData * d) const
{
  return d->getStorage();
}




/// @todo This function probably needs to be rewritten (with completely
/// different arguments, for that matter).
void Fit::makeCommands(ArgumentList * args, 
                       CommandEffector * singleFit,
                       CommandEffector * multiFit,
                       ArgumentList * originalOptions,
                       CommandEffector * sim)
{

  QByteArray pn = "Fit: ";
  pn += shortDesc;
  QByteArray sd = "Single buffer fit: ";
  sd += shortDesc;
  QByteArray ld = "Single buffer fit:\n";
  ld += longDesc;

  ArgumentList * fal = NULL;
  if(! args)
    args = fitArguments();
  if(args) 
    fal = new ArgumentList(*args);

  ArgumentList * options;
  if(! originalOptions) {
    originalOptions = new ArgumentList;
    ArgumentList * tmp = fitHardOptions();
    if(tmp)
      (*originalOptions) << *tmp;
    tmp = fitSoftOptions();
    if(tmp)
      (*originalOptions) << *tmp;
    *originalOptions << new IntegerArgument("debug", 
                                  "Debug level",
                                  "Debug level: 0 means no debug output, increasing values mean increasing details");
    if(false/* threadSafe()*/)
      *originalOptions << new IntegerArgument("threads", 
                                              "Threads",
                                              "Number of threads for computing the jacobian");

    *originalOptions << new FactoryArgument<FitEngineFactoryItem>
      ("engine", 
       "Fit engine",
       "The startup fit engine");

    options = new ArgumentList(*originalOptions);
  }
  else 
    options = new ArgumentList(*originalOptions);

  *options << new StringArgument("extra-parameters", 
                                 "Extra parameters",
                                 "defines supplementary parameters");

  *options << new FileArgument("parameters", 
                               "Parameters",
                               "pre-loads parameters");

  *options << new SeveralStringsArgument(QRegExp("\\s*,\\s*"),
                                         "set-from-meta", 
                                         "Set from meta-data",
                                         "sets parameter values from meta-data");



  // We don't declare the fit command when multiple datasets are
  // necessary.
  if(minDataSets == 1) {
    new Command((const char*)(QString("fit-") + name).toLocal8Bit(),
                singleFit ? singleFit : 
                effector(this, &Fit::runFitCurrentDataSet, true),
                "fits", fal, options, pn, sd, ld);
    options = new ArgumentList(*options); // Duplicate, as options
                                          // will be different for single and multi fits
  }
  *options << new BoolArgument("weight-buffers", 
                               "Weight buffers",
                               "whether or not to weight buffers (off by default)");

  *options << new StringArgument("perp-meta", 
                                 "Perpendicular coordinate",
                                 "if specified, it is the name of a meta-data that holds the perpendicular coordinates");

  
  ArgumentList * al = NULL;
  if(args) {
    al = args;
    (*al) << new SeveralDataSetArgument("datasets", 
                                        "Dataset",
                                        "datasets that will be fitted to",
                                        true);
  }
  else
    al = new 
      ArgumentList(QList<Argument *>()
                   << new SeveralDataSetArgument("datasets", 
                                                 "Dataset",
                                                 "datasets that will be fitted to",
                                                 true));

  pn = "Multi fit: ";
  pn += shortDesc;
  sd = "multi buffer fit: ";
  sd += shortDesc;
  ld = "multi buffer fit:\n";
  ld += longDesc;
  new Command((const char*)(QString("mfit-") + name).toLocal8Bit(),
              multiFit ? multiFit : 
              effector(this, &Fit::runFit, true),
              "fits", al, options, pn, sd, ld);

  if(! multiFit || sim) {
    /// @todo handle the case when there is a fit-specified effector.
    pn = "Simulation: ";
    pn += shortDesc;;
    sd = "fit simulation: ";
    sd += shortDesc;
    ld = "fit simulation:\n";
    ld += longDesc;
    ArgumentList * al2 = new ArgumentList(*al);
    al2->insert(al2->size()-1, 
                new FileArgument("parameters", 
                                 "Parameters",
                                 "file to load parameters from"));
    al2->setArgumentDescription("datasets", "the buffers whose X values will be used for simulations");

    ArgumentList * nopts = 
      (originalOptions ? new ArgumentList(*originalOptions) : 
       new ArgumentList());


    *nopts << new StringArgument("override",
                                 "Override parameters",
                                 "a comma-separated list of parameters "
                                 "to override")
           << new StringArgument("extra-parameters", 
                                 "Extra parameters",
                                 "defines supplementary parameters")
           << new SeveralStringsArgument(QRegExp("\\s*,\\s*"),
                                         "flags", 
                                         "Flags",
                                         "Flags to set on the created buffers")
           << new ChoiceArgument(QStringList() 
                                 << "compute"
                                 << "jacobian"
                                 << "reexport",
                                 "operation", 
                                 "What to do",
                                 "Whether to just compute the function, "
                                 "the full jacobian or reexport parameters "
                                 "with errors")

      ;
    new Command((const char*)(QString("sim-") + name).toLocal8Bit(),
                (sim ? sim : effector(this, &Fit::computeFit)),
                "simulations", al2, nopts, pn, sd, ld);
  }
}



void Fit::checkDatasets(const FitData * data) const
{
  int nb = data->datasets.size();
  if(nb < minDataSets)
    throw RuntimeError(QString("Fit %1 expects at least %2 datasets, "
                               "but found only %3").
                       arg(name).arg(minDataSets).arg(nb));
  if(maxDataSets > 0 && nb > maxDataSets)
    throw RuntimeError(QString("Fit %1 expects at most %2 datasets, "
                               "but found %3").
                       arg(name).arg(maxDataSets).arg(nb));
}


void Fit::runFitCurrentDataSet(const QString & n, const CommandOptions & opts)
{
  QList<const DataSet *> ds;
  ds << soas().currentDataSet();
  runFit(n, ds, opts);
}

void Fit::runFit(const QString &n, QList<const DataSet *> datasets,
                 const CommandOptions & opts)
{
  runFit([](FitData *) {}, n, datasets, opts);
}

// static FitEngineFactoryItem * getEngine(const CommandOptions & opts,
//                                         const QString & n = "engine")
// {
//   FitEngineFactoryItem * it = NULL;
//   updateFromOptions(opts, n, it);
//   return it;
// }

void Fit::runFit(std::function<void (FitData *)> hook,
                 const QString &, QList<const DataSet *> datasets,
                 const CommandOptions & opts)
{
  int debug = 0;
  updateFromOptions(opts, "debug", debug);

  if(debug > 0)                 // Looks nice...
    Debug::debug() << "Starting fit with instance of "
                   << typeid(*this).name() << endl;

  QString extraParams;
  updateFromOptions(opts, "extra-parameters", extraParams);
  QStringList ep = extraParams.split(",", QString::SkipEmptyParts);

  FitData data(this, datasets, debug, ep);
  updateFromOptions(opts, "engine", data.engineFactory);
  
  hook(&data);
  processOptions(opts, &data);
  data.finishInitialization();

  int threads = 1;
  updateFromOptions(opts, "threads", threads);
  if(debug > 0)
    Debug::debug() << "Asking for " << threads << " threads (ts: "
                   << threadSafe() << ")" << endl;
  if(threads != 1)
    data.setupThreads(threads);
  checkDatasets(&data);

  QString loadParameters;
  bool showWeights = false;
  updateFromOptions(opts, "parameters", loadParameters);
  updateFromOptions(opts, "weight-buffers", showWeights);

  QString perpMeta;
  updateFromOptions(opts, "perp-meta", perpMeta);


  FitDialog dlg(&data, showWeights, perpMeta);

  if(! loadParameters.isEmpty())
    dlg.loadParametersFile(loadParameters);


  {
    QStringList fromMeta;
    updateFromOptions(opts, "set-from-meta", fromMeta);
    
    // We transform that into a hash param = meta
    QHash<QString, QString> fm;
    QRegExp re("^\\s*(\\S+)\\s*=\\s*(\\S+)\\s*$");
    for(int i = 0; i < fromMeta.size(); i++) {
      if(re.indexIn(fromMeta[i]) == 0) {
        fm[re.cap(1)] = re.cap(2);
      }
      else
        Terminal::out << "Error parsing 'set-from-meta' '" 
                      << fromMeta[i] << "'" << endl;
    }

    int nb = 0;
    for(QHash<QString, QString>::iterator i = fm.begin(); i != fm.end(); i++) {

      const QString & param = i.key();
      const QString & meta = i.value();

      for(int j = 0; j < datasets.size(); j++) {
        const DataSet * ds = datasets[j];
        const ValueHash & vh = ds->getMetaData();
        if(vh.contains(meta)) {
          bool ok;
          double v = vh.doubleValue(meta, &ok);
          if(ok)
            dlg.setParameterValue(param, v, j);
          else
            Terminal::out << "Could not convert meta-data '" << meta 
                          << "' = '" <<  vh[meta].toString() 
                          << "' to double" << endl;
          ++nb;
        }
      }
    }
    if(nb > 0)
      dlg.compute();
  }

  dlg.exec();
}

void Fit::computeFit(const QString &n, QString file,
                     QList<const DataSet *> datasets,
                     const CommandOptions & opts)
{
  computeFit([](FitData *) {}, n, file, datasets, opts);
}


void Fit::computeFit(std::function<void (FitData *)> hook,
                     const QString &, QString file,
                     QList<const DataSet *> datasets,
                     const CommandOptions & opts)
{

  // Additional option: overrides

  QString overridesStr;

  updateFromOptions(opts, "override", overridesStr);

  QString extraParams;
  updateFromOptions(opts, "extra-parameters", extraParams);
  QStringList ep = extraParams.split(",", QString::SkipEmptyParts);

  QString what;
  updateFromOptions(opts, "operation", what);

  int debug = 0;
  updateFromOptions(opts, "debug", debug);

  FitData data(this, datasets, debug, ep);
  data.engineFactory = NULL;
  updateFromOptions(opts, "engine", data.engineFactory);
  hook(&data);
  processOptions(opts, &data);
  data.finishInitialization();

  int threads = 1;
  updateFromOptions(opts, "threads", threads);
  if(threads != 1)
    data.setupThreads(threads);

  checkDatasets(&data);

  FitWorkspace ws(&data);

  QStringList flags;
  updateFromOptions(opts, "flags", flags);

  ws.loadParameters(file, -1, false);



  /// @todo Use mutliple value strings/options when they are
  /// available.
  QStringList overrides = overridesStr.split(QRegExp("\\s*[,;]\\s*"), 
                                             QString::SkipEmptyParts);
  for(int i = 0; i < overrides.size(); i++) {
    QStringList spec = overrides[i].split(QRegExp("\\s*=\\s*"));
    if(spec.size() != 2)
      Terminal::out << "Override not understood: '" << overrides[i] 
                    << "'" << endl;
    else {
      bool ok;
      double value = spec[1].toDouble(&ok);
      if(! ok) {
        Terminal::out << "Not a number: '" << spec[1] 
                      << "'" << endl;
        continue;
      }
      ws.setValue(spec[0], value);
    }
  }
  
  ws.recompute();
  if(what == "reexport") {
    Terminal::out << "Reexporting parameters with errors" << endl;
    if(! data.engineFactory)
      data.engineFactory = FitEngine::namedFactoryItem("odrpack");
    ws.prepareFit();
    ws.recomputeJacobian();
    ws.exportToOutFile(true);
  }
  else if(what == "jacobian") {
    Terminal::out << "Computing and exporting the jacobian " << endl;

    /// @todo It shouldn't be necessary to do that !
    if(! data.engineFactory)
      data.engineFactory = FitEngine::namedFactoryItem("qsoas");
    ws.prepareFit();
    ws.computeAndPushJacobian();
  }
  else    
    ws.pushComputedData(flags);
}


Fit::~Fit()
{
}


QString Fit::annotateDataSet(int /*idx*/, FitData * /*data*/) const
{
  return QString();
}


bool Fit::hasSubFunctions(FitData * /*data*/) const
{
  return false;
}

bool Fit::displaySubFunctions(FitData * /*data*/) const
{
  return true;
}

void Fit::computeSubFunctions(const double * /*parameters*/,
                              FitData * /*data*/, 
                              QList<Vector> * /*targetData*/,
                              QStringList * /*targetAnnotations*/) const
{
  throw InternalError("subfunctions are not implemented");
}
