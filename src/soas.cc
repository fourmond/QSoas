/*
  soas.cc: implementation of the Soas class
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
#include <soas.hh>
#include <mainwin.hh>

#include <datastack.hh>
#include <curveview.hh>

#include <graphicssettings.hh>

#include <settings-templates.hh>

CurveView & Soas::view()
{
  return *mw->curveView;
}

Soas * Soas::theSoasInstance = NULL;

static SettingsValue<double> temperature("soas/temperature", 298);

Soas::Soas() : 
  mw(NULL)
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

DataSet * Soas::currentDataSet()
{
  return ds->currentDataSet(false);
}

void Soas::pushDataSet(DataSet * d, bool silent)
{
  return ds->pushDataSet(d, silent);
}
