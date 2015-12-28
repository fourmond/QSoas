/*
  ruby-interface.cc: interface between Ruby and QSoas
  Copyright 2015 by CNRS/AMU

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
#include <ruby.hh>
#include <ruby-templates.hh>

#include <soas.hh>
#include <command.hh>
#include <argument.hh>


static RUBY_VALUE cQSoas = rbw_nil;

static void qsoas_mark(void *)
{
}

static void qsoas_free(void *)
{
}

static RUBY_VALUE wrap_qsoas_instance(Soas * instance)
{
  return rbw_data_object_alloc(cQSoas, instance, &qsoas_mark, &qsoas_free);
}

static Soas * get_qsoas_instance(RUBY_VALUE obj)
{
  return static_cast<Soas*>(rbw_data_get_struct(obj));
}

static RUBY_VALUE qs_get_instance(RUBY_VALUE /*klass*/)
{
  return wrap_qsoas_instance(Soas::soasInstance());
}

static RUBY_VALUE qs_method_missing(int argc, RUBY_VALUE * argv, RUBY_VALUE obj)
{
  Soas * instance = get_qsoas_instance(obj);

  
  if(argc == 0)
    rbw_raise(rbw_eArgError(), "Call to method_missing without arguments, that is weird");
  


  QString cmd = Ruby::toQString(argv[0]);

  Command * command = Command::namedCommand(cmd, true);
  if(! command) {
    QString arg = QString("Unkown QSoas command: '%1'").arg(cmd);
    rbw_raise(rbw_eArgError(), arg.toLatin1().data());
  }

  command->runCommand(argc-1, argv+1);

  return obj;
}

void Ruby::initInterface()
{
  cQSoas = rbw_define_class("QSoas", rbw_cObject());

  rbw_define_singleton_method(cQSoas, "the_instance", (rbw_function) &::qs_get_instance, 0);

  rbw_define_method(cQSoas, "method_missing", (rbw_function) &::qs_method_missing, -1);

  
}
