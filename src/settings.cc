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

#include <exceptions.hh>

QList<Settings*> * Settings::settings = NULL;

void Settings::registerSettings(Settings * s)
{
  if(! settings)
    settings = new QList<Settings*>;
  *settings << s;
}

Settings::Settings(const QString & kn, const QString & dsc) :
  name(kn), description(dsc), fixed(false)
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

QString Settings::typeName() const
{
  return "internal";
}

QString Settings::stringValue() const
{
  return "(not modifiable)";
}

void Settings::setFromString(const QString & str)
{
  throw RuntimeError("Cannot set setting '%1': not modifiable type");
}

#include <terminal.hh>

void Settings::dumpSettings()
{
  if(! settings)
    return;
  QList<Settings *> sts(*settings);
  std::sort(sts.begin(), sts.end(),
            [](const Settings * a, const Settings * b) -> bool {
              return a->name < b->name;
            });

  for(const Settings * s : sts) {
    Terminal::out << " * " << s->name << " ("
                  << s->typeName() << ") :\t" << s->stringValue() << endl;
  }
}

void Settings::setSettingsValue(const QString & name, const QString & value)
{
  Settings * tgt = NULL;
  if(settings) {
    for(Settings * s : *settings) {
      if(s->name == name) {
        tgt = s;
        break;
      }
    }
  }
  if(!tgt)
    throw RuntimeError("Could not find settings named '%1'").arg(name);

  tgt->setFromString(value);
  tgt->fixed = true;
}
