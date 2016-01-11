/*
  gslfunction.cc: implementation of the GSLFunction classes
  Copyright 2012, 2014, 2015 by CNRS/AMU

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

#include <functions.hh>

#include <gsl/gsl_sf.h>
#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_const_num.h>

#include <gsl/gsl_randist.h>

QList<GSLFunction *> * GSLFunction::functions = NULL;

void GSLFunction::registerSelf()
{
  if(! functions)
    functions = new QList<GSLFunction *>;
  functions->append(this);
}


GSLFunction::GSLFunction(const QString & n, const QString & d,
                         const QString &u,
                         bool autoreg) : name(n), desc(d), url(u)
{
  rubyName = n.left(n.indexOf('('));
  if(autoreg)
    registerSelf();
}

RUBY_VALUE GSLFunction::registerAllFunctions()
{
  RUBY_VALUE mSpecial = rbw_mMath();

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

QStringList GSLFunction::availableFunctions()
{
  QList<GSLFunction *> sorted = *functions;
  qSort(sorted.begin(), sorted.end(),  &cmpFunctions);

  QStringList rv;
  for(int i = 0; i < sorted.size(); i++)
    rv << sorted[i]->name;
  return rv;
}


QString GSLFunction::functionDocumentation()
{
  if(! functions)
    return QString();
  QList<GSLFunction *> sorted = *functions;
  qSort(sorted.begin(), sorted.end(),  &cmpFunctions);

  QString retval;
  for(int i = 0; i < sorted.size(); i++) {
    QString lnk;
    if(! sorted[i]->url.isEmpty())
      lnk = QString(" (more information [there](%1))").arg(sorted[i]->url);
    retval += QString(" * `%1`: %2%3\n").
      arg(sorted[i]->name).arg(sorted[i]->description()).
      arg(lnk);
  }

  return retval;
}

QString GSLFunction::description() const
{
  return desc;
}



//////////////////////////////////////////////////////////////////////

template < double (*func)(double) > class GSLSimpleFunction : 
  public GSLFunction {

  static RUBY_VALUE rubyFunction(RUBY_VALUE /*mod*/, RUBY_VALUE x) {
    return rbw_float_new(func(rbw_num2dbl(x)));
  };

public:
  GSLSimpleFunction(const QString & n, const QString & d,
                    const QString & url = "") : 
    GSLFunction(n, d, url) {
  };

  virtual void registerFunction(RUBY_VALUE module) {
    rbw_define_method(module, rubyName.toLocal8Bit(), 
                               (RUBY_VALUE (*)()) rubyFunction, 1);
    rbw_define_singleton_method(module, rubyName.toLocal8Bit(), 
                               (RUBY_VALUE (*)()) rubyFunction, 1);
  };

};

static GSLSimpleFunction<gsl_sf_bessel_J0> 
bessel_J0("bessel_j0(x)", "Regular cylindrical Bessel function of "
          "0th order, $$J_0(x)$$", "http://www.gnu.org/software/gsl/manual/html_node/Regular-Cylindrical-Bessel-Functions.html");

static GSLSimpleFunction<gsl_sf_bessel_J1> 
bessel_J1("bessel_j1(x)", "Regular cylindrical Bessel function of "
          "first order, $$J_1(x)$$", "http://www.gnu.org/software/gsl/manual/html_node/Regular-Cylindrical-Bessel-Functions.html");

static GSLSimpleFunction<gsl_sf_bessel_Y0> 
bessel_Y0("bessel_y0(x)", "Irregular cylindrical Bessel function of "
          "0th order, $$Y_0(x)$$", "http://www.gnu.org/software/gsl/manual/html_node/Irregular-Cylindrical-Bessel-Functions.html");

static GSLSimpleFunction<gsl_sf_bessel_Y1> 
bessel_Y1("bessel_y1(x)", "Irregular cylindrical Bessel function of "
          "first order, $$Y_1(x)$$", "http://www.gnu.org/software/gsl/manual/html_node/Irregular-Cylindrical-Bessel-Functions.html");

static GSLSimpleFunction<gsl_sf_expint_E1> 
expint_E1("expint_e1(x)", "Exponential integral $$E_1(x) = "
          "\\int_{x}^{\\infty} \\frac{\\exp -t}{t} \\mathrm{d} t$$");
static GSLSimpleFunction<gsl_sf_expint_E2> 
expint_E2("expint_e2(x)", "Exponential integral $$E_2(x) = "
          "\\int_{x}^{\\infty} \\frac{\\exp -t}{t^2} \\mathrm{d} t$$");

// Lambert W function(s)
static GSLSimpleFunction<gsl_sf_lambert_W0> 
lambert_W0("lambert_W(x)", "Principal branch of the Lambert function "
           "$$W_0(x)$$", "http://www.gnu.org/software/gsl/manual/html_node/Lambert-W-Functions.html");

static GSLSimpleFunction<gsl_sf_lambert_Wm1> 
lambert_Wm1("lambert_Wm1(x)", "Secondary branch of the Lambert function "
            "$$W_{-1}(x)$$", "http://www.gnu.org/software/gsl/manual/html_node/Lambert-W-Functions.html");

// Dawson function
static GSLSimpleFunction<gsl_sf_dawson> 
dawson("dawson(x)", "Dawson integral, "
       "$$\\exp(-x^2)\\int_{0}^{x}\\exp(t^2)\\mathrm{d} t$$");

static GSLSimpleFunction<gsl_sf_debye_1> 
debye1("debye_1(x)", "Debye function of order 1, "
       "$$D_1 = (1/x) \\int_0^x \\mathrm{d}t (t/(e^t - 1))$$");

static GSLSimpleFunction<gsl_sf_debye_2> 
debye2("debye_2(x)", "Debye function of order 2, "
       "$$D_2 = (2/x^2) \\int_0^x \\mathrm{d}t (t^2/(e^t - 1))$$");

static GSLSimpleFunction<gsl_sf_debye_3> 
debye3("debye_3(x)", "Debye function of order 3, "
       "$$D_3 = (3/x^3) \\int_0^x \\mathrm{d}t (t^3/(e^t - 1))$$");

static GSLSimpleFunction<gsl_sf_debye_4> 
debye4("debye_4(x)", "Debye function of order 4, "
       "$$D_4 = (4/x^4) \\int_0^x \\mathrm{d}t (t^4/(e^t - 1))$$");

static GSLSimpleFunction<gsl_sf_debye_5> 
debye5("debye_5(x)", "Debye function of order 5, "
       "$$D_5 = (5/x^5) \\int_0^x \\mathrm{d}t (t^5/(e^t - 1))$$");

static GSLSimpleFunction<gsl_sf_debye_6> 
debye6("debye_6(x)", "Debye function of order 6, "
       "$$D_6 = (6/x^6) \\int_0^x \\mathrm{d}t (t^6/(e^t - 1))$$");

static GSLSimpleFunction<gsl_sf_dilog> 
dilog("dilog(x)", "The dilogarithm, "
      "$$Li_2(x) = - \\Re \\left(\\int_0^x \\mathrm{d}s \\log(1-s) / s\\right)$$");


static GSLSimpleFunction<gsl_ran_landau_pdf> 
landau("landau(x)", "Probability density of the Landau distribution, "
      "$$p(x) = 1/\\pi \\int_0^x \\mathrm{d}t \\exp(-t\\log(t) - xt)\\sin(\\pi t)$$");


static GSLSimpleFunction<gsl_sf_clausen> 
clausen("clausen(x)", "Clausen integral, "
        "$$Cl_2(x) = -\\int_0^x \\mathrm{d}t \\log(2\\sin(t/2))$$");


static GSLSimpleFunction<gsl_sf_fermi_dirac_m1> 
fermi_dirac_m1("fermi_dirac_m1(x)", "Complete Fermi-Dirac integral (index -1), "
               "$$F_{-1}(x) = e^x / (1 + e^x)$$");

static GSLSimpleFunction<gsl_sf_fermi_dirac_0> 
fermi_dirac_0("fermi_dirac_0(x)", "Complete Fermi-Dirac integral (index 0), "
              "$$F_0(x) = \\ln(1 + e^x)$$");

static GSLSimpleFunction<gsl_sf_fermi_dirac_1> 
fermi_dirac_1("fermi_dirac_1(x)", "Complete Fermi-Dirac integral (index 1), "
              "$$F_1(x) = \\int_0^\\infty \\mathrm{d}t (t /(\\exp(t-x)+1))$$");

static GSLSimpleFunction<gsl_sf_fermi_dirac_2> 
fermi_dirac_2("fermi_dirac_2(x)", "Complete Fermi-Dirac integral (index 2), "
              "$$F_2(x) = (1/2) \\int_0^\\infty \\mathrm{d}t (t^2 /(\\exp(t-x)+1))$$");

static GSLSimpleFunction<gsl_sf_fermi_dirac_mhalf> 
fermi_dirac_mhalf("fermi_dirac_mhalf(x)", "Complete Fermi-Dirac integral (index -1/2)",
                  "http://www.gnu.org/software/gsl/manual/html_node/Complete-Fermi_002dDirac-Integrals.html");

static GSLSimpleFunction<gsl_sf_fermi_dirac_half> 
fermi_dirac_half("fermi_dirac_half(x)", "Complete Fermi-Dirac integral (index 1/2)",
                 "http://www.gnu.org/software/gsl/manual/html_node/Complete-Fermi_002dDirac-Integrals.html");

static GSLSimpleFunction<gsl_sf_fermi_dirac_3half> 
fermi_dirac_3half("fermi_dirac_3half(x)", "Complete Fermi-Dirac integral (index 3/2)",
                  "http://www.gnu.org/software/gsl/manual/html_node/Complete-Fermi_002dDirac-Integrals.html");

static GSLSimpleFunction<gsl_sf_psi> 
psi("psi(x)", "Digamma function: $$\\psi(x) = \\Gamma'(x)/\\Gamma(x)$$",
    "http://www.gnu.org/software/gsl/manual/html_node/Digamma-Function.html");

static GSLSimpleFunction<gsl_sf_psi_1> 
psi_1("psi_1(x)", "Trigamma function: $$\\psi^{(1)} = \\frac{\\mathrm d \\Gamma'(x)/\\Gamma(x)}{\\mathrm d x}$$",
      "http://www.gnu.org/software/gsl/manual/html_node/Digamma-Function.html");


static GSLSimpleFunction<gsl_sf_erf> 
gsl_erf("gsl_erf(x)", "Error function $$\\mathrm{erf}(x) = \\frac{2}{\\sqrt{\\pi}}  \\int_0^x \\mathrm{d}t \\exp(-t^2)$$  -- GSL version", "http://www.gnu.org/software/gsl/manual/html_node/Error-Function.html");

static GSLSimpleFunction<gsl_sf_erfc> 
gsl_erfc("gsl_erfc(x)", "Complementary error function $$\\mathrm{erfc}(x) = 1 - \\mathrm{erf}(x)$$", "http://www.gnu.org/software/gsl/manual/html_node/Complementary-Error-Function.html");





//////////////////////////////////////////////////////////////////////
static GSLSimpleFunction<Functions::atanc> 
atanc("atanc(x)", "$$\\frac{\\tan^{-1} x}{x}$$");


static GSLSimpleFunction<Functions::atanhc> 
atanhc("atanhc(x)", "$$\\frac{\\tanh^{-1} x}{x}$$");



//////////////////////////////////////////////////////////////////////

template < double (*func)(int, double) > class GSLIndexedFunction : 
  public GSLFunction {

  static RUBY_VALUE rubyFunction(RUBY_VALUE /*mod*/, RUBY_VALUE n, RUBY_VALUE x) {
    return rbw_float_new(func(rbw_num2int(n), rbw_num2dbl(x)));
  };

public:
  GSLIndexedFunction(const QString & n, const QString & d, const QString & u = "") : 
    GSLFunction(n,d,u) {
  };

  virtual void registerFunction(RUBY_VALUE module) {
    rbw_define_method(module, rubyName.toLocal8Bit(), 
                     (RUBY_VALUE (*)()) rubyFunction, 2);
    rbw_define_singleton_method(module, rubyName.toLocal8Bit(), 
                               (RUBY_VALUE (*)()) rubyFunction, 2);
  };

};

static GSLIndexedFunction<gsl_sf_bessel_Jn> 
bessel_Jn("bessel_jn(x,n)", "Regular cylindrical Bessel function of "
          "n-th order, $$J_n(x)$$", "http://www.gnu.org/software/gsl/manual/html_node/Regular-Cylindrical-Bessel-Functions.html");

static GSLIndexedFunction<gsl_sf_bessel_Yn> 
bessel_Yn("bessel_yn(x,n)", "Irregular cylindrical Bessel function of "
          "n-th order, $$Y_n(x)$$", "http://www.gnu.org/software/gsl/manual/html_node/Irregular-Cylindrical-Bessel-Functions.html");

static GSLIndexedFunction<gsl_sf_expint_En> 
expint_En("expint_en(x,n)", "Exponential integral $$E_n(x) = "
          "\\int_{x}^{\\infty} \\frac{\\exp -t}{t^n} \\mathrm{d} t$$");

static GSLIndexedFunction<gsl_sf_fermi_dirac_int> 
fermi_dirac_n("fermi_dirac_n(x,n)", "Complete Fermi-Dirac integral of index $$n$$, "
              "$$F_n(x) = \\frac{1}{\\Gamma(n+1)} \\int_0^\\infty \\mathrm{d} t \\frac{t^n}{\\exp(t-x) + 1}$$",
              "http://www.gnu.org/software/gsl/manual/html_node/Complete-Fermi_002dDirac-Integrals.html");

static GSLIndexedFunction<gsl_sf_psi_n> 
psi_n("psi_n(x, n)", "Polygamma function: $$\\psi^{(n)} = \\frac{\\mathrm d^n \\Gamma'(x)/\\Gamma(x)}{\\mathrm d x\n}$$",
      "http://www.gnu.org/software/gsl/manual/html_node/Digamma-Function.html");

//////////////////////////////////////////////////////////////////////

/// For functions that have several "modes" of computation (precision)
template < double (*func)(double, gsl_mode_t) > class GSLModalFunction : 
  public GSLFunction {

  static RUBY_VALUE rubyFS(RUBY_VALUE /*mod*/, RUBY_VALUE x) {
    return rbw_float_new(func(rbw_num2dbl(x), GSL_PREC_SINGLE));
  };

  static RUBY_VALUE rubyFD(RUBY_VALUE /*mod*/, RUBY_VALUE x) {
    return rbw_float_new(func(rbw_num2dbl(x), GSL_PREC_DOUBLE));
  };

  static RUBY_VALUE rubyFF(RUBY_VALUE /*mod*/, RUBY_VALUE x) {
    return rbw_float_new(func(rbw_num2dbl(x), GSL_PREC_APPROX));
  };

public:
  GSLModalFunction(const QString & n, const QString & d, const QString & u = "") : 
    GSLFunction(n,d,u) {
  };

  /// @todo Find a way to mark these functions as having three
  /// evaluation modes.
  virtual void registerFunction(RUBY_VALUE module) {
    rbw_define_method(module, rubyName.toLocal8Bit(), 
                     (RUBY_VALUE (*)()) rubyFS, 1);
    rbw_define_singleton_method(module, rubyName.toLocal8Bit(), 
                               (RUBY_VALUE (*)()) rubyFS, 1);

    rbw_define_method(module, (rubyName + "_double").toLocal8Bit(), 
                     (RUBY_VALUE (*)()) rubyFD, 1);
    rbw_define_singleton_method(module, (rubyName + "_double").toLocal8Bit(), 
                               (RUBY_VALUE (*)()) rubyFD, 1);

    rbw_define_method(module, (rubyName + "_fast").toLocal8Bit(), 
                     (RUBY_VALUE (*)()) rubyFF, 1);
    rbw_define_singleton_method(module, (rubyName + "_fast").toLocal8Bit(), 
                               (RUBY_VALUE (*)()) rubyFF, 1);
  };

  virtual QString description() const {
    return QString("%1. Precision to about $$10^{-7}$$. Other variants "
                   "available: `%2_fast` is faster, "
                   "(precision $$5\\times10^{-4}$$) and `%2_double` slower, "
                   "(precision $$2\\times10^{-16}$$). ").arg(desc).
      arg(rubyName);
  };

};


static GSLModalFunction<gsl_sf_airy_Ai> 
airy_ai("airy_ai(x)", "Airy Ai function $$AiryAi(x)$$", "http://www.gnu.org/software/gsl/manual/html_node/Airy-Functions.html");
static GSLModalFunction<gsl_sf_airy_Bi> 
airy_bi("airy_bi(x)", "Airy Bi function $$AiryBi(x)$$", "http://www.gnu.org/software/gsl/manual/html_node/Airy-Functions.html");
static GSLModalFunction<gsl_sf_airy_Ai_deriv> 
airy_aid("airy_ai_deriv(x)", "First derivative of Airy Ai function $$\\mathrm{d}AiryAi(x)/\\mathrm{d}x$$", "http://www.gnu.org/software/gsl/manual/html_node/Derivatives-of-Airy-Functions.html");
static GSLModalFunction<gsl_sf_airy_Bi_deriv> 
airy_bid("airy_bi_deriv(x)", "First derivative of Airy Bi function $$\\mathrm{d}AiryBi(x)/\\mathrm{d}x$$", "http://www.gnu.org/software/gsl/manual/html_node/Derivatives-of-Airy-Functions.html");


//////////////////////////////////////////////////////////////////////

template < double (*func)(double, double) > class GSLDoubleFunction : 
  public GSLFunction {

  static RUBY_VALUE rubyFunction(RUBY_VALUE /*mod*/, RUBY_VALUE x, RUBY_VALUE y) {
    return rbw_float_new(func(rbw_num2dbl(x), rbw_num2dbl(y)));
  };

public:
  GSLDoubleFunction(const QString & n, const QString & d,
                    const QString & url = "") : 
    GSLFunction(n, d, url) {
  };

  virtual void registerFunction(RUBY_VALUE module) {
    rbw_define_method(module, rubyName.toLocal8Bit(), 
                               (RUBY_VALUE (*)()) rubyFunction, 2);
    rbw_define_singleton_method(module, rubyName.toLocal8Bit(), 
                               (RUBY_VALUE (*)()) rubyFunction, 2);
  };

};

static GSLDoubleFunction<gsl_ran_gaussian_pdf> 
gaussian("gaussian(x,sigma)", "Normalized gaussian: "
         "$$p(x,\\sigma) = \\frac{1}{\\sqrt{2 \\pi \\sigma^2}} \\exp (-x^2 / 2\\sigma^2)$$");

static GSLDoubleFunction<gsl_ran_cauchy_pdf> 
lorentzian("lorentzian(x,gamma)", "Normalized gaussian: "
           "$$p(x,\\gamma) = \\frac{1}{ \\gamma \\pi (1 + (x/\\gamma)^2) }$$");

static GSLDoubleFunction<gsl_sf_hyperg_0F1> 
hyperg_0F1("hyperg_0F1(c,x)", "Hypergeometric function $${}_0F_1$$",
           "http://www.gnu.org/software/gsl/manual/html_node/Hypergeometric-Functions.html");

static GSLDoubleFunction<Functions::marcusHushChidseyZeng> 
k_mhc_z("k_mhc_z(lambda, eta)", "Approximation to the Marcus-Hush-Chidsey "
       "integral described in Zeng et al, JEAC 2014, $$k(\\lambda, \\eta) "
       "\\approx \\int_{-\\infty}^{\\infty} "
        "\\exp\\left(\\frac{ - (x - \\lambda + \\eta)^2}"
        "{4\\lambda}\\right) \\times "
        "\\frac{1}{1 + \\exp x}\\,\\mathrm{d}x$$", "http://dx.doi.org/10.1016/j.jelechem.2014.09.038");

//////////////////////////////////////////////////////////////////////

template < double (*func)(double, double, double) > class GSLTripleFunction : 
  public GSLFunction {

  static RUBY_VALUE rubyFunction(RUBY_VALUE /*mod*/, RUBY_VALUE x, RUBY_VALUE y, RUBY_VALUE z) {
    return rbw_float_new(func(rbw_num2dbl(x), rbw_num2dbl(y), rbw_num2dbl(z)));
  };

public:
  GSLTripleFunction(const QString & n, const QString & d,
                    const QString & url = "") : 
    GSLFunction(n, d, url) {
  };

  virtual void registerFunction(RUBY_VALUE module) {
    rbw_define_method(module, rubyName.toLocal8Bit(), 
                               (RUBY_VALUE (*)()) rubyFunction, 3);
    rbw_define_singleton_method(module, rubyName.toLocal8Bit(), 
                               (RUBY_VALUE (*)()) rubyFunction, 3);
  };

};

static GSLTripleFunction<gsl_sf_hyperg_1F1> 
hyperg_1F1("hyperg_1F1(a,b,x)", "Hypergeometric function $${}_1F_1(a,b,x)$$",
           "http://www.gnu.org/software/gsl/manual/html_node/Hypergeometric-Functions.html");

static GSLTripleFunction<gsl_sf_hyperg_U> 
hyperg_U("hyperg_U(a,b,x)", "Hypergeometric function $$U(a,b,x)$$",
         "http://www.gnu.org/software/gsl/manual/html_node/Hypergeometric-Functions.html");

static GSLTripleFunction<Functions::pseudoVoigt> 
pseudo_voigt("pseudo_voigt(x, w, mu)", "Pseudo-Voigt function, defined by: "
             "$$\\frac{1-\\mu}{\\sqrt{2 \\pi w^2}} \\exp (-x^2 / 2w^2) + \\frac{\\mu}{ w \\pi (1 + (x/w)^2) }$$");


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
  RUBY_VALUE v = rbw_float_new(value);
  for(int i = 0; i < names.size(); i++)
    rbw_define_global_const(qPrintable(names[i]), v);
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

QStringList GSLConstant::availableConstants()
{
  if(! constants)
    return QStringList();
  QList<GSLConstant *> sorted = *constants;
  qSort(sorted.begin(), sorted.end(),  &cmpConstants);

  QStringList retval;
  
  for(int i = 0; i < sorted.size(); i++)
    retval += sorted[i]->names;
  return retval;
}

QString GSLConstant::constantsDocumentation()
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
