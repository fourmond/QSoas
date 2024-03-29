/*
  soas.cc: implementation of the Soas class
  Copyright 2011 by Vincent Fourmond
            2012, 2013, 2014, 2015 by CNRS/AMU

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
#include <soas.hh>
#include <mainwin.hh>

#include <datastack.hh>
#include <curveview.hh>

#include <graphicssettings.hh>

#include <settings-templates.hh>
#include <gslfunction.hh>


#include <commandlineparser.hh>
#include <commandeffector-templates.hh>


// All this is for writing the specs
#include <command.hh>
#include <commandcontext.hh>
#include <timedependentparameter.hh>
#include <odesolver.hh>
#include <integrator.hh>
#include <multiintegrator.hh>
#include <fitengine.hh>
#include <stylegenerator.hh>
#include <distribfit.hh>
#include <statistics.hh>
#include <commandwidget.hh>


// For version
#include <build.hh>
#include <gsl/gsl_version.h>


Soas * Soas::theSoasInstance = NULL;

bool defaultHeadless = false;

static SettingsValue<double> temperature("soas/temperature", 298);

Soas::Soas() : 
  mw(NULL), headless(defaultHeadless),
  shouldStopFit(false), throwFitExcept(false)
{
  startup = QDateTime::currentDateTime();
  theSoasInstance = this;
  ds = new DataStack;
  gs = new GraphicsSettings;
}


Soas::~Soas()
{
  QTextStream o(stdout);
  o << errorReport() << endl;
  theSoasInstance = NULL;
  delete ds;
  delete gs;
}

void Soas::enterPrompt(CommandWidget * prompt)
{
  prompts.insert(0, prompt);
}

void Soas::leavePrompt()
{
  prompts.takeFirst();
}

CommandContext & Soas::commandContext()
{
  return *(prompts.value(0)->promptContext());
}


double Soas::temperature() const {
  return ::temperature;
}

CurveView & Soas::view()
{
  return *mw->curveView;
}

void Soas::setTemperature(double d) {
  ::temperature = d;
}

QString Soas::currentCommandLine() const
{
  return Command::unsplitWords(mw->commandWidget->currentCommandLine());
}

void Soas::showMessage(const QString & str, int ms)
{
  mw->showMessage(str, ms);
}

DataSet * Soas::currentDataSet(bool silent)
{
  return ds->currentDataSet(silent);
}

void Soas::pushDataSet(DataSet * d, bool silent)
{
  return ds->pushDataSet(d, silent);
}

bool Soas::isHeadless() const
{
  return headless;
}

void Soas::setHeadless(bool hl) 
{
  headless = hl;
}

QString Soas::errorReport() const
{
  return QString("Errors: %1 runtime, %2 internal, %3 headless").
    arg(runtimeErrors).arg(internalErrors).arg(headlessErrors);
}

void Soas::writeSpecFile(QTextStream & out, bool full)
{
  out << "Commands:" << endl;
  CommandContext::writeSpecFile(out, full);

  out << "Functions:" << endl;
  out << " - " << GSLFunction::availableFunctions().join("\n - ") << endl;

  out << "Constants:" << endl;
  out << " - " << GSLConstant::availableConstants().join("\n - ") << endl;

  QStringList tdp = TimeDependentParameter::TDPFactory::availableItems();
  std::sort(tdp.begin(), tdp.end());
  out << "Time-dependent parameters:" << endl;
  out << " - " << tdp.join("\n - ") << endl;

  tdp = ODEStepperOptions::stepperTypes().keys();
  std::sort(tdp.begin(), tdp.end());
  out << "ODE steppers:" << endl;
  out << " - " << tdp.join("\n - ") << endl;

  tdp = IntegratorFactory::availableItems();
  std::sort(tdp.begin(), tdp.end());
  out << "Standard integrators:" << endl;
  out << " - " << tdp.join("\n - ") << endl;

  tdp = MultiIntegrator::MultiIntegratorFactory::availableItems();
  std::sort(tdp.begin(), tdp.end());
  out << "Multi integrators:" << endl;
  out << " - " << tdp.join("\n - ") << endl;

  tdp = FitEngineFactoryItem::availableItems();
  std::sort(tdp.begin(), tdp.end());
  out << "Fit engines:" << endl;
  for(const QString & f : tdp) {
    FitEngineFactoryItem * it = FitEngineFactoryItem::namedItem(f);
    out << " - " << f;
    if(it->isMultiCapable)
      out << " (massively multibuffer)";
    out << endl;
  }

  tdp = StyleGenerator::availableGenerators();
  std::sort(tdp.begin(), tdp.end());
  out << "Styles:" << endl;
  out << " - " << tdp.join("\n - ") << endl;

  tdp = Distribution::availableDistributions();
  std::sort(tdp.begin(), tdp.end());
  out << "Distribution:" << endl;
  out << " - " << tdp.join("\n - ") << endl;

  tdp = StatisticsValue::allSuffixes();
  out << "Statistics:" << endl;
  out << " - " << tdp.join("\n - ") << endl;

  out << "Command-line options:" << endl;
  CommandLineParser::globalParser()->writeSpecFile(out);

  
}

QString Soas::versionString()
{
  return QString("This is QSoas version " SOAS_VERSION
                 " running with mruby %1 and Qt %2\n" SOAS_BUILD_INFO
                 " with Qt " QT_VERSION_STR " and GSL version " GSL_VERSION).
    arg(MRUBY_VERSION).arg(qVersion());
}

ValueHash Soas::versionInfo()
{
  ValueHash rv;
  rv << "version" << SOAS_VERSION
     << "version-string" << versionString()
     << "time-dependent-parameters"
     << TimeDependentParameter::TDPFactory::availableItems()
     << "functions" << GSLFunction::availableFunctions()
     << "constants" << GSLConstant::availableConstants()
     << "ode-steppers" << ODEStepperOptions::stepperTypes().keys()
     << "integrators" << IntegratorFactory::availableItems()
     << "multi-integrators"
     << MultiIntegrator::MultiIntegratorFactory::availableItems()
     << "fit-engines" << FitEngineFactoryItem::availableItems()
     << "color-styles" << StyleGenerator::availableGenerators()
     << "parameter-distributions" << Distribution::availableDistributions()
     << "statistics" << StatisticsValue::allSuffixes();

  return rv;
}

//////////////////////////////////////////////////////////////////////
static CommandLineOption sp("--spec", [](const QStringList & /*args*/) {
    {
      QTextStream o(stdout);
      Soas::writeSpecFile(o, false);
    }
    ::exit(0);
  }, 0, "write command specs");

static CommandLineOption hl("--headless",
                            [](const QStringList & /*args*/) {
                              ::defaultHeadless = true;
                            }, 0, "run in headless mode");

static CommandLineOption lsc("--list-commands", [](const QStringList & /*args*/) {
    {
      QTextStream o(stdout);
      QStringList cmds = CommandContext::listAllCommands();
      std::sort(cmds.begin(), cmds.end());
      o << cmds.join("\n") << endl;
    }
    ::exit(0);
  }, 0, "write available commands");

static CommandLineOption fsp("--full-spec", [](const QStringList & /*args*/) {
    {
      QTextStream o(stdout);
      Soas::writeSpecFile(o, true);
    }
    ::exit(0);
  }, 0, "write command specs, with more details");

static CommandLineOption v("--version", [](const QStringList & /*args*/) {
    {
      QTextStream o(stdout);
      o << Soas::versionString();
    }
    ::exit(0);
  }, 0, "display QSoas version");
