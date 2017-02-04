/*
  tuneabledatadisplay.cc: Implementation of TuneableDataDisplay
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
#include <tuneabledatadisplay.hh>
#include <curvepoints.hh>
#include <xyiterable.hh>
#include <curveview.hh>
#include <curvemarker.hh>

#include <soas.hh>
#include <graphicssettings.hh>


static QPixmap solidColor(int size, const QColor & c)
{
  QPixmap px(size, size);
  px.fill(c);
  return px;
}


TuneableDataDisplay::TuneableDataDisplay(const QString & n,
                                         CurveView * v,
                                         bool h,
                                         const QColor & c,
                                         QWidget * parent):
  QWidget(parent), name(n), view(v), color(c), hidden(h)
{
  QHBoxLayout * layout = new QHBoxLayout(this);
  checkBox = new QCheckBox(name);
  layout->addWidget(checkBox, 1);

  layout->setContentsMargins(0, 1, 0, 1);

  connect(checkBox, SIGNAL(stateChanged(int)), SLOT(toggleDisplay(int)));

  QToolButton * bt = new QToolButton();
  bt->setAutoRaise(true);
  colorPickerButton = bt;
  // colorPickerButton->setFlat(true);
  layout->addWidget(colorPickerButton);
  connect(colorPickerButton, SIGNAL(clicked()), SLOT(promptChangeColor()));
  
  changeColor(color);
}

TuneableDataDisplay::~TuneableDataDisplay()
{
}

void TuneableDataDisplay::updateCurveColors()
{
  // if(marker) {
  //   marker->countBB = true;
  //   marker->size = 6;              // ??
  //   marker->brush = QBrush(color);
  // }
  // if(trajectoryMarker) {
  //   trajectoryMarker->countBB = true;
  //   trajectoryMarker->size = 6;              // ??
  //   trajectoryMarker->pen = color;
  //   trajectoryMarker->pen.setWidthF(2);
  // }
}

void TuneableDataDisplay::toggleDisplay(int display)
{
  if(display > 0) {
  }
  else {
  }
  
  /// Calls again this function by the generated signal, but no-op if
  /// already set.
  Qt::CheckState state = display ? Qt::Checked : Qt::Unchecked;
  if(checkBox->checkState() != state)
    checkBox->setCheckState(state);
}

void TuneableDataDisplay::changeColor(const QColor & col)
{
  color = col;
  colorPickerButton->setIcon(solidColor(12, color));
  updateCurveColors();
  view->update();
}

void TuneableDataDisplay::promptChangeColor()
{
  QColor c = QColorDialog::getColor(color);
  if(c.isValid())
    changeColor(c);
}
