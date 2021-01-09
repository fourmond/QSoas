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

#include <settings-templates.hh>
#include <commandlineparser.hh>

static SettingsValue<int> maxLines("command/maxlines", 1000);

Terminal Terminal::out;

Terminal::Terminal() :
  buffer(""), appendCursor(NULL), deleteCursor(NULL),
  totalLines(0), deletedLines(0)
{
  internalStream = new QTextStream(&buffer);
}

void Terminal::initializeCursors()
{
  if(appendCursor)
    return;                     // nothing to do
  if(! CommandWidget::theCommandWidget)
    return;                     // Nothing possible yet
  appendCursor = new QTextCursor(CommandWidget::theCommandWidget->
                                 terminalDisplay->document());
  appendCursor->movePosition(QTextCursor::End);
  deleteCursor = new QTextCursor(CommandWidget::theCommandWidget->
                                 terminalDisplay->document());
}

void Terminal::flushToTerminal()
{
  /// @todo If I want to use formatting with that, I need to use
  /// QTextDocumentFragment straight from here.
  if(buffer.size() == 0)
    return;                     // nothing to do Note that it means
                                // that you can chain format
                                // specifiers.
  
  initializeCursors();
  if(appendCursor) {
    QTextEdit * edit = CommandWidget::theCommandWidget->
      terminalDisplay;
    appendCursor->insertText(buffer, currentFormat);
    currentFormat = QTextCharFormat();
    
    totalLines += buffer.count('\n');

    // Remove lines at the beginning
    if(totalLines > ::maxLines) {
      int nbdel = totalLines - (::maxLines + deletedLines);
      deleteCursor->movePosition(QTextCursor::Start);
      deleteCursor->movePosition(QTextCursor::Down,
                                 QTextCursor::KeepAnchor,
                                 nbdel);
      deleteCursor->removeSelectedText();
      deletedLines += nbdel;
    }



    // and scroll to the bottom
    QScrollBar * sb = edit->verticalScrollBar();
    sb->setSliderPosition(sb->maximum());

    // Always clear undo stacks, we don't need those
    edit->document()->clearUndoRedoStacks();
  }
  else {
    QTextStream o(stdout);
    o << buffer;
  }

  for(int i = 0; i < spies.size(); i++)
    (*spies[i]) << buffer << flush;
  
  buffer.clear();
}

Terminal & Terminal::bold(Terminal & term)
{
  term.currentFormat.setFontWeight(QFont::Bold);
  return term;
}

Terminal & Terminal::red(Terminal & term)
{
  term.currentFormat.setForeground(Qt::red);
  return term;
}

// Terminal & Terminal::normal(Terminal & term)
// {
//   term.currentFormat = QTextCharFormat()
//   return term;
// }

void Terminal::addSpy(QTextStream * spy)
{
  spies << spy;
}

void Terminal::addSpy(QIODevice * spy)
{
  QTextStream * ns = new QTextStream(spy);
  addSpy(ns);
  ownedDevices << spy;
}

Terminal & Terminal::operator<<(QTextStreamFunction t)
{
  (*internalStream) << t;
  if(t == endl || t == flush)
    flushToTerminal();
  return *this;
}

Terminal & Terminal::operator<<(Terminal & fnc(Terminal &) )
{
  flushToTerminal();
  fnc(*this);
  return *this;
}

Terminal::~Terminal()
{
  if(! buffer.isEmpty())
    flushToTerminal();
  delete appendCursor;
  delete deleteCursor;
  delete internalStream;
  for(QTextStream * t : spies)
    delete t;
  for(QIODevice * d : ownedDevices)
    delete d;
}

static CommandLineOption sto("--stdout", [](const QStringList & /*args*/) {
    Terminal::out.addSpy(new QTextStream(stdout));
  }, 0, "write terminal output also to standard output");
