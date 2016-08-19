/*
  terminal.cc: implementation of the interface to the terminal
  Copyright 2011 by Vincent Fourmond

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
#include <terminal.hh>
#include <commandwidget.hh>

Terminal Terminal::out;

Terminal::Terminal() :
  buffer("")
{
  internalStream = new QTextStream(&buffer);
}

void Terminal::flushToTerminal()
{
  /// @todo If I want to use formatting with that, I need to use
  /// QTextDocumentFragment straight from here.

  // Convert line feeds to HTML
  // buffer.replace("\n", "<br/>\n");
  CommandWidget::logString(buffer);
  buffer.clear();
}

Terminal & Terminal::operator<<(QTextStreamFunction t)
{
  (*internalStream) << t;
  if(t == endl || t == flush)
    flushToTerminal();
  return *this;
}

Terminal::~Terminal()
{
  if(! buffer.isEmpty())
    flushToTerminal();
  delete internalStream;
}
