/**
   \file tuneabledatadisplay.hh
   A widget handling the display and tuning of one or several XYIterable objects
   Copyright 2017 by CNRS/AMU

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
#ifndef __TUNEABLEDATADISPLAY_HH
#define __TUNEABLEDATADISPLAY_HH

class CurvePoints;
class XYIterable;
class CurveView;
class DataSet;

/// This class handles a series of CurvePoints and a way to tune their
/// display characteristic through a widget.
/// Characteristics handled:
/// @li whether they are visible or not
/// @li and their color
class TuneableDataDisplay : public QWidget {
  Q_OBJECT;

private:

  /// The name to be used in the checkbox
  QString name;
  
  /// The checkbox
  QCheckBox * checkBox;

  /// The target CurveView
  CurveView * view;

  /// The list of items in control of the 
  QList<CurvePoints * > items;

  /// The current color
  QColor color;

  /// The color change button
  QAbstractButton * colorPickerButton;

  /// Returns true if at least one of the items' bounding box counts.
  bool bbCounts() const;

public:
  TuneableDataDisplay(const QString & name,
                      CurveView * view, bool hidden = true,
                      const QColor & c = QColor(),
                      QWidget * parent = NULL);

  /// Adds a source, returns the corresponding CurvePoints object to
  /// be tuned.
  ///
  /// Unless @autoadd is false, the created CurvePoints object is
  /// added to the main panel of the CurveView.
  CurvePoints * addSource(XYIterable * source, bool autoadd = true);

  /// Sets the source, i.e. clears all the sources and add this one
  CurvePoints * setSource(XYIterable * source, bool autoadd = true);

  /// Returns the item of the given @a index, or NULL if none exist.
  CurvePoints * item(int index) const;

  ~TuneableDataDisplay();

  /// Returns the underlying datasets
  QList<DataSet*> makeDataSets(bool onlyVisible = true);
public slots:
  /// Switches on/off the display (if display != 0)
  void toggleDisplay(int display);


  /// Prompt for a new color;
  void promptChangeColor();
  
  /// Change the color
  void changeColor(const QColor & color);

  /// update the color of the curves.
  void updateCurveColors();


};


#endif
