/**
   \file baselinehandler.hh
   The BaselineHandler, a common interface for commands that edit baselines
   Copyright 2013 by Vincent Fourmond

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
#ifndef __BASELINEHANDLER_HH
#define __BASELINEHANDLER_HH

#include <curveitems.hh>
#include <curvepanel.hh>


class DataSet;
class CurveView;
class EventHandler;

/// This helper class builds upon EventHandler to provide a single
/// consistent interface for all commands that deal with baselines of
/// any kind where you could want to:
///
/// @li display the signal, modified signal (or "baseline"), diff
/// @li leave replacing by the modified signal, the difference,
/// or even the signal divided by the modified one, the derivative
/// of the modified signal (even higher order derivatives)
/// @li hide the original signal
/// @li display in the bottom panel either the diff or the result of
/// the division
///
/// 
class BaselineHandler {
public:

  typedef enum  {
    None = 0,
    
  } Option;
  Q_DECLARE_FLAGS(Options, Option);

  /// The options of the BaselineHandler.
  Options options;

  /// The original signal
  const DataSet * signal;

  /// The bottom panel
  CurvePanel bottom;

  /// An object living in the main panel that represents the
  /// "modified" signal.
  CurveData modified;

  /// This object lives in the bottom panel and constantly represents
  /// the difference between the signal and the modified one.
  CurveData diff;

  /// A pointer to the item displaying the current dataset. (comes in
  /// useful to hide it, BTW)
  CurveItem * datasetDisplay;

  /// Adds all the necessary commands to the 
  static void addToEventHandler(EventHandler & target, Options opts);

public:

  BaselineHandler(CurveView & view, const DataSet * ds,
                  Options opts);
  ~BaselineHandler();

  /// Processes the next action returned by the handler
  bool nextAction(int eh, bool * shouldQuit);

};


Q_DECLARE_OPERATORS_FOR_FLAGS(BaselineHandler::Options);
#endif
