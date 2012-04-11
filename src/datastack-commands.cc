/**
   \file datastack-commands.cc commands for handling the data stack
   Copyright 2011 by Vincent Fourmond

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

static Group stack("stack", 1,
                   "Data Stack",
                   "Data stack manipulation");

//////////////////////////////////////////////////////////////////////


static void loadFilesAndDisplay(int nb, QStringList files)
{
  soas().view().disableUpdates();
  for(int i = 0; i < files.size(); i++) {
    // Terminal::out << "Loading file '" << files[i] << "'";
    try {
      DataSet * s = DataBackend::loadFile(files[i]);
      soas().stack().pushDataSet(s, true); // use the silent version
      // as we display ourselves
      if(nb > 0)
        soas().view().addDataSet(s);
      else
        soas().view().showDataSet(s);
      Terminal::out << " -- " << s->nbColumns() << " columns" << endl;
      nb++;
    }
    catch (const RuntimeError & e) {
      Terminal::out << "\n" << e.message() << endl;
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  soas().view().enableUpdates();
}

static ArgumentList 
loadArgs(QList<Argument *>() 
         << new SeveralFilesArgument("file", 
                                     "File",
                                     "Files to load !", true
                                     ));
                             


static void loadCommand(const QString &, QStringList files)
{
  /// @todo add the possibility to select the backend
  loadFilesAndDisplay(0, files);
}


/// @todo We should add options to:
/// @li only read within a certain X range
/// @li swap X and Y (/transpose=yes)
static Command 
load("load", // command name
     optionLessEffector(loadCommand), // action
     "stack",  // group name
     &loadArgs, // arguments
     NULL, // options
     "Load",
     "Loads one or several files",
     "Loads the given files and push them onto the data stack",
     "l");
//////////////////////////////////////////////////////////////////////

static void overlayFilesCommand(const QString &, QStringList files)
{
  loadFilesAndDisplay(1, files);
}


static Command 
ovl("overlay", // command name
    optionLessEffector(overlayFilesCommand), // action
    "stack",  // group name
    &loadArgs, // arguments
    NULL, // options
    "Overlay",
    "Loads files and overlay them",
    QT_TR_NOOP("Loads the given files and push them onto the data "
               "stack, adding them to the display at the same time"),
    "v");

//////////////////////////////////////////////////////////////////////


static void saveCommand(const QString &, QString file)
{
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
                                 &fileNameProvider));


static Command 
sv("save", // command name
   optionLessEffector(saveCommand), // action
   "stack",  // group name
   &saveArgs, // arguments
   NULL, // options
   "Save",
   "Saves the current buffer",
   "Saves the current buffer to a file",
   "s");

//////////////////////////////////////////////////////////////////////

static void saveBuffersCommand(const QString &, 
                               QList<const DataSet *> datasets)
{
  for(int i = 0; i < datasets.size(); i++)
    datasets[i]->write(datasets[i]->name);
}

static ArgumentList 
sBArgs(QList<Argument *>() 
         << new SeveralDataSetArgument("buffers", 
                                       "Buffers to save",
                                       "Buffers to save"));


static Command 
saveBuffers("save-buffers", // command name
            optionLessEffector(saveBuffersCommand), // action
            "stack",  // group name
            &sBArgs, // arguments
            NULL, // options
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
                                 "Limit the display to a given number"));


static Command 
showStack("show-stack", // command name
          effector(showStackCommand), // action
          "stack",  // group name
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

static void undoCommand(const QString &)
{
  soas().stack().undo();
}


static Command 
undo("undo", // command name
     optionLessEffector(undoCommand), // action
     "stack",  // group name
     NULL, // arguments
     NULL, // options
     "Undo",
     "Return to the previous buffer",
     QT_TR_NOOP("Returns to the previous buffer, and push the "
                "current to the redo stack"),
     "u");

//////////////////////////////////////////////////////////////////////

static void redoCommand(const QString &)
{
  soas().stack().redo();
}


static Command 
redo("redo", // command name
     optionLessEffector(redoCommand), // action
     "stack",  // group name
     NULL, // arguments
     NULL, // options
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
          "stack",  // group name
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
          "stack",  // group name
          &loadStackArgs, // arguments
          NULL, // options
          "Load stack",
          "Loads the stack from file",
          "Loads the stack as saved using save-stack");

//////////////////////////////////////////////////////////////////////


static void ovCommand(const QString &, QList<const DataSet *> ds)
{
  soas().view().disableUpdates();
  for(int i = 0; i < ds.size(); i++)
    soas().view().addDataSet(ds[i]);
  soas().view().enableUpdates();
}

static ArgumentList 
ovArgs(QList<Argument *>() 
       << new SeveralDataSetArgument("buffers", 
                                     "Buffers",
                                     "Buffers to overlay"));


static Command 
ovlb("overlay-buffer", // command name
     optionLessEffector(ovCommand), // action
     "stack",  // group name
     &ovArgs, // arguments
     NULL, // options
     "Overlay buffers",
     "Overlay buffer to the current one",
     "Overlay buffers that are already in memory "
     "on top of the current one",
     "V");

//////////////////////////////////////////////////////////////////////


static void clearCommand(const QString &)
{
  soas().view().showCurrentDataSet();
}

static Command 
clear("clear", // command name
      optionLessEffector(clearCommand), // action
      "stack",  // group name
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
       "stack",  // group name
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
browseStack("browse-stack", // command name
            optionLessEffector(browseStackCommand), // action
            "stack",  // group name
            NULL, // arguments
            NULL, // options
            "Browse stack",
            "Browse stack",
            "Browse stack",
            "K");

//////////////////////////////////////////////////////////////////////

static void browseFilesCommand(const QString &, const CommandOptions & opts)
{
  DatasetBrowser dlg;
  QString pattern = "*";
  updateFromOptions(opts, "pattern", pattern);
  QStringList files = Utils::glob(pattern);

  QList<DataSet *> dataSets;
  for(int i = 0; i < files.size(); i++) {
    try {
      DataSet * s = DataBackend::loadFile(files[i]);
      if(s->nbColumns() > 1 && s->nbRows() > 1)
          dataSets << s;
      else {
        Terminal::out << files[i] << " doesn't contain enough rows and/or columns, skipping" << endl;
        delete s;
      }

    }
    catch (const RuntimeError & e) {
      Terminal::out << e.message() << endl;
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  if(dataSets.size() > 0) {
    dlg.displayDataSets(dataSets);
    dlg.exec();
  }
  for(int i = 0; i < dataSets.size(); i++)
    delete dataSets[i];
}


static ArgumentList 
bfArgs(QList<Argument *>() 
       << new StringArgument("pattern", 
                             "Pattern",
                             "Files to browse", true
                             ));
                             

static Command 
browseFiles("browse", // command name
            effector(browseFilesCommand), // action
            "stack",  // group name
            NULL, // args
            &bfArgs, // options
            "Browse files",
            "Browse files",
            "Browse files",
            "W");
