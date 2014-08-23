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

class ConditionsFilesProvider : public MetaDataProvider {
protected:

  /// ID for conditions_for
  mutable ID conditionsForID;

  /// The ConditionsFile class
  mutable VALUE cConditionsFile;

  void ensureRubyFine() const {
    if(RTEST(cConditionsFile))
      return;
    conditionsForID = rb_intern("conditions_for");
    cConditionsFile = Ruby::eval("ConditionsFile");
  };

  VALUE metaDataFor(const QString & fileName) const {
    VALUE str = Ruby::fromQString(fileName);
    ensureRubyFine();
    
    return rb_funcall(cConditionsFile, conditionsForID, 1, str);
  };

public:

  /// Whether or not the provider has meta-data for the give file.
  virtual bool handlesFile(const QString & fileName) const {
    VALUE md = metaDataFor(fileName);
    return RTEST(md);
  };

  /// Returns the meta-data for the given file
  virtual ValueHash metaDataForFile(const QString & fileName) const {
    VALUE md = metaDataFor(fileName);
    if(RTEST(md))
      return ValueHash::fromRuby(md);
    return ValueHash();
  };

  ConditionsFilesProvider() : MetaDataProvider("conditions.rb"),
                              conditionsForID(0),
                              cConditionsFile(Qnil)
  {
  };

  
};


static ConditionsFilesProvider provider;
