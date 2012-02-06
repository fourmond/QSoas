/*
  ducksim.cc: implementation of DuckSim-based fits
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
#include <perdatasetfit.hh>
#include <dataset.hh>
#include <vector.hh>

#include <commandeffector.hh>
#include <file-arguments.hh>

#include <soas.hh>
#include <ducksim.hh>
#include <terminal.hh>

#include <gsl/gsl_const_mksa.h>

QString DuckSimFit::fullPath;

DuckSimFit * DuckSimFit::theInstance = NULL;



DuckSimFit::DuckSimFit() :
  PerDatasetFit("duck-sim", 
                "Diffusive electrochemical ",
                "...", 1, -1, false) 
{ 
  ArgumentList * opts = new 
    ArgumentList(QList<Argument *>()
                 << new FileArgument("system", 
                                     "System",
                                     "...")
                 << new FileArgument("base-parameters", 
                                     "Base parameters",
                                     "...")
                 );
  makeCommands(NULL, NULL, NULL, opts);
};

void DuckSimFit::removeTemporaryDirectory()
{
  if(tempDir.isEmpty())
    return;

  QDir tmp(tempDir);

  QStringList entries = tmp.entryList(QDir::NoDotAndDotDot);
  for(int i = 0; i < entries.size(); i++)
    tmp.remove(entries[i]);

  QDir::root().rmdir(tempDir);
  tempDir.clear();
}

QString DuckSimFit::runDuckSim(const QStringList & args, bool * success)
{
  QProcess proc;

  QStringList a = systemSpec;
  a << args;
  
  proc.setWorkingDirectory(tempDir);
  proc.start(fullPath, a);
  proc.waitForFinished(-1);
  if(success)
    *success = true;             // optimistic, isn't it ?
  return proc.readAllStandardOutput();
}


void DuckSimFit::processOptions(const CommandOptions & opts)
{
  systemSpec.clear();
  if(! tempDir.isEmpty())
    removeTemporaryDirectory(); // It means we're leaving files around...
  
  tempDir = QDir::tempPath() + "/qs-duck-sim-" + 
    QString::number(QDateTime::currentMSecsSinceEpoch(), 16);

  QDir::root().mkpath(tempDir);

  system = "";
  updateFromOptions(opts, "system", system);

  // Compile
  if(! system.isEmpty()) {
    /// @todo All this is highly Linux-specific
    QFile::copy(system, tempDir + "/system.sys");
    runDuckSim(QStringList() << "-C" << "system.sys");
    systemSpec << "-S" << "./system.so";
  }

  // Now acquire parameters:
  fitParameters.clear();
  initialValues.clear();
  fixedParameters.clear();

  QString baseParams = "";
  updateFromOptions(opts, "base-parameters", baseParams);

  QString param;
  if(baseParams.isEmpty())
    param = runDuckSim(QStringList() << "-L");
  else {
    QFile p(baseParams);
    p.open(QIODevice::ReadOnly);
    param = p.readAll();
  }
  QStringList lines = param.split("\n");
  

  additionalParameters.clear();

  // We first specify the surface.
  fitParameters << "area";
  initialValues << 0.07;        // diameter 3 mm, area in cm^-2
  fixedParameters << true;

  for(int i = 0; i < lines.size(); i++) {
    QStringList line = lines[i].split(QRegExp("\\s+=\\s+"));
    if(line.size() != 2) {
      additionalParameters.append(lines[i] + "\n");
      continue;
    }
    QString pa = line[0];
    double value = line[1].toDouble();
    bool fixed = false;
    if(pa == "temperature") {
      fixed = true;
      value = soas().temperature();
    }
    else if(pa == "f" || 
            pa == "E1" || 
            pa == "omega" || 
            pa == "deltaE" || 
            pa == "cycles" || 
            pa == "E2")
      continue;                 // not parameters

    else if(pa == "dx" ||
            pa == "nb" ||
            pa == "time_order" ||
            pa == "beta" ||
            pa == "v" ||
            pa == "dx" ||
            pa == "rpm" ||
            pa == "alpha" ||
            pa == "precision")
      fixed = true;
    fitParameters << pa;
    initialValues << value;
    fixedParameters << fixed;
  }

}

  
QString DuckSimFit::optionsString() const {
  return QString("System: %1").
    arg(system.isEmpty() ? "default" : system);
}



QList<ParameterDefinition> DuckSimFit::parameters() const {
  QList<ParameterDefinition> defs;
  for(int i = 0; i < fitParameters.size(); i++)
    defs << ParameterDefinition(fitParameters[i], fixedParameters[i]);
  return defs;
}

void DuckSimFit::initialGuess(FitData * params, 
                              const DataSet *ds,
                              double * a)
{
  for(int i = 0; i < initialValues.size(); i++)
    a[i] = initialValues[i];
}

void DuckSimFit::function(const double * a, FitData * data, 
                          const DataSet * ds , gsl_vector * target)
{
  const Vector & xv = ds->x();
  QFile params(tempDir + "/system.params");
  params.open(QIODevice::WriteOnly);
  QTextStream po(&params);
  int delta = ds->deltaSignChange(0);
  if(delta < 0) 
    throw RuntimeError("Must have a forward and backward scan !");

  po << additionalParameters << "\n";
  po << "E1 = " << xv[0] << endl;
  po << "E2 = " << xv[delta] << endl;
  po << "deltaE = " << fabs((xv[0] - xv[delta])/(delta + 1)) << endl;
  for(int i = 0; i < fitParameters.size(); i++)
    po << fitParameters[i] << " = " << a[i] << "\n";
  params.close();

  // Run the program...
  runDuckSim(QStringList() << "-P" << "system.params");

  // Now, read the data !

  QFile output(tempDir + "/data-00.dat");
  output.open(QIODevice::ReadOnly);

  QList<Vector> cols = Vector::readFromStream(&output);
  output.close();
  if(cols[2].size() < target->size)
    throw RuntimeError(QString("Size mismatch detected: %1 read for %2 "
                               "expected").
                       arg(cols[2].size()).arg(target->size));

  gsl_vector_const_view cv = 
    gsl_vector_const_view_array(cols[2].data(), target->size);
  gsl_vector_memcpy(target, &cv.vector);

  // Now we compute the final factor, based on:
  // * D in cm2.s-1
  // * concentrations in millimolar.
  double fact = a[0] * GSL_CONST_MKSA_FARADAY * 1e-6;
  gsl_vector_scale(target, fact);
}

void DuckSimFit::initialize()
{
  fullPath = QDir::home().absoluteFilePath("Prog/DuckSim/duck-sim");
  QFileInfo info(fullPath);
  if(! info.exists())
    return;
  Terminal::out << "Found duck-sim at " << fullPath << endl;
  theInstance = new DuckSimFit();
}
