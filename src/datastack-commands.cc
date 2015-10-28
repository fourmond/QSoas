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
#include <ruby.hh>
#include <idioms.hh>

static Group stack("view", 1,
                   "View",
                   "Control viewing options");

//////////////////////////////////////////////////////////////////////


static void saveCommand(const QString &, QString file, 
                        const CommandOptions & opts)
{
  bool overwrite = false;
  bool mkpath = false;
  updateFromOptions(opts, "overwrite", overwrite);
  updateFromOptions(opts, "mkpath", mkpath);
  file = Utils::expandTilde(file);
  if(! overwrite)
    Utils::confirmOverwrite(file);
  if(mkpath)
    QDir::current().mkpath(QFileInfo(file).dir().path());
  soas().currentDataSet()->write(file);
  soas().currentDataSet()->name = file;
  soas().view().repaint();
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
         << new BoolArgument("overwrite", 
                             "Overwrite",
                             "If true, overwrite without prompting")
         << new BoolArgument("mkpath",
                             "Make path",
                             "If true, creates all necessary directories")
         );


static Command 
sv("save", // command name
   effector(saveCommand), // action
   "save",  // group name
   &saveArgs, // arguments
   &saveOpts, // options
   "Save",
   "Saves the current buffer",
   "Saves the current buffer to a file",
   "s");

//////////////////////////////////////////////////////////////////////

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

  bool rename = false;
  bool save = true;
  if(mode == "both")
    rename = true;
  if(mode == "rename") {
    save = false;
    rename = true;
  }
    
  
  for(int i = 0; i < datasets.size(); i++) {
    QString nm = datasets[i]->name;
    if(! fmt.isEmpty()) {
      char buffer[fmt.size()*2 + 100];
      snprintf(buffer, sizeof(buffer), fmt.toUtf8(), i);
      nm = QString::fromUtf8(buffer);
    }
    else if(! expr.isEmpty()) {
      nm = Ruby::toQString(datasets[i]->evaluateWithMeta(expr));
    }
    if(rename) {
      /// @hack And a nice const-cast...
      DataSet * ds = const_cast<DataSet * >(datasets[i]);
      ds->name = nm;
    }
    if(save)
      datasets[i]->write(nm);
  }
}

static ArgumentList 
sBArgs(QList<Argument *>() 
         << new SeveralDataSetArgument("buffers", 
                                       "Buffers to save",
                                       "Buffers to save"));

static ArgumentList 
sBOpts(QList<Argument *>() 
       << new StringArgument("format", 
                             "File name format",
                             "Overrides buffer names if present")
       << new StringArgument("expression", 
                             "A full Ruby expression",
                             "A full Ruby expression ")
       << new ChoiceArgument(QStringList() << "save" << "both" << "rename", 
                             "mode", 
                             "How to rename buffers",
                             "If using format or expression, whether or not to rename buffers ")
       );


static Command 
saveBuffers("save-buffers", // command name
            effector(saveBuffersCommand), // action
            "save",  // group name
            &sBArgs, // arguments
            &sBOpts, // options
            "Save",
            "Saves specified buffers",
            "Saves the designated buffers to file");

//////////////////////////////////////////////////////////////////////

static void showStackCommand(const QString &, const CommandOptions & opts)
{
  int number = 0;
  updateFromOptions(opts, "number", number);
  soas().stack().showStackContents(number);
}

static ArgumentList 
showSOpts(QList<Argument *>() 
          << new IntegerArgument("number", 
                                 "Limit display",
                                 "Limit the display to a given number",
                                 true));


static Command 
showStack("show-stack", // command name
          effector(showStackCommand), // action
          "view",  // group name
          NULL, // arguments
          &showSOpts, // options
          "Show stack",
          "Shows the stack contents",
          "Shows a small summary of what the stack is made of",
          "k");

//////////////////////////////////////////////////////////////////////

/// @bug Must clear the current overlays if they were removed. (else,
/// segfault !)
///
/// @todo a /but option ? (or I should have that built into the
/// dataset selection ?)
static void dropDataSetCommand(const QString &, const CommandOptions & opts)
{
  if(opts.contains("buffers")) {
    QList<const DataSet *> buffers = 
      opts["buffers"]->value<QList<const DataSet *> >();
    for(int i = 0; i < buffers.size(); i++)
      soas().stack().dropDataSet(buffers[i]);
  }
  else {
    soas().stack().dropDataSet(0);
  }
}

static ArgumentList 
dropOps(QList<Argument *>() 
        << new SeveralDataSetArgument("buffers", 
                                      "Buffers",
                                      "Buffers to drop", true, true));


static Command 
drop("drop", // command name
     effector(dropDataSetCommand), // action
     "stack",  // group name
     NULL, // arguments
     &dropOps, // options
     "Drop dataset",
     "Drops the current dataset",
     "Drops the current dataset (or the ones specified in the "
     "buffers options) and frees the associated memory");

//////////////////////////////////////////////////////////////////////

static ArgumentList 
unredoOps(QList<Argument *>() 
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
     &unredoOps, // options
     "Undo",
     "Return to the previous buffer",
     "Returns to the previous buffer, and push the "
     "current to the redo stack",
     "u");

//////////////////////////////////////////////////////////////////////

static void redoCommand(const QString &, const CommandOptions & opts)
{
  int number = 1;
  updateFromOptions(opts, "number", number);
  while(number-- > 0)
    soas().stack().redo();
}


static Command 
redo("redo", // command name
     effector(redoCommand), // action
     "stack",  // group name
     NULL, // arguments
     &unredoOps, // options
     "Redo",
     "Retrieves the last undone buffer",
     QT_TR_NOOP("Pops the last buffer from the redo stack and set it "
                "as the current buffer"),
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
    "Removes all the buffers from both normal and redo stack",
    "delstack"
    );

//////////////////////////////////////////////////////////////////////

static void saveStackCommand(const QString &, QString fileName)
{
  QFile file(fileName);
  Utils::open(&file, QIODevice::WriteOnly);
  QDataStream out(&file);
  out << soas().stack();
}

static ArgumentList 
saveStackArgs(QList<Argument *>() 
              << new FileSaveArgument("file", 
                                      "File name",
                                      "File name for saving stack",
                                      "stack.bin"));


static Command 
saveStack("save-stack", // command name
          optionLessEffector(saveStackCommand), // action
          "save",  // group name
          &saveStackArgs, // arguments
          NULL, // options
          "Save stack",
          "Saves the stack for later use",
          "Saves the contents of the stack for later use, in a private "
          "binary format");

//////////////////////////////////////////////////////////////////////

static void loadStackCommand(const QString &, QString fileName)
{
  QFile file(fileName);
  Utils::open(&file, QIODevice::ReadOnly);

  QDataStream in(&file);
  in >> soas().stack();
}

static ArgumentList 
loadStackArgs(QList<Argument *>() 
              << new FileArgument("file", 
                                  "File name",
                                  "File name for saving stack"));


static Command 
loadStack("load-stack", // command name
          optionLessEffector(loadStackCommand), // action
          "load",  // group name
          &loadStackArgs, // arguments
          NULL, // options
          "Load stack",
          "Loads the stack from file",
          "Loads the stack as saved using save-stack");

//////////////////////////////////////////////////////////////////////


static void ovCommand(const QString &, QList<const DataSet *> ds,
                      const CommandOptions & opts)
{
  QString style;
  updateFromOptions(opts, "style", style);
  QScopedPointer<StyleGenerator> 
    gen(StyleGenerator::fromText(style, ds.size()));
  soas().view().disableUpdates();
  for(int i = 0; i < ds.size(); i++)
    soas().view().addDataSet(ds[i], gen.data());
  soas().view().enableUpdates();
}

static ArgumentList 
ovArgs(QList<Argument *>() 
       << new SeveralDataSetArgument("buffers", 
                                     "Buffers",
                                     "Buffers to overlay"));

static ArgumentList 
styleOpts(QList<Argument *>() 
          << new StyleGeneratorArgument("style", 
                                        "Style",
                                        "Style for curves display"));
static Command 
ovlb("overlay-buffer", // command name
     effector(ovCommand), // action
     "view",  // group name
     &ovArgs, // arguments
     &styleOpts, // options
     "Overlay buffers",
     "Overlay buffer to the current one",
     "Overlay buffers that are already in memory "
     "on top of the current one",
     "V");

//////////////////////////////////////////////////////////////////////


static void hideCommand(const QString &, QList<const DataSet *> ds,
                        const CommandOptions & )
{
  soas().view().disableUpdates();
  for(int i = 0; i < ds.size(); i++)
    soas().view().removeDataSet(ds[i]);
  soas().view().enableUpdates();
}

static Command 
hide("hide-buffer", // command name
     effector(hideCommand), // action
     "view",  // group name
     &ovArgs, // arguments
     NULL, // options
     "Hide buffers",
     "Hide buffers from the view",
     "",
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
      "Clear the current view",
      "Removes all datasets but the current one from the display"
     );

//////////////////////////////////////////////////////////////////////

static void toggleMarkersCommand(const QString &)
{
  CurveView & v = soas().view();
  v.setPaintMarkers(! v.paintingMarkers());
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
       "Shows all the points of datasets displayed.",
       "poi");


//////////////////////////////////////////////////////////////////////

static void browseStackCommand(const QString &)
{
  DatasetBrowser dlg;
  dlg.displayDataSets(soas().stack().allDataSets());
  dlg.exec();
}


static Command 
browseStack("browse-stack",     // command name
            optionLessEffector(browseStackCommand, true), // action
            "view",            // group name
            NULL,               // arguments
            NULL,               // options
            "Browse stack",
            "Browse stack",
            "Browse stack",
            "K");

//////////////////////////////////////////////////////////////////////

static void fetchCommand(const QString &, QList<const DataSet *> buffers)
{
  for(int i = 0; i < buffers.size(); i++)
    soas().pushDataSet(new DataSet(*buffers[i]));
}

static ArgumentList 
fetchArgs(QList<Argument *>() 
          << new SeveralDataSetArgument("buffers", 
                                        "Buffers",
                                        "Buffers to fetch", true));


static Command 
fetch("fetch", // command name
      optionLessEffector(fetchCommand), // action
      "stack",  // group name
      &fetchArgs, // arguments
      NULL, // options
      "Fetch an old buffer",
      "Fetch old buffers from the stack and put them back on "
      "the top of the stack.",
      "...");

//////////////////////////////////////////////////////////////////////

static void pushOntoStack(const QList<const DataSet*> & lst)
{
  soas().view().disableUpdates();
  for(int i = 0; i < lst.size(); i++) {
    DataSet * ds = new DataSet(*lst[i]);
    soas().stack().pushDataSet(ds);
    if(i > 0)
        soas().view().addDataSet(ds);
    else
      soas().view().showDataSet(ds);
  }
  soas().view().enableUpdates();
}

static void browseFilesCommand(const QString &, const CommandOptions & opts)
{
  DatasetBrowser dlg;
  QString pattern = "*";
  updateFromOptions(opts, "pattern", pattern);
  QStringList files = Utils::glob(pattern);


  
  QString frm;
  updateFromOptions(opts, "for-which", frm);

  QList<DataSet *> dataSets;
  for(int i = 0; i < files.size(); i++) {
    try {
      QList<DataSet *> sets = DataBackend::loadFile(files[i]);
      for(int j = 0; j < sets.size(); j++) {
        DataSet * s = sets[j];
        s->stripNaNColumns();
        if(s->nbColumns() > 1 && s->nbRows() > 1) {
          if(! frm.isEmpty()) {
            try {
              if(! s->matches(frm)) {
                Terminal::out << files[i] << " [" << j 
                              << "] does not match the selection rule" 
                              << endl;
                continue;
              }
            }
            catch(const RuntimeError & re) {
              Terminal::out << "Error evaluating expression with " << files[i] << " [" << j 
                            << "]:" << re.message()
                            << endl;
              continue;
            }
          }
          dataSets << s;
        }
        else {
          Terminal::out << files[i] << " doesn't contain enough rows and/or columns, skipping" << endl;
          delete s;
        }
      }
    }
    catch (const RuntimeError & e) {
      Terminal::out << e.message() << endl;
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  if(dataSets.size() > 0) {
    dlg.addButton("Push onto stack", &::pushOntoStack);
    dlg.displayDataSets(dataSets);
    dlg.exec();
  }
  for(int i = 0; i < dataSets.size(); i++)
    delete dataSets[i];
}


static ArgumentList 
bfOpts(QList<Argument *>() 
       << new StringArgument("pattern", 
                             "Pattern",
                             "Files to browse", true)
       << new StringArgument("for-which", 
                             "For which",
                             "Select on formula")
);
                             

static Command 
browseFiles("browse", // command name
            effector(browseFilesCommand), // action
            "view",  // group name
            NULL, // args
            &bfOpts, // options
            "Browse files",
            "Browse files",
            "Browse files",
            "W");

//////////////////////////////////////////////////////////////////////

static void flagUnFlag(const CommandOptions & opts, 
                       bool flagged = true)
{
  QList<const DataSet *> buffers;
  if(opts.contains("buffers"))
    buffers = opts["buffers"]->value<QList<const DataSet *> >();
  else
    buffers << soas().currentDataSet();

  QStringList flags;
  updateFromOptions(opts, "flags", flags);
  if(flagged && flags.isEmpty()) 
    flags << "default";

  QString forWhich;
  updateFromOptions(opts, "for-which", forWhich);
  int matched = 0;

  bool set = false;
  updateFromOptions(opts, "set", set);

  for(int i = 0; i < buffers.size(); i++) {
    DataSet * ds = const_cast<DataSet *>(buffers[i]);
    if(! forWhich.isEmpty()) {
      try {
        if(! ds->matches(forWhich))
          continue;               // Not flagging
        matched += 1;
      }
      catch(const RuntimeError & re) {
        Terminal::out << "Error evaluating expression with dataset #" << i
                      << ": " << re.message() << endl;
        continue;
      }
    }
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

  Terminal::out << (flagged ? "Flagged ": "Unflagged ");
  if(! forWhich.isEmpty())
    Terminal::out << matched << " buffers (out of "
                  << buffers.size() << ")" << endl;
  else
    Terminal::out << buffers.size() << " buffers" << endl;

}

static void flagDataSetsCommand(const QString &, const CommandOptions & opts)
{
  flagUnFlag(opts, true);
}

static ArgumentList 
muOps(QList<Argument *>() 
      << new SeveralDataSetArgument("buffers", 
                                    "Buffers",
                                    "Buffers to flag/unflag", true, true)
      << new StringArgument("for-which", 
                            "For which",
                            "Select on formula")
      << new SeveralStringsArgument(QRegExp("\\s*,\\s*"),
                                    "flags", 
                                    "Buffers",
                                    "Buffers to flag/unflag"));


static ArgumentList 
flOps(QList<Argument *>(muOps)
      << new BoolArgument("set", 
                          "Set flags",
                          "If on, clears all the previous flags"));
      



static Command 
flag("flag", // command name
     effector(flagDataSetsCommand), // action
     "flags",  // group name
     NULL, // arguments
     &flOps, // options
     "Flag datasets",
     "Flag datasets", "M");


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
     "Unflag datasets", "U");

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
         << new SeveralDataSetArgument("buffers", 
                                       "Buffers to show",
                                       "Buffers to show"));


static Command 
showBuffers("show", // command name
            optionLessEffector(showBuffersCommand), // action
            "buffer",  // group name
            &ssBArgs, // arguments
            NULL, // options
            "Show information",
            "Show details (meta-data and such) about the given buffers", "");
