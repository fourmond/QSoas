/*
  settings.cc: implementation of application-wide settings
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
#include <settings.hh>

QList<Settings*> * Settings::settings = NULL;

void Settings::registerSettings(Settings * s)
{
  if(! settings)
    settings = new QList<Settings*>;
  *settings << s;
}

Settings::Settings(const QString & kn) :
  name(kn)
{
  registerSettings(this);
}

void Settings::loadSettings(const QString & org, const QString & app)
{
  QSettings se(org, app);
  if(! settings)
    return;
  for(int i = 0; i < settings->size(); i++)
    (*settings)[i]->load(&se);
}

void Settings::saveSettings(const QString & org, const QString & app)
{
  QSettings se(org, app);
  if(! settings)
    return;
  for(int i = 0; i < settings->size(); i++)
    (*settings)[i]->save(&se);
}
