/*
  ruby-complex.cc: support of formulas with complex numbers
  Copyright 2018 by CNRS/AMU

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


class Complex {
public:
  double r;
  double i;
  Complex(double _r, double _i) : r(_r), i(_i) {
  };

  QString toString() const {
    return QString("%1 + %2i").arg(r).arg(i);
  };

  Complex & operator*=(double val) {
    r *= val;
    i *= val;
    return *this;
  };

  Complex & operator*=(const Complex & c) {
    double rv = r*c.r - i*c.i;
    double iv = r*c.i + i*c.r;
    r = rv;
    i = iv;
    return *this;
  };

  
};

static void co_free(mrb_state *mrb, void *p)
{
  Complex * c = (Complex *) p;
  delete c;
}


static const struct mrb_data_type co_data_type = {
  "complex", co_free,
};

static Complex * co_get_c(mrb_state * mrb, mrb_value self)
{
  void * f = mrb_data_get_ptr(mrb, self, &co_data_type);
  if(! f) {
    // pb
  }
  return (Complex*) f;
}

static mrb_value co_wrap(mrb_state * mrb, Complex * c)
{
  MRuby * m = MRuby::ruby();
  return mrb_obj_value(Data_Wrap_Struct(mrb, m->cComplex, &co_data_type, c));
}


static mrb_value co_mk(mrb_state * mrb, struct RClass * c)
{
  mrb_float r, i;
  mrb_get_args(mrb, "ff", &r, &i);

  Complex * co = new Complex(r, i);

  return mrb_obj_value(Data_Wrap_Struct(mrb, c, &co_data_type, co));
}




static mrb_value co_make(mrb_state * mrb, mrb_value cls)
{
  RClass * c = mrb_class_ptr(cls);
  return co_mk(mrb, c);
}

static mrb_value co_gbl_make(mrb_state * mrb, mrb_value self)
{
  MRuby * m = MRuby::ruby();
  return co_mk(mrb, m->cComplex);
}

static mrb_value co_to_s(mrb_state * mrb, mrb_value self)
{
  Complex * c = co_get_c(mrb, self);
  QString s = c->toString();
  MRuby * m = MRuby::ruby();
  return m->fromQString(s);
}


// Operations
static mrb_value co_mul(mrb_state * mrb, mrb_value self)
{
  Complex * c = co_get_c(mrb, self);
  mrb_value v;
  mrb_get_args(mrb, "o", &v);
  MRuby * m = MRuby::ruby();
  Complex * nc = new Complex(*c);
  if(mrb_obj_is_kind_of(mrb, v, m->cComplex)) {
    const Complex * c2 = co_get_c(mrb, v);
    *(nc) *= *c2;
  }
  else {
    mrb_float f;
    mrb_get_args(mrb, "f", &f);
    *(nc) *= f;
  }
  return co_wrap(mrb, nc);
}


void MRuby::initializeComplex()
{
  cComplex = mrb_define_class(mrb, "Complex", mrb->object_class);

  mrb_define_class_method(mrb, cComplex, "new",
                          &::co_make, MRB_ARGS_REQ(2));


  // Unsure it is a good idea -- make _(1,2) a complex number
  mrb_define_method(mrb, mrb->kernel_module, "_",
                    &::co_gbl_make, MRB_ARGS_REQ(2));


  // Operations...
  mrb_define_method(mrb, cComplex, "to_s",
                    &::co_to_s, MRB_ARGS_REQ(0));
  mrb_define_method(mrb, cComplex, "inspect",
                    &::co_to_s, MRB_ARGS_REQ(0));
  mrb_define_method(mrb, cComplex, "*",
                    &::co_mul, MRB_ARGS_REQ(1));
}
