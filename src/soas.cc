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

CurveView & Soas::view()
{
  return *mw->curveView;
}

Soas * Soas::theSoasInstance = NULL;

Soas::Soas(MainWin * w) : 
  mw(w), antialias(false), opengl(false)
{
  theSoasInstance = this;
  ds = new DataStack;
}

void Soas::setAntiAlias(bool b)
{
  antialias = b;
}

void Soas::setOpenGL(bool b)
{
  opengl = b;
  view().setOpenGL(opengl);
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
  return ds->currentDataSet();
}
