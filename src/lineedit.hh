/**
   \file lineedit.hh
   A derived class of QLineEdit that provides history
   Copyright 2011 by Vincent Fourmond
   Copyright 2014 by AMU/CNRS

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
#ifndef __LINEEDIT_HH
#define __LINEEDIT_HH

/// A QLineEdit with history.
class LineEdit : public QLineEdit {

  Q_OBJECT;
protected:

  /// Saved history, by reverse order of age (0 = most recent)
  QStringList savedHistory;

  /// The last history item used.
  int historyItem;

  /// Saving the currently edited line when recalling history
  QString savedLine;

  /// Wether we save history automatically.
  bool autoSaveHistory;


  /// Perform history substitution
  void doHistoryExpansion();

  /// Performs automatic completion.
  void doCompletion();

public:

  LineEdit();
  virtual ~LineEdit();

  /// Returns the list of history elements starting with the given
  /// string.
  QStringList historyMatching(const QString & str) const;

  /// Returns the whole history.
  QStringList history() const {
    return savedHistory;
  };


public slots:
  void addHistoryItem(const QString & str);

protected:
  virtual void keyPressEvent(QKeyEvent * event);

protected slots:
  void recordHistory();

};

#endif
