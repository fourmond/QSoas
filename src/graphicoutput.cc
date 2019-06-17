/*
  graphicoutput.cc: output to graphics file
  Copyright 2019 by CNRS/AMU

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
#include <graphicoutput.hh>


#include <soas.hh>
#include <curveview.hh>


GraphicOutput::GraphicOutput(const QString & t) :
  pageSize(1000,1000), resolution(288), title(t)
{
  
}

void GraphicOutput::setOutputFile(const QString & file)
{
  fileOutput = file;
}


void GraphicOutput::makePDF(CurveView * view)
{
  std::unique_ptr<QPrinter> printer(new QPrinter);
  printer->setOutputFileName(fileOutput);

  QPageSize page(pageSize, QPageSize::Point);
  
  printer->setPageSize(page);
  printer->setFullPage(true);

  QRectF rect = printer->pageRect(QPrinter::Point);

  QPainter painter;
  painter.begin(printer.get());
  view->render(&painter, rect.height(), rect.toAlignedRect(), title);
  painter.end();
}

void GraphicOutput::shipOut(CurveView * view)
{
  if(fileOutput.isEmpty())
    throw RuntimeError("Not implemented yet");

  QFileInfo info(fileOutput);
  if(info.suffix() == "pdf") {
    makePDF(view);
  }
}

