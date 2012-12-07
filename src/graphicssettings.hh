/**
   \file graphicssettings.hh
   A unique storage place for all graphics-related settings
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
#ifndef __GRAPHICSSETTINGS_HH
#define __GRAPHICSSETTINGS_HH

#include <settings-templates.hh>

/// This class contains a whole bunch of graphics-related settings,
/// for.
class GraphicsSettings {

  SettingsValue<bool> antialias;
  SettingsValue<bool> opengl;

  /// The base line width for all pens (can be multiplied later)
  SettingsValue<double> baseLineWidth;
  

public:

  GraphicsSettings();


  void setAntiAlias(bool b);
  bool antiAlias() const;

  void setOpenGL(bool b);
  bool openGL() const;

  /// GraphicsSettings comes with predefined (customizable-to-be) pens
  /// that are appropriate for different uses.
  ///
  /// Cyclic pens for CurveDataSet display are handled somewhere else.
  ///
  /// @todo There is probably a lot more rationalization to do with
  /// these names, but this can come easily at a later time.
  typedef enum {
    /// pen to draw DataSet segments
    SegmentsPen, 
    /// pen to draw a vertical (or horizontal ?) separation
    SeparationPen, 
    /// pen to draw peaks ?
    PeaksPen, 
    /// pen to draw left sides of regions
    LeftSidePen, 
    /// pen to draw right sides of regions
    RightSidePen, 
    /// pen to draw a linear regression
    ReglinPen, 
    /// pen to draw resulting signal (filters and so on)
    ResultPen, 
    /// pen to draw baselines when applicable
    BaselinePen, 
    /// pen to draw fits
    FitsPen, 
  } PenRoles;

  /// Returns the pen for the given role
  ///
  /// @todo Implement variants in terms of color.
  QPen getPen(PenRoles role) const;

  /// Returns a pen suitable for the n-th dataset
  QPen dataSetPen(int n) const;

};



#endif
