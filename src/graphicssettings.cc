/**
   graphicssettings.cc: implementation of GraphicsSettings
   Copyright 2012, 2013 by CNRS/AMU

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


#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>

#include <terminal.hh>


GraphicsSettings::GraphicsSettings() :
  antialias("graphics/antialias", false),
  opengl("graphics/opengl", false),
  baseLineWidth("graphics/pen-width", 1.5)
{
}

void GraphicsSettings::initialSetup(bool silent)
{
  setOpenGL(openGL());
  if(! silent)
    showCurrentSettings();
}

void GraphicsSettings::showCurrentSettings()
{
  Terminal::out << "Antialias is " << (antialias ? "on" : "off") 
                << "\n" << (opengl ? "Using" : "Not using") 
                << " OpenGL for acceleration" << endl;
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
  soas().view().setOpenGL(openGL());
}

bool GraphicsSettings::openGL() const
{
  if(qApp->platformName() == "offscreen")
    return false;               // Never enable
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
    return QPen(QColor(200,0,200), baseLineWidth);
  case FitsPen:
    return QPen(QColor("darkGreen"), baseLineWidth, Qt::DashLine);
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
QPen GraphicsSettings::dataSetPen(int nb, bool alternative) const
{
  QPen a(QColor(colors[nb % nbColors]), 
         1.3 * baseLineWidth);
  if(alternative)
    a.setStyle(Qt::DashLine);
  //    a.setDashPattern(QVector<qreal>() << 15 << 7 << 10 << 7); // 
  return a;
}

// Settings-related commands:

//////////////////////////////////////////////////////////////////////

void graphicsSettingsCommand(const QString &, 
                             const CommandOptions & opts)
{
  GraphicsSettings & gs = soas().graphicsSettings();
  if(opts.size() == 0) {
    
    // display settings
    return;
  }

  /// @todo Make a template function for that !
  double lw = gs.baseLineWidth;
  updateFromOptions(opts, "line-width", lw);
  if(lw != gs.baseLineWidth) {
    gs.baseLineWidth = lw;
    Terminal::out << "Setting base line width to " << lw << endl;
  }


  bool opengl = gs.openGL();
  updateFromOptions(opts, "opengl",opengl);
  if(opengl != gs.openGL())
    gs.setOpenGL(opengl);

  bool antialias = gs.antiAlias();
  updateFromOptions(opts, "antialias", antialias);
  if(antialias != gs.antiAlias())
    gs.setAntiAlias(antialias);

  // Display current settings ?
  gs.showCurrentSettings();
  
}

static ArgumentList 
gsOpts(QList<Argument *>() 
       << new NumberArgument("line-width", 
                             "Line with",
                             "Sets the base line width for all lines/curves")
       << new BoolArgument("opengl", 
                           "OpenGL",
                           "Turns on/off the use of OpenGL acceleration")
       << new BoolArgument("antialias", 
                           "Antialias",
                           "Turns on/off the use of antialised graphics")
       );


static Command 
gs("graphics-settings", // command name
   effector(graphicsSettingsCommand), // action
   "file",  // group name
   NULL, // arguments
   &gsOpts, // options
   "Graphics settings",
   "Display/sets graphics settings");

