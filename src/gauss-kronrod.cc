/*
  gauss-kronrod.cc: implementation of the Gauss-Kronrod adaptive integrators
  Copyright 2018 by CNRS/AMU

  Some parts of this code, namely the xgk, wg and wgk come from the GSL 
  and are
  Copyright (C) 1996, 1997, 1998, 1999, 2000, 2007 Brian Gough


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
#include <multiintegrator.hh>

#include <vector.hh>
#include <dataset.hh>

#include <possessive-containers.hh>

/// Gauss Kronrod integrators. This code is greatly inspired from the
/// GSL code. It is much simplified, though.
class GaussKronrodMultiIntegrator : public MultiIntegrator {
protected:

  /// The size of the abscissae. Total number of points is 2*size - 1.
  /// Number of Gauss points: size - 1
  int size;
  
  /// The abscissae of the points. Size is @a size
  /// The odd elements are also the gauss positions.
  const double * abscissae;

  /// The kronrod weights. Size @a size
  const double * kWeights;

  /// The gauss weights. Size @a (size+1)/2
  const double * gWeights;

  /// Performs a Gauss-Kronrod quadrature of the given interval, and
  /// stores the results and the error estimate in @a target and @a
  /// errors, resp.
  void gKQuadrature(double a, double b, gsl_vector * target,
                    gsl_vector * errors) {
    // the values (in the order of increasing x values)
    gsl_vector * values[2*size - 1];
    double dx = 0.5 * (b-a);
    double xa =  0.5 * (a+b);

    for(int i = 0; i < size; i++) {
      double x1 = xa + dx * abscissae[i];
      values[i] = functionForValue(x1);
      double x2 = xa - dx * abscissae[i];
      values[2*size - 2 - i] = functionForValue(x2); // sometimes the
                                                     // same as x1 (but not recomputed)
    }

    for(size_t j = 0; j < target->size; j++) {
      double gauss = 0;
      double kronrod = 0;
      for(int i = 0; i < 2*size-1; i++) {
        int idx = i < size ? i : 2*size - 2 - i;
        kronrod += kWeights[idx] * gsl_vector_get(values[i], j);
        if(i % 2 == 1)
          gauss += gWeights[idx/2] * gsl_vector_get(values[i], j);
      }
      /// @todo This is greatly simplified with respect to the GSL
      /// code. See more about that in the GSL source
      gsl_vector_set(errors, j, fabs((kronrod - gauss) * dx));
      gsl_vector_set(target, j, kronrod * dx);
    }
    
  };
protected:
  /// Represents an integration segment
  class Segment {
  public:
    gsl_vector * sum;
    double left;
    double right;
    gsl_vector * error;

    Segment(double l, double r, size_t s) {
      left = l;
      right = r;
      sum = gsl_vector_alloc(s);
      error = gsl_vector_alloc(s);
    };

    ~Segment() {
      if(sum)
        gsl_vector_free(sum);
      if(error)
        gsl_vector_free(error);
    };

    double relativeError() const {
      double min, max;
      gsl_vector_minmax(sum, &min, &max);
      /// Below this threshold, we don't look at relative error
      /// anymore.
      double thresh = 1e-3 * std::max(fabs(min), fabs(max));

      double e = 0;
      for(size_t i = 0; i < sum->size; i++) {
        double v = fabs(gsl_vector_get(sum, i));
        if(v < thresh)
          v = thresh;
        e += gsl_vector_get(error, i)/v;
      }
      return e/sum->size;
    };
  };

  void doSegment(Segment * seg) {
    gKQuadrature(seg->left,seg->right, seg->sum, seg->error);
  }

public:

  GaussKronrodMultiIntegrator(Function fnc, int dim, double rel, double abs, int maxc, int gsSize, const double * ab, const double * kw, const double * gw) :
    MultiIntegrator(fnc, dim, rel, abs, maxc),
    size(gsSize), abscissae(ab), kWeights(kw), gWeights(gw)
  {
  }

  virtual double integrate(gsl_vector * res, double a, double b) override {
    PossessiveList<Segment> segs;
    segs << new Segment(a, b, res->size);
    doSegment(segs[0]);

    int nbit = 0;
    double error = 0;
    do {
      error = 0;
      double em = -1;
      int idx = -1;
      for(int i = 0; i < segs.size(); i++) {
        const Segment & s = *(segs[i]);
        double e = s.relativeError();
        double re = (s.right - s.left) * e;
        if(re > em) {
          idx = i;
          em = re;
        }
        error += e;
      }

      if(error < relativePrec)
        break;
      

      // bisect:
      Segment * sl = segs[idx];
      double l = sl->left;
      double r = sl->right;
      sl->right = (l+r)*0.5;
      doSegment(sl);
      Segment * s = new Segment((l+r)*0.5, r, res->size);
      segs << s;
      doSegment(s);
      nbit += 1;
    } while(nbit < 100);         // yep

    gsl_vector_set_zero(res);
    for(int i = 0; i < segs.size(); i++)
      gsl_blas_daxpy(1, segs[i]->sum, res);
    
    return error;
  };

public:
  MultiIntegrator * dup() const override {
    return new GaussKronrodMultiIntegrator(function, dimension, relativePrec,
                                           absolutePrec, maxfuncalls, size,
                                           abscissae, kWeights, gWeights);
  };
};


//////////////////////////////////////////////////////////////////////

namespace __gk15 {

  /* Gauss quadrature weights and kronrod quadrature abscissae and
     weights as evaluated with 80 decimal digit arithmetic by
     L. W. Fullerton, Bell Labs, Nov. 1981. */
  
  static const double xgk[8] =    /* abscissae of the 15-point kronrod rule */
    {
      0.991455371120812639206854697526329,
      0.949107912342758524526189684047851,
      0.864864423359769072789712788640926,
      0.741531185599394439863864773280788,
      0.586087235467691130294144838258730,
      0.405845151377397166906606412076961,
      0.207784955007898467600689403773245,
      0.000000000000000000000000000000000
    };

  /* xgk[1], xgk[3], ... abscissae of the 7-point gauss rule. 
     xgk[0], xgk[2], ... abscissae to optimally extend the 7-point gauss rule */

  static const double wg[4] =     /* weights of the 7-point gauss rule */
    {
      0.129484966168869693270611432679082,
      0.279705391489276667901467771423780,
      0.381830050505118944950369775488975,
      0.417959183673469387755102040816327
    };

  static const double wgk[8] =    /* weights of the 15-point kronrod rule */
    {
      0.022935322010529224963732008058970,
      0.063092092629978553290700663189204,
      0.104790010322250183839876322541518,
      0.140653259715525918745189590510238,
      0.169004726639267902826583426598550,
      0.190350578064785409913256402421014,
      0.204432940075298892414161999234649,
      0.209482141084727828012999174891714
    };

  static MultiIntegrator::MultiIntegratorFactory
  gk("gk15",
     "15 points adaptive Gauss-Kronrod",
     [](MultiIntegrator::Function fnc, int dim, double rel, double abs, int maxc) -> MultiIntegrator * {
       return new GaussKronrodMultiIntegrator(fnc, dim, rel, abs, maxc,
                                              8, xgk, wgk, wg);
     }
     );

}

//////////////////////////////////////////////////////////////////////

namespace __gk21 {

  /* Gauss quadrature weights and kronrod quadrature abscissae and
     weights as evaluated with 80 decimal digit arithmetic by
     L. W. Fullerton, Bell Labs, Nov. 1981. */

  static const double xgk[11] =   /* abscissae of the 21-point kronrod rule */
    {
      0.995657163025808080735527280689003,
      0.973906528517171720077964012084452,
      0.930157491355708226001207180059508,
      0.865063366688984510732096688423493,
      0.780817726586416897063717578345042,
      0.679409568299024406234327365114874,
      0.562757134668604683339000099272694,
      0.433395394129247190799265943165784,
      0.294392862701460198131126603103866,
      0.148874338981631210884826001129720,
      0.000000000000000000000000000000000
    };

  /* xgk[1], xgk[3], ... abscissae of the 10-point gauss rule. 
     xgk[0], xgk[2], ... abscissae to optimally extend the 10-point gauss rule */

  static const double wg[5] =     /* weights of the 10-point gauss rule */
    {
      0.066671344308688137593568809893332,
      0.149451349150580593145776339657697,
      0.219086362515982043995534934228163,
      0.269266719309996355091226921569469,
      0.295524224714752870173892994651338
    };

  static const double wgk[11] =   /* weights of the 21-point kronrod rule */
    {
      0.011694638867371874278064396062192,
      0.032558162307964727478818972459390,
      0.054755896574351996031381300244580,
      0.075039674810919952767043140916190,
      0.093125454583697605535065465083366,
      0.109387158802297641899210590325805,
      0.123491976262065851077958109831074,
      0.134709217311473325928054001771707,
      0.142775938577060080797094273138717,
      0.147739104901338491374841515972068,
      0.149445554002916905664936468389821
    };

  static MultiIntegrator::MultiIntegratorFactory
  gk("gk21",
     "21 points adaptive Gauss-Kronrod",
     [](MultiIntegrator::Function fnc, int dim, double rel, double abs, int maxc) -> MultiIntegrator * {
       return new GaussKronrodMultiIntegrator(fnc, dim, rel, abs, maxc,
                                              11, xgk, wgk, wg);
     }
     );

}

//////////////////////////////////////////////////////////////////////

namespace __gk31 {

  /* Gauss quadrature weights and kronrod quadrature abscissae and
     weights as evaluated with 80 decimal digit arithmetic by
     L. W. Fullerton, Bell Labs, Nov. 1981. */

  static const double xgk[16] =   /* abscissae of the 31-point kronrod rule */
    {
      0.998002298693397060285172840152271,
      0.987992518020485428489565718586613,
      0.967739075679139134257347978784337,
      0.937273392400705904307758947710209,
      0.897264532344081900882509656454496,
      0.848206583410427216200648320774217,
      0.790418501442465932967649294817947,
      0.724417731360170047416186054613938,
      0.650996741297416970533735895313275,
      0.570972172608538847537226737253911,
      0.485081863640239680693655740232351,
      0.394151347077563369897207370981045,
      0.299180007153168812166780024266389,
      0.201194093997434522300628303394596,
      0.101142066918717499027074231447392,
      0.000000000000000000000000000000000
    };

  /* xgk[1], xgk[3], ... abscissae of the 15-point gauss rule. 
     xgk[0], xgk[2], ... abscissae to optimally extend the 15-point gauss rule */

  static const double wg[8] =     /* weights of the 15-point gauss rule */
    {
      0.030753241996117268354628393577204,
      0.070366047488108124709267416450667,
      0.107159220467171935011869546685869,
      0.139570677926154314447804794511028,
      0.166269205816993933553200860481209,
      0.186161000015562211026800561866423,
      0.198431485327111576456118326443839,
      0.202578241925561272880620199967519
    };

  static const double wgk[16] =   /* weights of the 31-point kronrod rule */
    {
      0.005377479872923348987792051430128,
      0.015007947329316122538374763075807,
      0.025460847326715320186874001019653,
      0.035346360791375846222037948478360,
      0.044589751324764876608227299373280,
      0.053481524690928087265343147239430,
      0.062009567800670640285139230960803,
      0.069854121318728258709520077099147,
      0.076849680757720378894432777482659,
      0.083080502823133021038289247286104,
      0.088564443056211770647275443693774,
      0.093126598170825321225486872747346,
      0.096642726983623678505179907627589,
      0.099173598721791959332393173484603,
      0.100769845523875595044946662617570,
      0.101330007014791549017374792767493
    };

  static MultiIntegrator::MultiIntegratorFactory
  gk("gk31",
     "31 points adaptive Gauss-Kronrod",
     [](MultiIntegrator::Function fnc, int dim, double rel, double abs, int maxc) -> MultiIntegrator * {
       return new GaussKronrodMultiIntegrator(fnc, dim, rel, abs, maxc,
                                              16, xgk, wgk, wg);
     }
     );
};

//////////////////////////////////////////////////////////////////////

namespace __gk41 {

  /* Gauss quadrature weights and kronrod quadrature abscissae and
     weights as evaluated with 80 decimal digit arithmetic by
     L. W. Fullerton, Bell Labs, Nov. 1981. */

  static const double xgk[21] =   /* abscissae of the 41-point kronrod rule */
    {
      0.998859031588277663838315576545863,
      0.993128599185094924786122388471320,
      0.981507877450250259193342994720217,
      0.963971927277913791267666131197277,
      0.940822633831754753519982722212443,
      0.912234428251325905867752441203298,
      0.878276811252281976077442995113078,
      0.839116971822218823394529061701521,
      0.795041428837551198350638833272788,
      0.746331906460150792614305070355642,
      0.693237656334751384805490711845932,
      0.636053680726515025452836696226286,
      0.575140446819710315342946036586425,
      0.510867001950827098004364050955251,
      0.443593175238725103199992213492640,
      0.373706088715419560672548177024927,
      0.301627868114913004320555356858592,
      0.227785851141645078080496195368575,
      0.152605465240922675505220241022678,
      0.076526521133497333754640409398838,
      0.000000000000000000000000000000000
    };

  /* xgk[1], xgk[3], ... abscissae of the 20-point gauss rule. 
     xgk[0], xgk[2], ... abscissae to optimally extend the 20-point gauss rule */

  static const double wg[11] =    /* weights of the 20-point gauss rule */
    {
      0.017614007139152118311861962351853,
      0.040601429800386941331039952274932,
      0.062672048334109063569506535187042,
      0.083276741576704748724758143222046,
      0.101930119817240435036750135480350,
      0.118194531961518417312377377711382,
      0.131688638449176626898494499748163,
      0.142096109318382051329298325067165,
      0.149172986472603746787828737001969,
      0.152753387130725850698084331955098
    };

  static const double wgk[21] =   /* weights of the 41-point kronrod rule */
    {
      0.003073583718520531501218293246031,
      0.008600269855642942198661787950102,
      0.014626169256971252983787960308868,
      0.020388373461266523598010231432755,
      0.025882133604951158834505067096153,
      0.031287306777032798958543119323801,
      0.036600169758200798030557240707211,
      0.041668873327973686263788305936895,
      0.046434821867497674720231880926108,
      0.050944573923728691932707670050345,
      0.055195105348285994744832372419777,
      0.059111400880639572374967220648594,
      0.062653237554781168025870122174255,
      0.065834597133618422111563556969398,
      0.068648672928521619345623411885368,
      0.071054423553444068305790361723210,
      0.073030690332786667495189417658913,
      0.074582875400499188986581418362488,
      0.075704497684556674659542775376617,
      0.076377867672080736705502835038061,
      0.076600711917999656445049901530102
    };

  static MultiIntegrator::MultiIntegratorFactory
  gk("gk41",
     "41 points adaptive Gauss-Kronrod",
     [](MultiIntegrator::Function fnc, int dim, double rel, double abs, int maxc) -> MultiIntegrator * {
       return new GaussKronrodMultiIntegrator(fnc, dim, rel, abs, maxc,
                                              21, xgk, wgk, wg);
     }
     );
};

//////////////////////////////////////////////////////////////////////

namespace __gk51 {


  /* Gauss quadrature weights and kronrod quadrature abscissae and
     weights as evaluated with 80 decimal digit arithmetic by
     L. W. Fullerton, Bell Labs, Nov. 1981. */

  static const double xgk[26] =   /* abscissae of the 51-point kronrod rule */
    {
      0.999262104992609834193457486540341,
      0.995556969790498097908784946893902,
      0.988035794534077247637331014577406,
      0.976663921459517511498315386479594,
      0.961614986425842512418130033660167,
      0.942974571228974339414011169658471,
      0.920747115281701561746346084546331,
      0.894991997878275368851042006782805,
      0.865847065293275595448996969588340,
      0.833442628760834001421021108693570,
      0.797873797998500059410410904994307,
      0.759259263037357630577282865204361,
      0.717766406813084388186654079773298,
      0.673566368473468364485120633247622,
      0.626810099010317412788122681624518,
      0.577662930241222967723689841612654,
      0.526325284334719182599623778158010,
      0.473002731445714960522182115009192,
      0.417885382193037748851814394594572,
      0.361172305809387837735821730127641,
      0.303089538931107830167478909980339,
      0.243866883720988432045190362797452,
      0.183718939421048892015969888759528,
      0.122864692610710396387359818808037,
      0.061544483005685078886546392366797,
      0.000000000000000000000000000000000
    };

  /* xgk[1], xgk[3], ... abscissae of the 25-point gauss rule. 
     xgk[0], xgk[2], ... abscissae to optimally extend the 25-point gauss rule */

  static const double wg[13] =    /* weights of the 25-point gauss rule */
    {
      0.011393798501026287947902964113235,
      0.026354986615032137261901815295299,
      0.040939156701306312655623487711646,
      0.054904695975835191925936891540473,
      0.068038333812356917207187185656708,
      0.080140700335001018013234959669111,
      0.091028261982963649811497220702892,
      0.100535949067050644202206890392686,
      0.108519624474263653116093957050117,
      0.114858259145711648339325545869556,
      0.119455763535784772228178126512901,
      0.122242442990310041688959518945852,
      0.123176053726715451203902873079050
    };

  static const double wgk[26] =   /* weights of the 51-point kronrod rule */
    {
      0.001987383892330315926507851882843,
      0.005561932135356713758040236901066,
      0.009473973386174151607207710523655,
      0.013236229195571674813656405846976,
      0.016847817709128298231516667536336,
      0.020435371145882835456568292235939,
      0.024009945606953216220092489164881,
      0.027475317587851737802948455517811,
      0.030792300167387488891109020215229,
      0.034002130274329337836748795229551,
      0.037116271483415543560330625367620,
      0.040083825504032382074839284467076,
      0.042872845020170049476895792439495,
      0.045502913049921788909870584752660,
      0.047982537138836713906392255756915,
      0.050277679080715671963325259433440,
      0.052362885806407475864366712137873,
      0.054251129888545490144543370459876,
      0.055950811220412317308240686382747,
      0.057437116361567832853582693939506,
      0.058689680022394207961974175856788,
      0.059720340324174059979099291932562,
      0.060539455376045862945360267517565,
      0.061128509717053048305859030416293,
      0.061471189871425316661544131965264,
      0.061580818067832935078759824240066
    };

    static MultiIntegrator::MultiIntegratorFactory
    gk("gk51",
     "51 points adaptive Gauss-Kronrod",
     [](MultiIntegrator::Function fnc, int dim, double rel, double abs, int maxc) -> MultiIntegrator * {
       return new GaussKronrodMultiIntegrator(fnc, dim, rel, abs, maxc,
                                              26, xgk, wgk, wg);
     }
     );
}

//////////////////////////////////////////////////////////////////////

namespace __gk61 {
  /* Gauss quadrature weights and kronrod quadrature abscissae and
     weights as evaluated with 80 decimal digit arithmetic by
     L. W. Fullerton, Bell Labs, Nov. 1981. */

  static const double xgk[31] =   /* abscissae of the 61-point kronrod rule */
    {
      0.999484410050490637571325895705811,
      0.996893484074649540271630050918695,
      0.991630996870404594858628366109486,
      0.983668123279747209970032581605663,
      0.973116322501126268374693868423707,
      0.960021864968307512216871025581798,
      0.944374444748559979415831324037439,
      0.926200047429274325879324277080474,
      0.905573307699907798546522558925958,
      0.882560535792052681543116462530226,
      0.857205233546061098958658510658944,
      0.829565762382768397442898119732502,
      0.799727835821839083013668942322683,
      0.767777432104826194917977340974503,
      0.733790062453226804726171131369528,
      0.697850494793315796932292388026640,
      0.660061064126626961370053668149271,
      0.620526182989242861140477556431189,
      0.579345235826361691756024932172540,
      0.536624148142019899264169793311073,
      0.492480467861778574993693061207709,
      0.447033769538089176780609900322854,
      0.400401254830394392535476211542661,
      0.352704725530878113471037207089374,
      0.304073202273625077372677107199257,
      0.254636926167889846439805129817805,
      0.204525116682309891438957671002025,
      0.153869913608583546963794672743256,
      0.102806937966737030147096751318001,
      0.051471842555317695833025213166723,
      0.000000000000000000000000000000000
    };

  /* xgk[1], xgk[3], ... abscissae of the 30-point gauss rule. 
     xgk[0], xgk[2], ... abscissae to optimally extend the 30-point gauss rule */

  static const double wg[15] =    /* weights of the 30-point gauss rule */
    {
      0.007968192496166605615465883474674,
      0.018466468311090959142302131912047,
      0.028784707883323369349719179611292,
      0.038799192569627049596801936446348,
      0.048402672830594052902938140422808,
      0.057493156217619066481721689402056,
      0.065974229882180495128128515115962,
      0.073755974737705206268243850022191,
      0.080755895229420215354694938460530,
      0.086899787201082979802387530715126,
      0.092122522237786128717632707087619,
      0.096368737174644259639468626351810,
      0.099593420586795267062780282103569,
      0.101762389748405504596428952168554,
      0.102852652893558840341285636705415
    };

  static const double wgk[31] =   /* weights of the 61-point kronrod rule */
    {
      0.001389013698677007624551591226760,
      0.003890461127099884051267201844516,
      0.006630703915931292173319826369750,
      0.009273279659517763428441146892024,
      0.011823015253496341742232898853251,
      0.014369729507045804812451432443580,
      0.016920889189053272627572289420322,
      0.019414141193942381173408951050128,
      0.021828035821609192297167485738339,
      0.024191162078080601365686370725232,
      0.026509954882333101610601709335075,
      0.028754048765041292843978785354334,
      0.030907257562387762472884252943092,
      0.032981447057483726031814191016854,
      0.034979338028060024137499670731468,
      0.036882364651821229223911065617136,
      0.038678945624727592950348651532281,
      0.040374538951535959111995279752468,
      0.041969810215164246147147541285970,
      0.043452539701356069316831728117073,
      0.044814800133162663192355551616723,
      0.046059238271006988116271735559374,
      0.047185546569299153945261478181099,
      0.048185861757087129140779492298305,
      0.049055434555029778887528165367238,
      0.049795683427074206357811569379942,
      0.050405921402782346840893085653585,
      0.050881795898749606492297473049805,
      0.051221547849258772170656282604944,
      0.051426128537459025933862879215781,
      0.051494729429451567558340433647099
    };

  static MultiIntegrator::MultiIntegratorFactory
  gk("gk61",
     "61 points adaptive Gauss-Kronrod",
     [](MultiIntegrator::Function fnc, int dim, double rel, double abs, int maxc) -> MultiIntegrator * {
       return new GaussKronrodMultiIntegrator(fnc, dim, rel, abs, maxc,
                                              31, xgk, wgk, wg);
     }
     );
}

