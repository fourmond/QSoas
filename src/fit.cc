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
#include <commandcontext.hh>
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

#include <datastackhelper.hh>
#include <datasetlist.hh>

static Group fits("fits", 0,
                  "Fits",
                  "Fitting of data");

static Group sfit("sfits", 0,
                  "Single-dataset fits",
                  "Fitting of data (one dataset at a time)", &fits);

static Group mfit("mfits", 1,
                  "Multi-dataset fits",
                  "Fitting of data (several datasets together)", &fits);

QHash<QString, Fit*> * Fit::fitsByName = NULL;


Fit::Fit(const QString & n, const QString & sd, 
         const QString & desc,
         int min, int max, bool mkCmds) :
  name(n), shortDesc(sd), longDesc(desc),
  fitCommand(NULL), mfitCommand(NULL), simCommand(NULL),
  minDataSets(min), maxDataSets(max)
{ 
  registerFit(this);
  if(mkCmds)
    makeCommands();
  // QTextStream o(stdout);
  // o << "Making fit " << n << " at " << this << endl;
}

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

void Fit::deleteCommands()
{
  delete fitCommand;
  fitCommand = NULL;
  delete mfitCommand;
  mfitCommand = NULL;
  delete simCommand;
  simCommand = NULL;
}

bool Fit::isCustom() const
{
  if(mfitCommand)
    return mfitCommand->isCustom();
  return false;                 // Should not happen.
}

void Fit::unregisterFit(Fit * fit, bool deleteCommands)
{
  if(! fitsByName)
    return;
  fitsByName->remove(fit->name);

  QStringList cmds;
  cmds << "fit-" << "mfit-" << "sim-";
  for(int i = 0; i < cmds.size(); i++) {
    Command * cmd = CommandContext::globalContext()->
      namedCommand(cmds[i] + fit->name);
    if(cmd) {
      CommandContext::globalContext()->unregisterCommand(cmd);
    }
  }
  if(deleteCommands)
    fit->deleteCommands();
}


void Fit::clearupCustomFits()
{
  if(! fitsByName)
    return;

  // QTextStream o(stdout);
  for(const QString & s : fitsByName->keys()) {
    Fit * fit = (*fitsByName)[s];
    // o << "Fit: " << s << " -> " << fit << endl;
    if(fit->isCustom())
      delete fit;
  }
}

void Fit::safelyRedefineFit(const QString & name, bool overwrite)
{
  Fit * oldFit = namedFit(name);
  if(oldFit) {
    Command * cmd = CommandContext::globalContext()->
      namedCommand("mfit-" + name);
    if(! cmd)
      throw InternalError("Found fit %1 but not command mfit-%2").
        arg(name).arg(name);
    if(! cmd->isCustom())
      throw RuntimeError("Fit '%1' is a standard QSoas fit, you cannot redefine it").
        arg(name);
    if(overwrite) {
      Terminal::out << "Replacing fit '" << name  
                    << "' with a new definition" << endl;
      Fit::unregisterFit(oldFit, true);
      delete oldFit;
    }
    else
      throw RuntimeError("Fit '%1' is already defined - if you want to "
                         "redefine it, use the /redefine=true option").
        arg(name);
  }
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


ArgumentList Fit::fitArguments() const
{
  return ArgumentList();
}

ArgumentList Fit::fitHardOptions() const
{
  return ArgumentList();
}

ArgumentList Fit::fitSoftOptions() const
{
  return ArgumentList();
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

/// Private class for the fit engine arguments
class FitEngineArgument : public FactoryArgument<FitEngineFactoryItem>
{
public:
  FitEngineArgument(const char * cn, const char * pn,
                    const char * d = "", bool def = false) :
    FactoryArgument<FitEngineFactoryItem>(cn, pn, d, def)
  {
    choiceName = "engine";
  }

  virtual QString typeDescription() const override {
    return QString("[Fit engine](#fit-engines), one of: `%1`").
      arg(sortedChoices().join("`, `"));
  };
};


class OverrideArgument : public SeveralStringsArgument {
public:
  OverrideArgument(const char * cn, const char * pn,
                   const char * d = "", bool g = true, 
                   bool def = false) :
    SeveralStringsArgument(QRegExp("\\s*[,;]\\s*"), cn, pn, d, g, def) {
  };

  virtual QString typeName() const override {
    return "overrides";
  };

  virtual QString typeDescription() const override {
    return "several parameter=value assignments, separated by , or ;";
  };

};



/// @todo This function probably needs to be rewritten (with completely
/// different arguments, for that matter).
void Fit::makeCommands(const ArgumentList &args, 
                       CommandEffector * singleFit,
                       CommandEffector * multiFit,
                       const ArgumentList &originalOptions,
                       CommandEffector * sim)
{

  QString pn = "Fit: ";
  pn += shortDesc;
  QString sd = "Single dataset fit: ";
  sd += shortDesc;

  ArgumentList fal = args;
  if(args.size() == 0) 
    fal = fitArguments();

  ArgumentList options;
  ArgumentList baseOptions;
  if(originalOptions.size() == 0) {
    baseOptions << fitHardOptions()
                << fitSoftOptions();
  }
  else 
    baseOptions = originalOptions;

  

  // Bits common to all commands
  baseOptions << new IntegerArgument("debug", 
                                     "Debug level",
                                     "Debug level: 0 means no debug output, increasing values mean increasing details");
  if(false/* threadSafe()*/)
    baseOptions << new IntegerArgument("threads", 
                                       "Threads",
                                       "Number of threads for computing the jacobian");
  
  baseOptions << new FitEngineArgument("engine", 
                                        "Fit engine",
                                        "The startup fit engine");


  baseOptions << new StringArgument("extra-parameters", 
                                     "Extra parameters",
                                     "defines supplementary parameters");

  /// Now things specific to some commands
  options = baseOptions;


  options << new FileArgument("parameters", 
                              "Parameters",
                              "pre-loads parameters");

  options << new FileArgument("script", 
                              "Script",
                              "runs a script file");

  options << new FileArgument("arg1", 
                              "Argument 1",
                              "first argument of the script file")
          << new FileArgument("arg2", 
                              "Argument 2",
                              "second argument of the script file")
          << new FileArgument("arg3", 
                              "Argument 3",
                              "third argument of the script file");

  options << (new SeveralStringsArgument(QRegExp("\\s*,\\s*"),
                                         "set-from-meta", 
                                         "Set from meta-data",
                                         "sets parameter values from meta-data"))->describe("comma-separated list of *parameter*`=`*meta* speficiations", "parameters-meta-data (see [there](#fits))");
  options << new BoolArgument("expert", 
                              "Expert mode",
                              "runs the fit in expert mode");


  options << new StringArgument("window-title", 
                                "Window title",
                                "defines the title of the fit window");


  // We don't declare the fit command when multiple datasets are
  // necessary.
  if(minDataSets == 1) {
    fitCommand =
      new Command("fit-" + name,
                  singleFit ? singleFit : 
                  effector(this, &Fit::runFitCurrentDataSet, true),
                  "sfits", fal, options, pn, sd);
  }
  
  options << new BoolArgument("weight-buffers", 
                               "Weight buffers",
                               "whether or not to weight datasets (off by default)");

  options << new StringArgument("perp-meta", 
                                "Perpendicular coordinate",
                                "if specified, it is the name of a meta-data that holds the perpendicular coordinates");

  
  ArgumentList al = fal;
  al << new SeveralDataSetArgument("datasets", 
                                   "Dataset",
                                   "datasets that will be fitted to",
                                   true);
  pn = "Multi fit: ";
  pn += shortDesc;
  sd = "multi dataset fit: ";
  sd += shortDesc;
  mfitCommand = 
    new Command("mfit-" + name,
                multiFit ? multiFit : 
                effector(this, &Fit::runFit, true),
                "mfits", al, options, pn, sd);

  if(! multiFit || sim) {
    /// @todo handle the case when there is a fit-specified effector.
    pn = "Simulation: ";
    pn += shortDesc;
    sd = "fit simulation: ";
    sd += shortDesc;

    al = fal;
    al << new FileArgument("parameters", 
                           "Parameters",
                           "file to load parameters from")
       << new SeveralDataSetArgument("datasets", 
                                     "Dataset",
                                     "the datasets whose X values will be "
                                     "used for simulations", 
                                     true);

    ArgumentList nopts = baseOptions;
    nopts << new OverrideArgument("override",
                                  "Override parameters",
                                  "a comma-separated list of parameters "
                                  "to override")
          << DataStackHelper::helperOptions()
          << DataSetList::listOptions("datasets on which to base the computations", true, true)
          << new ChoiceArgument(QStringList() 
                                << "annotate"
                                << "compute"
                                << "subfunctions"
                                << "residuals"
                                << "jacobian"
                                << "reexport"
                                << "push",
                                "operation", 
                                "What to do",
                                "Whether to just compute the function, "
                                "the full jacobian, reexport parameters "
                                "with errors or just annotate datasets");
    simCommand = 
      new Command("sim-" + name,
                  (sim ? sim : effector(this, &Fit::computeFit)),
                  "simulations", al, nopts, pn, sd);
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

  if(datasets.size() == 0)
    throw RuntimeError("No datasets to fit");

  {
    QStringList pbs;
    QList<int> idx;
    for(int i = 0; i < datasets.size(); i++) {
      const DataSet * ds = datasets[i];
      if(ds->hasNotFinite()) {
        pbs << "'" + ds->name + "'";
        idx << i;
      }
    }
    
    if(pbs.size() > 0) {
      QString s = "The datasets " + pbs.join(", ") + " contains NaNs or infinite numbers. <br>\n"
        "NaNs either result from incorrect parsing of text lines or from operations giving undefined results (such as 0.0/0.0).<p>\n" +
        "You cannot fit these datasets.<p>Do you want to remove the problematic points and proceed ?" +
        "<p> Alternatively, you may use strip-if !y.finite?";
      if(Utils::askConfirmation(s, "Strip NaNs and infinite numbers")) {
        for(int i = 0; i < idx.size(); i++) {
          DataSet * nds = datasets[idx[i]]->derivedDataSet("_stripped.dat");
          nds->stripNotFinite();
          datasets[idx[i]] = nds;
          soas().pushDataSet(nds);
        }
      }
      else
        throw RuntimeError("Stopping because of the presence of NaNs and/or infinite numbers in the datasets -- you can use strip-if !y.finite? to remove them");
      
    }
      
  }

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
  bool showWeights = true;
  updateFromOptions(opts, "parameters", loadParameters);
  updateFromOptions(opts, "weight-buffers", showWeights);

  QString perpMeta;
  updateFromOptions(opts, "perp-meta", perpMeta);

  QString script;
  updateFromOptions(opts, "script", script);

  QStringList args;
  if(opts.contains("arg1")) {
    QString a;
    updateFromOptions(opts, "arg1", a);
    args << a;
    if(opts.contains("arg2")) {
      updateFromOptions(opts, "arg2", a);
      args << a;
      if(opts.contains("arg3")) {
        updateFromOptions(opts, "arg3", a);
        args << a;
      }
    }
  }

  bool expert = true;
  updateFromOptions(opts, "expert", expert);

  if(expert)
    showWeights = true;         // always shown in expert mode

  QString wt;
  updateFromOptions(opts, "window-title", wt);
  if(! wt.isEmpty())
    wt = " -- " + wt;

  FitDialog dlg(&data, showWeights, perpMeta, expert, wt);

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
    for(QHash<QString, QString>::iterator i = fm.begin(); i != fm.end(); ++i) {

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

  if(! script.isEmpty()) {
    if(expert)
      QMetaObject::invokeMethod(&dlg, "runCommandFile",
                                Qt::QueuedConnection,
                                Q_ARG(const QString &, script),
                                Q_ARG(const QStringList &, args)
                                );
    else
      Terminal::out << "Can only run scripts with /expert=true" << endl;
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
                     QList<const DataSet *> dss,
                     const CommandOptions & opts)
{
  DataStackHelper pusher(opts);
  DataSetList list(opts, dss);
  QList<const DataSet *> datasets = list;

  
  
  if(datasets.size() == 0)
    throw RuntimeError("No datasets for computing");

  // Additional option: overrides

  QStringList overrides;

  updateFromOptions(opts, "override", overrides);

  QString extraParams;
  updateFromOptions(opts, "extra-parameters", extraParams);
  QStringList ep = extraParams.split(",", QString::SkipEmptyParts);

  QString what = "compute";
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

  QString perpMeta;
  updateFromOptions(opts, "perp-meta", perpMeta);
  ws.computePerpendicularCoordinates(perpMeta);

  ws.loadParameters(file, -1, false);



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

  // ensure the recompute throws an exception
  ws.recompute(false, false);
  if(what == "reexport") {
    Terminal::out << "Reexporting parameters with errors" << endl;
    if(! data.engineFactory)
      data.engineFactory = FitEngine::namedFactoryItem("odrpack");
    ws.prepareFit();
    ws.recomputeJacobian();
    ws.exportToOutFile(true);
  }
  else if(what == "push") {
    Terminal::out << "Pushing parameters with errors" << endl;
    if(! data.engineFactory)
      data.engineFactory = FitEngine::namedFactoryItem("odrpack");
    ws.prepareFit();
    ws.recomputeJacobian();
    pusher.pushDataSet(ws.exportAsDataSet(true, true));
  }
  else if(what == "jacobian") {
    Terminal::out << "Computing and exporting the jacobian " << endl;

    /// @todo It shouldn't be necessary to do that !
    if(! data.engineFactory)
      data.engineFactory = FitEngine::namedFactoryItem("qsoas");
    ws.prepareFit();
    ws.computeAndPushJacobian();
  }
  else if(what == "annotate") {
    Terminal::out << "Annotating datasets " << endl;
    ws.pushAnnotatedData(&pusher);
  }
  else if(what == "residuals") {
    ws.computeResiduals(true);
    ws.pushComputedData(true, false, &pusher);
    Terminal::out << "Computed residuals: " << ws.overallPointResiduals
                  << endl;
  }
  else if(what == "compute" || what == "subfunctions") {
    ws.pushComputedData(false, what == "subfunctions", &pusher);
  }
  else
    throw RuntimeError("Could not understand which operation for sim-: '%1'").
      arg(what);
}


Fit::~Fit()
{
  // QTextStream o(stdout);
  // o << "Deleting fit " << name << " at " << this << endl;

  // Unsure the fits are correctly removed
  if(fitsByName && fitsByName->contains(name) && (*fitsByName)[name] == this)
    fitsByName->remove(name);
  
  deleteCommands();
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
