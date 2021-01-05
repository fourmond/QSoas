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

#include <mruby.hh>

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

void GSLFunction::registerAllFunctions(MRuby * mr)
{
  RClass * mMath = mr->defineModule("Math");

  if(! functions)
    return;

  for(int i = 0; i < functions->size(); i++)
    functions->value(i)->registerFunction(mr, mMath);

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

 
  static mrb_value mrFunction(mrb_state * mrb, mrb_value /*self*/) {
    mrb_float flt;
    mrb_get_args(mrb, "f", &flt);
    return mrb_float_value(mrb, func(flt));
  };

public:
  GSLSimpleFunction(const QString & n, const QString & d,
                    const QString & url = "") : 
    GSLFunction(n, d, url) {
  };

  virtual void registerFunction(MRuby * mr, struct RClass * cls) override {
    mr->defineModuleFunction(cls, rubyName.toLocal8Bit(), &mrFunction,
                             MRB_ARGS_REQ(1));
  };

};

static GSLSimpleFunction<gsl_sf_bessel_J0> 
bessel_J0("bessel_j0(x)", "Regular cylindrical Bessel function of "
          "0th order, $$J_0(x)$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#bessel-functions");

static GSLSimpleFunction<gsl_sf_bessel_J1> 
bessel_J1("bessel_j1(x)", "Regular cylindrical Bessel function of "
          "first order, $$J_1(x)$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#bessel-functions");

static GSLSimpleFunction<gsl_sf_bessel_Y0> 
bessel_Y0("bessel_y0(x)", "Irregular cylindrical Bessel function of "
          "0th order, $$Y_0(x)$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#irregular-cylindrical-bessel-functions");

static GSLSimpleFunction<gsl_sf_bessel_Y1> 
bessel_Y1("bessel_y1(x)", "Irregular cylindrical Bessel function of "
          "first order, $$Y_1(x)$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#irregular-cylindrical-bessel-functions");

static GSLSimpleFunction<gsl_sf_expint_E1> 
expint_E1("expint_e1(x)", "Exponential integral $$E_1(x) = "
          "\\int_{x}^{\\infty} \\frac{\\exp -t}{t} \\mathrm{d} t$$");
static GSLSimpleFunction<gsl_sf_expint_E2> 
expint_E2("expint_e2(x)", "Exponential integral $$E_2(x) = "
          "\\int_{x}^{\\infty} \\frac{\\exp -t}{t^2} \\mathrm{d} t$$");

// Lambert W function(s)
static GSLSimpleFunction<gsl_sf_lambert_W0> 
lambert_W0("lambert_W(x)", "Principal branch of the Lambert function "
           "$$W_0(x)$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#lambert-w-functions");

static GSLSimpleFunction<gsl_sf_lambert_Wm1> 
lambert_Wm1("lambert_Wm1(x)", "Secondary branch of the Lambert function "
            "$$W_{-1}(x)$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#lambert-w-functions");

// Dawson function
static GSLSimpleFunction<gsl_sf_dawson> 
dawson("dawson(x)", "Dawson integral, "
       "$$\\exp(-x^2)\\int_{0}^{x}\\exp(t^2)\\mathrm{d} t$$");

static GSLSimpleFunction<gsl_sf_debye_1> 
debye1("debye_1(x)", "Debye function of order 1, "
       "$$D_1 = (1/x) \\int_0^x \\mathrm{d}t (t/(e^t - 1))$$",
       "https://www.gnu.org/software/gsl/doc/html/specfunc.html#debye-functions");

static GSLSimpleFunction<gsl_sf_debye_2> 
debye2("debye_2(x)", "Debye function of order 2, "
       "$$D_2 = (2/x^2) \\int_0^x \\mathrm{d}t (t^2/(e^t - 1))$$",
       "https://www.gnu.org/software/gsl/doc/html/specfunc.html#debye-functions");

static GSLSimpleFunction<gsl_sf_debye_3> 
debye3("debye_3(x)", "Debye function of order 3, "
       "$$D_3 = (3/x^3) \\int_0^x \\mathrm{d}t (t^3/(e^t - 1))$$",
       "https://www.gnu.org/software/gsl/doc/html/specfunc.html#debye-functions");

static GSLSimpleFunction<gsl_sf_debye_4> 
debye4("debye_4(x)", "Debye function of order 4, "
       "$$D_4 = (4/x^4) \\int_0^x \\mathrm{d}t (t^4/(e^t - 1))$$",
       "https://www.gnu.org/software/gsl/doc/html/specfunc.html#debye-functions");

static GSLSimpleFunction<gsl_sf_debye_5> 
debye5("debye_5(x)", "Debye function of order 5, "
       "$$D_5 = (5/x^5) \\int_0^x \\mathrm{d}t (t^5/(e^t - 1))$$",
       "https://www.gnu.org/software/gsl/doc/html/specfunc.html#debye-functions");

static GSLSimpleFunction<gsl_sf_debye_6> 
debye6("debye_6(x)", "Debye function of order 6, "
       "$$D_6 = (6/x^6) \\int_0^x \\mathrm{d}t (t^6/(e^t - 1))$$",
       "https://www.gnu.org/software/gsl/doc/html/specfunc.html#debye-functions");

static GSLSimpleFunction<gsl_sf_dilog> 
dilog("dilog(x)", "The dilogarithm, "
      "$$Li_2(x) = - \\Re \\left(\\int_0^x \\mathrm{d}s \\log(1-s) / s\\right)$$",
      "https://www.gnu.org/software/gsl/doc/html/specfunc.html#dilogarithm");


static GSLSimpleFunction<gsl_ran_landau_pdf> 
landau("landau(x)", "Probability density of the Landau distribution, "
       "$$p(x) = 1/\\pi \\int_0^x \\mathrm{d}t \\exp(-t\\log(t) - xt)\\sin(\\pi t)$$",
       "https://www.gnu.org/software/gsl/doc/html/randist.html#the-landau-distribution");


static GSLSimpleFunction<gsl_sf_clausen> 
clausen("clausen(x)", "Clausen integral, "
        "$$Cl_2(x) = -\\int_0^x \\mathrm{d}t \\log(2\\sin(t/2))$$",
        "https://www.gnu.org/software/gsl/doc/html/specfunc.html#clausen-functions");


static GSLSimpleFunction<gsl_sf_fermi_dirac_m1> 
fermi_dirac_m1("fermi_dirac_m1(x)", "Complete Fermi-Dirac integral (index -1), "
               "$$F_{-1}(x) = e^x / (1 + e^x)$$",
               "https://www.gnu.org/software/gsl/doc/html/specfunc.html#complete-fermi-dirac-integrals");

static GSLSimpleFunction<gsl_sf_fermi_dirac_0> 
fermi_dirac_0("fermi_dirac_0(x)", "Complete Fermi-Dirac integral (index 0), "
              "$$F_0(x) = \\ln(1 + e^x)$$",
              "https://www.gnu.org/software/gsl/doc/html/specfunc.html#complete-fermi-dirac-integrals");

static GSLSimpleFunction<gsl_sf_fermi_dirac_1> 
fermi_dirac_1("fermi_dirac_1(x)", "Complete Fermi-Dirac integral (index 1), "
              "$$F_1(x) = \\int_0^\\infty \\mathrm{d}t (t /(\\exp(t-x)+1))$$",
              "https://www.gnu.org/software/gsl/doc/html/specfunc.html#complete-fermi-dirac-integrals");

static GSLSimpleFunction<gsl_sf_fermi_dirac_2> 
fermi_dirac_2("fermi_dirac_2(x)", "Complete Fermi-Dirac integral (index 2), "
              "$$F_2(x) = (1/2) \\int_0^\\infty \\mathrm{d}t (t^2 /(\\exp(t-x)+1))$$",
              "https://www.gnu.org/software/gsl/doc/html/specfunc.html#complete-fermi-dirac-integrals");

static GSLSimpleFunction<gsl_sf_fermi_dirac_mhalf> 
fermi_dirac_mhalf("fermi_dirac_mhalf(x)", "Complete Fermi-Dirac integral (index -1/2)",
                  "https://www.gnu.org/software/gsl/doc/html/specfunc.html#complete-fermi-dirac-integrals");

static GSLSimpleFunction<gsl_sf_fermi_dirac_half> 
fermi_dirac_half("fermi_dirac_half(x)", "Complete Fermi-Dirac integral (index 1/2)",
                 "https://www.gnu.org/software/gsl/doc/html/specfunc.html#complete-fermi-dirac-integrals");

static GSLSimpleFunction<gsl_sf_fermi_dirac_3half> 
fermi_dirac_3half("fermi_dirac_3half(x)", "Complete Fermi-Dirac integral (index 3/2)",
                  "https://www.gnu.org/software/gsl/doc/html/specfunc.html#complete-fermi-dirac-integrals");

static GSLSimpleFunction<gsl_sf_psi> 
psi("psi(x)", "Digamma function: $$\\psi(x) = \\Gamma'(x)/\\Gamma(x)$$",
    "https://www.gnu.org/software/gsl/doc/html/specfunc.html#digamma-function");

static GSLSimpleFunction<gsl_sf_psi_1> 
psi_1("psi_1(x)", "Trigamma function: $$\\psi^{(1)} = \\frac{\\mathrm d \\Gamma'(x)/\\Gamma(x)}{\\mathrm d x}$$",
      "https://www.gnu.org/software/gsl/doc/html/specfunc.html#trigamma-function");


static GSLSimpleFunction<gsl_sf_erf> 
gsl_erf("gsl_erf(x)", "Error function $$\\mathrm{erf}(x) = \\frac{2}{\\sqrt{\\pi}}  \\int_0^x \\mathrm{d}t \\exp(-t^2)$$  -- GSL version", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#error-functions");

static GSLSimpleFunction<gsl_sf_erfc> 
gsl_erfc("gsl_erfc(x)", "Complementary error function $$\\mathrm{erfc}(x) = 1 - \\mathrm{erf}(x)$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#error-functions");

static GSLSimpleFunction<gsl_sf_log_erfc> 
gsl_log_erfc("ln_erfc(x)", "Logarithm of the complementary error function $$\\log(\\mathop{erfc}(x))$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#error-functions");

static GSLSimpleFunction<gsl_sf_gamma> 
gsl_gamma("gamma(x)", "The Gauss gamma function $$\\Gamma(x) = \\int_0^{\\infty} dt t^{x-1} \\exp(-t)$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#gamma-and-beta-functions");

static GSLSimpleFunction<gsl_sf_lngamma> 
gsl_ln_gamma("ln_gamma(x)", "The logarithm of the gamma function $$\\log (\\Gamma(x))$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#gamma-and-beta-functions");





//////////////////////////////////////////////////////////////////////
static GSLSimpleFunction<Functions::atanc> 
atanc("atanc(x)", "$$\\frac{\\tan^{-1} x}{x}$$");


static GSLSimpleFunction<Functions::atanhc> 
atanhc("atanhc(x)", "$$\\frac{\\tanh^{-1} x}{x}$$");


//////////////////////////////////////////////////////////////////////

// Import of a few general functions

// static GSLSimpleFunction<::fabs> 
// abs_func("abs(x)", "$$|x|$$");

static GSLSimpleFunction<::log1p> 
log1p_func("log1p(x)", "$$\\ln (1 + x)$$, but accurate for $$x$$ close to 0");



//////////////////////////////////////////////////////////////////////

template < double (*realf)(double),
           std::complex<double> (*complexf)(const std::complex<double> &)
           > class GSLDualFunction : 
  public GSLFunction {

 
  static mrb_value mrFunction(mrb_state * mrb, mrb_value /*self*/) {
    mrb_value v;
    MRuby * mr = MRuby::ruby();
    mrb_get_args(mrb, "o", &v);
    if(mr->isComplex(v))
      return mr->newComplex(complexf(mr->complexValue_up(v)));
    
    mrb_float flt;
    mrb_get_args(mrb, "f", &flt);
    return mrb_float_value(mrb, realf(flt));
  };

public:
  GSLDualFunction(const QString & n, const QString & d,
                  const QString & url = "") : 
    GSLFunction(n, d, url) {
  };

  virtual void registerFunction(MRuby * mr, struct RClass * cls) override {
    mr->defineModuleFunction(cls, rubyName.toLocal8Bit(), &mrFunction,
                             MRB_ARGS_REQ(1));
  };

};

static GSLDualFunction<::exp, std::exp> 
d_exp_func("exp(x)", "$$\\exp x$$, works on complex numbers too");

static GSLDualFunction<::log, std::log> 
d_log_func("log(x)", "$$\\log x$$, works on complex numbers too");

//////////////////////////////////////////////////////////////////////

template < double (*realf)(double),
           double (*complexf)(const std::complex<double> &)
           > class GSLDDualFunction : 
  public GSLFunction {

 
  static mrb_value mrFunction(mrb_state * mrb, mrb_value /*self*/) {
    mrb_value v;
    MRuby * mr = MRuby::ruby();
    mrb_get_args(mrb, "o", &v);
    double rv;
    if(mr->isComplex(v))
      rv = complexf(mr->complexValue_up(v));
    else {
      mrb_float flt;
      mrb_get_args(mrb, "f", &flt);
      rv = realf(flt);
    }
    return mrb_float_value(mrb, rv);
  };

public:
  GSLDDualFunction(const QString & n, const QString & d,
                  const QString & url = "") : 
    GSLFunction(n, d, url) {
  };

  virtual void registerFunction(MRuby * mr, struct RClass * cls) override {
    mr->defineModuleFunction(cls, rubyName.toLocal8Bit(), &mrFunction,
                             MRB_ARGS_REQ(1));
  };

};

static GSLDDualFunction<::fabs, std::abs> 
d_abs_func("abs(x)", "$$\\left| x\\right|$$, works on complex numbers too");

static double rarg(double v)
{
  return 0;
}

static GSLDDualFunction<::rarg, std::arg> 
d_arg_func("arg(x)", "$$\\arg  x$$, the argument of the complex number");



//////////////////////////////////////////////////////////////////////

template < double (*func)(int, double) > class GSLIndexedFunction : 
  public GSLFunction {

  static mrb_value mrFunction(mrb_state * mrb, mrb_value /*self*/) {
    mrb_float flt;
    mrb_int i;
    mrb_get_args(mrb, "fi", &flt, &i);
    return mrb_float_value(mrb, func(i, flt));
  };


public:
  GSLIndexedFunction(const QString & n, const QString & d, const QString & u = "") : 
    GSLFunction(n,d,u) {
  };

  virtual void registerFunction(MRuby * mr, struct RClass * cls) override {
    mr->defineModuleFunction(cls, rubyName.toLocal8Bit(), &mrFunction,
                             MRB_ARGS_REQ(2));
  };

};

static GSLIndexedFunction<gsl_sf_bessel_Jn> 
bessel_Jn("bessel_jn(x,n)", "Regular cylindrical Bessel function of "
          "n-th order, $$J_n(x)$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#bessel-functions");

static GSLIndexedFunction<gsl_sf_bessel_Yn> 
bessel_Yn("bessel_yn(x,n)", "Irregular cylindrical Bessel function of "
          "n-th order, $$Y_n(x)$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#irregular-cylindrical-bessel-functions");

static GSLIndexedFunction<gsl_sf_expint_En> 
expint_En("expint_en(x,n)", "Exponential integral $$E_n(x) = "
          "\\int_{x}^{\\infty} \\frac{\\exp -t}{t^n} \\mathrm{d} t$$");

static GSLIndexedFunction<gsl_sf_fermi_dirac_int> 
fermi_dirac_n("fermi_dirac_n(x,n)", "Complete Fermi-Dirac integral of index $$n$$, "
              "$$F_n(x) = \\frac{1}{\\Gamma(n+1)} \\int_0^\\infty \\mathrm{d} t \\frac{t^n}{\\exp(t-x) + 1}$$",
              "https://www.gnu.org/software/gsl/doc/html/specfunc.html#complete-fermi-dirac-integrals");

static GSLIndexedFunction<gsl_sf_psi_n> 
psi_n("psi_n(x, n)", "Polygamma function: $$\\psi^{(n)} = \\frac{\\mathrm d^n \\Gamma'(x)/\\Gamma(x)}{\\mathrm d x\n}$$",
      "https://www.gnu.org/software/gsl/doc/html/specfunc.html#polygamma-function");

//////////////////////////////////////////////////////////////////////

/// For functions that have several "modes" of computation (precision)
template < double (*func)(double, gsl_mode_t) > class GSLModalFunction : 
  public GSLFunction {

  static mrb_value mrFS(mrb_state * mrb, mrb_value /*self*/) {
    mrb_float flt;
    mrb_get_args(mrb, "f", &flt);
    return mrb_float_value(mrb, func(flt, GSL_PREC_SINGLE));
  };

  static mrb_value mrFD(mrb_state * mrb, mrb_value /*self*/) {
    mrb_float flt;
    mrb_get_args(mrb, "f", &flt);
    return mrb_float_value(mrb, func(flt, GSL_PREC_DOUBLE));
  };

  static mrb_value mrFF(mrb_state * mrb, mrb_value /*self*/) {
    mrb_float flt;
    mrb_get_args(mrb, "f", &flt);
    return mrb_float_value(mrb, func(flt, GSL_PREC_APPROX));
  };


public:
  GSLModalFunction(const QString & n, const QString & d, const QString & u = "") : 
    GSLFunction(n,d,u) {
  };

  virtual void registerFunction(MRuby * mr, struct RClass * cls) override {
    mr->defineModuleFunction(cls, rubyName.toLocal8Bit(), &mrFS,
                             MRB_ARGS_REQ(1));

    mr->defineModuleFunction(cls, (rubyName + "_double").toLocal8Bit(), &mrFD,
                             MRB_ARGS_REQ(1));

    mr->defineModuleFunction(cls, (rubyName + "_fast").toLocal8Bit(), &mrFF,
                             MRB_ARGS_REQ(1));
  }

  virtual QString description() const override {
    return QString("%1. Precision to about $$10^{-7}$$. Other variants "
                   "available: `%2_fast` is faster, "
                   "(precision $$5\\times10^{-4}$$) and `%2_double` slower, "
                   "(precision $$2\\times10^{-16}$$). ").arg(desc).
      arg(rubyName);
  };

};


static GSLModalFunction<gsl_sf_airy_Ai> 
airy_ai("airy_ai(x)", "Airy Ai function $$AiryAi(x)$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#airy-functions-and-derivatives");
static GSLModalFunction<gsl_sf_airy_Bi> 
airy_bi("airy_bi(x)", "Airy Bi function $$AiryBi(x)$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#airy-functions-and-derivatives");
static GSLModalFunction<gsl_sf_airy_Ai_deriv> 
airy_aid("airy_ai_deriv(x)", "First derivative of Airy Ai function $$\\mathrm{d}AiryAi(x)/\\mathrm{d}x$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#airy-functions-and-derivatives");
static GSLModalFunction<gsl_sf_airy_Bi_deriv> 
airy_bid("airy_bi_deriv(x)", "First derivative of Airy Bi function $$\\mathrm{d}AiryBi(x)/\\mathrm{d}x$$", "https://www.gnu.org/software/gsl/doc/html/specfunc.html#airy-functions-and-derivatives");


//////////////////////////////////////////////////////////////////////

template < double (*func)(double, double) > class GSLDoubleFunction : 
  public GSLFunction {

  static mrb_value mrFunction(mrb_state * mrb, mrb_value /*self*/) {
    mrb_float flt1, flt2;
    mrb_get_args(mrb, "ff", &flt1, &flt2);
    return mrb_float_value(mrb, func(flt1, flt2));
  };

public:
  GSLDoubleFunction(const QString & n, const QString & d,
                    const QString & url = "") : 
    GSLFunction(n, d, url) {
  };

  virtual void registerFunction(MRuby * mr, struct RClass * cls) override {
    mr->defineModuleFunction(cls, rubyName.toLocal8Bit(), &mrFunction,
                             MRB_ARGS_REQ(2));
  };

};

static GSLDoubleFunction<gsl_ran_gaussian_pdf> 
gaussian("gaussian(x,sigma)", "Normalized gaussian: "
         "$$p(x,\\sigma) = \\frac{1}{\\sqrt{2 \\pi \\sigma^2}} \\exp (-x^2 / 2\\sigma^2)$$");

static GSLDoubleFunction<gsl_ran_cauchy_pdf> 
lorentzian("lorentzian(x,gamma)", "Normalized lorentzian: "
           "$$p(x,\\gamma) = \\frac{1}{ \\gamma \\pi (1 + (x/\\gamma)^2) }$$");

static GSLDoubleFunction<gsl_sf_hyperg_0F1> 
hyperg_0F1("hyperg_0F1(c,x)", "Hypergeometric function $${}_0F_1$$",
           "https://www.gnu.org/software/gsl/doc/html/specfunc.html#hypergeometric-functions");

static GSLDoubleFunction<Functions::marcusHushChidseyZeng> 
k_mhc_z("k_mhc_z(lambda, eta)", "Approximation to the Marcus-Hush-Chidsey "
       "integral described in Zeng et al, JEAC 2014, $$k(\\lambda, \\eta) "
       "\\approx \\int_{-\\infty}^{\\infty} "
        "\\exp\\left(\\frac{ - (x - \\lambda + \\eta)^2}"
        "{4\\lambda}\\right) \\times "
        "\\frac{1}{1 + \\exp x}\\,\\mathrm{d}x$$", "http://dx.doi.org/10.1016/j.jelechem.2014.09.038");

static GSLDoubleFunction<Functions::marcusHushChidseyZeng> 
k_mhc_n("k_mhc_n(lambda, eta)", "Approximation to the Marcus-Hush-Chidsey "
        "integral described in Nahir, JEAC 2002, $$k(\\lambda, \\eta) "
        "\\approx \\int_{-\\infty}^{\\infty} "
        "\\exp\\left(\\frac{ - (x - \\lambda + \\eta)^2}"
        "{4\\lambda}\\right) \\times "
        "\\frac{1}{1 + \\exp x}\\,\\mathrm{d}x$$", "http://dx.doi.org/10.1016/S0022-0728(01)00688-X");

static GSLDoubleFunction<Functions::marcusHushChidsey> 
k_mhc("k_mhc(lambda, eta)", "Marcus-Hush-Chidsey "
      "integral $$k(\\lambda, \\eta) "
      "= \\int_{-\\infty}^{\\infty} "
      "\\exp\\left(\\frac{ - (x - \\lambda + \\eta)^2}"
      "{4\\lambda}\\right) \\times "
      "\\frac{1}{1 + \\exp x}\\,\\mathrm{d}x$$. Single precision, computed using the fast trapezoid method.",
      "http://dx.doi.org/10.1016/j.jelechem.2020.114762"); 

static GSLDoubleFunction<Functions::marcusHushChidseyDouble> 
k_mhcd("k_mhc_double(lambda, eta)", "Marcus-Hush-Chidsey "
       "integral $$k(\\lambda, \\eta) "
       "= \\int_{-\\infty}^{\\infty} "
       "\\exp\\left(\\frac{ - (x - \\lambda + \\eta)^2}"
       "{4\\lambda}\\right) \\times "
       "\\frac{1}{1 + \\exp x}\\,\\mathrm{d}x$$. Double precision, computed using the series by Bieniasz, JEAC 2012.", "http://dx.doi.org/10.1016/j.jelechem.2012.08.015");



// Incomplete gamma functions

static GSLDoubleFunction<gsl_sf_gamma_inc> 
gamma_inc("gamma_inc(a,x)", "Incomplete gamma function $$\\Gamma(a,x) = \\int_x^\\infty dt t^{a-1} \\exp(-t)$$",
           "https://www.gnu.org/software/gsl/doc/html/specfunc.html#incomplete-gamma-functions");

static GSLDoubleFunction<gsl_sf_gamma_inc_Q> 
gamma_inc_q("gamma_inc_q(a,x)", "Normalized incomplete gamma function $$\\Gamma_Q(a,x) = \\frac{1}{\\Gamma(a)}\\int_x^\\infty dt t^{a-1} \\exp(-t)$$",
           "https://www.gnu.org/software/gsl/doc/html/specfunc.html#incomplete-gamma-functions");

static GSLDoubleFunction<gsl_sf_gamma_inc_P> 
gamma_inc_p("gamma_inc_p(a,x)", "Complementary normalized incomplete gamma function $$\\Gamma_P(a,x) = 1 - \\Gamma_Q(a,x) = 1 - \\frac{1}{\\Gamma(a)}\\int_x^\\infty dt t^{a-1} \\exp(-t)$$",
           "https://www.gnu.org/software/gsl/doc/html/specfunc.html#incomplete-gamma-functions");


//////////////////////////////////////////////////////////////////////

template < double (*func)(double, double, double) > class GSLTripleFunction : 
  public GSLFunction {

  static mrb_value mrFunction(mrb_state * mrb, mrb_value /*self*/) {
    mrb_float flt1, flt2, flt3;
    mrb_get_args(mrb, "fff", &flt1, &flt2, &flt3);
    return mrb_float_value(mrb, func(flt1, flt2, flt3));
  };

public:
  GSLTripleFunction(const QString & n, const QString & d,
                    const QString & url = "") : 
    GSLFunction(n, d, url) {
  };

  virtual void registerFunction(MRuby * mr, struct RClass * cls) override {
    mr->defineModuleFunction(cls, rubyName.toLocal8Bit(), &mrFunction,
                             MRB_ARGS_REQ(3));
  };

};

static GSLTripleFunction<gsl_sf_hyperg_1F1> 
hyperg_1F1("hyperg_1F1(a,b,x)", "Hypergeometric function $${}_1F_1(a,b,x)$$",
           "https://www.gnu.org/software/gsl/doc/html/specfunc.html#hypergeometric-functions");

static GSLTripleFunction<gsl_sf_hyperg_U> 
hyperg_U("hyperg_U(a,b,x)", "Hypergeometric function $$U(a,b,x)$$",
         "https://www.gnu.org/software/gsl/doc/html/specfunc.html#hypergeometric-functions");

static GSLTripleFunction<Functions::pseudoVoigt> 
pseudo_voigt("pseudo_voigt(x, w, mu)", "Pseudo-Voigt function, defined by: "
             "$$\\frac{1-\\mu}{\\sqrt{2 \\pi w^2}} \\exp (-x^2 / 2w^2) + \\frac{\\mu}{ w \\pi (1 + (x/w)^2) }$$");

static GSLTripleFunction<Functions::trumpetBV> 
trumpet_bv("trumpet_bv(m, alpha, prec)", "Position of an oxidative adsorbed 1-electron peak. $$m$$ is the coefficient defined by Laviron, the value is returned in units of $$RT/F$$");


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
  // RUBY_VALUE v = rbw_float_new(value);
  // for(int i = 0; i < names.size(); i++)
  //   rbw_define_global_const(qPrintable(names[i]), v);
}

void GSLConstant::registerConstant(MRuby * mr)
{
  mrb_value v = mrb_float_value(mr->mrb, value);
  for(int i = 0; i < names.size(); i++)
    mr->defineGlobalConstant(qPrintable(names[i]), v);
}

void GSLConstant::registerAllConstants()
{
  for(int i = 0; i < constants->size(); i++)
    constants->value(i)->registerConstant();
}

void GSLConstant::registerAllConstants(MRuby * mr)
{
  for(int i = 0; i < constants->size(); i++)
    constants->value(i)->registerConstant(mr);
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
static GSLConstant Eps0("Eps_0", "The permittivity of vacuum, $$\\epsilon_0$$", GSL_CONST_MKSA_VACUUM_PERMITTIVITY);
static GSLConstant Mu0("Mu_0", "The permeability of vacuum, $$\\mu_0$$", GSL_CONST_MKSA_VACUUM_PERMEABILITY);
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
