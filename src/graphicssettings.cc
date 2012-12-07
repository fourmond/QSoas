/**
   graphicssettings.cc: implementation of GraphicsSettings
   Copyright 2012 by Vincent Fourmond

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
#include <graphicssettings.hh>

#include <soas.hh>
#include <curveview.hh>

GraphicsSettings::GraphicsSettings() :
  antialias("graphics/antialias", false),
  opengl("graphics/opengl", false),
  baseLineWidth("graphics/pen-width", 1.5)
{
}


void GraphicsSettings::setAntiAlias(bool b)
{
  antialias = b;
}

bool GraphicsSettings::antiAlias() const {
  return antialias;
}

void GraphicsSettings::setOpenGL(bool b)
{
  opengl = b;
  soas().view().setOpenGL(opengl);
}

bool GraphicsSettings::openGL() const
{
  return opengl;
}

/// @todo Customize + make the defaults a little nicer ;-)...
QPen GraphicsSettings::getPen(PenRoles role) const
{
  switch(role) {
  case SegmentsPen:
    return QPen(QColor("darkBlue"), baseLineWidth, Qt::DotLine);
  case SeparationPen:
    return QPen(QColor("darkMagenta"), baseLineWidth, Qt::DashLine);
  case PeaksPen:
    return QPen(QColor("darkCyan"), baseLineWidth, Qt::DotLine);
  case LeftSidePen:
    return QPen(QColor("darkGreen"), baseLineWidth, Qt::DashLine);
  case RightSidePen:
    return QPen(QColor("darkRed"), baseLineWidth, Qt::DashLine);
  case BaselinePen:
    return QPen(QColor("blue"), baseLineWidth*0.8, Qt::DotLine);
  case ReglinPen:
    return QPen(QColor("blue"), baseLineWidth, Qt::DotLine);
  case ResultPen:
    return QPen(QColor("red"), baseLineWidth);
  default:
    return QPen();
  };
  return QPen();
}

/// @todo Fix this !
static const char * colors[] = 
  { "red", "blue", "#080", "orange", "purple"};
static int nbColors = sizeof(colors)/sizeof(colors[0]);

/// @todo Customize + make the defaults a little nicer ;-)...
QPen GraphicsSettings::dataSetPen(int nb) const
{
  return QPen(QColor(colors[nb % nbColors]), 
              1.3 * baseLineWidth);
}
