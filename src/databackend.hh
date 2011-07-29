/**
   \file databackend.hh
   Backends for reading data files.
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

#ifndef __DATABACKEND_HH
#define __DATABACKEND_HH

class DataSet;

/// The base class of a series that reads data from files.
///
/// It supports auto-detection...
class DataBackend {
  /// A global hash holding a correspondance name->databackend
  static QList<DataBackend*> * availableBackends;

  /// Registers the given backend to the static registry
  static void registerBackend(DataBackend * backend);

protected:
  /// A short code-like name
  QString name;

public:


};

#endif
