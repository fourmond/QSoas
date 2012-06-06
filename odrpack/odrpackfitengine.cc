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

#include "cfortran/cfortran.h"

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


  void freeBeforeInitialization();

public:

  ODRPACKFitEngine(FitData * data);
  virtual ~ODRPACKFitEngine();


  virtual void initialize(const double * initialGuess);
  virtual const gsl_vector * currentParameters() const;
  virtual void computeCovarianceMatrix(gsl_matrix * target) const;
  virtual int iterate();
  virtual double residuals() const;
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
  

  // We are currently fitting this fit
  ::engine = this;
}

const gsl_vector * ODRPACKFitEngine::currentParameters() const
{
  return &pmview.vector;
}

void ODRPACKFitEngine::computeCovarianceMatrix(gsl_matrix * target) const
{
  /// @todo Hmmm, this is bad, and it will have to be dealt with soon !
}

double ODRPACKFitEngine::residuals() const
{
  /// @todo A great pain to extract...
  return 0;                     // Surely a bad choice for now !
}


     //  SUBROUTINE DODR
     // +   (FCN,
     // +   N,M,NP,NQ,
     // +   BETA,
     // +   Y,LDY,X,LDX,
     // +   WE,LDWE,LD2WE,WD,LDWD,LD2WD,
     // +   JOB,
     // +   IPRINT,LUNERR,LUNRPT,
     // +   WORK,LWORK,IWORK,LIWORK,
     // +   INFO)

// Prototype for the above call, using the same lines
PROTOCCALLSFSUB25(DODR,dodr,                                    \
                  ROUTINE,                                      \
                  INT,INT,INT,INT,                              \
                  DOUBLEV,                                      \
                  DOUBLEV, INT, DOUBLEV, INT,                   \
                  DOUBLEV, INT, INT, DOUBLEV, INT, INT,         \
                  INT,                                          \
                  INT, INT, INT,                                \
                  DOUBLEV, INT, INTV, INT,                      \
                  PINT                                          \
                  );

#define call_dodr(A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN,AO,AP) \
  CCALLSFSUB25(DODR,dodr,                                               \
               ROUTINE,                                                 \
               INT,INT,INT,INT,                                         \
               DOUBLEV,                                                 \
               DOUBLEV, INT, DOUBLEV, INT,                              \
               DOUBLEV, INT, INT, DOUBLEV, INT, INT,                    \
               INT,                                                     \
               INT, INT, INT,                                           \
               DOUBLEV, INT, INTV, INT,                                 \
               PINT,                                                    \
               A1,A2,A3,A4,A5,A6,A7,A8,A9,AA,AB,AC,AD,AE,AF,AG,AH,AI,AJ,AK,AL,AM,AN,AO,AP)

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

  double weight = -1;
  int info = 0; 

  call_dodr(&fcn, nb, m, np, q, //
            parameters, 
            yValues, nb, dummyXValues, nb, // We're at LDX
            &weight,one,one, &weight, one, one, // We're at LD2WD
            job,
            neg,neg,neg,
            workVector, wvSize,
            workIntegers, wiSize,
            info);

  return GSL_SUCCESS;
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
