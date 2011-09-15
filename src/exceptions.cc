/*
  exceptions.cc: exceptions definitions
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
#include <exceptions.hh>

Exception::Exception(const QString & m) throw() : 
  msg(m) {
};

const char * Exception::what() const throw()
{
  return msg.toLocal8Bit().constData();
}

QString Exception::message() const throw()
{
  return msg;
}
