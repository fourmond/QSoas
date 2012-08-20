/*
  gslfunction.cc: implementation of the GSLFunction classes
  Copyright 2012 by Vincent Fourmond

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
#include <gslfunction.hh>

#include <gsl/gsl_sf.h>


QList<GSLFunction *> * GSLFunction::functions = NULL;

void GSLFunction::registerSelf()
{
  if(! functions)
    functions = new QList<GSLFunction *>;
  functions->append(this);
}


GSLFunction::GSLFunction(const QString & n, bool autoreg) : name(n)
{
  if(autoreg)
    registerSelf();
}

VALUE GSLFunction::registerAllFunctions()
{
  VALUE mSpecial = rb_define_module("Special");

  if(! functions)
    return mSpecial;            // Nothing to do
  
  for(int i = 0; i < functions->size(); i++)
    functions->value(i)->registerFunction(mSpecial);

  // What about duplicated functions ?
  return mSpecial;
}



//////////////////////////////////////////////////////////////////////

template < double (*func)(double) > class GSLSimpleFunction : 
  public GSLFunction {

  static VALUE rubyFunction(VALUE /*mod*/, VALUE x) {
    return rb_float_new(func(NUM2DBL(x)));
  };

public:
  GSLSimpleFunction(const QString & n) : GSLFunction(n) {
  };

  virtual void registerFunction(VALUE module) {
    rb_define_method(module, name.toLocal8Bit(), 
                     (VALUE (*)(...)) rubyFunction, 1);
  };

};

GSLSimpleFunction<gsl_sf_bessel_J0> bessel_J0("bessel_j0");
GSLSimpleFunction<gsl_sf_bessel_J1> bessel_J1("bessel_j1");
GSLSimpleFunction<gsl_sf_expint_E1> expint_E1("expint_e1");
GSLSimpleFunction<gsl_sf_expint_E2> expint_E2("expint_e2");

//////////////////////////////////////////////////////////////////////

template < double (*func)(int, double) > class GSLIndexedFunction : 
  public GSLFunction {

  static VALUE rubyFunction(VALUE /*mod*/, VALUE n, VALUE x) {
    return rb_float_new(func(NUM2INT(n), NUM2DBL(x)));
  };

public:
  GSLIndexedFunction(const QString & n) : GSLFunction(n) {
  };

  virtual void registerFunction(VALUE module) {
    rb_define_method(module, name.toLocal8Bit(), 
                     (VALUE (*)(...)) rubyFunction, 2);
  };

};

GSLIndexedFunction<gsl_sf_bessel_Jn> bessel_Jn("bessel_jn");
GSLIndexedFunction<gsl_sf_expint_En> expint_En("expint_en");
