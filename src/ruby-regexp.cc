/*
  ruby-regexp.cc: regexp between Ruby and QSoas
  Copyright 2017, 2018 by CNRS/AMU

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

#include <mruby/data.h>
#include <mruby/class.h>
#include <mruby/variable.h>

static void re_free(mrb_state *mrb, void *p)
{
  QRegExp * re = (QRegExp *) p;
  delete re;
}

static const struct mrb_data_type re_data_type = {
  "qregexp", re_free,
};


static mrb_value qs_compile(mrb_state * mrb, mrb_value cls)
{
  static QString es;
  char * pat;
  char * opts;
  mrb_bool has_opts = false;
  mrb_get_args(mrb, "z|z?", &pat, &opts, &has_opts);

  // later: parsing of pat + checking the syntax is valid
  QRegExp * re = new QRegExp(pat);
  if(! re->isValid()) {
    printf("Stderr: %p\n", mrb->eException_class);
    printf("Stderr: %p\n", mrb->eStandardError_class);
    es = QString("Error while compiling Regexp: %1").
      arg(re->errorString());
    delete re;
    mrb_raise(mrb, mrb->eStandardError_class, es.toLocal8Bit().constData());
  }

  RClass * c = mrb_class_ptr(cls);
  return mrb_obj_value(Data_Wrap_Struct(mrb, c, &re_data_type, re));
}

static mrb_value qs_match(mrb_state * mrb, mrb_value self)
{
  char * str;
  mrb_get_args(mrb, "z", &str);

  void * f = mrb_data_get_ptr(mrb, self, &re_data_type);
  if(! f) {
    // pb
  }
  QRegExp * re = (QRegExp *) f;
  QString s = str;
  int idx = re->indexIn(str);

  // clear all variables:
  mrb_gv_set(mrb, mrb_intern_lit(mrb, "$&"), mrb_nil_value());

  // Clear captured texts
  for(int i = 1; i < 10; i++) {
    char buf[3] = "$1";
    buf[1] = '0' + i;
    mrb_gv_set(mrb, mrb_intern_cstr(mrb, buf), mrb_nil_value());
  }
  
  if(idx >= 0) {
    // set all variables:
    QByteArray bt = re->cap(0).toLocal8Bit();
    mrb_gv_set(mrb, mrb_intern_lit(mrb, "$&"),
               mrb_str_new_cstr(mrb, bt.constData()));
    
    // Clear captured texts
    for(int i = 1; i <= std::min(re->captureCount(), 9); i++) {
      char buf[3] = "$1";
      buf[1] = '0' + i;
      bt = re->cap(i).toLocal8Bit();
      mrb_gv_set(mrb, mrb_intern_cstr(mrb, buf),
                 mrb_str_new_cstr(mrb, bt.constData()));
    }
    
    return mrb_fixnum_value(idx);
  }
  else
    return mrb_false_value();
}

void MRuby::initializeRegexp()
{
  struct RClass * re = mrb_define_class(mrb, "Regexp", mrb->object_class);

  mrb_define_class_method(mrb, re, "compile",
                          &::qs_compile, MRB_ARGS_REQ(1)|MRB_ARGS_OPT(1));

  mrb_define_method(mrb, re, "=~",
                    &::qs_match, MRB_ARGS_REQ(1));
}
