/**
   \file helpbrowser.hh
   Browser for inline help
   Copyright 2020 by CNRS/AMU

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
#ifndef __HELPBROWSER_HH
#define __HELPBROWSER_HH

class Command;

/// A browser to display the help texts
class HelpBrowser : public QWidget {
  Q_OBJECT;

  QHelpEngine * engine;

  static HelpBrowser * theBrowser;

  /// The browser part of the deal.
  QTextBrowser * browser;

  void setupFrame();


  static HelpBrowser * getBrowser();

public:
  HelpBrowser();
  virtual ~HelpBrowser();


  /// Browses the given location
  static void browseLocation(const QString & location);

  /// Browses the documentation of the given command.
  static void browseCommand(const Command * command);

public slots:
  /// Shows the given location, i.e. a relative file path, with
  /// optionally a #position
  void showLocation(const QString & location);
  void showLocation(const QUrl & location);

};



#endif
