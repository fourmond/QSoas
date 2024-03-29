/**
   \file datastack-commands.cc commands for handling the data stack
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
#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>
#include <terminal.hh>

#include <datastack.hh>
#include <databackend.hh>
#include <curveview.hh>
#include <soas.hh>
#include <utils.hh>

#include <datasetbrowser.hh>
#include <stylegenerator.hh>

#include <statistics.hh>
#include <mruby.hh>
#include <idioms.hh>

#include <commandlineparser.hh>
#include <hook.hh>

#include <datasetlist.hh>
#include <datastackhelper.hh>

#include <curve-effectors.hh>


#include <file.hh>

static Group stack("view", 1,
                   "View",
                   "Control viewing options");

//////////////////////////////////////////////////////////////////////

#include <datasetwriter.hh>


static void saveCommand(const QString &, QString file, 
                        const CommandOptions & opts)
{
  File f(file, File::TextWrite, opts);
  DataSetWriter writer;
  writer.setFromOptions(opts);
  DataSet * ds = soas().currentDataSet();
  writer.writeDataSet(&f, ds);
  ds->name = file;
  // soas().view().repaint();
}

static QString fileNameProvider() 
{
  return soas().currentDataSet()->cleanedName() + ".dat";
}

static ArgumentList 
saveArgs(QList<Argument *>() 
         << new FileSaveArgument("file", 
                                 "File name",
                                 "File name for saving",
                                 &fileNameProvider, false));

static ArgumentList 
saveOpts(QList<Argument *>() 
         << File::fileOptions(File::OverwriteOption|File::MkPathOption)
         << DataSetWriter::writeOptions()
         );


static Command 
sv("save", // command name
   effector(saveCommand), // action
   "save",  // group name
   &saveArgs, // arguments
   &saveOpts, // options
   "Save",
   "",
   "s");

//////////////////////////////////////////////////////////////////////

/// @todo Use File consistently here. Not so easy.
static void saveBuffersCommand(const QString &, 
                               QList<const DataSet *> datasets, 
                               const CommandOptions & opts)
{
  QString fmt;
  updateFromOptions(opts, "format", fmt);
  QString expr;
  updateFromOptions(opts, "expression", expr);
  QString mode;
  updateFromOptions(opts, "mode", mode);
  bool mkpath = false;
  updateFromOptions(opts, "mkpath", mkpath);
  bool overwrite = true;
  updateFromOptions(opts, "overwrite", overwrite);

  bool rename = false;
  bool save = true;
  if(mode == "both")
    rename = true;
  if(mode == "rename") {
    save = false;
    rename = true;
  }

  DataSetWriter writer;
  writer.setFromOptions(opts);
  DataSet * ds = soas().currentDataSet();

  
  for(int i = 0; i < datasets.size(); i++) {
    QString nm = datasets[i]->name;
    if(! fmt.isEmpty()) {
      nm = Utils::safeAsprintf(fmt, i);
    }
    else if(! expr.isEmpty()) {
      MRuby * mr = MRuby::ruby();
      nm = mr->toQString(datasets[i]->evaluateWithMeta(expr));
    }
    if(rename) {
      /// @hack And a nice const-cast...
      DataSet * ds = const_cast<DataSet * >(datasets[i]);
      ds->name = nm;
    }
    if(save) {
      if(! overwrite && QFile::exists(nm)) {
        // This is to prevent
        Terminal::out << "Not overwriting " << nm
                      << " as requested" << endl;
        continue;
      }
      /// @todo This should be handled by File
      if(mkpath)
        QDir::current().mkpath(QFileInfo(nm).dir().path());
      File f(nm, File::TextOverwrite, opts);
      writer.writeDataSet(&f, datasets[i]);
    }
  }
}

static ArgumentList 
sBArgs(QList<Argument *>() 
         << new SeveralDataSetArgument("datasets", 
                                       "Datasets to save",
                                       "datasets to save"));

static ArgumentList 
sBOpts(QList<Argument *>() 
       << new StringArgument("format", 
                             "File name format",
                             "overrides dataset names if present")
       << new StringArgument("expression", 
                             "A full Ruby expression",
                             "a Ruby expression to make file names")
       << new ChoiceArgument(QStringList() << "save" << "both" << "rename", 
                             "mode", 
                             "What to do with datasets",
                             "if using `/format` or `/expression`, whether to just `save`, to just `rename` or `both` (defaults to 'both')")
       << new BoolArgument("mkpath",
                           "Make path",
                           "if true, creates all necessary directories (defaults to false)")
       << new BoolArgument("overwrite",
                           "Overwrite",
                           "if false, will not overwrite existing files (warning: default is true)")
       << DataSetWriter::writeOptions()
       );


static Command 
saveBuffers("save-datasets", // command name
            effector(saveBuffersCommand), // action
            "save",  // group name
            &sBArgs, // arguments
            &sBOpts, // options
            "Save",
            "Saves specified buffers",
            "save-buffers");

//////////////////////////////////////////////////////////////////////

static void showStackCommand(const QString &, const CommandOptions & opts)
{
  int number = 0;
  updateFromOptions(opts, "number", number);
  QStringList meta;
  updateFromOptions(opts, "meta", meta);
  
  soas().stack().showStackContents(number, meta);
}

static ArgumentList 
showSOpts(QList<Argument *>() 
          << new IntegerArgument("number", 
                                 "Limit display",
                                 "Display only that many datasets around 0",
                                 true)
          << new SeveralStringsArgument(QRegExp("\\s*,\\s*"), "meta", 
                                        "Meta-data",
                                        "also lists the comma-separated meta-data")
          );


static Command 
showStack("show-stack", // command name
          effector(showStackCommand), // action
          "view",  // group name
          NULL, // arguments
          &showSOpts, // options
          "Show stack",
          "Shows the stack contents",
          "k");

//////////////////////////////////////////////////////////////////////

/// @bug Must clear the current overlays if they were removed. (else,
/// segfault !)
///
/// @todo a /but option ? (or I should have that built into the
/// dataset selection ?)
static void dropDataSetCommand(const QString &, const CommandOptions & opts)
{
  DataStack & stack = soas().stack();
  DataSetList buffers(opts);
  for(const DataSet * ds : buffers) {
    stack.dropDataSet(ds);
  }
  // Shift the redo stack into the normal stack if the latter is empty
  if(stack.stackSize() == 0 && stack.redoStackSize() > 0) {
    Terminal::out << "Normal stack is empty, shifting up the redo stack"
                  << endl;
    stack.redo();
  }
}

static ArgumentList 
dropOps(QList<Argument *>() 
        << DataSetList::listOptions("Datasets to permanently remove")
        );


static Command 
drop("drop", // command name
     effector(dropDataSetCommand), // action
     "stack",  // group name
     NULL, // arguments
     &dropOps, // options
     "Drop dataset",
     "Drops the current dataset");

//////////////////////////////////////////////////////////////////////

static void popCommand(const QString &, const CommandOptions & opts)
{
  DataStack & stack = soas().stack();
  bool status = false;
  updateFromOptions(opts, "status", status);

  if(status) {
    const DataSet * ds = stack.peekAccumulator();
    if(ds) 
      Terminal::out << "Accumulator has " << ds->nbRows() << " rows and "
                    << ds->nbColumns() << " columns\n"
                    << "Column names: " << ds->mainColumnNames().join(", ")
                    << endl;
    else
      Terminal::out << "No current accumulator" << endl;
    return;
  }

  
  DataSet * ds = stack.popAccumulator();
  bool drop = false;
  updateFromOptions(opts, "drop", drop);
  if(drop)
    delete ds;
  else {
    if(! ds)
      throw RuntimeError("No data in accumulator");
    ds->name = "accumulator";
    soas().pushDataSet(ds);
  }
}

// static ArgumentList 
// popOps(QList<Argument *>() );

static ArgumentList 
popOps(QList<Argument *>() 
       << new BoolArgument("drop", 
                           "Drop",
                           "Drop the accumulator instead of pushing it on the stack")
       << new BoolArgument("status", 
                           "Status",
                           "Gets the status of the accumulator")
       );


static Command 
popCmd("pop", // command name
       effector(popCommand), // action
       "stack",  // group name
       NULL, // arguments
       &popOps, // options
       "Pop accumulator",
       "Pops the contents of the accumulator");


//////////////////////////////////////////////////////////////////////

static ArgumentList 
undoOps(QList<Argument *>() 
        << new IntegerArgument("number", 
                              "Number",
                              "Number of operations to undo", true));

static void undoCommand(const QString &, const CommandOptions & opts)
{
  int number = 1;
  updateFromOptions(opts, "number", number);
  while(number-- > 0)
    soas().stack().undo();
}


static Command 
undo("undo", // command name
     effector(undoCommand), // action
     "stack",  // group name
     NULL, // arguments
     &undoOps, // options
     "Undo",
     "Return to the previous buffer",
     "u");

//////////////////////////////////////////////////////////////////////

static void redoCommand(const QString &, const CommandOptions & opts)
{
  int number = 1;
  updateFromOptions(opts, "number", number);
  while(number-- > 0)
    soas().stack().redo();
}

static ArgumentList 
redoOps(QList<Argument *>() 
        << new IntegerArgument("number", 
                              "Number",
                              "Number of operations to redo", true));


static Command 
redo("redo", // command name
     effector(redoCommand), // action
     "stack",  // group name
     NULL, // arguments
     &redoOps, // options
     "Redo",
     "Retrieves the last undone buffer",
     "r");

//////////////////////////////////////////////////////////////////////

static void clearStackCommand(const QString &)
{
  soas().stack().clear();
}


static Command 
cls("clear-stack", // command name
    optionLessEffector(clearStackCommand), // action
    "stack",  // group name
    NULL, // arguments
    NULL, // options
    "Clear stack",
    "Removes all buffers from the stack",
    "delstack"
    );

//////////////////////////////////////////////////////////////////////

static void saveStackCommand(const QString &, QString fileName, const CommandOptions & opts)
{
  File file(fileName, File::BinaryWrite, opts);
  QDataStream out(file);
  out << soas().stack();
}

static ArgumentList 
saveStackArgs(QList<Argument *>() 
              << new FileSaveArgument("file", 
                                      "File name",
                                      "File name for saving stack",
                                      "qsoas-stack.qst", false));

static ArgumentList 
saveStackOpts(QList<Argument *>() 
              << File::fileOptions(File::OverwriteOption|File::RotationOption));


static Command 
saveStack("save-stack", // command name
          effector(saveStackCommand), // action
          "save",  // group name
          &saveStackArgs, // arguments
          &saveStackOpts, // options
          "Save stack",
          "Saves the stack for later use");

//////////////////////////////////////////////////////////////////////

static void loadStackCommand(const QString &, QString fileName,
                             const CommandOptions & opts)
{
  File file(fileName, File::BinaryRead);

  bool merge = false;
  updateFromOptions(opts, "merge", merge);

  Terminal::out << "Loading stack file: " << fileName << endl;

  QDataStream in(file);
  if(merge) {
    DataStack s(true);
    in >> s;
    soas().stack().insertStack(s);
  }
  else
    in >> soas().stack();
}

static ArgumentList 
loadStackArgs(QList<Argument *>() 
              << new FileArgument("file", 
                                  "File name",
                                  "File name for saving stack"));

static ArgumentList 
loadStackOpts(QList<Argument *>() 
              << new BoolArgument("merge", 
                                  "Merge",
                                  "If true, merges into the current stack rather than overwriting"));


static Command 
loadStack("load-stack", // command name
          effector(loadStackCommand), // action
          "load",  // group name
          &loadStackArgs, // arguments
          &loadStackOpts, // options
          "Load stack",
          "Loads the stack from file");


//////////////////////////////////////////////////////////////////////

// The corresponding command-line option !

//////////////////////////////////////////////////////////////////////
static CommandLineOption sp("--load-stack", [](const QStringList & args) {
    QString f = args[0];
    new Hook([f]() {
        CommandOptions opts;
        loadStackCommand("--", f, opts);
      });
  }, 1, "loads a binary stack file", true, true);


//////////////////////////////////////////////////////////////////////


// This handles both overlaying and hiding
static void ovhCommand(const QString &name,
                      const CommandOptions & opts)
{
  // This is probably the only command that should not use
  // DataStackHelper, as the datasets handled are not created, but
  // just displayed.
  QString style;
  updateFromOptions(opts, "style", style);

  bool hiding = false;
  if(name == "hide-buffer" || name =="H")
    hiding = true;

  DataSetList datasets(opts);
  QScopedPointer<StyleGenerator> 
    gen(StyleGenerator::fromText(style, datasets.size()));


  {
    WDisableUpdates eff(& soas().view());
    for(const DataSet * ds : datasets) {
      if(hiding)
        soas().view().removeDataSet(ds);
      else
        soas().view().addDataSet(ds, gen.data());
    }
  }
}

// static ArgumentList 
// ovArgs(QList<Argument *>() 
//        << new SeveralDataSetArgument("buffers", 
//                                      "Buffers",
//                                      "Buffers to overlay"));

static ArgumentList 
styleOpts(QList<Argument *>() 
          << new StyleGeneratorArgument("style", 
                                        "Style",
                                        "Style for curves display"));


static ArgumentList 
ovbOpts(ArgumentList()
        << styleOpts
        << DataSetList::listOptions("Buffers to overlay")
        );


static Command 
ovlb("overlay-buffer", // command name
     effector(ovhCommand), // action
     "view",  // group name
     NULL, // arguments
     &ovbOpts, // options
     "Overlay buffers",
     "Overlay buffer to the current one",
     "V");

//////////////////////////////////////////////////////////////////////


static ArgumentList 
hdOpts(ArgumentList()
        << DataSetList::listOptions("Buffers to hide")
        );

static Command 
hide("hide-buffer", // command name
     effector(ovhCommand), // action
     "view",  // group name
     NULL,    // arguments
     &hdOpts, // options
     "Hide buffers",
     "hide buffers from the view",
     "H");


//////////////////////////////////////////////////////////////////////


static void clearCommand(const QString &)
{
  soas().view().showCurrentDataSet();
}

static Command 
clear("clear", // command name
      optionLessEffector(clearCommand), // action
      "view",  // group name
      NULL, // arguments
      NULL, // options
      "Clear view",
      "Clear the current view");

//////////////////////////////////////////////////////////////////////

static void toggleMarkersCommand(const QString &)
{
  CurveView & v = soas().view();
  v.setPaintMarkers(! v.paintingMarkers());
  v.invalidateCaches();
  if(v.paintingMarkers())
    Terminal::out << "Now showing data points" << endl;
  else
    Terminal::out << "Not showing data points anymore" << endl;

}


static Command 
poiCmd("points", // command name
       optionLessEffector(toggleMarkersCommand), // action
       "view",  // group name
       NULL, // arguments
       NULL, // options
       "Show points",
       "Shows individual points in the datasets",
       "poi");


//////////////////////////////////////////////////////////////////////

static void browseStackCommand(const QString &, const CommandOptions & opts)
{
  DatasetBrowser dlg(opts);

  DataSetList datasets(opts, true);
  
  if(datasets.size() == 0)
    throw RuntimeError("No datasets to show");

  dlg.displayDataSets(datasets);
  dlg.addButton("Drop from stack", [](const QList<const DataSet*> & lst) {
      DataStack & s = soas().stack();
      for(int i = 0; i < lst.size(); i++)
        s.dropDataSet(lst[i]);
    });
  dlg.exec();
}

static ArgumentList 
bsOpts(QList<Argument *>()
       << DatasetBrowser::browserOptions()
       << DataSetList::listOptions("Datasets to show")
       );

static Command 
browseStack("browse-stack",     // command name
            effector(browseStackCommand, true), // action
            "view",            // group name
            NULL,               // arguments
            &bsOpts,               // options
            "Browse stack",
            "Browse stack",
            "K");

//////////////////////////////////////////////////////////////////////

static void fetchCommand(const QString &, QList<const DataSet *> dss, const CommandOptions & opts)
{
  DataSetList buffers(opts, dss);
  DataStackHelper pusher(opts);
  for(const DataSet * ds : buffers) {
    if(ds)
      pusher << new DataSet(*ds);
  }
}

static ArgumentList 
fetchArgs(QList<Argument *>() 
          << new SeveralDataSetArgument("datasets", 
                                        "Datasets",
                                        "Datasets to fetch", true));

static ArgumentList 
fetchOpts(QList<Argument *>() 
          << DataSetList::listOptions("datasets to fetch", true, true)
          << DataStackHelper::helperOptions()
          );


static Command 
fetch("fetch", // command name
      effector(fetchCommand), // action
      "stack",  // group name
      &fetchArgs, // arguments
      &fetchOpts, // options
      "Fetch datasets from the stack");

                             
//////////////////////////////////////////////////////////////////////

static void flagUnFlag(const CommandOptions & opts, 
                       bool flagged = true)
{
  DataSetList buffers(opts);

  QStringList flags;
  updateFromOptions(opts, "flags", flags);
  if(flagged && flags.isEmpty()) 
    flags << "default";

  QString forWhich;
  updateFromOptions(opts, "for-which", forWhich);
  int matched = 0;

  bool set = false;
  updateFromOptions(opts, "set", set);

  bool exclusive = false;
  updateFromOptions(opts, "exclusive", exclusive);

  if(exclusive && flagged) {
    // remove the given flags from all the bufers
    QList<const DataSet *> ads = soas().stack().allDataSets();
    for(int i = 0; i < ads.size(); i++) {
      /// @hack const-cast
      DataSet * ds = const_cast<DataSet*>(ads[i]);
      ds->clearFlags(flags);
    }
  }

  int nb = 0;
  for(const DataSet * d : buffers) {
    nb++;
    DataSet * ds = const_cast<DataSet *>(d);
    if(flagged) {
      if(set)
        ds->clearFlags();
      ds->setFlags(flags);
    }
    else {
      if(flags.isEmpty())
        ds->clearFlags();
      else
        ds->clearFlags(flags);
    }
  }

  Terminal::out << (flagged ? "Flagged ": "Unflagged ")
                << nb << " datasets" << endl;

}

static void flagDataSetsCommand(const QString &, const CommandOptions & opts)
{
  flagUnFlag(opts, true);
}

static ArgumentList 
muOps(QList<Argument *>()
      << DataSetList::listOptions("Buffers to flag/unflag")
      << new SeveralStringsArgument(QRegExp("\\s*,\\s*"),
                                    "flags", 
                                    "Flags",
                                    "Flags to set/unset"));


static ArgumentList 
flOps(ArgumentList()
      << muOps
      << new BoolArgument("set", 
                          "Set flags",
                          "If on, clears all the previous flags")
      << new BoolArgument("exclusive", 
                          "Set flags exclusively",
                          "If on, clears the given flags on all the datasets but the ones specified"));




static Command 
flag("flag", // command name
     effector(flagDataSetsCommand), // action
     "flags",  // group name
     NULL, // arguments
     &flOps, // options
     "Flag datasets",
     "Flag datasets");


static void unflagDataSetsCommand(const QString &, const CommandOptions & opts)
{
  flagUnFlag(opts, false);
}

static Command 
unflag("unflag", // command name
     effector(unflagDataSetsCommand), // action
     "flags",  // group name
     NULL, // arguments
     &muOps, // options
     "Unflag datasets",
     "Unflag datasets");

//////////////////////////////////////////////////////////////////////

static void autoFlagCommand(const QString &, const CommandOptions & opts)
{
  QStringList flags;
  updateFromOptions(opts, "flags", flags);
  soas().stack().setAutoFlags(QSet<QString>::fromList(flags));
}

static ArgumentList 
afOps(QList<Argument *>() 
      << new SeveralStringsArgument(QRegExp("\\s*,\\s*"),
                                    "flags", 
                                    "Flags",
                                    "Flags", false, true));

static Command 
autoFlag("auto-flag", // command name
         effector(autoFlagCommand), // action
         "flags",  // group name
         NULL, // arguments
         &afOps, // options
         "Auto flag",
         "Auto flag datasets");

//////////////////////////////////////////////////////////////////////

static void showBuffersCommand(const QString &, 
                               QList<const DataSet *> datasets)
{
  for(int i = 0; i < datasets.size(); i++)
    Terminal::out << "Dataset " << datasets[i]->stringDescription(true) << endl;
}

static ArgumentList 
ssBArgs(QList<Argument *>() 
         << new SeveralDataSetArgument("datasets", 
                                       "Datasets to show",
                                       "Datasets to show"));


static Command 
showBuffers("show", // command name
            optionLessEffector(showBuffersCommand), // action
            "buffer",  // group name
            &ssBArgs, // arguments
            NULL, // options
            "Show information");

//////////////////////////////////////////////////////////////////////

#include <datasetexpression.hh>

static void sortDatasetsCommand(const QString &, 
                                QList<const DataSet *> datasets,
                                QString code,
                                const CommandOptions & opts)
{
  bool useStats = false;
  updateFromOptions(opts, "use-stats", useStats);

  bool reversed = false;
  updateFromOptions(opts, "reversed", reversed);

  QList<QPair<mrb_value, const DataSet *> > toSort;
  for(const DataSet * ds : datasets) {
    DataSetExpression ex(ds, useStats, true);
    toSort << QPair<mrb_value, const DataSet *>(ex.evaluate(code), ds);
  }
  MRuby * mrb = MRuby::ruby();
  std::sort(toSort.begin(), toSort.end(),
            [mrb,reversed](QPair<mrb_value, const DataSet *> a,
                  QPair<mrb_value, const DataSet *> b) -> bool {
              if(reversed)
                return mrb->isInferior(b.first, a.first);
              else
                return mrb->isInferior(a.first, b.first);
            });
  datasets.clear();
  for(const QPair<mrb_value, const DataSet *> & p : toSort)
    datasets << p.second;
  soas().stack().reorderDatasets(datasets);
}

static ArgumentList 
sDArgs(QList<Argument *>() 
         << new SeveralDataSetArgument("datasets", 
                                       "Datasets to sort",
                                       "Datasets to sort")
         << new CodeArgument("key", 
                             "Sorting key",
                             "Sorting key (a ruby expression)")
        );

static ArgumentList 
sDOpts(QList<Argument *>() 
       << new BoolArgument("use-stats", 
                           "Use statistics",
                           "Use statistics in the expressions")
       << new BoolArgument("reversed", 
                           "Reversed",
                           "Sorts in the reverse order")
       );


static Command 
sortDatasets("sort-datasets", // command name
             effector(sortDatasetsCommand), // action
             "buffer",  // group name
             &sDArgs, // arguments
             &sDOpts, // options
             "Sort datasets");
