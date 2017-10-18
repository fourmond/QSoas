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
#include <mruby.hh>

#include <soas.hh>
#include <command.hh>
#include <argument.hh>

#include <valuehash.hh>

static mrb_value qs_method_missing(mrb_state * mrb, mrb_value self)
{
  mrb_sym sym;
  mrb_int nb;
  mrb_value * args;
  mrb_get_args(mrb, "n*", &sym, &args, &nb);

  QString cmd = mrb_sym2name(mrb, sym);

  // fprintf(stdout, "Called: %s with %d args:\n", qPrintable(cmd),
  //         nb);
  // for(int i = 0; i < nb; i++) {
  //   printf("Arg %d: ", i);
  //   mrb_p(mrb, args[i]);
  //   // mrb_gc_register(mrb, args[i]);
  // } 

  Command * command = Command::namedCommand(cmd, true);
  if(! command) {
    QString arg = QString("Unkown QSoas command: '%1'").arg(cmd);
    mrb_raise(mrb, mrb->eStandardError_class, arg.toLatin1().constData());
  }

  try {
    command->runCommand(nb, args);
  }
  catch(const RuntimeError & er) {
    mrb_raise(mrb, mrb->eStandardError_class, er.message().
              toLatin1().constData());
  }
  catch(const Exception & er) {
    mrb_raise(mrb, mrb->eStandardError_class, er.message().
              toLatin1().constData());
  }

  return self;
}

mrb_value qs_interface(mrb_state * /*mrb*/, mrb_value /*self*/)
{
  MRuby * mr = MRuby::ruby();
  return mr->soasInstance;
}

mrb_value qs_config(mrb_state * /*mrb*/, mrb_value /*self*/)
{
  return Soas::versionInfo().toRuby();
}

void MRuby::initializeInterface()
{
  if(! Soas::soasInstance())
    throw InternalError("Initializing the Ruby interface without "
                        "a Soas object");
  cQSoasInterface = mrb_define_class(mrb, "QSoasInterface", mrb->object_class);

  mrb_define_method(mrb, cQSoasInterface, "method_missing",
                    &::qs_method_missing, MRB_ARGS_ANY());

  mrb_define_class_method(mrb, cQSoasInterface, "the_instance",
                          &::qs_interface, MRB_ARGS_NONE());

  mrb_define_method(mrb, cQSoasInterface, "config",
                    &::qs_config, MRB_ARGS_NONE());

  
  soasInstance = mrb_obj_new(mrb, cQSoasInterface, 0, NULL);
  defineGlobalConstant("Soas", soasInstance);
  defineGlobalConstant("QSoas", soasInstance);
}
