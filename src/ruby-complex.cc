/*
  ruby-complex.cc: support of formulas with complex numbers
  Copyright 2018,2021 by CNRS/AMU

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


//////////////////////////////////////////////////////////////////////
// Memory management



static void co_free(mrb_state *mrb, void *p)
{
  std::complex<double> * c = (std::complex<double> *) p;
  delete c;
}


static const struct mrb_data_type co_data_type = {
  "cplx", co_free,
};

static std::complex<double> * co_get_c(mrb_state * mrb, mrb_value self)
{
  void * f = mrb_data_get_ptr(mrb, self, &co_data_type);
  return (std::complex<double> *) f;
}

static mrb_value co_wrap(mrb_state * mrb, std::complex<double> * c)
{
  MRuby * m = MRuby::ruby();
  return mrb_obj_value(Data_Wrap_Struct(mrb, m->cCplx,
                                        &co_data_type, c));
}

//////////////////////////////////////////////////////////////////////
// Object creation

static mrb_value co_create(mrb_state * mrb, struct RClass * c,
                           const std::complex<double> & z)
{
  std::complex<double> * co = new std::complex<double>(z);
  return mrb_obj_value(Data_Wrap_Struct(mrb, c, &co_data_type, co));
}


static mrb_value co_create(mrb_state * mrb, struct RClass * c,
                           mrb_float real,
                           mrb_float imag)
{
  std::complex<double> * co = new std::complex<double>(real, imag);
  return mrb_obj_value(Data_Wrap_Struct(mrb, c, &co_data_type, co));
}

static mrb_value co_mk(mrb_state * mrb, struct RClass * c)
{
  mrb_float real, imag = 0.0;
  mrb_get_args(mrb, "f|f", &real, &imag);
  return co_create(mrb, c, real, imag);
}


static mrb_value co_make(mrb_state * mrb, mrb_value cls)
{
  RClass * c = mrb_class_ptr(cls);
  return co_mk(mrb, c);
}

static mrb_value co_new(mrb_state * mrb, mrb_value slf)
{
  mrb_float real, imag = 0.0;
  mrb_get_args(mrb, "f|f", &real, &imag);
  MRuby * mr = MRuby::ruby();
  return co_create(mrb, mr->cCplx, real, imag);
}




//////////////////////////////////////////////////////////////////////
// Helper functions


static std::complex<double> argAsComplex(mrb_state * mrb)
{
  mrb_value v;
  mrb_get_args(mrb, "o", &v);
  MRuby * m = MRuby::ruby();
  if(mrb_obj_is_kind_of(mrb, v, m->cCplx))
    return *co_get_c(mrb, v);
  mrb_float f;
  mrb_get_args(mrb, "f", &f);
  return std::complex<double>(f, 0);
}

//////////////////////////////////////////////////////////////////////
// Mathematic operations

static mrb_value co_mul(mrb_state * mrb, mrb_value self)
{
  std::complex<double> * c =
    new std::complex<double>(*co_get_c(mrb, self));
  *c *= argAsComplex(mrb);
  return co_wrap(mrb, c);
}

static mrb_value co_div(mrb_state * mrb, mrb_value self)
{
  std::complex<double> * c =
    new std::complex<double>(*co_get_c(mrb, self));
  *c /= argAsComplex(mrb);
  return co_wrap(mrb, c);
}

static mrb_value co_add(mrb_state * mrb, mrb_value self)
{
  std::complex<double> * c =
    new std::complex<double>(*co_get_c(mrb, self));
  *c += argAsComplex(mrb);
  return co_wrap(mrb, c);
}

static mrb_value co_sub(mrb_state * mrb, mrb_value self)
{
  std::complex<double> * c =
    new std::complex<double>(*co_get_c(mrb, self));
  *c -= argAsComplex(mrb);
  return co_wrap(mrb, c);
}

static mrb_value co_pow(mrb_state * mrb, mrb_value self)
{
  std::complex<double> * c =
    new std::complex<double>(*co_get_c(mrb, self));
  *c = std::pow(*c, argAsComplex(mrb));
  return co_wrap(mrb, c);
}

//////////////////////////////////////////////////////////////////////
// Mathematic functions

static mrb_value co_abs(mrb_state * mrb, mrb_value self)
{
  const std::complex<double> * c = co_get_c(mrb, self);
  return mrb_float_value(mrb, std::abs(*c));
}

static mrb_value co_arg(mrb_state * mrb, mrb_value self)
{
  const std::complex<double> * c = co_get_c(mrb, self);
  return mrb_float_value(mrb, std::arg(*c));
}

static mrb_value co_real(mrb_state * mrb, mrb_value self)
{
  const std::complex<double> * c = co_get_c(mrb, self);
  return mrb_float_value(mrb, c->real());
}

static mrb_value co_imag(mrb_state * mrb, mrb_value self)
{
  const std::complex<double> * c = co_get_c(mrb, self);
  return mrb_float_value(mrb, c->imag());
}

static mrb_value co_conj(mrb_state * mrb, mrb_value self)
{
  std::complex<double> * c =
    new std::complex<double>(std::conj(*co_get_c(mrb, self)));
  return co_wrap(mrb, c);
}



//////////////////////////////////////////////////////////////////////
// Utility functions
static mrb_value co_to_s(mrb_state * mrb, mrb_value self)
{
  std::complex<double> * c = co_get_c(mrb, self);
  QString s = QString("%1 + %2*i").arg(c->real()).
    arg(c->imag());
  MRuby * m = MRuby::ruby();
  return m->fromQString(s);
}


//////////////////////////////////////////////////////////////////////
// MRuby class functions related to complex numbers

void MRuby::initializeComplex()
{
  cCplx = mrb_define_class(mrb, "Cplx", mrb->object_class);

  mrb_define_class_method(mrb, cCplx, "new",
                          &::co_make, MRB_ARGS_REQ(1)|MRB_ARGS_OPT(1));

  mrb_define_method(mrb, mrb->kernel_module, "Cplx", &::co_new,
                    MRB_ARGS_REQ(1)|MRB_ARGS_OPT(1));
  mrb_define_method(mrb, mrb->kernel_module, "Z", &::co_new,
                    MRB_ARGS_REQ(1)|MRB_ARGS_OPT(1));


  // // Unsure it is a good idea -- make _(1,2) a complex number
  // mrb_define_method(mrb, mrb->kernel_module, "_",
  //                   &::co_gbl_make, MRB_ARGS_REQ(2));


  // mrb_define_method(mrb, cComplex, "inspect",
  //                   &::co_to_s, MRB_ARGS_REQ(0));

  
  // Operations...
  mrb_define_method(mrb, cCplx, "+",
                    &::co_add, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cCplx, "-",
                    &::co_sub, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cCplx, "*",
                    &::co_mul, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cCplx, "/",
                    &::co_div, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cCplx, "**",
                    &::co_pow, MRB_ARGS_REQ(1));

  // Mathematic functions
  mrb_define_method(mrb, cCplx, "abs",
                    &::co_abs, MRB_ARGS_NONE());
  mrb_define_method(mrb, cCplx, "arg",
                    &::co_arg, MRB_ARGS_NONE());
  mrb_define_method(mrb, cCplx, "conj",
                    &::co_conj, MRB_ARGS_NONE());
  mrb_define_method(mrb, cCplx, "real",
                    &::co_real, MRB_ARGS_NONE());
  mrb_define_method(mrb, cCplx, "imag",
                    &::co_imag, MRB_ARGS_NONE());

  mrb_value v = co_create(mrb, cCplx, 0, 1);

  mrb_define_global_const(mrb, "I", v);

  // Utility functions
  mrb_define_method(mrb, cCplx, "to_s",
                    &::co_to_s, MRB_ARGS_REQ(0));

}

bool MRuby::isComplex(mrb_value value)
{
  return mrb_obj_is_kind_of(mrb, value, cCplx);
}

std::complex<double> MRuby::complexValue(mrb_value value)
{
  std::complex<double> rv;
  protect([this, value, &rv]() -> mrb_value {
      rv = complexValue_up(value);
      return mrb_nil_value();
    }
    );
  return rv;
}

std::complex<double> MRuby::complexValue_up(mrb_value value)
{
  return *co_get_c(mrb, value);
}

mrb_value MRuby::newComplex(const std::complex<double> & z)
{
  return co_create(mrb, cCplx, z);
}
