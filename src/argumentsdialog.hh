/**
   \file argumentsdialog.hh
   A dialog box for prompting all the arguments/options of a command.
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
#ifndef __ARGUMENTSDIALOG_HH
#define __ARGUMENTSDIALOG_HH

class Command;
class Argument;


class ArgumentsWidget : public QWidget {
  const Argument * argument;

  bool isOption;
public:
  ArgumentsWidget(const Argument * arg, bool isOption, QWidget * parent = NULL);
  ~ArgumentsWidget();
};


/// A dialog box for prompting values of 
class ArgumentsDialog : public QDialog {
  Q_OBJECT;

  /// The underlying command.
  const Command * command;
  
public:

  ArgumentsDialog(const Command * command);
  ~ArgumentsDialog();


};



#endif
