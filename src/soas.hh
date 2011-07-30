/**
   \file soas.hh
   The Soas meta-container class
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

#ifndef __SOAS_HH
#define __SOAS_HH

#include <datastack.hh>

class MainWin;
class CurveView;
class DataSet;

/// The class holding all information/actor related to Soas.
///
/// This class wraps all the information and the effectors that make
/// up a Soas session (but it does not own them). Most of the time,
/// you'll access the application-wide instance using the soas()
/// convenience function.
class Soas {

  MainWin * mw;
  DataStack ds;

  static Soas * theSoasInstance;
  
public:

  Soas(MainWin * mw);

  MainWin & mainWin() { return *mw; };
  DataStack & stack() { return ds; };
  CurveView & view();


  static Soas * soasInstance() {
    return theSoasInstance;
  };

};

/// Returns the application-wide Soas instance
inline Soas & soas() {
  return *Soas::soasInstance();
};

#endif
