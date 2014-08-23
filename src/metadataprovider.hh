/**
   \file metadataprovider.hh
   Meta-data providers, ie stuff that decorate datasets with meta-data
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
#ifndef __METADATAPROVIDER_HH
#define __METADATAPROVIDER_HH

#include <namedinstance.hh>
#include <valuehash.hh>

/// Meta-data providers are objects that, given a file name, return
/// the corresponding meta-data. Multiple providers can be used on the
/// same file. Backends are allowed to read also directly meta-data,
/// when it can. As a consequence, subclasses should in principle
/// handle meta-data "outside the file", ie based on other files.
///
/// Ideas for providers:
/// @li meta-data from .ixw and other GPES files
/// @li meta-data from condition files
class MetaDataProvider : 
  public NamedInstance<MetaDataProvider> {
protected:

  friend class NamedInstance<MetaDataProvider>;

  /// The provider name
  QString name;

  /// Whether or not the provider is enabled.
  bool enabled;

  MetaDataProvider(const QString & name);
public:


  virtual ~MetaDataProvider();

  /// Whether or not the provider has meta-data for the give file.
  virtual bool handlesFile(const QString & fileName) const = 0;

  /// Returns the meta-data for the given file
  virtual ValueHash metaDataForFile(const QString & fileName) const = 0;


  /// Loops over all enabled providers and returns all the meta-data.
  static ValueHash allMetaDataForFile(const QString & fileName);
  

};

#endif
