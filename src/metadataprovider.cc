/*
  metadataprovider.cc: implementation of the MetaDataProvider base class
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

#include <terminal.hh>
#include <exceptions.hh>

MetaDataProvider::~MetaDataProvider()
{
}

MetaDataProvider::MetaDataProvider(const QString & n) :
  name(n), enabled(true)
{
  NamedInstance<MetaDataProvider>::registerInstance(this);
}

ValueHash MetaDataProvider::allMetaDataForFile(const QString & fileName)
{
  ValueHash ret;
  InstanceHash::iterator it;
  for(it = NamedInstance<MetaDataProvider>::begin(); 
      it != NamedInstance<MetaDataProvider>::end(); ++it) {
    MetaDataProvider * prov = it.value();
    if(prov->enabled && prov->handlesFile(fileName)) {
      try {
        ret.merge(prov->metaDataForFile(fileName));
      }
      catch(const RuntimeError & err) {
        Terminal::out << "Error with meta-data provider '"
                      << it.key() << "': " << err.message() << endl;
      }
      catch(const InternalError & err) {
        Terminal::out << "Internal error with meta-data provider '"
                      << it.key() << "': " << err.message() << endl;
      }
    }
  }
  return ret;
}


