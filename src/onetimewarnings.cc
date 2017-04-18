/*
  onetimewarnings.cc: implementation of OneTimeWarnings
  Copyright 2017 by CNRS/AMU

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
#include <onetimewarnings.hh>
#include <exceptions.hh>

bool OneTimeWarnings::warnOnce(QWidget * parent,
                               const QString & codeName,
                               const QString & title,const QString & warning)
{
  if(warningShown.value(codeName, false))
    return true;                // No need to show

  QMessageBox::StandardButton bt =
    QMessageBox::warning(parent, title, warning,
                         QMessageBox::Abort|QMessageBox::Ignore,
                         QMessageBox::Abort);
  warningShown[codeName] = true; 
  if(bt == QMessageBox::Ignore)
    return true;
  if(bt == QMessageBox::Escape)
    warningShown[codeName] = false; // Yep
  return false;
}
