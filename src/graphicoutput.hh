/**
   \file graphicoutput.hh
   The class handling all the graphic output to print/graphics file
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
#ifndef __GRAPHICOUTPUT_HH
#define __GRAPHICOUTPUT_HH

class CurveView;

/// A helper class to draw print previews
class GraphicOutput : public QObject {

  Q_OBJECT;

  /// The page size, in postscript points = (1/72th of an inch).
  ///
  /// The idea is that the postscript point will also be the 
  QSize pageSize;

  /// The resolution in dpi. Not useful for anything else than bitmap
  /// output
  int resolution;

  /// The output file, or empty if one should use a print dialog or such.
  QString fileOutput;

  void makePDF(CurveView * view);

  /// The title of the output
  QString title;

public:


  GraphicOutput(const QString & title);
  
  /// Sets the output size in natural units, i.e. 6cmx12in
  void setOutputSize(const QString & spec);

  /// Sets the output size in postscript points
  void setOutputSize(int width, int height);

  /// Sets the output file name.
  void setOutputFile(const QString & file);


  /// Do the actual printing job.
  /// As of now, will fail if there is no file output.
  void shipOut(CurveView * view);
  
  

};

#endif
