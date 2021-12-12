/**
   \file commandprompt.hh
   A derived class of QLineEdit that provides readline-like completion
   Copyright 2011 by Vincent Fourmond
             2012, 2014 by CNRS/AMU

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
#ifndef __COMMANDPROMPT_HH
#define __COMMANDPROMPT_HH

#include <lineedit.hh>

/// A QLineEdit with automatic completion and history facilities.
///
/// Think of it as a Qt version of readline...
class CommandPrompt : public LineEdit {

  Q_OBJECT;

  /// The number of successive hits on the TAB key
  int nbSuccessiveTabs;


  /// Wether the last key pressed was a TAB?
  bool lastTab;

  /// The current completions
  QStringList completions;

  /// Internal class to represent the current completion context
  class CompletionContext {
  public:
    QString word;
    int begin;
    int end;
    int next;
    int index;
    /// The number of arguments which are truly arguments and not
    /// options.
    int argumentNumber;
    bool unfinishedOption;
    QStringList allWords;
    QList<int> annotations;
    
    CompletionContext(const QString & s, int b, int e, int n, int i,
                      int an, bool uo,
                      const QStringList & aw, const QList<int> & l) : 
      word(s), begin(b), end(e), next(n), index(i),
      argumentNumber(an), unfinishedOption(uo),
      allWords(aw), annotations(l) {;};

    /// Here, some helpful functions ?
  };

  /// Returns the current word
  CompletionContext getCompletionContext() const;

  /// Get all the possible completions for the current argument.
  QStringList getCompletions(const CompletionContext & c, 
                             QString * reason, bool * complete) const;

  /// Replace the given Word by the new string, and go at the end of
  /// it. Optionnally adds a space.
  void replaceWord(const CompletionContext & c, const QString & str, 
                   bool full = true);

  /// Performs automatic completion.
  void doCompletion();

  
public:

  CommandPrompt();
  virtual ~CommandPrompt();


protected:
  virtual void keyPressEvent(QKeyEvent * event) override;
  virtual bool event(QEvent *event) override;

signals:

  /// Signals the parent that a scroll is requested. The number is the
  /// number of half screens.
  void scrollRequested(int nb);

  /// Signals that the terminal's content should be copied
  void shouldCopyTerminal();

public slots:
  /// Sets the prompt to busy mode and sets the given message if the
  /// string isn't empty, or switch back from busy mode if not
  void busy(const QString & message = QString());

};

#endif
