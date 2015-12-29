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

#include <command.hh>

#include <commandlineparser.hh>
#include <commandeffector-templates.hh>

#include <timedependentparameter.hh>

#include <odesolver.hh>
#include <integrator.hh>
#include <multiintegrator.hh>
#include <fitengine.hh>
#include <stylegenerator.hh>

CurveView & Soas::view()
{
  return *mw->curveView;
}

Soas * Soas::theSoasInstance = NULL;

static SettingsValue<double> temperature("soas/temperature", 298);

Soas::Soas() : 
  mw(NULL), shouldStopFit(false)
{
  startup = QDateTime::currentDateTime();
  theSoasInstance = this;
  ds = new DataStack;
  gs = new GraphicsSettings;
}

double Soas::temperature() const {
  return ::temperature;
}

void Soas::setTemperature(double d) {
  ::temperature = d;
}

CommandWidget & Soas::prompt() 
{
  return *mw->commandWidget;
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

void Soas::writeSpecFile(QTextStream & out)
{
  out << "Commands:" << endl;
  Command::writeSpecFile(out);

  out << "Functions:" << endl;
  out << " - " << GSLFunction::availableFunctions().join("\n - ") << endl;

  out << "Constants:" << endl;
  out << " - " << GSLConstant::availableConstants().join("\n - ") << endl;

  QStringList tdp = TimeDependentParameter::TDPFactory::availableItems();
  qSort(tdp);
  out << "Time-dependent parameters:" << endl;
  out << " - " << tdp.join("\n - ") << endl;

  tdp = ODEStepperOptions::stepperTypes().keys();
  qSort(tdp);
  out << "ODE steppers:" << endl;
  out << " - " << tdp.join("\n - ") << endl;

  tdp = IntegratorFactory::availableItems();
  qSort(tdp);
  out << "Standard integrators:" << endl;
  out << " - " << tdp.join("\n - ") << endl;

  tdp = MultiIntegrator::MultiIntegratorFactory::availableItems();
  qSort(tdp);
  out << "Multi integrators:" << endl;
  out << " - " << tdp.join("\n - ") << endl;

  tdp = FitEngineFactoryItem::availableItems();
  qSort(tdp);
  out << "Fit engines:" << endl;
  out << " - " << tdp.join("\n - ") << endl;

  tdp = StyleGenerator::availableGenerators();
  qSort(tdp);
  out << "Styles:" << endl;
  out << " - " << tdp.join("\n - ") << endl;

  
}

//////////////////////////////////////////////////////////////////////
CommandLineOption sp("--spec", [](const QStringList & /*args*/) {
    {
      QTextStream o(stdout);
      Soas::writeSpecFile(o);
    }
    ::exit(0);
  }, 0, "write command specs");
