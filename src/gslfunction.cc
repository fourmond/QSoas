/*
  gslfunction.cc: implementation of the GSLFunction classes
  Copyright 2012, 2014 by CNRS/AMU

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
#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_const_num.h>

QList<GSLFunction *> * GSLFunction::functions = NULL;

void GSLFunction::registerSelf()
{
  if(! functions)
    functions = new QList<GSLFunction *>;
  functions->append(this);
}


GSLFunction::GSLFunction(const QString & n, const QString & d,
                         bool autoreg) : name(n), description(d)
{
  if(autoreg)
    registerSelf();
}

VALUE GSLFunction::registerAllFunctions()
{
  VALUE mSpecial = //rb_define_module("Special");
    rb_mMath;

  if(! functions)
    return mSpecial;            // Nothing to do
  
  for(int i = 0; i < functions->size(); i++)
    functions->value(i)->registerFunction(mSpecial);

  // What about duplicated functions ?
  return mSpecial;
}

static bool cmpFunctions(GSLFunction * a, GSLFunction * b)
{
  return a->name < b->name;
}

QString GSLFunction::availableFunctions()
{
  if(! functions)
    return QString();
  QList<GSLFunction *> sorted = *functions;
  qSort(sorted.begin(), sorted.end(),  &cmpFunctions);

  QString retval;
  for(int i = 0; i < sorted.size(); i++)
    retval += QString(" * `%1`: %2\n").
      arg(sorted[i]->name).arg(sorted[i]->description);

  return retval;
}



//////////////////////////////////////////////////////////////////////

template < double (*func)(double) > class GSLSimpleFunction : 
  public GSLFunction {

  static VALUE rubyFunction(VALUE /*mod*/, VALUE x) {
    return rb_float_new(func(NUM2DBL(x)));
  };

public:
  GSLSimpleFunction(const QString & n, const QString & d) : 
    GSLFunction(n, d) {
  };

  virtual void registerFunction(VALUE module) {
    rb_define_method(module, name.toLocal8Bit(), 
                               (VALUE (*)(...)) rubyFunction, 1);
    rb_define_singleton_method(module, name.toLocal8Bit(), 
                               (VALUE (*)(...)) rubyFunction, 1);
  };

};

static GSLSimpleFunction<gsl_sf_bessel_J0> 
bessel_J0("bessel_j0", "Regular cylindrical Bessel function of "
          "0th order, $$J_0(x)$$");

static GSLSimpleFunction<gsl_sf_bessel_J1> 
bessel_J1("bessel_j1", "Regular cylindrical Bessel function of "
          "first order, $$J_1(x)$$");

static GSLSimpleFunction<gsl_sf_expint_E1> 
expint_E1("expint_e1", "Exponential integral $$E_1(x) = "
          "\\int_{x}^{\\infty} \\frac{\\exp -t}{t} \\mathrm{d} t$$");
static GSLSimpleFunction<gsl_sf_expint_E2> 
expint_E2("expint_e2", "Exponential integral $$E_2(x) = "
          "\\int_{x}^{\\infty} \\frac{\\exp -t}{t^2} \\mathrm{d} t$$");

// Lambert W function(s)
static GSLSimpleFunction<gsl_sf_lambert_W0> 
lambert_W0("lambert_W", "Principal branch of the Lambert function "
           "$$W_0(x)$$");

static GSLSimpleFunction<gsl_sf_lambert_Wm1> 
lambert_Wm1("lambert_Wm1", "Secondary branch of the Lambert function "
           "$$W_{-1}(x)$$");

// Dawson function
static GSLSimpleFunction<gsl_sf_dawson> 
dawson("dawson", "Dawson integral, "
       "$$\\exp(-x^2)\\int_{0}^{x}\\exp(t^2)\\mathrm{d} t$$");

static GSLSimpleFunction<gsl_sf_debye_1> 
debye1("debye_1", "Debye function of order 1, "
       "$$D_1 = (1/x) \\int_0^x \\mathrm{d}t (t/(e^t - 1))$$");

static GSLSimpleFunction<gsl_sf_debye_2> 
debye2("debye_2", "Debye function of order 2, "
       "$$D_2 = (2/x^2) \\int_0^x \\mathrm{d}t (t^2/(e^t - 1))$$");

static GSLSimpleFunction<gsl_sf_debye_3> 
debye3("debye_3", "Debye function of order 3, "
       "$$D_3 = (3/x^3) \\int_0^x \\mathrm{d}t (t^3/(e^t - 1))$$");

static GSLSimpleFunction<gsl_sf_debye_4> 
debye4("debye_4", "Debye function of order 4, "
       "$$D_4 = (4/x^4) \\int_0^x \\mathrm{d}t (t^4/(e^t - 1))$$");

static GSLSimpleFunction<gsl_sf_debye_5> 
debye5("debye_5", "Debye function of order 5, "
       "$$D_5 = (5/x^5) \\int_0^x \\mathrm{d}t (t^5/(e^t - 1))$$");

static GSLSimpleFunction<gsl_sf_debye_6> 
debye6("debye_6", "Debye function of order 6, "
       "$$D_6 = (6/x^6) \\int_0^x \\mathrm{d}t (t^6/(e^t - 1))$$");

static GSLSimpleFunction<gsl_sf_dilog> 
dilog("dilog", "The dilogarithm, "
      "$$Li_2(x) = - \\Re \\left(\\int_0^x \\mathrm{d}s \\log(1-s) / s\\right)$$");





//////////////////////////////////////////////////////////////////////

// Now, the implementation of the various special functions we need
// internally
//
// atan:
//
//             3        5        7        9      10
//    x - 1/3 x  + 1/5 x  - 1/7 x  + 1/9 x  + O(x  )

double qsoas_atanc(double x)
{
  if(x < 0.01 && x > -0.01)
    return 1 - x*x/3 + gsl_pow_4(x)/5 - gsl_pow_6(x)/7 + gsl_pow_8(x)/9;
  else
    return atan(x)/x;
}

static GSLSimpleFunction<qsoas_atanc> 
atanc("atanc", "$$\\frac{\\tan^{-1} x}{x}$$");


//
// atanh:    
//             3        5        7        9      10
//    x + 1/3 x  + 1/5 x  + 1/7 x  + 1/9 x  + O(x  )

double qsoas_atanhc(double x)
{
  if(x < 0.01 && x > -0.01)
    return 1 + x*x/3 + gsl_pow_4(x)/5 + gsl_pow_6(x)/7 + gsl_pow_8(x)/9;
  else
    return atanh(x)/x;
}

static GSLSimpleFunction<qsoas_atanhc> 
atanhc("atanhc", "$$\\frac{\\tanh^{-1} x}{x}$$");



//////////////////////////////////////////////////////////////////////

template < double (*func)(int, double) > class GSLIndexedFunction : 
  public GSLFunction {

  static VALUE rubyFunction(VALUE /*mod*/, VALUE n, VALUE x) {
    return rb_float_new(func(NUM2INT(n), NUM2DBL(x)));
  };

public:
  GSLIndexedFunction(const QString & n, const QString & d) : 
    GSLFunction(n,d) {
  };

  virtual void registerFunction(VALUE module) {
    rb_define_method(module, name.toLocal8Bit(), 
                     (VALUE (*)(...)) rubyFunction, 2);
    rb_define_singleton_method(module, name.toLocal8Bit(), 
                               (VALUE (*)(...)) rubyFunction, 2);
  };

};

GSLIndexedFunction<gsl_sf_bessel_Jn> 
bessel_Jn("bessel_jn", "Regular cylindrical Bessel function of "
          "n-th order, $$J_n(x)$$");
GSLIndexedFunction<gsl_sf_expint_En> 
expint_En("expint_en", "Exponential integral $$E_n(x) = "
          "\\int_{x}^{\\infty} \\frac{\\exp -t}{t^n} \\mathrm{d} t$$");


//////////////////////////////////////////////////////////////////////

/// For functions that have several "modes" of computation (precision)
template < double (*func)(double, gsl_mode_t) > class GSLModalFunction : 
  public GSLFunction {

  static VALUE rubyFS(VALUE /*mod*/, VALUE x) {
    return rb_float_new(func(NUM2DBL(x), GSL_PREC_SINGLE));
  };

  static VALUE rubyFD(VALUE /*mod*/, VALUE x) {
    return rb_float_new(func(NUM2DBL(x), GSL_PREC_DOUBLE));
  };

  static VALUE rubyFF(VALUE /*mod*/, VALUE x) {
    return rb_float_new(func(NUM2DBL(x), GSL_PREC_APPROX));
  };

public:
  GSLModalFunction(const QString & n, const QString & d) : 
    GSLFunction(n,d) {
  };

  /// @todo Find a way to mark these functions as having three
  /// evaluation modes.
  virtual void registerFunction(VALUE module) {
    rb_define_method(module, name.toLocal8Bit(), 
                     (VALUE (*)(...)) rubyFS, 1);
    rb_define_singleton_method(module, name.toLocal8Bit(), 
                               (VALUE (*)(...)) rubyFS, 1);

    rb_define_method(module, (name + "_double").toLocal8Bit(), 
                     (VALUE (*)(...)) rubyFD, 1);
    rb_define_singleton_method(module, (name + "_double").toLocal8Bit(), 
                               (VALUE (*)(...)) rubyFD, 1);

    rb_define_method(module, (name + "_fast").toLocal8Bit(), 
                     (VALUE (*)(...)) rubyFF, 1);
    rb_define_singleton_method(module, (name + "_fast").toLocal8Bit(), 
                               (VALUE (*)(...)) rubyFF, 1);
  };

};


static GSLModalFunction<gsl_sf_airy_Ai> 
airy_ai("airy_ai", "Airy Ai function (three modes) $$AiryAi(x)$$");
static GSLModalFunction<gsl_sf_airy_Bi> 
airy_bi("airy_bi", "Airy Bi function (three modes) $$AiryBi(x)$$");
static GSLModalFunction<gsl_sf_airy_Ai_deriv> 
airy_aid("airy_ai_deriv", "First derivative of Airy Ai function (three modes) $$\\mathrm{d}AiryAi(x)/\\mathrm{d}x$$");
static GSLModalFunction<gsl_sf_airy_Bi_deriv> 
airy_bid("airy_bi_deriv", "First derivative of Airy Bi function (three modes) $$\\mathrm{d}AiryBi(x)/\\mathrm{d}x$$");


//////////////////////////////////////////////////////////////////////

QList<GSLConstant *> * GSLConstant::constants = NULL;

void GSLConstant::registerSelf()
{
  if(! constants)
    constants = new QList<GSLConstant *>;
  constants->append(this);
}


GSLConstant::GSLConstant(const QStringList & ns, const QString & d, 
                         double v, bool autoreg) : 
  names(ns), description(d), value(v)
{
  if(autoreg)
    registerSelf();
}

GSLConstant::GSLConstant(const QString & n, const QString & d,
                         double v, bool autoreg) : 
  description(d), value(v)
{
  names << n;
  if(autoreg)
    registerSelf();
}

void GSLConstant::registerConstant()
{
  VALUE v = rb_float_new(value);
  for(int i = 0; i < names.size(); i++)
    rb_define_global_const(qPrintable(names[i]), v);
}

void GSLConstant::registerAllConstants()
{
  for(int i = 0; i < constants->size(); i++)
    constants->value(i)->registerConstant();
}

static bool cmpConstants(GSLConstant * a, GSLConstant * b)
{
  return a->names.first() < b->names.first();
}

QString GSLConstant::availableConstants()
{
  if(! constants)
    return QString();
  QList<GSLConstant *> sorted = *constants;
  qSort(sorted.begin(), sorted.end(),  &cmpConstants);

  QString retval;
  
  for(int i = 0; i < sorted.size(); i++) {
    QStringList qn;
    const GSLConstant * cst = sorted[i];
    for(int j = 0; j < cst->names.size(); j++)
      qn << QString("`%1`").arg(cst->names[j]);
    retval += QString(" * %1: %2 -- %3\n").
      arg(qn.join(", ")).arg(cst->description).arg(cst->value);
  }

  return retval;
}


// General constants
static GSLConstant f("F", "Faraday's constant, $$F$$", GSL_CONST_MKSA_FARADAY);
static GSLConstant pi(QStringList() << "Pi" << "PI", "$$\\pi$$", M_PI);
static GSLConstant r("R", "Molar gas constant, $$R$$", GSL_CONST_MKSA_MOLAR_GAS);
static GSLConstant c("C", "The speed of light in vacuum, $$c$$", GSL_CONST_MKSA_SPEED_OF_LIGHT);
static GSLConstant Na("Na", "The Avogadro number, $$N_A$$", GSL_CONST_NUM_AVOGADRO);
static GSLConstant Eps0("Eps_0", "The permeability of vacuum, $$\\epsilon_0$$", GSL_CONST_MKSA_VACUUM_PERMEABILITY);
static GSLConstant Mu0("Mu_0", "The permittivity of vacuum, $$\\mu_0$$", GSL_CONST_MKSA_VACUUM_PERMITTIVITY);
static GSLConstant H("H", "The Planck constant, $$h$$", GSL_CONST_MKSA_PLANCKS_CONSTANT_H);
static GSLConstant Hbar("Hbar", "$$\\hbar = h/2\\pi$$", GSL_CONST_MKSA_PLANCKS_CONSTANT_HBAR);
static GSLConstant Kb("Kb", "Boltzmann's constant", GSL_CONST_MKSA_BOLTZMANN);
static GSLConstant sigma("Sigma", "The Stefan-Boltzmann radiation constant", GSL_CONST_MKSA_STEFAN_BOLTZMANN_CONSTANT);

// Nuclear physics constants
static GSLConstant qe("Q_e", "The absolute value of the charge of the electron, $$e$$", GSL_CONST_MKSA_ELECTRON_CHARGE);
static GSLConstant me("M_e", "The mass of the electron, $$m_e$$", GSL_CONST_MKSA_MASS_ELECTRON);
static GSLConstant mp("M_p", "The mass of the proton, $$m_p$$", GSL_CONST_MKSA_MASS_PROTON);
static GSLConstant mn("M_n", "The mass of the neutron, $$m_n$$", GSL_CONST_MKSA_MASS_NEUTRON);
static GSLConstant mmu("M_mu", "The mass of the mu, $$m_\\mu$$", GSL_CONST_MKSA_MASS_MUON);
static GSLConstant alpha("Alpha", "The fine structure constant, $$\\alpha$$", GSL_CONST_NUM_FINE_STRUCTURE);
static GSLConstant mub("Mu_B", "The Bohr Magneton, $$\\mu_B$$", GSL_CONST_MKSA_BOHR_MAGNETON);
static GSLConstant ryd("Ry", "The Rydberg constant, $$Ry$$", GSL_CONST_MKSA_RYDBERG);
