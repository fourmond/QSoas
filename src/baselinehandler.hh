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
/// No operation is performed here, the BaselineHandler relies on the
/// outside function to populate modified, derivative, diff and
/// divided. For the last two, however, a convenience function is
/// provided.
///
/// @todo Find a smooth way to handle resampling ?
///
/// @todo Provide an other additional object that lives in the bottom
/// panel, that could be shown/quit replaced on demand like divide
/// (for exponential division for instance in reg).
///
/// @todo Most/all actions should be made into functions
///
/// @todo Get rid of the shoudlComputeDerivative stuff...
class BaselineHandler {
public:

  typedef enum  {
    None = 0,
    
  } Option;
  Q_DECLARE_FLAGS(Options, Option);


  typedef enum {
    HandlerActionsBase = 1000,
    ToggleDerivative = HandlerActionsBase,
    HideDataset = HandlerActionsBase+1,
    QuitReplacing = HandlerActionsBase+2,
    QuitSubtracting = HandlerActionsBase+3,
    QuitDividing =   HandlerActionsBase+4,
    ToggleDivision = HandlerActionsBase+5,
  } HandlerActions;

  /// The options of the BaselineHandler.
  Options options;

  /// The original signal
  const DataSet * signal;

  /// The bottom panel
  CurvePanel bottom;

  /// An object living in the main panel that represents the
  /// "modified" signal.
  CurveData modified;

  /// This object lives in the top panel, is hidden by default, and
  /// is meant to contain the derivative.
  CurveData derivative;

  /// This object lives in the bottom panel and constantly represents
  /// the difference between the signal and the modified one.
  CurveData diff;

  /// This object lives in the bottom panel and represents the effect
  /// of dividing the signal by the modified thing.
  CurveData divided;

  /// A pointer to the item displaying the current dataset. (comes in
  /// useful to hide it, BTW)
  CurveItem * datasetDisplay;

  /// Whether we are showing derivative or not.
  bool showingDerivative;

  /// Whether we are showing the divided signal
  bool showingDivided;

  /// A suffix whose derived names are based on.
  QString suffix;

  /// Adds all the necessary commands to the 
  static EventHandler & addToEventHandler(EventHandler & target, Options opts);

public:

  BaselineHandler(CurveView & view, const DataSet * ds,
                  const QString & suffix,
                  Options opts = None);
  ~BaselineHandler();

  /// Processes the next action returned by the handler.
  ///
  /// Returns true if the action was processed by the handler. In
  /// addition, it stores a series of parameters into the bool
  /// parameters
  bool nextAction(int eh, bool * shouldQuit, 
                  bool * shouldComputeDerivative = NULL,
                  bool * shouldRecompute = NULL);


  /// Bottom curves can be computed automatically provided modified is
  /// up-to-date.
  void updateBottom();
};


Q_DECLARE_OPERATORS_FOR_FLAGS(BaselineHandler::Options);
#endif
