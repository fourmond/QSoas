/**
   \file curve-effectors.hh
   Series of objects providing scoped modifications to a CurvePanel object
   Copyright 2013 by CNRS/AMU

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
#ifndef __CURVE_EFFECTORS_HH
#define __CURVE_EFFECTORS_HH

class CurvePanel;
class CurveItem;

/// Hides everything currently displayed in a panel, until the
/// effector goes out of scope.
class CEHideAll {
  /// The list of objects that are initially visible
  QList< QPointer<CurveItem> > initiallyVisibleItems;

  /// The target panel
  CurvePanel * target;
public:
  explicit CEHideAll(CurvePanel * tg, bool doHide = true);
  ~CEHideAll();
};


/// Disables (or enables) the updates on the target widget. Restores the
/// previous state when the object goes out of scope.
///
/// @todo Merge with TemporarilyDisableWidget
class WDisableUpdates {
  QWidget * target;

  bool initial;
public:
  explicit WDisableUpdates(QWidget * tg, bool disable = true);
  ~WDisableUpdates();
};

#endif
