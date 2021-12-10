/**
   \file settings-templates.cc
   Special template instanciations for Settings
   Copyright 2021 by CNRS/AMU

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
#include <settings-templates.hh>
#include <utils.hh>

template<> QString SettingsValue<int>::stringValue() const
{
  return QString::number(value);
}


template<> QString SettingsValue<QString>::stringValue() const
{
  return value;
}

template<> QString SettingsValue<bool>::stringValue() const
{
  if(value)
    return "true";
  return "false";
}

template<> QString SettingsValue<double>::stringValue() const
{
  return QString::number(value);
}

template<> QString SettingsValue<QSize>::stringValue() const
{
  return QString("%1x%2").
    arg(value.width()).arg(value.height());
}


//////////////////////////////////////////////////////////////////////

template<> QString SettingsValue<QString>::typeName() const
{
  return "text";
}

template<> QString SettingsValue<int>::typeName() const
{
  return "integer";
}

template<> QString SettingsValue<double>::typeName() const
{
  return "number";
}

template<> QString SettingsValue<bool>::typeName() const
{
  return "boolean";
}

template<> QString SettingsValue<QSize>::typeName() const
{
  return "size";
}

//////////////////////////////////////////////////////////////////////

template<> void SettingsValue<QString>::setFromString(const QString & s)
{
  value = s;
}

template<> void SettingsValue<int>::setFromString(const QString & s)
{
  value = Utils::stringToInt(s);
}

template<> void SettingsValue<double>::setFromString(const QString & s)
{
  value = Utils::stringToDouble(s);
}
