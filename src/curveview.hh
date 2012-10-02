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

#include <headers.hh>
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
  CurvePanel panel;

  /// Additional panels, most of the time transient. For now, they'll
  /// just be stacked vertically. (I can't think of any old soas
  /// command that resulted in another way).
  QList<QPointer<CurvePanel> > additionalPanels;

  QList<CurvePanel*> allPanels();
  
  /// Number of curves that got a style so far, also counting the ones
  /// that were potentially removed. 
  int nbStyled;

  /// Returns a pen for the next curve to be added.
  QPen penForNextCurve();

  /// Layouts out the CurvePanel objects onto the given rectangle
  void layOutPanels(const QRect & r);

  /// Layouts out the CurvePanel objects
  void layOutPanels();

  /// Returns the CurvePanel that contains this point, or NULL
  CurvePanel * panelAt(const QPoint & pos);


  /// @name Event loop related functions/attributes
  ///
  /// 
  /// Handling of event loops.
  /// @{

  friend class CurveEventLoop;
  CurveEventLoop * eventLoop;

  void enterLoop(CurveEventLoop * loop);
  void leaveLoop();

  /// The saved focus proxy
  QWidget * savedProxy;

  /// @}

  bool paintMarkers;

  /// Trigger a repaint of the view, when necessary
  void doRepaint();

  /// Whether or not updating has been disabled
  bool repaintDisabled;

  /// Whether or not at least one repaint request has been issued
  /// since repaint is disabled
  bool repaintRequested;

public:

  CurveView();
  virtual ~CurveView();

  /// Adds an item to the CurveView. It goes to the panel()
  void addItem(CurveItem * item);

  /// Adds a panel.
  ///
  /// The CurveView does not take ownership of the panels !
  void addPanel(CurvePanel * panel);

  /// Sets the given panel
  void setPanel(int i, CurvePanel * panel);

  /// Remove everything from the display
  void clear();

  /// Whether or not to use opengl for rendering.
  void setOpenGL(bool opengl);

  /// Returns the main panel
  CurvePanel * mainPanel() {
    return &panel;
  };

  /// Render the view into the given painter.
  ///
  /// See CurvePanel::render() for parameters
  void render(QPainter * painter,
              int innerPanelHeight, const QRect & targetRectangle,
              const QString & title = "");

  /// Prints the list of CurveView
  ///
  /// @todo Add the possibility to add plain text (such as fit
  /// parameters ?)
  ///
  /// @todo Add title...
  static void nupPrint(QPrinter * printer, 
                       const QList<CurveView *> &views,
                       int cols, int rows, int individualHeight = -1);

  /// Forces a repaint of the CurveView
  void repaint() {
    doRepaint();
  };


public slots:

  /// Shows the current data set (see DataStack::currentDataSet)
  void showCurrentDataSet();

  /// Adds a DataSet to the display.
  void addDataSet(const DataSet * ds);

  /// Shows the given DataSet (and forget about the other things)
  void showDataSet(const DataSet * ds);

  /// Whether or not datasets should display markers when applicable.
  void setPaintMarkers(bool enabled);

  /// Temporarily disable updating the view, to speed up rendering !
  void disableUpdates();

  /// Reenable on-change updates that were disabled using
  /// disableUpdates()
  void enableUpdates();


public:

  /// Whether or not we are painting markers...
  bool paintingMarkers() const {
    return paintMarkers;
  };


protected:
  virtual void resizeEvent(QResizeEvent * event);
  virtual void paintEvent(QPaintEvent * event);


  virtual bool event(QEvent * event);

  virtual void mouseMoveEvent(QMouseEvent * event);
  virtual void mousePressEvent(QMouseEvent * event);
  virtual void mouseReleaseEvent(QMouseEvent * event);

  virtual void helpEvent(QHelpEvent * event);

  virtual void keyPressEvent(QKeyEvent * event);
  virtual void wheelEvent(QWheelEvent * event);
};

#endif
