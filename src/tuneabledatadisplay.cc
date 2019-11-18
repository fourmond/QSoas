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
                                         QWidget * parent) :
  QWidget(parent), name(n), view(v), color(c)
{
  QHBoxLayout * layout = new QHBoxLayout(this);

  QToolButton * bt = new QToolButton();
  bt->setAutoRaise(true);
  colorPickerButton = bt;
  // colorPickerButton->setFlat(true);
  layout->addWidget(colorPickerButton);
  connect(colorPickerButton, SIGNAL(clicked()), SLOT(promptChangeColor()));

  checkBox = new QCheckBox(name);
  layout->addWidget(checkBox, 1);

  layout->setContentsMargins(0, 1, 0, 1);

  connect(checkBox, SIGNAL(stateChanged(int)), SLOT(toggleDisplay(int)));

  
  changeColor(color);

  Qt::CheckState state = h ? Qt::Unchecked : Qt::Checked;
  checkBox->setCheckState(state);

}

TuneableDataDisplay::~TuneableDataDisplay()
{
}

void TuneableDataDisplay::setName(const QString & n)
{
  name = n;
  checkBox->setText(n);
}

QColor TuneableDataDisplay::currentColor() const
{
  return color;
}

CurvePoints * TuneableDataDisplay::addSource(XYIterable * source,
                                             bool autoadd)
{
  CurvePoints * cp = new CurvePoints(source);
  items << cp;
  if(autoadd)
    view->addItem(cp);
  cp->hidden = checkBox->checkState() == Qt::Unchecked;
  updateCurveColors();
  return cp;
}

CurvePoints * TuneableDataDisplay::setSource(XYIterable * source,
                                             bool autoadd)
{
  for(CurvePoints * cp : items)
    delete cp;
  items.clear();
  return addSource(source, autoadd);
}

void TuneableDataDisplay::updateCurveColors()
{
  // We update both the brush and the color
  for(int i = 0; i < items.size(); i++) {
    CurvePoints * cp = items[i];
    cp->brush.setColor(color);
    cp->pen.setColor(color);
  }
}

bool TuneableDataDisplay::bbCounts() const
{
  // We update both the brush and the color
  for(int i = 0; i < items.size(); i++) {
    if(items[i]->countBB)
      return true;
  }
  return false;
}

void TuneableDataDisplay::toggleDisplay(int display)
{
  bool hidden = display == 0;
  for(int i = 0; i < items.size(); i++)
    items[i]->hidden = hidden;
  
  /// Calls again this function by the generated signal, but no-op if
  /// already set.
  Qt::CheckState state = hidden ? Qt::Unchecked : Qt::Checked;
  if(checkBox->checkState() != state)
    checkBox->setCheckState(state);
  
  // Schedule a repaint
  // if(bbCounts())            /// @todo Maybe this should be tunable ?
  //   view->mainPanel()->zoomIn(QRectF());
  view->repaint();
}

void TuneableDataDisplay::changeColor(const QColor & col)
{
  color = col;
  colorPickerButton->setIcon(solidColor(12, color));
  updateCurveColors();
  view->repaint();
}

void TuneableDataDisplay::promptChangeColor()
{
  QColor c = QColorDialog::getColor(color);
  if(c.isValid())
    changeColor(c);
}

QList<DataSet*> TuneableDataDisplay::makeDataSets(bool onlyVisible)
{
  QList<DataSet*> rv;
  if( (! onlyVisible) || checkBox->checkState() == Qt::Checked) {
    for(int i = 0; i < items.size(); i++)
      rv << items[i]->makeDataSet();
  }
  return rv;
}
