/*
  conditionsprovider.cc: meta-data provider based on "condition files"
  Copyright 2014 by CNRS/AMU

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
#include <metadataprovider.hh>
#include <ruby.hh>

#include <ruby-templates.hh>

class ConditionsFilesProvider : public MetaDataProvider {
protected:

  /// ID for conditions_for
  mutable RUBY_ID conditionsForID;

  /// The ConditionsFile class
  mutable RUBY_VALUE cConditionsFile;

  void ensureRubyFine() const {
    if(rbw_test(cConditionsFile))
      return;
    conditionsForID = rbw_intern("conditions_for");
    cConditionsFile = Ruby::eval("ConditionsFile");
  };

  RUBY_VALUE metaDataFor(const QString & fileName) const {
    RUBY_VALUE str = Ruby::fromQString(fileName);
    ensureRubyFine();

    return Ruby::wrapFuncall(cConditionsFile, conditionsForID, 1, &str);
  };

public:

  /// Whether or not the provider has meta-data for the give file.
  virtual bool handlesFile(const QString & fileName) const {
    RUBY_VALUE md = metaDataFor(fileName);
    return rbw_test(md);
  };

  /// Returns the meta-data for the given file
  virtual ValueHash metaDataForFile(const QString & fileName) const {
    RUBY_VALUE md = metaDataFor(fileName);
    if(rbw_test(md))
      return ValueHash::fromRuby(md);
    return ValueHash();
  };

  ConditionsFilesProvider() : MetaDataProvider("conditions.rb"),
                              conditionsForID(0),
                              cConditionsFile(rbw_nil)
  {
  };

  
};


static ConditionsFilesProvider provider;
