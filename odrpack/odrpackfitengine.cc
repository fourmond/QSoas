/**
   \file odrpackfitengine.cc
   The ODRPACK-based fit engine
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
#include <fitdata.hh>
#include <fitengine.hh>
#include <exceptions.hh>

class ODRPACKFitEngine : public FitEngine {

protected:

  /// The FCN call from odrpack. I hope I got the argument list
  /// right...
  static void fcn(const int * n, const int * m, 
                  const int * np, const int * nq,
                  const int * ldn, const int * ldm, 
                  const int * ldnp,
                  const double * beta,
                  const double * xplusd,
                  const int * ifixb, const int * ifixx,
                  const int * ldifx,
                  const int * ideval,
                  double * f, double * fjacb, double * fjacd,
                  int * istop);

  /// Not used, really.
  double * dummyXValues;


  double * yValues;

  /// The parameters
  double * parameters;

  /// An array stating that all parameters are free
  int * parametersAreFree;

  gsl_vector_view pmview;

  /// The work vector
  double * workVector;

  /// ... its size;
  int wvSize;

  /// The work integer storage
  int * workIntegers;

  /// ... its size;
  int wiSize;

  /// The number of iterations since the beginning
  int iterationNumber;


  /// @name Digging the work vector
  ///
  /// These are indices of importance in the work vector
  ///
  /// @{
  
  /// Standard deviation
  int standardDeviationIdx;

  /// Covariance matrix
  int covarianceMatrixIdx;

  /// Sum of squares
  int residualsIdx;

  /// Condition number
  int conditionNumberIdx;
  


  /// @}


  void freeBeforeInitialization();

public:

  ODRPACKFitEngine(FitData * data);
  virtual ~ODRPACKFitEngine();

  virtual bool handlesWeights() const {
    return false;
  };

  virtual void initialize(const double * initialGuess);
  virtual const gsl_vector * currentParameters() const;
  virtual void computeCovarianceMatrix(gsl_matrix * target) const;
  virtual int iterate();
  virtual double residuals() const;


  virtual void recomputeJacobian();
};

ODRPACKFitEngine::ODRPACKFitEngine(FitData * data) :
  FitEngine(data)
{
  dummyXValues = new double[fitData->dataPoints()];
  for(int i = 0; i < fitData->dataPoints(); i++)
    dummyXValues[i] = i;        // Good default choice, isn't it ?

  yValues = new double[fitData->dataPoints()];

  parameters = NULL;
  parametersAreFree = NULL;
  workVector = NULL;
  workIntegers = NULL;
  residualsIdx = -1;
  covarianceMatrixIdx = -1;

  // Initialize with the data:
  gsl_vector_view yv = gsl_vector_view_array(yValues, fitData->dataPoints());
  gsl_vector_set_zero(&yv.vector);
  // cumbersome, but quick
  fitData->subtractData(&yv.vector);
  gsl_vector_scale(&yv.vector, -1); // Yep !
  fitData->weightVector(&yv.vector);
}

void ODRPACKFitEngine::freeBeforeInitialization()
{
  delete[] parameters;
  parameters = NULL;

  delete[] parametersAreFree;
  parametersAreFree = NULL;

  delete[] workVector;
  workVector = NULL;


  delete[] workIntegers;
  workIntegers = NULL;

  residualsIdx = -1;
  covarianceMatrixIdx = -1;
}


ODRPACKFitEngine::~ODRPACKFitEngine()
{
  freeBeforeInitialization();
  delete[] dummyXValues;
  delete[] yValues;
}

/// @todo Transform that into something more object-oriented for the
/// future ! (at least reentrant would be good)
static ODRPACKFitEngine * engine = NULL;

void ODRPACKFitEngine::initialize(const double * initialGuess)
{
  freeBeforeInitialization();

  // We only do allocation here...
  parameters = new double[fitData->freeParameters()];
  parametersAreFree = new int[fitData->freeParameters()];
  for(int i = 0; i < fitData->freeParameters(); i++)
    parametersAreFree[i] = 1;
  
  pmview = gsl_vector_view_array(parameters, fitData->freeParameters());
  fitData->packParameters(initialGuess, &pmview.vector);



  // Work vector allocation
  int p = fitData->freeParameters();
  int n = fitData->dataPoints();
  int q = 1;
  int m = 1;
  wvSize = 18 + 11 * p + p * p +
    m + m*m + 4 * n * q + 2 * n * m +
    2 * n * p * q + 5 * q + q * (p + m) + (1 * 1) * q; // no weights here
  workVector = new double[wvSize];

  wiSize = 20 + p + q * (p + m);
  workIntegers = new int[wiSize];

  iterationNumber = 0;
  

  // We are currently fitting this fit
  ::engine = this;
}

const gsl_vector * ODRPACKFitEngine::currentParameters() const
{
  return &pmview.vector;
}

void ODRPACKFitEngine::computeCovarianceMatrix(gsl_matrix * target) const
{
  if(covarianceMatrixIdx < 0)
    gsl_matrix_set_zero(target);
  else {
    gsl_matrix_view v = 
      gsl_matrix_view_array(workVector + covarianceMatrixIdx - 1,
                            fitData->freeParameters(), 
                            fitData->freeParameters());
    gsl_matrix_memcpy(target, (const gsl_matrix *)&v.matrix);
  }
}

double ODRPACKFitEngine::residuals() const
{
  if(residualsIdx >= 0)
    return sqrt(workVector[residualsIdx - 1]); // Argh !
  return 0;                     // Surely a bad choice for now !
}

typedef void (*U_fp)(...);

/// @todo These functions headers will eventually need to a dedicated
/// header file, which should allow me to change a few int * into
/// const int *, and possibly even into "int" !
extern "C" 
int dodr_(U_fp fcn, int *n, int *m, int *np, 
          int *nq, double *beta, double *y, int *ldy, 
          double *x, int *ldx, double *we, int *ldwe, int *
          ld2we, double *wd, int *ldwd, int *ld2wd, int *job, 
          int *iprint, int *lunerr, int *lunrpt, double *work, 
          int *lwork, int *iwork, int *liwork, int *info);

extern "C" 
int dodrc_(U_fp fcn, int *n, int *m, int *np, 
           int *nq, double *beta, double *y, int *ldy, 
           double *x, int *ldx, double *we, int *ldwe, int *
           ld2we, double *wd, int *ldwd, int *ld2wd, int *ifixb, 
           int *ifixx, int *ldifx, int *job, int *ndigit, 
           double *taufac, double *sstol, double *partol, int *
           maxit, int *iprint, int *lunerr, int *lunrpt, double *
           stpb, double *stpd, int *ldstpd, double *sclb, double 
           *scld, int *ldscld, double *work, int *lwork, int *
           iwork, int *liwork, int *info);

extern "C" 
int dwinf_(int *n, int *m, int *np, int *nq, 
           int *ldwe, int *ld2we, int *isodr, int *deltai, 
           int *epsi, int *xplusi, int *fni, int *sdi, int *
           vcvi, int *rvari, int *wssi, int *wssdei, int *wssepi,
           int *rcondi, int *etai, int *olmavi, int *taui, 
           int *alphai, int *actrsi, int *pnormi, int *rnorsi, 
           int *prersi, int *partli, int *sstoli, int *taufci, 
           int *epsmai, int *beta0i, int *betaci, int *betasi, 
           int *betani, int *si, int *ssi, int *ssfi, int *
           qrauxi, int *ui, int *fsi, int *fjacbi, int *we1i, 
           int *diffi, int *deltsi, int *deltni, int *ti, 
           int *tti, int *omegai, int *fjacdi, int *wrk1i, 
           int *wrk2i, int *wrk3i, int *wrk4i, int *wrk5i, 
           int *wrk6i, int *wrk7i, int *lwkmn);



int ODRPACKFitEngine::iterate()
{
  /// @warning This specific bit makes the fit engine
  /// non-reentrant. But as there is no reason why iterate on another
  /// instance would be called during this function, this is nothing
  /// to worry about for now.
  ::engine = this;
  int nb = fitData->dataPoints();
  int m = 1;
  int np = fitData->freeParameters();
  int q = 1;
  int one = 1;
  int neg = -1;
  int job = 2;                  // OLS + derivatives by forward
                                // difference
  int ndigit = 0;

  double weight = -1;
  int info = 0; 
  double taufac = -1;
  double dneg = -1;
  int maxit = 1;                // We go only one iteration !

  if(iterationNumber > 0)
    job += 10000;               // Fit is a restart
  else {
    int isodr = 0;
    int data[50];
    int *d = data;  // for not interesting variables
    // We prepare the position of the parameters...

    dwinf_(&nb, &m, &np, &q, &one, &one, &isodr, // These are the *in*
           // parameters
           d+1, d+2, d+3, d+4,                  // FNI
           &standardDeviationIdx, &covarianceMatrixIdx, d+5, // RVARI
           &residualsIdx, d+6, d+7, &conditionNumberIdx, d+8, // ETAI
           d+9, d+10, d+11, // ALPHAI
           d+12, d+13, d+14, d+15, d+16, d+17, d+18, d+19, d+20, d+21, d+22, d+23, //BETANI
           d+24, d+25, d+26, d+27, d+28, d+29, d+30, d+31, d+32, d+33, d+34, d+35, //TI
           d+36, d+37, d+38, d+39, d+40, d+41, d+42, d+43, d+44, d+45, d+46 //LWKMN
           );
  }


  iterationNumber++;

  dodrc_((U_fp) &fcn, &nb, &m, &np, &q, //
         parameters, 
         yValues, &nb, dummyXValues, &nb, // We're at LDX
         &weight,&one,&one, &weight, &one, &one, // We're at LD2WD
         // Now the fixed stuff
         parametersAreFree, &neg, &one,
         &job, &ndigit, &taufac,
         &dneg, &dneg, &maxit,
         &neg,&neg,&neg,        // We're at LUNRPRT
         &dneg, &dneg, &one,    // We're at LDSTPD
         &dneg, &dneg, &one,    // We're at LDSCLD
         workVector, &wvSize,
         workIntegers, &wiSize,
         &info);

  /// @todo Better error handling...
  if(info == 4)
    return GSL_CONTINUE;

  QTextStream o(stdout);
  o << "Fitting procedure report: info is " << info << endl;

  return GSL_SUCCESS;
}


void ODRPACKFitEngine::recomputeJacobian()
{
  /// @warning This specific bit makes the fit engine
  /// non-reentrant. But as there is no reason why iterate on another
  /// instance would be called during this function, this is nothing
  /// to worry about for now.
  ::engine = this;
  int nb = fitData->dataPoints();
  int m = 1;
  int np = fitData->freeParameters();
  int q = 1;
  int one = 1;
  int neg = -1;
  int job = 2;                  // OLS + derivatives by forward
                                // difference
  int ndigit = 0;

  double weight = -1;
  int info = 0; 
  double taufac = -1;
  double dneg = -1;
  int maxit = 0;                // We go only one iteration !

  int isodr = 0;
  int data[50];
  int *d = data;  // for not interesting variables
  // We prepare the position of the parameters...

  dwinf_(&nb, &m, &np, &q, &one, &one, &isodr, // These are the *in*
         // parameters
         d+1, d+2, d+3, d+4,                  // FNI
         &standardDeviationIdx, &covarianceMatrixIdx, d+5, // RVARI
         &residualsIdx, d+6, d+7, &conditionNumberIdx, d+8, // ETAI
         d+9, d+10, d+11, // ALPHAI
         d+12, d+13, d+14, d+15, d+16, d+17, d+18, d+19, d+20, d+21, d+22, d+23, //BETANI
         d+24, d+25, d+26, d+27, d+28, d+29, d+30, d+31, d+32, d+33, d+34, d+35, //TI
         d+36, d+37, d+38, d+39, d+40, d+41, d+42, d+43, d+44, d+45, d+46 //LWKMN
         );


  iterationNumber++;

  dodrc_((U_fp) &fcn, &nb, &m, &np, &q, //
         parameters, 
         yValues, &nb, dummyXValues, &nb, // We're at LDX
         &weight,&one,&one, &weight, &one, &one, // We're at LD2WD
         // Now the fixed stuff
         parametersAreFree, &neg, &one,
         &job, &ndigit, &taufac,
         &dneg, &dneg, &maxit,
         &neg,&neg,&neg,        // We're at LUNRPRT
         &dneg, &dneg, &one,    // We're at LDSTPD
         &dneg, &dneg, &one,    // We're at LDSCLD
         workVector, &wvSize,
         workIntegers, &wiSize,
         &info);
}


void ODRPACKFitEngine::fcn(const int * n, const int * m, 
                           const int * np, const int * nq,
                           const int * ldn, const int * ldm, 
                           const int * ldnp,
                           const double * beta,
                           const double * xplusd,
                           const int * ifixb, const int * ifixx,
                           const int * ldifx,
                           const int * ideval,
                           double * f, double * fjacb, double * fjacd,
                           int * istop)
{
  *istop = 0;
  gsl_vector_const_view params = gsl_vector_const_view_array(beta, *np);
  gsl_vector_view target = gsl_vector_view_array(f, *n);
  try {
    engine->fitData->f(&params.vector, &target.vector, false);
  }
  catch(const RuntimeError & re) {
    *istop = 1;
  }
}

static FitEngine * odrpack(FitData * d)
{
  return new ODRPACKFitEngine(d);
}

static FitEngineFactoryItem odr("odrpack", "ODRPACK", &odrpack);
