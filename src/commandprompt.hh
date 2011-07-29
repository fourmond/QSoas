/**
   \file commandprompt.hh
   A derived class of QLineEdit that provides readline-like completion
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

#ifndef __COMMANDPROMPT_HH
#define __COMMANDPROMPT_HH

/// A QLineEdit with automatic completion facilities.
class CommandPrompt : public QLineEdit {

  Q_OBJECT;

  /// The number of successive hits on the TAB key
  int nbSuccessiveTabs;

  /// The current completions
  QStringList completions;

  /// Saved history, by reverse order of age (0 = most recent)
  QStringList savedHistory;

  /// The last history item used.
  int historyItem;

  /// Saving the currently edited line when recalling history
  QString savedLine;

  /// Internal class to represent the current word
  class Word {
  public:
    QString word;
    int begin;
    int end;
    int next;
    int index;
    QStringList allWords;
    
    Word(const QString & s, int b, int e, int n, int i,
         const QStringList & aw) : 
      word(s), begin(b), end(e), next(n), index(i),
      allWords(aw) {;};
  };

  /// Returns the current word
  Word getCurrentWord() const;

  /// Get all the possible completions for the current argument.
  QStringList getCompletions(const Word & w) const;

  /// Replace the given Word by the new string, and go at the end of
  /// it. Optionnally adds a space.
  void replaceWord(const Word & w, const QString & str, 
                   bool full = true);


public:

  CommandPrompt();
  virtual ~CommandPrompt();

public slots:
  void addHistoryItem(const QString & str);

protected:
  virtual void keyPressEvent(QKeyEvent * event);

};

#endif
