/**
   \file settings-templates.hh
   Template classes for application-wide settings.
   Copyright 2011 by Vincent Fourmond
             2012, 2013 by CNRS/AMU

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
#ifndef __SETTINGS_TEMPLATES_HH
#define __SETTINGS_TEMPLATES_HH

#include <settings.hh>

/// Values
template<typename T> class SettingsValue : public Settings {
protected:

  T value;

  /// Save the settings
  virtual void save(QSettings * target) override {
    target->setValue(name, QVariant::fromValue(value));
  };

  /// Load the settings
  virtual void load(QSettings * source) override {
    if(source->contains(name)) {
      QVariant v = source->value(name);
      /// @todo type checking ?
      value = v.value<T>();
    }
  };

public:
  
  SettingsValue(const QString & n, const T & v,
                const QString & dsc = QString()) : Settings(n, dsc), value(v) {
  };

  operator T () const {
    return value;
  };

  const T & operator=(const T & val) {
    if(!fixed)
      value = val;
    return val;
  };

  T * operator->() {
    return &value;
  };

  T & ref() {
    return value;
  };

  virtual QString typeName() const override {
    return Settings::typeName();
  };

  virtual QString stringValue() const override {
    return Settings::stringValue();
  };

  virtual void setFromString(const QString & s) override {
    Settings::setFromString(s);
  };


};


#endif
