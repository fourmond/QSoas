/**
   \file dataset-commands.cc commands for tweaking datasets
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
#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <terminal.hh>

#include <dataset.hh>
#include <soas.hh>

namespace DataSetCommands {
  static Group grp("buffer", 2,
                   QT_TR_NOOP("Buffer"),
                   QT_TR_NOOP("Buffer manipulations"));

//////////////////////////////////////////////////////////////////////

  /// Splits the given data at dx sign change
  static void splitDataSet(const DataSet * ds, bool first)
  {
    int idx = ds->deltaSignChange(0);
    if(idx < 0) {
      Terminal::out << QObject::tr("No dx sign change, nothing to do !") 
                    << endl;
      return ;
    }
    DataSet * newDS;
    ds->splitAt(idx, (first ? &newDS : NULL), (first ? NULL : &newDS));
    soas().pushDataSet(newDS);
  }
  
  static void splitaCommand(const QString &)
  {
    const DataSet * ds = soas().currentDataSet();
    splitDataSet(ds, true);
  }


  static Command 
  sa("splita", // command name
     CommandEffector::functionEffectorOptionLess(splitaCommand), // action
     "buffer",  // group name
     NULL, // arguments
     NULL, // options
     QT_TR_NOOP("Split first"),
     QT_TR_NOOP("Splits buffer until dx sign change"),
     QT_TR_NOOP("Returns the first part of the buffer, until "
                "the first change of sign of dx"));
    
  static void splitbCommand(const QString &)
  {
    const DataSet * ds = soas().currentDataSet();
    splitDataSet(ds, false);
  }
        

  static Command 
  sb("splitb", // command name
     CommandEffector::functionEffectorOptionLess(splitbCommand), // action
     "stack",  // group name
     NULL, // arguments
     NULL, // options
     QT_TR_NOOP("Split second"),
     QT_TR_NOOP("Splits buffer after first dx sign change"),
     QT_TR_NOOP("Returns the part of the buffer after "
                "the first change of sign of dx"));

}
