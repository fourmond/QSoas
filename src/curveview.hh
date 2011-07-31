/**
   \file curveview.hh
   The widget handling all "terminal" interaction
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

#ifndef __CURVEVIEW_HH
#define __CURVEVIEW_HH

#include <curvepanel.hh>
class CurveEventLoop;
class DataSet;

/// This widget displays CurveItem (or children thereof). -- hmm, not
/// that anymore.
///
/// It is the main graphical window of Soas (if not the only one !)
class CurveView : public QAbstractScrollArea {

  Q_OBJECT;

  /// The panel displaying stuff.
  ///
  /// @todo turn that into a 
  CurvePanel panel;
  
  /// Number of curves that got a style so far, also counting the ones
  /// that were potentially removed. 
  int nbStyled;

  /// Returns a pen for the next curve to be added.
  QPen penForNextCurve();

  /// Layouts out the CurvePanel objects
  void layOutPanels();


  /// @name Event loop related functions/attributes
  ///
  /// 
  /// Handling of event loops.
  /// @{

  friend class CurveEventLoop;
  CurveEventLoop * eventLoop;

  void enterLoop(CurveEventLoop * loop);
  void leaveLoop();

  /// @}

  /// Returns the closest DataSet to the given point.
  const DataSet * closestDataSet(const QPointF &point, 
                                 double * dist, int * idx) const;

public:

  CurveView();
  virtual ~CurveView();

  /// Adds a DataSet to the display.
  void addDataSet(const DataSet * ds);

  /// Shows the given DataSet (and forget about the other things)
  void showDataSet(const DataSet * ds);

  /// Adds a transient item
  void addItem(CurveItem * item);

  /// Remove everything from the display
  void clear();

  /// Whether or not to use opengl for rendering.
  void setOpenGL(bool opengl);

protected:
  virtual void resizeEvent(QResizeEvent * event);
  virtual void paintEvent(QPaintEvent * event);


  virtual bool event(QEvent * event);

  virtual void mouseMoveEvent(QMouseEvent * event);
  virtual void mousePressEvent(QMouseEvent * event);
  virtual void mouseReleaseEvent(QMouseEvent * event);

  virtual void helpEvent(QHelpEvent * event);

  virtual void keyPressEvent(QKeyEvent * event);
};

#endif
