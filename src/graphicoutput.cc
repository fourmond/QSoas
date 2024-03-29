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
#include <fileinfo.hh>


#include <soas.hh>
#include <curveview.hh>
#include <utils.hh>

GraphicOutput::GraphicOutput(const QString & t) :
  resolution(288), title(t)
{
  setOutputSize(600, 600);
}

void GraphicOutput::setOutputFile(const QString & file)
{
  fileOutput = file;
}

void GraphicOutput::setOutputSize(int width, int height, int res)
{
  bool landscape = false;
  if(width > height) {
    landscape = true;
    std::swap(width, height);
  }
  resolution = res;
  QPageSize sz(QSize(width, height), QString(), QPageSize::ExactMatch);
  layout.setPageSize(sz);
  layout.setMargins(QMarginsF(0,0,0,0));
  layout.setMode(QPageLayout::FullPageMode);
  layout.setOrientation(landscape ? QPageLayout::Landscape :
                        QPageLayout::Portrait);

}

void GraphicOutput::setOutputSize(const QString & spec, int res)
{
  QStringList lst = spec.split("x");
  if(lst.size() != 2)
    throw RuntimeError("Invalid page size specification: '%1'").arg(spec);
  double wd = parseDimension(lst[0]);
  double ht = parseDimension(lst[1]);
  setOutputSize(wd, ht, res);
}


void GraphicOutput::makePDF(CurveView * view)
{
  std::unique_ptr<QPrinter> printer(new QPrinter);
  printer->setOutputFileName(fileOutput);

  printer->setPageLayout(layout);
  printer->setFullPage(true);

  QRectF rect = printer->pageRect(QPrinter::Point);

  // QTextStream o(stdout);
  // o << "Printer device pixel ratio: " << printer->devicePixelRatioF()
  //   << " -- height: " << printer->height()
  //   << " -- phys DPI: " << printer->physicalDpiX()
  //   << " -- log DPI: " << printer->logicalDpiX()
  //   << endl;
  // o << "Printer rect: ";
  // Utils::dumpRectangle(o, rect);
  // o << "\nLayout rect: ";
  // Utils::dumpRectangle(o, layout.fullRectPoints());
  // o << endl;

  QPainter painter;
  painter.begin(printer.get());
  double scale = printer->logicalDpiX()/72.0;
  painter.scale(scale, scale);

  view->render(&painter, rect.height(), rect.toAlignedRect(), title);
  painter.end();
}

void GraphicOutput::shipOut(CurveView * view)
{
  if(fileOutput.isEmpty())
    throw RuntimeError("Not implemented yet");

  FileInfo info(fileOutput);
  if(info.suffix() == "pdf") {
    makePDF(view);
  }
}

static QHash<QString, double> dimensions( {
    {"in", 72},
      {"cm", 72/2.54},
        {"mm", 72/25.4},
          {"pt", 1}
          });
                                   

double GraphicOutput::parseDimension(const QString & dim)
{
  QRegExp re("^\\s*([0-9.]+)\\s*([a-z]+)?\\s*$");
  if(re.indexIn(dim) < 0)
    throw RuntimeError("Invalid size specification: '%1'").
      arg(dim);
  double val = re.cap(1).toDouble();
  QString unit = re.cap(2);
  double uv = 1;
  if(unit.size() > 0) {
    if(! ::dimensions.contains(unit))
      throw RuntimeError("Unkown unit: '%1'").arg(unit);
    uv = ::dimensions[unit];
  }
  return val * uv;
}
