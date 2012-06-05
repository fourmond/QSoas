/* d_odr.f -- translated by f2c (version 20100827).
   You must link the resulting object file with libf2c:
	on Microsoft Windows system, link with libf2c.lib;
	on Linux or Unix systems, link with .../path/to/libf2c.a -lm
	or, if you install libf2c.a in a standard place, with -lf2c -lm
	-- in that order, at the end of the command line, as in
		cc *.o -lf2c -lm
	Source for libf2c is in /netlib/f2c/libf2c.zip, e.g.,

		http://www.netlib.org/f2c/libf2c.zip
*/

#include "f2c.h"

/* Table of constant values */

static integer c__1 = 1;
static integer c__3 = 3;
static logical c_true = TRUE_;
static logical c_false = FALSE_;
static integer c__0 = 0;
static doublereal c_b150 = .25;
static integer c__2 = 2;
static doublereal c_b666 = .975;
static integer c__4 = 4;
static integer c__1000 = 1000;

/* DODR */
/* Subroutine */ int dodr_(U_fp fcn, integer *n, integer *m, integer *np, 
	integer *nq, doublereal *beta, doublereal *y, integer *ldy, 
	doublereal *x, integer *ldx, doublereal *we, integer *ldwe, integer *
	ld2we, doublereal *wd, integer *ldwd, integer *ld2wd, integer *job, 
	integer *iprint, integer *lunerr, integer *lunrpt, doublereal *work, 
	integer *lwork, integer *iwork, integer *liwork, integer *info)
{
    /* Initialized data */

    static doublereal negone = -1.;
    static doublereal zero = 0.;

    /* System generated locals */
    integer wd_dim1, wd_dim2, wd_offset, we_dim1, we_dim2, we_offset, x_dim1, 
	    x_offset, y_dim1, y_offset;

    /* Local variables */
    static doublereal wd1[1]	/* was [1][1][1] */, sclb[1], scld[1]	/* 
	    was [1][1] */, stpb[1], stpd[1]	/* was [1][1] */;
    static integer ifixb[1], ldifx, maxit, ifixx[1]	/* was [1][1] */;
    static logical short__;
    static doublereal sstol, taufac;
    static integer ldscld;
    extern /* Subroutine */ int dodcnt_(logical *, U_fp, integer *, integer *,
	     integer *, integer *, doublereal *, doublereal *, integer *, 
	    doublereal *, integer *, doublereal *, integer *, integer *, 
	    doublereal *, integer *, integer *, integer *, integer *, integer 
	    *, integer *, integer *, doublereal *, doublereal *, doublereal *,
	     integer *, integer *, integer *, integer *, doublereal *, 
	    doublereal *, integer *, doublereal *, doublereal *, integer *, 
	    doublereal *, integer *, integer *, integer *, integer *);
    static integer ndigit, ldstpd;
    static doublereal partol;

/* ***BEGIN PROLOGUE  DODR */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***CATEGORY NO.  G2E,I1B1 */
/* ***KEYWORDS  ORTHOGONAL DISTANCE REGRESSION, */
/*             NONLINEAR LEAST SQUARES, */
/*             MEASUREMENT ERROR MODELS, */
/*             ERRORS IN VARIABLES */
/* ***AUTHOR  BOGGS, PAUL T. */
/*             APPLIED AND COMPUTATIONAL MATHEMATICS DIVISION */
/*             NATIONAL INSTITUTE OF STANDARDS AND TECHNOLOGY */
/*             GAITHERSBURG, MD 20899 */
/*           BYRD, RICHARD H. */
/*             DEPARTMENT OF COMPUTER SCIENCE */
/*             UNIVERSITY OF COLORADO, BOULDER, CO 80309 */
/*           ROGERS, JANET E. */
/*             APPLIED AND COMPUTATIONAL MATHEMATICS DIVISION */
/*             NATIONAL INSTITUTE OF STANDARDS AND TECHNOLOGY */
/*             BOULDER, CO 80303-3328 */
/*           SCHNABEL, ROBERT B. */
/*             DEPARTMENT OF COMPUTER SCIENCE */
/*             UNIVERSITY OF COLORADO, BOULDER, CO 80309 */
/*             AND */
/*             APPLIED AND COMPUTATIONAL MATHEMATICS DIVISION */
/*             NATIONAL INSTITUTE OF STANDARDS AND TECHNOLOGY */
/*             BOULDER, CO 80303-3328 */
/* ***PURPOSE  DOUBLE PRECISION DRIVER ROUTINE FOR FINDING */
/*            THE WEIGHTED EXPLICIT OR IMPLICIT ORTHOGONAL DISTANCE */
/*            REGRESSION (ODR) OR ORDINARY LINEAR OR NONLINEAR LEAST */
/*            SQUARES (OLS) SOLUTION (SHORT CALL STATEMENT) */
/* ***DESCRIPTION */
/*   FOR DETAILS, SEE ODRPACK USER'S REFERENCE GUIDE. */
/* ***REFERENCES  BOGGS, P. T., R. H. BYRD, J. R. DONALDSON, AND */
/*                 R. B. SCHNABEL (1989), */
/*                 "ALGORITHM 676 --- ODRPACK: SOFTWARE FOR WEIGHTED */
/*                 ORTHOGONAL DISTANCE REGRESSION," */
/*                 ACM TRANS. MATH. SOFTWARE., 15(4):348-364. */
/*               BOGGS, P. T., R. H. BYRD, J. E. ROGERS, AND */
/*                 R. B. SCHNABEL (1992), */
/*                 "USER'S REFERENCE GUIDE FOR ODRPACK VERSION 2.01, */
/*                 SOFTWARE FOR WEIGHTED ORTHOGONAL DISTANCE REGRESSION," */
/*                 NATIONAL INSTITUTE OF STANDARDS AND TECHNOLOGY */
/*                 INTERNAL REPORT NUMBER 92-4834. */
/*               BOGGS, P. T., R. H. BYRD, AND R. B. SCHNABEL (1987), */
/*                 "A STABLE AND EFFICIENT ALGORITHM FOR NONLINEAR */
/*                 ORTHOGONAL DISTANCE REGRESSION," */
/*                 SIAM J. SCI. STAT. COMPUT., 8(6):1052-1078. */
/* ***ROUTINES CALLED  DODCNT */
/* ***END PROLOGUE  DODR */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...LOCAL ARRAYS */
/* ...EXTERNAL SUBROUTINES */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --beta;
    y_dim1 = *ldy;
    y_offset = 1 + y_dim1;
    y -= y_offset;
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    we_dim1 = *ldwe;
    we_dim2 = *ld2we;
    we_offset = 1 + we_dim1 * (1 + we_dim2);
    we -= we_offset;
    wd_dim1 = *ldwd;
    wd_dim2 = *ld2wd;
    wd_offset = 1 + wd_dim1 * (1 + wd_dim2);
    wd -= wd_offset;
    --work;
    --iwork;

    /* Function Body */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:     THE USER-SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   INFO:    THE VARIABLE DESIGNATING WHY THE COMPUTATIONS WERE STOPPED. */
/*   IPRINT:  THE PRINT CONTROL VARIABLE. */
/*   IWORK:   THE INTEGER WORK SPACE. */
/*   JOB:     THE VARIABLE CONTROLLING PROBLEM INITIALIZATION AND */
/*            COMPUTATIONAL METHOD. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LDSCLD:  THE LEADING DIMENSION OF ARRAY SCLD. */
/*   LDSTPD:  THE LEADING DIMENSION OF ARRAY STPD. */
/*   LDWD:    THE LEADING DIMENSION OF ARRAY WD. */
/*   LDWE:    THE LEADING DIMENSION OF ARRAY WE. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   LDY:     THE LEADING DIMENSION OF ARRAY Y. */
/*   LD2WD:   THE SECOND DIMENSION OF ARRAY WD. */
/*   LD2WE:   THE SECOND DIMENSION OF ARRAY WE. */
/*   LIWORK:  THE LENGTH OF VECTOR IWORK. */
/*   LUNERR:  THE LOGICAL UNIT NUMBER FOR ERROR MESSAGES. */
/*   LUNRPT:  THE LOGICAL UNIT NUMBER FOR COMPUTATION REPORTS. */
/*   LWORK:   THE LENGTH OF VECTOR WORK. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   MAXIT:   THE MAXIMUM NUMBER OF ITERATIONS ALLOWED. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NEGONE:  THE VALUE -1.0D0. */
/*   NDIGIT:  THE NUMBER OF ACCURATE DIGITS IN THE FUNCTION RESULTS, AS */
/*            SUPPLIED BY THE USER. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   PARTOL:  THE PARAMETER CONVERGENCE STOPPING TOLERANCE. */
/*   SCLB:    THE SCALING VALUES FOR BETA. */
/*   SCLD:    THE SCALING VALUES FOR DELTA. */
/*   STPB:    THE RELATIVE STEP FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO BETA. */
/*   STPD:    THE RELATIVE STEP FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO DELTA. */
/*   SHORT:   THE VARIABLE DESIGNATING WHETHER THE USER HAS INVOKED */
/*            ODRPACK BY THE SHORT-CALL (SHORT=.TRUE.) OR THE LONG-CALL */
/*            (SHORT=.FALSE.). */
/*   SSTOL:   THE SUM-OF-SQUARES CONVERGENCE STOPPING TOLERANCE. */
/*   TAUFAC:  THE FACTOR USED TO COMPUTE THE INITIAL TRUST REGION */
/*            DIAMETER. */
/*   WD:      THE DELTA WEIGHTS. */
/*   WD1:     A DUMMY ARRAY USED WHEN WD(1,1,1)=0.0D0. */
/*   WE:      THE EPSILON WEIGHTS. */
/*   WORK:    THE DOUBLE PRECISION WORK SPACE. */
/*   X:       THE EXPLANATORY VARIABLE. */
/*   Y:       THE DEPENDENT VARIABLE.  UNUSED WHEN THE MODEL IS IMPLICIT. */
/* ***FIRST EXECUTABLE STATEMENT  DODR */
/*  INITIALIZE NECESSARY VARIABLES TO INDICATE USE OF DEFAULT VALUES */
    ifixb[0] = -1;
    ifixx[0] = -1;
    ldifx = 1;
    ndigit = -1;
    taufac = negone;
    sstol = negone;
    partol = negone;
    maxit = -1;
    stpb[0] = negone;
    stpd[0] = negone;
    ldstpd = 1;
    sclb[0] = negone;
    scld[0] = negone;
    ldscld = 1;
    short__ = TRUE_;
    if (wd[(wd_dim2 + 1) * wd_dim1 + 1] != zero) {
	dodcnt_(&short__, (U_fp)fcn, n, m, np, nq, &beta[1], &y[y_offset], 
		ldy, &x[x_offset], ldx, &we[we_offset], ldwe, ld2we, &wd[
		wd_offset], ldwd, ld2wd, ifixb, ifixx, &ldifx, job, &ndigit, &
		taufac, &sstol, &partol, &maxit, iprint, lunerr, lunrpt, stpb,
		 stpd, &ldstpd, sclb, scld, &ldscld, &work[1], lwork, &iwork[
		1], liwork, info);
    } else {
	wd1[0] = negone;
	dodcnt_(&short__, (U_fp)fcn, n, m, np, nq, &beta[1], &y[y_offset], 
		ldy, &x[x_offset], ldx, &we[we_offset], ldwe, ld2we, wd1, &
		c__1, &c__1, ifixb, ifixx, &ldifx, job, &ndigit, &taufac, &
		sstol, &partol, &maxit, iprint, lunerr, lunrpt, stpb, stpd, &
		ldstpd, sclb, scld, &ldscld, &work[1], lwork, &iwork[1], 
		liwork, info);
    }
    return 0;
} /* dodr_ */

/* DODRC */
/* Subroutine */ int dodrc_(U_fp fcn, integer *n, integer *m, integer *np, 
	integer *nq, doublereal *beta, doublereal *y, integer *ldy, 
	doublereal *x, integer *ldx, doublereal *we, integer *ldwe, integer *
	ld2we, doublereal *wd, integer *ldwd, integer *ld2wd, integer *ifixb, 
	integer *ifixx, integer *ldifx, integer *job, integer *ndigit, 
	doublereal *taufac, doublereal *sstol, doublereal *partol, integer *
	maxit, integer *iprint, integer *lunerr, integer *lunrpt, doublereal *
	stpb, doublereal *stpd, integer *ldstpd, doublereal *sclb, doublereal 
	*scld, integer *ldscld, doublereal *work, integer *lwork, integer *
	iwork, integer *liwork, integer *info)
{
    /* Initialized data */

    static doublereal negone = -1.;
    static doublereal zero = 0.;

    /* System generated locals */
    integer scld_dim1, scld_offset, stpd_dim1, stpd_offset, wd_dim1, wd_dim2, 
	    wd_offset, we_dim1, we_dim2, we_offset, x_dim1, x_offset, y_dim1, 
	    y_offset, ifixx_dim1, ifixx_offset;

    /* Local variables */
    static doublereal wd1[1]	/* was [1][1][1] */;
    static logical short__;
    extern /* Subroutine */ int dodcnt_(logical *, U_fp, integer *, integer *,
	     integer *, integer *, doublereal *, doublereal *, integer *, 
	    doublereal *, integer *, doublereal *, integer *, integer *, 
	    doublereal *, integer *, integer *, integer *, integer *, integer 
	    *, integer *, integer *, doublereal *, doublereal *, doublereal *,
	     integer *, integer *, integer *, integer *, doublereal *, 
	    doublereal *, integer *, doublereal *, doublereal *, integer *, 
	    doublereal *, integer *, integer *, integer *, integer *);

/* ***BEGIN PROLOGUE  DODRC */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***CATEGORY NO.  G2E,I1B1 */
/* ***KEYWORDS  ORTHOGONAL DISTANCE REGRESSION, */
/*             NONLINEAR LEAST SQUARES, */
/*             MEASUREMENT ERROR MODELS, */
/*             ERRORS IN VARIABLES */
/* ***AUTHOR  BOGGS, PAUL T. */
/*             APPLIED AND COMPUTATIONAL MATHEMATICS DIVISION */
/*             NATIONAL INSTITUTE OF STANDARDS AND TECHNOLOGY */
/*             GAITHERSBURG, MD 20899 */
/*           BYRD, RICHARD H. */
/*             DEPARTMENT OF COMPUTER SCIENCE */
/*             UNIVERSITY OF COLORADO, BOULDER, CO 80309 */
/*           ROGERS, JANET E. */
/*             APPLIED AND COMPUTATIONAL MATHEMATICS DIVISION */
/*             NATIONAL INSTITUTE OF STANDARDS AND TECHNOLOGY */
/*             BOULDER, CO 80303-3328 */
/*           SCHNABEL, ROBERT B. */
/*             DEPARTMENT OF COMPUTER SCIENCE */
/*             UNIVERSITY OF COLORADO, BOULDER, CO 80309 */
/*             AND */
/*             APPLIED AND COMPUTATIONAL MATHEMATICS DIVISION */
/*             NATIONAL INSTITUTE OF STANDARDS AND TECHNOLOGY */
/*             BOULDER, CO 80303-3328 */
/* ***PURPOSE  DOUBLE PRECISION DRIVER ROUTINE FOR FINDING */
/*            THE WEIGHTED EXPLICIT OR IMPLICIT ORTHOGONAL DISTANCE */
/*            REGRESSION (ODR) OR ORDINARY LINEAR OR NONLINEAR LEAST */
/*            SQUARES (OLS) SOLUTION (LONG CALL STATEMENT) */
/* ***DESCRIPTION */
/*   FOR DETAILS, SEE ODRPACK USER'S REFERENCE GUIDE. */
/* ***REFERENCES  BOGGS, P. T., R. H. BYRD, J. R. DONALDSON, AND */
/*                 R. B. SCHNABEL (1989), */
/*                 "ALGORITHM 676 --- ODRPACK: SOFTWARE FOR WEIGHTED */
/*                 ORTHOGONAL DISTANCE REGRESSION," */
/*                 ACM TRANS. MATH. SOFTWARE., 15(4):348-364. */
/*               BOGGS, P. T., R. H. BYRD, J. E. ROGERS, AND */
/*                 R. B. SCHNABEL (1992), */
/*                 "USER'S REFERENCE GUIDE FOR ODRPACK VERSION 2.01, */
/*                 SOFTWARE FOR WEIGHTED ORTHOGONAL DISTANCE REGRESSION," */
/*                 NATIONAL INSTITUTE OF STANDARDS AND TECHNOLOGY */
/*                 INTERNAL REPORT NUMBER 92-4834. */
/*               BOGGS, P. T., R. H. BYRD, AND R. B. SCHNABEL (1987), */
/*                 "A STABLE AND EFFICIENT ALGORITHM FOR NONLINEAR */
/*                 ORTHOGONAL DISTANCE REGRESSION," */
/*                 SIAM J. SCI. STAT. COMPUT., 8(6):1052-1078. */
/* ***ROUTINES CALLED  DODCNT */
/* ***END PROLOGUE  DODRC */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...LOCAL ARRAYS */
/* ...EXTERNAL SUBROUTINES */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --sclb;
    --stpb;
    --ifixb;
    --beta;
    y_dim1 = *ldy;
    y_offset = 1 + y_dim1;
    y -= y_offset;
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    we_dim1 = *ldwe;
    we_dim2 = *ld2we;
    we_offset = 1 + we_dim1 * (1 + we_dim2);
    we -= we_offset;
    wd_dim1 = *ldwd;
    wd_dim2 = *ld2wd;
    wd_offset = 1 + wd_dim1 * (1 + wd_dim2);
    wd -= wd_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;
    stpd_dim1 = *ldstpd;
    stpd_offset = 1 + stpd_dim1;
    stpd -= stpd_offset;
    scld_dim1 = *ldscld;
    scld_offset = 1 + scld_dim1;
    scld -= scld_offset;
    --work;
    --iwork;

    /* Function Body */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:     THE USER-SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   INFO:    THE VARIABLE DESIGNATING WHY THE COMPUTATIONS WERE STOPPED. */
/*   IPRINT:  THE PRINT CONTROL VARIABLE. */
/*   IWORK:   THE INTEGER WORK SPACE. */
/*   JOB:     THE VARIABLE CONTROLLING PROBLEM INITIALIZATION AND */
/*            COMPUTATIONAL METHOD. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LDSCLD:  THE LEADING DIMENSION OF ARRAY SCLD. */
/*   LDSTPD:  THE LEADING DIMENSION OF ARRAY STPD. */
/*   LDWD:    THE LEADING DIMENSION OF ARRAY WD. */
/*   LDWE:    THE LEADING DIMENSION OF ARRAY WE. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   LDY:     THE LEADING DIMENSION OF ARRAY Y. */
/*   LD2WD:   THE SECOND DIMENSION OF ARRAY WD. */
/*   LD2WE:   THE SECOND DIMENSION OF ARRAY WE. */
/*   LIWORK:  THE LENGTH OF VECTOR IWORK. */
/*   LUNERR:  THE LOGICAL UNIT NUMBER FOR ERROR MESSAGES. */
/*   LUNRPT:  THE LOGICAL UNIT NUMBER FOR COMPUTATION REPORTS. */
/*   LWORK:   THE LENGTH OF VECTOR WORK. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   MAXIT:   THE MAXIMUM NUMBER OF ITERATIONS ALLOWED. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NDIGIT:  THE NUMBER OF ACCURATE DIGITS IN THE FUNCTION RESULTS, AS */
/*            SUPPLIED BY THE USER. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   PARTOL:  THE PARAMETER CONVERGENCE STOPPING TOLERANCE. */
/*   SCLB:    THE SCALING VALUES FOR BETA. */
/*   SCLD:    THE SCALING VALUES FOR DELTA. */
/*   STPB:    THE RELATIVE STEP FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO BETA. */
/*   STPD:    THE RELATIVE STEP FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO DELTA. */
/*   SHORT:   THE VARIABLE DESIGNATING WHETHER THE USER HAS INVOKED */
/*            ODRPACK BY THE SHORT-CALL (SHORT=.TRUE.) OR THE LONG-CALL */
/*            (SHORT=.FALSE.). */
/*   SSTOL:   THE SUM-OF-SQUARES CONVERGENCE STOPPING TOLERANCE. */
/*   TAUFAC:  THE FACTOR USED TO COMPUTE THE INITIAL TRUST REGION */
/*            DIAMETER. */
/*   WD:      THE DELTA WEIGHTS. */
/*   WD1:     A DUMMY ARRAY USED WHEN WD(1,1,1)=0.0D0. */
/*   WE:      THE EPSILON WEIGHTS. */
/*   WORK:    THE DOUBLE PRECISION WORK SPACE. */
/*   X:       THE EXPLANATORY VARIABLE. */
/*   Y:       THE DEPENDENT VARIABLE.  UNUSED WHEN THE MODEL IS IMPLICIT. */
/* ***FIRST EXECUTABLE STATEMENT  DODRC */
    short__ = FALSE_;
    if (wd[(wd_dim2 + 1) * wd_dim1 + 1] != zero) {
	dodcnt_(&short__, (U_fp)fcn, n, m, np, nq, &beta[1], &y[y_offset], 
		ldy, &x[x_offset], ldx, &we[we_offset], ldwe, ld2we, &wd[
		wd_offset], ldwd, ld2wd, &ifixb[1], &ifixx[ifixx_offset], 
		ldifx, job, ndigit, taufac, sstol, partol, maxit, iprint, 
		lunerr, lunrpt, &stpb[1], &stpd[stpd_offset], ldstpd, &sclb[1]
		, &scld[scld_offset], ldscld, &work[1], lwork, &iwork[1], 
		liwork, info);
    } else {
	wd1[0] = negone;
	dodcnt_(&short__, (U_fp)fcn, n, m, np, nq, &beta[1], &y[y_offset], 
		ldy, &x[x_offset], ldx, &we[we_offset], ldwe, ld2we, wd1, &
		c__1, &c__1, &ifixb[1], &ifixx[ifixx_offset], ldifx, job, 
		ndigit, taufac, sstol, partol, maxit, iprint, lunerr, lunrpt, 
		&stpb[1], &stpd[stpd_offset], ldstpd, &sclb[1], &scld[
		scld_offset], ldscld, &work[1], lwork, &iwork[1], liwork, 
		info);
    }
    return 0;
} /* dodrc_ */

/* DACCES */
/* Subroutine */ int dacces_(integer *n, integer *m, integer *np, integer *nq,
	 integer *ldwe, integer *ld2we, doublereal *work, integer *lwork, 
	integer *iwork, integer *liwork, logical *access, logical *isodr, 
	integer *jpvt, integer *omega, integer *u, integer *qraux, integer *
	sd, integer *vcv, integer *wrk1, integer *wrk2, integer *wrk3, 
	integer *wrk4, integer *wrk5, integer *wrk6, integer *nnzw, integer *
	npp, integer *job, doublereal *partol, doublereal *sstol, integer *
	maxit, doublereal *taufac, doublereal *eta, integer *neta, integer *
	lunrpt, integer *ipr1, integer *ipr2, integer *ipr2f, integer *ipr3, 
	doublereal *wss, doublereal *rvar, integer *idf, doublereal *tau, 
	doublereal *alpha, integer *niter, integer *nfev, integer *njev, 
	integer *int2, doublereal *olmavg, doublereal *rcond, integer *irank, 
	doublereal *actrs, doublereal *pnorm, doublereal *prers, doublereal *
	rnorms, integer *istop)
{
    static integer si, ti, ui, fni, sdi, fsi, ssi, tti, we1i, idfi, etai, 
	    jobi, msgb, msgd, epsi, taui, ssfi, nppi, vcvi, wssi, int2i, 
	    wrk1i, wrk2i, wrk3i, wrk4i, wrk5i, wrk6i, wrk7i, diffi, netai, 
	    nfevi;
    extern /* Subroutine */ int dwinf_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, logical *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *);
    static integer njevi, ldtti, rvari, ntoli, lwkmn, jpvti, nrowi, beta0i, 
	    nnzwi, fjacbi, fjacdi, betaci, alphai, betani, deltai, omegai, 
	    betasi, taufci, iranki, epsmai, deltni, rcondi;
    extern /* Subroutine */ int diwinf_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *);
    static integer deltsi, actrsi, olmavi, iprini, maxiti, niteri, partli, 
	    luneri, wssdei, liwkmn, pnormi, iprint, prersi, istopi, lunrpi, 
	    qrauxi, wssepi, rnorsi, sstoli, xplusi;

/* ***BEGIN PROLOGUE  DACCES */
/* ***REFER TO DODR,DODRC */
/* ***ROUTINES CALLED  DIWINF,DWINF */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  ACCESS OR STORE VALUES IN THE WORK ARRAYS */
/* ***END PROLOGUE  DACESS */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ACCESS:  THE VARIABLE DESIGNATING WHETHER INFORMATION IS TO BE */
/*            ACCESSED FROM THE WORK ARRAYS (ACCESS=TRUE) OR STORED IN */
/*            THEM (ACCESS=FALSE). */
/*   ACTRS:   THE SAVED ACTUAL RELATIVE REDUCTION IN THE SUM-OF-SQUARES. */
/*   ACTRSI:  THE LOCATION IN ARRAY WORK OF VARIABLE ACTRS. */
/*   ALPHA:   THE LEVENBERG-MARQUARDT PARAMETER. */
/*   ALPHAI:  THE LOCATION IN ARRAY WORK OF VARIABLE ALPHA. */
/*   BETACI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY BETAC. */
/*   BETANI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY BETAN. */
/*   BETASI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY BETAS. */
/*   BETA0I:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY BETA0. */
/*   DELTAI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY DELTA. */
/*   DELTNI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY DELTAN. */
/*   DELTSI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY DELTAS. */
/*   DIFFI:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY DIFF. */
/*   EPSI:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY EPS. */
/*   EPSMAI:  THE LOCATION IN ARRAY WORK OF VARIABLE EPSMAC. */
/*   ETA:     THE RELATIVE NOISE IN THE FUNCTION RESULTS. */
/*   ETAI:    THE LOCATION IN ARRAY WORK OF VARIABLE ETA. */
/*   FJACBI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY FJACB. */
/*   FJACDI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY FJACD. */
/*   FNI:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY FN. */
/*   FSI:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY FS. */
/*   IDF:     THE DEGREES OF FREEDOM OF THE FIT, EQUAL TO THE NUMBER OF */
/*            OBSERVATIONS WITH NONZERO WEIGHTED DERIVATIVES MINUS THE */
/*            NUMBER OF PARAMETERS BEING ESTIMATED. */
/*   IDFI:    THE STARTING LOCATION IN ARRAY IWORK OF VARIABLE IDF. */
/*   INT2:    THE NUMBER OF INTERNAL DOUBLING STEPS. */
/*   INT2I:   THE LOCATION IN ARRAY IWORK OF VARIABLE INT2. */
/*   IPR1:    THE VALUE OF THE FOURTH DIGIT (FROM THE RIGHT) OF IPRINT, */
/*            WHICH CONTROLS THE INITIAL SUMMARY REPORT. */
/*   IPR2:    THE VALUE OF THE THIRD DIGIT (FROM THE RIGHT) OF IPRINT, */
/*            WHICH CONTROLS THE ITERATION REPORTS. */
/*   IPR2F:   THE VALUE OF THE SECOND DIGIT (FROM THE RIGHT) OF IPRINT, */
/*            WHICH CONTROLS THE FREQUENCY OF THE ITERATION REPORTS. */
/*   IPR3:    THE VALUE OF THE FIRST DIGIT (FROM THE RIGHT) OF IPRINT, */
/*            WHICH CONTROLS THE FINAL SUMMARY REPORT. */
/*   IPRINI:  THE LOCATION IN ARRAY IWORK OF VARIABLE IPRINT. */
/*   IPRINT:  THE PRINT CONTROL VARIABLE. */
/*   IRANK:   THE RANK DEFICIENCY OF THE JACOBIAN WRT BETA. */
/*   IRANKI:  THE LOCATION IN ARRAY IWORK OF VARIABLE IRANK. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS TO BE */
/*            FOUND BY ODR (ISODR=TRUE) OR BY OLS (ISODR=FALSE). */
/*   ISTOP:   THE VARIABLE DESIGNATING WHETHER THERE ARE PROBLEMS */
/*            COMPUTING THE FUNCTION AT THE CURRENT BETA AND DELTA. */
/*   ISTOPI:  THE LOCATION IN ARRAY IWORK OF VARIABLE ISTOP. */
/*   IWORK:   THE INTEGER WORK SPACE. */
/*   JOB:     THE VARIABLE CONTROLING PROBLEM INITIALIZATION AND */
/*            COMPUTATIONAL METHOD. */
/*   JOBI:    THE LOCATION IN ARRAY IWORK OF VARIABLE JOB. */
/*   JPVT:    THE PIVOT VECTOR. */
/*   JPVTI:   THE STARTING LOCATION IN ARRAY IWORK OF VARIABLE JPVT. */
/*   LDTTI:   THE STARTING LOCATION IN ARRAY IWORK OF VARIABLE LDTT. */
/*   LDWE:    THE LEADING DIMENSION OF ARRAY WE. */
/*   LD2WE:   THE SECOND DIMENSION OF ARRAY WE. */
/*   LIWORK:  THE LENGTH OF VECTOR IWORK. */
/*   LUNERI:  THE LOCATION IN ARRAY IWORK OF VARIABLE LUNERR. */
/*   LUNERR:  THE LOGICAL UNIT NUMBER USED FOR ERROR MESSAGES. */
/*   LUNRPI:  THE LOCATION IN ARRAY IWORK OF VARIABLE LUNRPT. */
/*   LUNRPT:  THE LOGICAL UNIT NUMBER USED FOR COMPUTATION REPORTS. */
/*   LWKMN:   THE MINIMUM ACCEPTABLE LENGTH OF ARRAY WORK. */
/*   LWORK:   THE LENGTH OF VECTOR WORK. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   MAXIT:   THE MAXIMUM NUMBER OF ITERATIONS ALLOWED. */
/*   MAXITI:  THE LOCATION IN ARRAY IWORK OF VARIABLE MAXIT. */
/*   MSGB:    THE STARTING LOCATION IN ARRAY IWORK OF ARRAY MSGB. */
/*   MSGD:    THE STARTING LOCATION IN ARRAY IWORK OF ARRAY MSGD. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NETA:    THE NUMBER OF ACCURATE DIGITS IN THE FUNCTION RESULTS. */
/*   NETAI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NETA. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NFEVI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NFEV. */
/*   NITER:   THE NUMBER OF ITERATIONS TAKEN. */
/*   NITERI:  THE LOCATION IN ARRAY IWORK OF VARIABLE NITER. */
/*   NJEV:    THE NUMBER OF JACOBIAN EVALUATIONS. */
/*   NJEVI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NJEV. */
/*   NNZW:    THE NUMBER OF NONZERO WEIGHTED OBSERVATIONS. */
/*   NNZWI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NNZW. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NPP:     THE NUMBER OF FUNCTION PARAMETERS ACTUALLY ESTIMATED. */
/*   NPPI:    THE LOCATION IN ARRAY IWORK OF VARIABLE NPP. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   NROWI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NROW. */
/*   NTOLI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NTOL. */
/*   OLMAVG:  THE AVERAGE NUMBER OF LEVENBERG-MARQUARDT STEPS PER */
/*            ITERATION. */
/*   OLMAVI:  THE LOCATION IN ARRAY WORK OF VARIABLE OLMAVG. */
/*   OMEGA:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY OMEGA. */
/*   OMEGAI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY OMEGA. */
/*   PARTLI:  THE LOCATION IN ARRAY WORK OF VARIABLE PARTOL. */
/*   PARTOL:  THE PARAMETER CONVERGENCE STOPPING TOLERANCE. */
/*   PNORM:   THE NORM OF THE SCALED ESTIMATED PARAMETERS. */
/*   PNORMI:  THE LOCATION IN ARRAY WORK OF VARIABLE PNORM. */
/*   PRERS:   THE SAVED PREDICTED RELATIVE REDUCTION IN THE */
/*            SUM-OF-SQUARES. */
/*   PRERSI:  THE LOCATION IN ARRAY WORK OF VARIABLE PRERS. */
/*   QRAUX:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY QRAUX. */
/*   QRAUXI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY QRAUX. */
/*   RCOND:   THE APPROXIMATE RECIPROCAL CONDITION OF FJACB. */
/*   RCONDI:  THE LOCATION IN ARRAY WORK OF VARIABLE RCOND. */
/*   RESTRT:  THE VARIABLE DESIGNATING WHETHER THE CALL IS A RESTART */
/*            (RESTRT=TRUE) OR NOT (RESTRT=FALSE). */
/*   RNORMS:  THE NORM OF THE SAVED WEIGHTED EPSILONS AND DELTAS. */
/*   RNORSI:  THE LOCATION IN ARRAY WORK OF VARIABLE RNORMS. */
/*   RVAR:    THE RESIDUAL VARIANCE, I.E. STANDARD DEVIATION SQUARED. */
/*   RVARI:   THE LOCATION IN ARRAY WORK OF VARIABLE RVAR. */
/*   SCLB:    THE SCALING VALUES USED FOR BETA. */
/*   SCLD:    THE SCALING VALUES USED FOR DELTA. */
/*   SD:      THE STARTING LOCATION IN ARRAY WORK OF ARRAY SD. */
/*   SDI:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY SD. */
/*   SHORT:   THE VARIABLE DESIGNATING WHETHER THE USER HAS INVOKED */
/*            ODRPACK BY THE SHORT-CALL (SHORT=TRUE) OR THE LONG- */
/*            CALL (SHORT=FALSE). */
/*   SI:      THE STARTING LOCATION IN ARRAY WORK OF ARRAY S. */
/*   SSFI:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY SSF. */
/*   SSI:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY SS. */
/*   SSTOL:   THE SUM-OF-SQUARES CONVERGENCE STOPPING TOLERANCE. */
/*   SSTOLI:  THE LOCATION IN ARRAY WORK OF VARIABLE SSTOL. */
/*   TAU:     THE TRUST REGION DIAMETER. */
/*   TAUFAC:  THE FACTOR USED TO COMPUTE THE INITIAL TRUST REGION */
/*            DIAMETER. */
/*   TAUFCI:  THE LOCATION IN ARRAY WORK OF VARIABLE TAUFAC. */
/*   TAUI:    THE LOCATION IN ARRAY WORK OF VARIABLE TAU. */
/*   TI:      THE STARTING LOCATION IN ARRAY WORK OF ARRAY T. */
/*   TTI:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY TT. */
/*   U:       THE STARTING LOCATION IN ARRAY WORK OF ARRAY U. */
/*   UI:      THE STARTING LOCATION IN ARRAY WORK OF ARRAY U. */
/*   VCV:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY VCV. */
/*   VCVI:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY VCV. */
/*   WE1I:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY WE1. */
/*   WORK:    THE DOUBLE PRECISION WORK SPACE. */
/*   WRK1:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK1. */
/*   WRK1I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK1. */
/*   WRK2:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK2. */
/*   WRK2I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK2. */
/*   WRK3:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK3. */
/*   WRK3I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK3. */
/*   WRK4:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK4. */
/*   WRK4I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK4. */
/*   WRK5:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK5. */
/*   WRK5I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK5. */
/*   WRK6:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK6. */
/*   WRK6I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK6. */
/*   WRK7I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK7. */
/*   WSS:     THE SUM OF THE SQUARES OF THE WEIGHTED EPSILONS AND DELTAS, */
/*            THE SUM OF THE SQUARES OF THE WEIGHTED DELTAS, AND */
/*            THE SUM OF THE SQUARES OF THE WEIGHTED EPSILONS. */
/*   WSSI:    THE STARTING LOCATION IN ARRAY WORK OF VARIABLE WSS(1). */
/*   WSSDEI:  THE STARTING LOCATION IN ARRAY WORK OF VARIABLE WSS(2). */
/*   WSSEPI:  THE STARTING LOCATION IN ARRAY WORK OF VARIABLE WSS(3). */
/*   XPLUSI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY XPLUSD. */
/* ***FIRST EXECUTABLE STATEMENT  DACCES */
/*  FIND STARTING LOCATIONS WITHIN INTEGER WORKSPACE */
    /* Parameter adjustments */
    --work;
    --iwork;
    --wss;

    /* Function Body */
    diwinf_(m, np, nq, &msgb, &msgd, &jpvti, &istopi, &nnzwi, &nppi, &idfi, &
	    jobi, &iprini, &luneri, &lunrpi, &nrowi, &ntoli, &netai, &maxiti, 
	    &niteri, &nfevi, &njevi, &int2i, &iranki, &ldtti, &liwkmn);
/*  FIND STARTING LOCATIONS WITHIN DOUBLE PRECISION WORK SPACE */
    dwinf_(n, m, np, nq, ldwe, ld2we, isodr, &deltai, &epsi, &xplusi, &fni, &
	    sdi, &vcvi, &rvari, &wssi, &wssdei, &wssepi, &rcondi, &etai, &
	    olmavi, &taui, &alphai, &actrsi, &pnormi, &rnorsi, &prersi, &
	    partli, &sstoli, &taufci, &epsmai, &beta0i, &betaci, &betasi, &
	    betani, &si, &ssi, &ssfi, &qrauxi, &ui, &fsi, &fjacbi, &we1i, &
	    diffi, &deltsi, &deltni, &ti, &tti, &omegai, &fjacdi, &wrk1i, &
	    wrk2i, &wrk3i, &wrk4i, &wrk5i, &wrk6i, &wrk7i, &lwkmn);
    if (*access) {
/*  SET STARTING LOCATIONS FOR WORK VECTORS */
	*jpvt = jpvti;
	*omega = omegai;
	*qraux = qrauxi;
	*sd = sdi;
	*vcv = vcvi;
	*u = ui;
	*wrk1 = wrk1i;
	*wrk2 = wrk2i;
	*wrk3 = wrk3i;
	*wrk4 = wrk4i;
	*wrk5 = wrk5i;
	*wrk6 = wrk6i;
/*  ACCESS VALUES FROM THE WORK VECTORS */
	*actrs = work[actrsi];
	*alpha = work[alphai];
	*eta = work[etai];
	*olmavg = work[olmavi];
	*partol = work[partli];
	*pnorm = work[pnormi];
	*prers = work[prersi];
	*rcond = work[rcondi];
	wss[1] = work[wssi];
	wss[2] = work[wssdei];
	wss[3] = work[wssepi];
	*rvar = work[rvari];
	*rnorms = work[rnorsi];
	*sstol = work[sstoli];
	*tau = work[taui];
	*taufac = work[taufci];
	*neta = iwork[netai];
	*irank = iwork[iranki];
	*job = iwork[jobi];
	*lunrpt = iwork[lunrpi];
	*maxit = iwork[maxiti];
	*nfev = iwork[nfevi];
	*niter = iwork[niteri];
	*njev = iwork[njevi];
	*nnzw = iwork[nnzwi];
	*npp = iwork[nppi];
	*idf = iwork[idfi];
	*int2 = iwork[int2i];
/*  SET UP PRINT CONTROL VARIABLES */
	iprint = iwork[iprini];
	*ipr1 = iprint % 10000 / 1000;
	*ipr2 = iprint % 1000 / 100;
	*ipr2f = iprint % 100 / 10;
	*ipr3 = iprint % 10;
    } else {
/*  STORE VALUES INTO THE WORK VECTORS */
	work[actrsi] = *actrs;
	work[alphai] = *alpha;
	work[olmavi] = *olmavg;
	work[partli] = *partol;
	work[pnormi] = *pnorm;
	work[prersi] = *prers;
	work[rcondi] = *rcond;
	work[wssi] = wss[1];
	work[wssdei] = wss[2];
	work[wssepi] = wss[3];
	work[rvari] = *rvar;
	work[rnorsi] = *rnorms;
	work[sstoli] = *sstol;
	work[taui] = *tau;
	iwork[iranki] = *irank;
	iwork[istopi] = *istop;
	iwork[nfevi] = *nfev;
	iwork[niteri] = *niter;
	iwork[njevi] = *njev;
	iwork[idfi] = *idf;
	iwork[int2i] = *int2;
    }
    return 0;
} /* dacces_ */

/* DESUBI */
/* Subroutine */ int desubi_(integer *n, integer *m, doublereal *wd, integer *
	ldwd, integer *ld2wd, doublereal *alpha, doublereal *tt, integer *
	ldtt, integer *i__, doublereal *e)
{
    /* Initialized data */

    static doublereal zero = 0.;

    /* System generated locals */
    integer e_dim1, e_offset, tt_dim1, tt_offset, wd_dim1, wd_dim2, wd_offset,
	     i__1, i__2;
    doublereal d__1, d__2;

    /* Local variables */
    static integer j, j1, j2;
    extern /* Subroutine */ int dzero_(integer *, integer *, doublereal *, 
	    integer *);

/* ***BEGIN PROLOGUE  DESUBI */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DZERO */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  COMPUTE E = WD + ALPHA*TT**2 */
/* ***END PROLOGUE  DESUBI */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    e_dim1 = *m;
    e_offset = 1 + e_dim1;
    e -= e_offset;
    wd_dim1 = *ldwd;
    wd_dim2 = *ld2wd;
    wd_offset = 1 + wd_dim1 * (1 + wd_dim2);
    wd -= wd_offset;
    tt_dim1 = *ldtt;
    tt_offset = 1 + tt_dim1;
    tt -= tt_offset;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ALPHA:  THE LEVENBERG-MARQUARDT PARAMETER. */
/*   E:      THE VALUE OF THE ARRAY E = WD + ALPHA*TT**2 */
/*   I:      AN INDEXING VARIABLE. */
/*   J:      AN INDEXING VARIABLE. */
/*   J1:     AN INDEXING VARIABLE. */
/*   J2:     AN INDEXING VARIABLE. */
/*   LDWD:   THE LEADING DIMENSION OF ARRAY WD. */
/*   LD2WD:  THE SECOND DIMENSION OF ARRAY WD. */
/*   M:      THE NUMBER OF COLUMNS OF DATA IN THE INDEPENDENT VARIABLE. */
/*   N:      THE NUMBER OF OBSERVATIONS. */
/*   NP:     THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   TT:     THE SCALING VALUES USED FOR DELTA. */
/*   WD:     THE SQUARED DELTA WEIGHTS, D**2. */
/*   ZERO:   THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DESUBI */
/*   N.B. THE LOCATIONS OF WD AND TT ACCESSED DEPEND ON THE VALUE */
/*        OF THE FIRST ELEMENT OF EACH ARRAY AND THE LEADING DIMENSIONS */
/*        OF THE MULTIPLY SUBSCRIPTED ARRAYS. */
    if (*n == 0 || *m == 0) {
	return 0;
    }
    if (wd[(wd_dim2 + 1) * wd_dim1 + 1] >= zero) {
	if (*ldwd >= *n) {
/*  THE ELEMENTS OF WD HAVE BEEN INDIVIDUALLY SPECIFIED */
	    if (*ld2wd == 1) {
/*  THE ARRAYS STORED IN WD ARE DIAGONAL */
		dzero_(m, m, &e[e_offset], m);
		i__1 = *m;
		for (j = 1; j <= i__1; ++j) {
		    e[j + j * e_dim1] = wd[*i__ + (j * wd_dim2 + 1) * wd_dim1]
			    ;
/* L10: */
		}
	    } else {
/*  THE ARRAYS STORED IN WD ARE FULL POSITIVE SEMIDEFINITE MATRICES */
		i__1 = *m;
		for (j1 = 1; j1 <= i__1; ++j1) {
		    i__2 = *m;
		    for (j2 = 1; j2 <= i__2; ++j2) {
			e[j1 + j2 * e_dim1] = wd[*i__ + (j1 + j2 * wd_dim2) * 
				wd_dim1];
/* L20: */
		    }
/* L30: */
		}
	    }
	    if (tt[tt_dim1 + 1] > zero) {
		if (*ldtt >= *n) {
		    i__1 = *m;
		    for (j = 1; j <= i__1; ++j) {
/* Computing 2nd power */
			d__1 = tt[*i__ + j * tt_dim1];
			e[j + j * e_dim1] += *alpha * (d__1 * d__1);
/* L110: */
		    }
		} else {
		    i__1 = *m;
		    for (j = 1; j <= i__1; ++j) {
/* Computing 2nd power */
			d__1 = tt[j * tt_dim1 + 1];
			e[j + j * e_dim1] += *alpha * (d__1 * d__1);
/* L120: */
		    }
		}
	    } else {
		i__1 = *m;
		for (j = 1; j <= i__1; ++j) {
/* Computing 2nd power */
		    d__1 = tt[tt_dim1 + 1];
		    e[j + j * e_dim1] += *alpha * (d__1 * d__1);
/* L130: */
		}
	    }
	} else {
/*  WD IS AN M BY M MATRIX */
	    if (*ld2wd == 1) {
/*  THE ARRAY STORED IN WD IS DIAGONAL */
		dzero_(m, m, &e[e_offset], m);
		i__1 = *m;
		for (j = 1; j <= i__1; ++j) {
		    e[j + j * e_dim1] = wd[(j * wd_dim2 + 1) * wd_dim1 + 1];
/* L140: */
		}
	    } else {
/*  THE ARRAY STORED IN WD IS A FULL POSITIVE SEMIDEFINITE MATRICES */
		i__1 = *m;
		for (j1 = 1; j1 <= i__1; ++j1) {
		    i__2 = *m;
		    for (j2 = 1; j2 <= i__2; ++j2) {
			e[j1 + j2 * e_dim1] = wd[(j1 + j2 * wd_dim2) * 
				wd_dim1 + 1];
/* L150: */
		    }
/* L160: */
		}
	    }
	    if (tt[tt_dim1 + 1] > zero) {
		if (*ldtt >= *n) {
		    i__1 = *m;
		    for (j = 1; j <= i__1; ++j) {
/* Computing 2nd power */
			d__1 = tt[*i__ + j * tt_dim1];
			e[j + j * e_dim1] += *alpha * (d__1 * d__1);
/* L210: */
		    }
		} else {
		    i__1 = *m;
		    for (j = 1; j <= i__1; ++j) {
/* Computing 2nd power */
			d__1 = tt[j * tt_dim1 + 1];
			e[j + j * e_dim1] += *alpha * (d__1 * d__1);
/* L220: */
		    }
		}
	    } else {
		i__1 = *m;
		for (j = 1; j <= i__1; ++j) {
/* Computing 2nd power */
		    d__1 = tt[tt_dim1 + 1];
		    e[j + j * e_dim1] += *alpha * (d__1 * d__1);
/* L230: */
		}
	    }
	}
    } else {
/*  WD IS A DIAGONAL MATRIX WITH ELEMENTS ABS(WD(1,1,1)) */
	dzero_(m, m, &e[e_offset], m);
	if (tt[tt_dim1 + 1] > zero) {
	    if (*ldtt >= *n) {
		i__1 = *m;
		for (j = 1; j <= i__1; ++j) {
/* Computing 2nd power */
		    d__2 = tt[*i__ + j * tt_dim1];
		    e[j + j * e_dim1] = (d__1 = wd[(wd_dim2 + 1) * wd_dim1 + 
			    1], abs(d__1)) + *alpha * (d__2 * d__2);
/* L310: */
		}
	    } else {
		i__1 = *m;
		for (j = 1; j <= i__1; ++j) {
/* Computing 2nd power */
		    d__2 = tt[j * tt_dim1 + 1];
		    e[j + j * e_dim1] = (d__1 = wd[(wd_dim2 + 1) * wd_dim1 + 
			    1], abs(d__1)) + *alpha * (d__2 * d__2);
/* L320: */
		}
	    }
	} else {
	    i__1 = *m;
	    for (j = 1; j <= i__1; ++j) {
/* Computing 2nd power */
		d__2 = tt[tt_dim1 + 1];
		e[j + j * e_dim1] = (d__1 = wd[(wd_dim2 + 1) * wd_dim1 + 1], 
			abs(d__1)) + *alpha * (d__2 * d__2);
/* L330: */
	    }
	}
    }
    return 0;
} /* desubi_ */

/* DETAF */
/* Subroutine */ int detaf_(S_fp fcn, integer *n, integer *m, integer *np, 
	integer *nq, doublereal *xplusd, doublereal *beta, doublereal *epsmac,
	 integer *nrow, doublereal *partmp, doublereal *pv0, integer *ifixb, 
	integer *ifixx, integer *ldifx, integer *istop, integer *nfev, 
	doublereal *eta, integer *neta, doublereal *wrk1, doublereal *wrk2, 
	doublereal *wrk6, doublereal *wrk7)
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal p1 = .1;
    static doublereal p2 = .2;
    static doublereal p5 = .5;
    static doublereal one = 1.;
    static doublereal two = 2.;
    static doublereal hundrd = 100.;

    /* System generated locals */
    integer pv0_dim1, pv0_offset, wrk1_dim1, wrk1_dim2, wrk1_offset, 
	    wrk2_dim1, wrk2_offset, wrk6_dim1, wrk6_dim2, wrk6_offset, 
	    xplusd_dim1, xplusd_offset, ifixx_dim1, ifixx_offset, i__1;
    doublereal d__1, d__2;

    /* Builtin functions */
    double d_lg10(doublereal *);

    /* Local variables */
    static doublereal a, b;
    static integer j, k, l;
    static doublereal fac, stp;

/* ***BEGIN PROLOGUE  DETAF */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  FCN */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  COMPUTE NOISE AND NUMBER OF GOOD DIGITS IN FUNCTION RESULTS */
/*            (ADAPTED FROM STARPAC SUBROUTINE ETAFUN) */
/* ***END PROLOGUE  DETAF */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    xplusd_dim1 = *n;
    xplusd_offset = 1 + xplusd_dim1;
    xplusd -= xplusd_offset;
    --ifixb;
    --partmp;
    --beta;
    wrk7 -= 3;
    wrk6_dim1 = *n;
    wrk6_dim2 = *np;
    wrk6_offset = 1 + wrk6_dim1 * (1 + wrk6_dim2);
    wrk6 -= wrk6_offset;
    wrk2_dim1 = *n;
    wrk2_offset = 1 + wrk2_dim1;
    wrk2 -= wrk2_offset;
    wrk1_dim1 = *n;
    wrk1_dim2 = *m;
    wrk1_offset = 1 + wrk1_dim1 * (1 + wrk1_dim2);
    wrk1 -= wrk1_offset;
    pv0_dim1 = *n;
    pv0_offset = 1 + pv0_dim1;
    pv0 -= pv0_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;

    /* Function Body */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:      THE USER SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   A:       PARAMETERS OF THE LOCAL FIT. */
/*   B:       PARAMETERS OF THE LOCAL FIT. */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   EPSMAC:  THE VALUE OF MACHINE PRECISION. */
/*   ETA:     THE NOISE IN THE MODEL RESULTS. */
/*   FAC:     A FACTOR USED IN THE COMPUTATIONS. */
/*   HUNDRD:  THE VALUE 1.0D2. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   ISTOP:   THE VARIABLE DESIGNATING WHETHER THERE ARE PROBLEMS */
/*            COMPUTING THE FUNCTION AT THE CURRENT BETA AND DELTA. */
/*   J:       AN INDEX VARIABLE. */
/*   K:       AN INDEX VARIABLE. */
/*   L:       AN INDEX VARIABLE. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NETA:    THE NUMBER OF ACCURATE DIGITS IN THE MODEL RESULTS. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   NROW:    THE ROW NUMBER AT WHICH THE DERIVATIVE IS TO BE CHECKED. */
/*   ONE:     THE VALUE 1.0D0. */
/*   P1:      THE VALUE 0.1D0. */
/*   P2:      THE VALUE 0.2D0. */
/*   P5:      THE VALUE 0.5D0. */
/*   PARTMP:  THE MODEL PARAMETERS. */
/*   PV0:     THE ORIGINAL PREDICTED VALUES. */
/*   STP:     A SMALL VALUE USED TO PERTURB THE PARAMETERS. */
/*   WRK1:    A WORK ARRAY OF (N BY M BY NQ) ELEMENTS. */
/*   WRK2:    A WORK ARRAY OF (N BY NQ) ELEMENTS. */
/*   WRK6:    A WORK ARRAY OF (N BY NP BY NQ) ELEMENTS. */
/*   WRK7:    A WORK ARRAY OF (5 BY NQ) ELEMENTS. */
/*   XPLUSD:  THE VALUES OF X + DELTA. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DETAF */
    stp = hundrd * *epsmac;
    *eta = *epsmac;
    for (j = -2; j <= 2; ++j) {
	if (j == 0) {
	    i__1 = *nq;
	    for (l = 1; l <= i__1; ++l) {
		wrk7[j + l * 5] = pv0[*nrow + l * pv0_dim1];
/* L10: */
	    }
	} else {
	    i__1 = *np;
	    for (k = 1; k <= i__1; ++k) {
		if (ifixb[1] < 0) {
		    partmp[k] = beta[k] + j * stp * beta[k];
		} else if (ifixb[k] != 0) {
		    partmp[k] = beta[k] + j * stp * beta[k];
		} else {
		    partmp[k] = beta[k];
		}
/* L20: */
	    }
	    *istop = 0;
	    (*fcn)(n, m, np, nq, n, m, np, &partmp[1], &xplusd[xplusd_offset],
		     &ifixb[1], &ifixx[ifixx_offset], ldifx, &c__3, &wrk2[
		    wrk2_offset], &wrk6[wrk6_offset], &wrk1[wrk1_offset], 
		    istop);
	    if (*istop != 0) {
		return 0;
	    } else {
		++(*nfev);
	    }
	    i__1 = *nq;
	    for (l = 1; l <= i__1; ++l) {
		wrk7[j + l * 5] = wrk2[*nrow + l * wrk2_dim1];
/* L30: */
	    }
	}
/* L40: */
    }
    i__1 = *nq;
    for (l = 1; l <= i__1; ++l) {
	a = zero;
	b = zero;
	for (j = -2; j <= 2; ++j) {
	    a += wrk7[j + l * 5];
	    b += j * wrk7[j + l * 5];
/* L50: */
	}
	a = p2 * a;
	b = p1 * b;
	if (wrk7[l * 5] != zero && (d__1 = wrk7[l * 5 + 1] + wrk7[l * 5 - 1], 
		abs(d__1)) > hundrd * *epsmac) {
	    fac = one / (d__1 = wrk7[l * 5], abs(d__1));
	} else {
	    fac = one;
	}
	for (j = -2; j <= 2; ++j) {
	    wrk7[j + l * 5] = (d__1 = (wrk7[j + l * 5] - (a + j * b)) * fac, 
		    abs(d__1));
/* Computing MAX */
	    d__1 = wrk7[j + l * 5];
	    *eta = max(d__1,*eta);
/* L60: */
	}
/* L100: */
    }
/* Computing MAX */
    d__1 = two, d__2 = p5 - d_lg10(eta);
    *neta = (integer) max(d__1,d__2);
    return 0;
} /* detaf_ */

/* DEVJAC */
/* Subroutine */ int devjac_(S_fp fcn, logical *anajac, logical *cdjac, 
	integer *n, integer *m, integer *np, integer *nq, doublereal *betac, 
	doublereal *beta, doublereal *stpb, integer *ifixb, integer *ifixx, 
	integer *ldifx, doublereal *x, integer *ldx, doublereal *delta, 
	doublereal *xplusd, doublereal *stpd, integer *ldstpd, doublereal *
	ssf, doublereal *tt, integer *ldtt, integer *neta, doublereal *fn, 
	doublereal *stp, doublereal *wrk1, doublereal *wrk2, doublereal *wrk3,
	 doublereal *wrk6, doublereal *fjacb, logical *isodr, doublereal *
	fjacd, doublereal *we1, integer *ldwe, integer *ld2we, integer *njev, 
	integer *nfev, integer *istop, integer *info)
{
    /* Initialized data */

    static doublereal zero = 0.;

    /* System generated locals */
    integer delta_dim1, delta_offset, fjacb_dim1, fjacb_dim2, fjacb_offset, 
	    fjacd_dim1, fjacd_dim2, fjacd_offset, fn_dim1, fn_offset, 
	    stpd_dim1, stpd_offset, tt_dim1, tt_offset, we1_dim1, we1_dim2, 
	    we1_offset, wrk1_dim1, wrk1_dim2, wrk1_offset, wrk2_dim1, 
	    wrk2_offset, wrk6_dim1, wrk6_dim2, wrk6_offset, x_dim1, x_offset, 
	    xplusd_dim1, xplusd_offset, ifixx_dim1, ifixx_offset, i__1, i__2, 
	    i__3;

    /* Local variables */
    static integer j, k, l, k1;
    extern doublereal ddot_(integer *, doublereal *, integer *, doublereal *, 
	    integer *);
    extern /* Subroutine */ int dxpy_(integer *, integer *, doublereal *, 
	    integer *, doublereal *, integer *, doublereal *, integer *), 
	    difix_(integer *, integer *, integer *, integer *, doublereal *, 
	    integer *, doublereal *, integer *), dwght_(integer *, integer *, 
	    doublereal *, integer *, integer *, doublereal *, integer *, 
	    doublereal *, integer *);
    static logical error;
    extern /* Subroutine */ int djaccd_(S_fp, integer *, integer *, integer *,
	     integer *, doublereal *, doublereal *, integer *, doublereal *, 
	    doublereal *, integer *, integer *, integer *, doublereal *, 
	    doublereal *, integer *, doublereal *, doublereal *, integer *, 
	    integer *, doublereal *, doublereal *, doublereal *, doublereal *,
	     doublereal *, doublereal *, logical *, doublereal *, integer *, 
	    integer *), djacfd_(S_fp, integer *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, doublereal *, 
	    doublereal *, integer *, integer *, integer *, doublereal *, 
	    doublereal *, integer *, doublereal *, doublereal *, integer *, 
	    integer *, doublereal *, doublereal *, doublereal *, doublereal *,
	     doublereal *, doublereal *, doublereal *, logical *, doublereal *
	    , integer *, integer *);
    static integer ideval;
    extern /* Subroutine */ int dunpac_(integer *, doublereal *, doublereal *,
	     integer *);

/* ***BEGIN PROLOGUE  DEVJAC */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  FCN,DDOT,DIFIX,DJACCD,DJACFD,DWGHT,DUNPAC,DXPY */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  COMPUTE THE WEIGHTED JACOBIANS WRT BETA AND DELTA */
/* ***END PROLOGUE  DEVJAC */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...EXTERNAL FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --stp;
    xplusd_dim1 = *n;
    xplusd_offset = 1 + xplusd_dim1;
    xplusd -= xplusd_offset;
    delta_dim1 = *n;
    delta_offset = 1 + delta_dim1;
    delta -= delta_offset;
    --wrk3;
    --ssf;
    --ifixb;
    --stpb;
    --beta;
    --betac;
    fjacd_dim1 = *n;
    fjacd_dim2 = *m;
    fjacd_offset = 1 + fjacd_dim1 * (1 + fjacd_dim2);
    fjacd -= fjacd_offset;
    fjacb_dim1 = *n;
    fjacb_dim2 = *np;
    fjacb_offset = 1 + fjacb_dim1 * (1 + fjacb_dim2);
    fjacb -= fjacb_offset;
    wrk6_dim1 = *n;
    wrk6_dim2 = *np;
    wrk6_offset = 1 + wrk6_dim1 * (1 + wrk6_dim2);
    wrk6 -= wrk6_offset;
    wrk2_dim1 = *n;
    wrk2_offset = 1 + wrk2_dim1;
    wrk2 -= wrk2_offset;
    wrk1_dim1 = *n;
    wrk1_dim2 = *m;
    wrk1_offset = 1 + wrk1_dim1 * (1 + wrk1_dim2);
    wrk1 -= wrk1_offset;
    fn_dim1 = *n;
    fn_offset = 1 + fn_dim1;
    fn -= fn_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    stpd_dim1 = *ldstpd;
    stpd_offset = 1 + stpd_dim1;
    stpd -= stpd_offset;
    tt_dim1 = *ldtt;
    tt_offset = 1 + tt_dim1;
    tt -= tt_offset;
    we1_dim1 = *ldwe;
    we1_dim2 = *ld2we;
    we1_offset = 1 + we1_dim1 * (1 + we1_dim2);
    we1 -= we1_offset;

    /* Function Body */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:     THE USER-SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ANAJAC:  THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE */
/*            COMPUTED BY FINITE DIFFERENCES (ANAJAC=FALSE) OR NOT */
/*            (ANAJAC=TRUE). */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   BETAC:   THE CURRENT ESTIMATED VALUES OF THE UNFIXED BETA'S. */
/*   CDJAC:   THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE */
/*            COMPUTED BY CENTRAL DIFFERENCES (CDJAC=TRUE) OR BY FORWARD */
/*            DIFFERENCES (CDJAC=FALSE). */
/*   DELTA:   THE ESTIMATED VALUES OF DELTA. */
/*   ERROR:   THE VARIABLE DESIGNATING WHETHER ODRPACK DETECTED NONZERO */
/*            VALUES IN ARRAY DELTA IN THE OLS CASE, AND THUS WHETHER */
/*            THE USER MAY HAVE OVERWRITTEN IMPORTANT INFORMATION */
/*            BY COMPUTING FJACD IN THE OLS CASE. */
/*   FJACB:   THE JACOBIAN WITH RESPECT TO BETA. */
/*   FJACD:   THE JACOBIAN WITH RESPECT TO DELTA. */
/*   FN:      THE PREDICTED VALUES OF THE FUNCTION AT THE CURRENT POINT. */
/*   IDEVAL:  THE VARIABLE DESIGNATING WHAT COMPUTATIONS ARE TO BE */
/*            PERFORMED BY USER-SUPPLIED SUBROUTINE FCN. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF DELTA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   INFO:    THE VARIABLE DESIGNATING WHY THE COMPUTATIONS WERE STOPPED. */
/*   ISTOP:   THE VARIABLE DESIGNATING THAT THE USER WISHES THE */
/*            COMPUTATIONS STOPPED. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR OLS (ISODR=FALSE). */
/*   J:       AN INDEXING VARIABLE. */
/*   K:       AN INDEXING VARIABLE. */
/*   K1:      AN INDEXING VARIABLE. */
/*   L:       AN INDEXING VARIABLE. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LDSTPD:  THE LEADING DIMENSION OF ARRAY STPD. */
/*   LDTT:    THE LEADING DIMENSION OF ARRAY TT. */
/*   LDWE:    THE LEADING DIMENSION OF ARRAYS WE AND WE1. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   LD2WE:   THE SECOND DIMENSION OF ARRAYS WE AND WE1. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE INDEPENDENT VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NETA:    THE NUMBER OF ACCURATE DIGITS IN THE FUNCTION RESULTS. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NJEV:    THE NUMBER OF JACOBIAN EVALUATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   SSF:     THE SCALE USED FOR THE BETA'S. */
/*   STP:     THE STEP USED FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO DELTA. */
/*   STPB:    THE RELATIVE STEP USED FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO BETA. */
/*   STPD:    THE RELATIVE STEP USED FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO DELTA. */
/*   TT:      THE SCALING VALUES USED FOR DELTA. */
/*   WE1:     THE SQUARE ROOTS OF THE EPSILON WEIGHTS IN ARRAY WE. */
/*   WRK1:    A WORK ARRAY OF (N BY M BY NQ) ELEMENTS. */
/*   WRK2:    A WORK ARRAY OF (N BY NQ) ELEMENTS. */
/*   WRK3:    A WORK ARRAY OF (NP) ELEMENTS. */
/*   WRK6:    A WORK ARRAY OF (N BY NP BY NQ) ELEMENTS. */
/*   X:       THE INDEPENDENT VARIABLE. */
/*   XPLUSD:  THE VALUES OF X + DELTA. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DEVJAC */
/*  INSERT CURRENT UNFIXED BETA ESTIMATES INTO BETA */
    dunpac_(np, &betac[1], &beta[1], &ifixb[1]);
/*  COMPUTE XPLUSD = X + DELTA */
    dxpy_(n, m, &x[x_offset], ldx, &delta[delta_offset], n, &xplusd[
	    xplusd_offset], n);
/*  COMPUTE THE JACOBIAN WRT THE ESTIMATED BETAS (FJACB) AND */
/*          THE JACOBIAN WRT DELTA (FJACD) */
    *istop = 0;
    if (*isodr) {
	ideval = 110;
    } else {
	ideval = 10;
    }
    if (*anajac) {
	(*fcn)(n, m, np, nq, n, m, np, &beta[1], &xplusd[xplusd_offset], &
		ifixb[1], &ifixx[ifixx_offset], ldifx, &ideval, &wrk2[
		wrk2_offset], &fjacb[fjacb_offset], &fjacd[fjacd_offset], 
		istop);
	if (*istop != 0) {
	    return 0;
	} else {
	    ++(*njev);
	}
/*  MAKE SURE FIXED ELEMENTS OF FJACD ARE ZERO */
	if (*isodr) {
	    i__1 = *nq;
	    for (l = 1; l <= i__1; ++l) {
		difix_(n, m, &ifixx[ifixx_offset], ldifx, &fjacd[(l * 
			fjacd_dim2 + 1) * fjacd_dim1 + 1], n, &fjacd[(l * 
			fjacd_dim2 + 1) * fjacd_dim1 + 1], n);
/* L10: */
	    }
	}
    } else if (*cdjac) {
	djaccd_((S_fp)fcn, n, m, np, nq, &beta[1], &x[x_offset], ldx, &delta[
		delta_offset], &xplusd[xplusd_offset], &ifixb[1], &ifixx[
		ifixx_offset], ldifx, &stpb[1], &stpd[stpd_offset], ldstpd, &
		ssf[1], &tt[tt_offset], ldtt, neta, &stp[1], &wrk1[
		wrk1_offset], &wrk2[wrk2_offset], &wrk3[1], &wrk6[wrk6_offset]
		, &fjacb[fjacb_offset], isodr, &fjacd[fjacd_offset], nfev, 
		istop);
    } else {
	djacfd_((S_fp)fcn, n, m, np, nq, &beta[1], &x[x_offset], ldx, &delta[
		delta_offset], &xplusd[xplusd_offset], &ifixb[1], &ifixx[
		ifixx_offset], ldifx, &stpb[1], &stpd[stpd_offset], ldstpd, &
		ssf[1], &tt[tt_offset], ldtt, neta, &fn[fn_offset], &stp[1], &
		wrk1[wrk1_offset], &wrk2[wrk2_offset], &wrk3[1], &wrk6[
		wrk6_offset], &fjacb[fjacb_offset], isodr, &fjacd[
		fjacd_offset], nfev, istop);
    }
    if (*istop < 0) {
	return 0;
    } else if (! (*isodr)) {
/*  TRY TO DETECT WHETHER THE USER HAS COMPUTED JFACD */
/*  WITHIN FCN IN THE OLS CASE */
	i__1 = *n * *m;
	error = ddot_(&i__1, &delta[delta_offset], &c__1, &delta[delta_offset]
		, &c__1) != zero;
	if (error) {
	    *info = 50300;
	    return 0;
	}
    }
/*  WEIGHT THE JACOBIAN WRT THE ESTIMATED BETAS */
    if (ifixb[1] < 0) {
	i__1 = *np;
	for (k = 1; k <= i__1; ++k) {
	    i__2 = *n * *np;
	    i__3 = *n * *np;
	    dwght_(n, nq, &we1[we1_offset], ldwe, ld2we, &fjacb[(k + 
		    fjacb_dim2) * fjacb_dim1 + 1], &i__2, &fjacb[(k + 
		    fjacb_dim2) * fjacb_dim1 + 1], &i__3);
/* L20: */
	}
    } else {
	k1 = 0;
	i__1 = *np;
	for (k = 1; k <= i__1; ++k) {
	    if (ifixb[k] >= 1) {
		++k1;
		i__2 = *n * *np;
		i__3 = *n * *np;
		dwght_(n, nq, &we1[we1_offset], ldwe, ld2we, &fjacb[(k + 
			fjacb_dim2) * fjacb_dim1 + 1], &i__2, &fjacb[(k1 + 
			fjacb_dim2) * fjacb_dim1 + 1], &i__3);
	    }
/* L30: */
	}
    }
/*  WEIGHT THE JACOBIAN'S WRT DELTA AS APPROPRIATE */
    if (*isodr) {
	i__1 = *m;
	for (j = 1; j <= i__1; ++j) {
	    i__2 = *n * *m;
	    i__3 = *n * *m;
	    dwght_(n, nq, &we1[we1_offset], ldwe, ld2we, &fjacd[(j + 
		    fjacd_dim2) * fjacd_dim1 + 1], &i__2, &fjacd[(j + 
		    fjacd_dim2) * fjacd_dim1 + 1], &i__3);
/* L40: */
	}
    }
    return 0;
} /* devjac_ */

/* DFCTR */
/* Subroutine */ int dfctr_(logical *oksemi, doublereal *a, integer *lda, 
	integer *n, integer *info)
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal ten = 10.;

    /* System generated locals */
    integer a_dim1, a_offset, i__1, i__2, i__3;
    doublereal d__1;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static integer j, k;
    static doublereal s, t, xi;
    extern doublereal ddot_(integer *, doublereal *, integer *, doublereal *, 
	    integer *), dmprec_(void);

/* ***BEGIN PROLOGUE  DFCTR */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DDOT */
/* ***DATE WRITTEN   910706   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  FACTOR THE POSITIVE (SEMI)DEFINITE MATRIX A USING A */
/*            MODIFIED CHOLESKY FACTORIZATION */
/*            (ADAPTED FROM LINPACK SUBROUTINE DPOFA) */
/* ***REFERENCES  DONGARRA J.J., BUNCH J.R., MOLER C.B., STEWART G.W., */
/*                 *LINPACK USERS GUIDE*, SIAM, 1979. */
/* ***END PROLOGUE  DFCTR */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL FUNCTIONS */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    a_dim1 = *lda;
    a_offset = 1 + a_dim1;
    a -= a_offset;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   A:       THE ARRAY TO BE FACTORED.  UPON RETURN, A CONTAINS THE */
/*            UPPER TRIANGULAR MATRIX  R  SO THAT  A = TRANS(R)*R */
/*            WHERE THE STRICT LOWER TRIANGLE IS SET TO ZERO */
/*            IF  INFO .NE. 0 , THE FACTORIZATION IS NOT COMPLETE. */
/*   I:       AN INDEXING VARIABLE. */
/*   INFO:    AN IDICATOR VARIABLE, WHERE IF */
/*            INFO = 0  THEN FACTORIZATION WAS COMPLETED */
/*            INFO = K  SIGNALS AN ERROR CONDITION.  THE LEADING MINOR */
/*                      OF ORDER  K  IS NOT POSITIVE (SEMI)DEFINITE. */
/*   J:       AN INDEXING VARIABLE. */
/*   LDA:     THE LEADING DIMENSION OF ARRAY A. */
/*   N:       THE NUMBER OF ROWS AND COLUMNS OF DATA IN ARRAY A. */
/*   OKSEMI:  THE INDICATING WHETHER THE FACTORED ARRAY CAN BE POSITIVE */
/*            SEMIDEFINITE (OKSEMI=TRUE) OR WHETHER IT MUST BE FOUND TO */
/*            BE POSITIVE DEFINITE (OKSEMI=FALSE). */
/*   TEN:     THE VALUE 10.0D0. */
/*   XI:      A VALUE USED TO TEST FOR NON POSITIVE SEMIDEFINITENESS. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DFCTR */
/*  SET RELATIVE TOLERANCE FOR DETECTING NON POSITIVE SEMIDEFINITENESS. */
    xi = -ten * dmprec_();
/*  COMPUTE FACTORIZATION, STORING IN UPPER TRIANGULAR PORTION OF A */
    i__1 = *n;
    for (j = 1; j <= i__1; ++j) {
	*info = j;
	s = zero;
	i__2 = j - 1;
	for (k = 1; k <= i__2; ++k) {
	    if (a[k + k * a_dim1] == zero) {
		t = zero;
	    } else {
		i__3 = k - 1;
		t = a[k + j * a_dim1] - ddot_(&i__3, &a[k * a_dim1 + 1], &
			c__1, &a[j * a_dim1 + 1], &c__1);
		t /= a[k + k * a_dim1];
	    }
	    a[k + j * a_dim1] = t;
	    s += t * t;
/* L10: */
	}
	s = a[j + j * a_dim1] - s;
/*     ......EXIT */
	if (a[j + j * a_dim1] < zero || s < xi * (d__1 = a[j + j * a_dim1], 
		abs(d__1))) {
	    return 0;
	} else if (! (*oksemi) && s <= zero) {
	    return 0;
	} else if (s <= zero) {
	    a[j + j * a_dim1] = zero;
	} else {
	    a[j + j * a_dim1] = sqrt(s);
	}
/* L20: */
    }
    *info = 0;
/*  ZERO OUT LOWER PORTION OF A */
    i__1 = *n;
    for (j = 2; j <= i__1; ++j) {
	i__2 = j - 1;
	for (k = 1; k <= i__2; ++k) {
	    a[j + k * a_dim1] = zero;
/* L30: */
	}
/* L40: */
    }
    return 0;
} /* dfctr_ */

/* DFCTRW */
/* Subroutine */ int dfctrw_(integer *n, integer *m, integer *nq, integer *
	npp, logical *isodr, doublereal *we, integer *ldwe, integer *ld2we, 
	doublereal *wd, integer *ldwd, integer *ld2wd, doublereal *wrk0, 
	doublereal *wrk4, doublereal *we1, integer *nnzw, integer *info)
{
    /* Initialized data */

    static doublereal zero = 0.;

    /* System generated locals */
    integer we_dim1, we_dim2, we_offset, we1_dim1, we1_dim2, we1_offset, 
	    wd_dim1, wd_dim2, wd_offset, wrk0_dim1, wrk0_offset, wrk4_dim1, 
	    wrk4_offset, i__1, i__2, i__3, i__4;
    doublereal d__1;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static integer i__, j, l, j1, j2, l1, l2, inf;
    extern /* Subroutine */ int dfctr_(logical *, doublereal *, integer *, 
	    integer *, integer *);
    static logical notzro;

/* ***BEGIN PROLOGUE  DFCTRW */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DFCTR */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  CHECK INPUT PARAMETERS, INDICATING ERRORS FOUND USING */
/*            NONZERO VALUES OF ARGUMENT INFO AS DESCRIBED IN THE */
/*            ODRPACK REFERENCE GUIDE */
/* ***END PROLOGUE  DFCTRW */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    wrk4_dim1 = *m;
    wrk4_offset = 1 + wrk4_dim1;
    wrk4 -= wrk4_offset;
    wrk0_dim1 = *nq;
    wrk0_offset = 1 + wrk0_dim1;
    wrk0 -= wrk0_offset;
    we1_dim1 = *ldwe;
    we1_dim2 = *ld2we;
    we1_offset = 1 + we1_dim1 * (1 + we1_dim2);
    we1 -= we1_offset;
    we_dim1 = *ldwe;
    we_dim2 = *ld2we;
    we_offset = 1 + we_dim1 * (1 + we_dim2);
    we -= we_offset;
    wd_dim1 = *ldwd;
    wd_dim2 = *ld2wd;
    wd_offset = 1 + wd_dim1 * (1 + wd_dim2);
    wd -= wd_offset;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   I:       AN INDEXING VARIABLE. */
/*   INFO:    THE VARIABLE DESIGNATING WHY THE COMPUTATIONS WERE STOPPED. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR BY OLS (ISODR=FALSE). */
/*   J:       AN INDEXING VARIABLE. */
/*   J1:      AN INDEXING VARIABLE. */
/*   J2:      AN INDEXING VARIABLE. */
/*   L:       AN INDEXING VARIABLE. */
/*   L1:      AN INDEXING VARIABLE. */
/*   L2:      AN INDEXING VARIABLE. */
/*   LAST:    THE LAST ROW OF THE ARRAY TO BE ACCESSED. */
/*   LDWD:    THE LEADING DIMENSION OF ARRAY WD. */
/*   LDWE:    THE LEADING DIMENSION OF ARRAY WE. */
/*   LD2WD:   THE SECOND DIMENSION OF ARRAY WD. */
/*   LD2WE:   THE SECOND DIMENSION OF ARRAY WE. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NNZW:    THE NUMBER OF NONZERO WEIGHTED OBSERVATIONS. */
/*   NOTZRO:  THE VARIABLE DESIGNATING WHETHER A GIVEN COMPONENT OF THE */
/*            WEIGHT ARRAY WE CONTAINS A NONZERO ELEMENT (NOTZRO=FALSE) */
/*            OR NOT (NOTZRO=TRUE). */
/*   NPP:     THE NUMBER OF FUNCTION PARAMETERS BEING ESTIMATED. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATIONS. */
/*   WE:      THE (SQUARED) EPSILON WEIGHTS. */
/*   WE1:     THE FACTORED EPSILON WEIGHTS, S.T. TRANS(WE1)*WE1 = WE. */
/*   WD:      THE (SQUARED) DELTA WEIGHTS. */
/*   WRK0:    A WORK ARRAY OF (NQ BY NQ) ELEMENTS. */
/*   WRK4:    A WORK ARRAY OF (M BY M) ELEMENTS. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DFCTRW */
/*  CHECK EPSILON WEIGHTS, AND STORE FACTORIZATION IN WE1 */
    if (we[(0 + (0 + (1 + (1 + 1 * we_dim2) * we_dim1 << 3))) / 8] < zero) {
/*  WE CONTAINS A SCALAR */
	we1[(we1_dim2 + 1) * we1_dim1 + 1] = -sqrt((d__1 = we[(we_dim2 + 1) * 
		we_dim1 + 1], abs(d__1)));
	*nnzw = *n;
    } else {
	*nnzw = 0;
	if (*ldwe == 1) {
	    if (*ld2we == 1) {
/*  WE CONTAINS A DIAGONAL MATRIX */
		i__1 = *nq;
		for (l = 1; l <= i__1; ++l) {
		    if (we[(l * we_dim2 + 1) * we_dim1 + 1] > zero) {
			*nnzw = *n;
			we1[(l * we1_dim2 + 1) * we1_dim1 + 1] = sqrt(we[(l * 
				we_dim2 + 1) * we_dim1 + 1]);
		    } else if (we[(l * we_dim2 + 1) * we_dim1 + 1] < zero) {
			*info = 30010;
			goto L300;
		    }
/* L110: */
		}
	    } else {
/*  WE CONTAINS A FULL NQ BY NQ SEMIDEFINITE MATRIX */
		i__1 = *nq;
		for (l1 = 1; l1 <= i__1; ++l1) {
		    i__2 = *nq;
		    for (l2 = l1; l2 <= i__2; ++l2) {
			wrk0[l1 + l2 * wrk0_dim1] = we[(l1 + l2 * we_dim2) * 
				we_dim1 + 1];
/* L120: */
		    }
/* L130: */
		}
		dfctr_(&c_true, &wrk0[wrk0_offset], nq, nq, &inf);
		if (inf != 0) {
		    *info = 30010;
		    goto L300;
		} else {
		    i__1 = *nq;
		    for (l1 = 1; l1 <= i__1; ++l1) {
			i__2 = *nq;
			for (l2 = 1; l2 <= i__2; ++l2) {
			    we1[(l1 + l2 * we1_dim2) * we1_dim1 + 1] = wrk0[
				    l1 + l2 * wrk0_dim1];
/* L140: */
			}
			if (we1[(l1 + l1 * we1_dim2) * we1_dim1 + 1] != zero) 
				{
			    *nnzw = *n;
			}
/* L150: */
		    }
		}
	    }
	} else {
	    if (*ld2we == 1) {
/*  WE CONTAINS AN ARRAY OF  DIAGONAL MATRIX */
		i__1 = *n;
		for (i__ = 1; i__ <= i__1; ++i__) {
		    notzro = FALSE_;
		    i__2 = *nq;
		    for (l = 1; l <= i__2; ++l) {
			if (we[i__ + (l * we_dim2 + 1) * we_dim1] > zero) {
			    notzro = TRUE_;
			    we1[i__ + (l * we1_dim2 + 1) * we1_dim1] = sqrt(
				    we[i__ + (l * we_dim2 + 1) * we_dim1]);
			} else if (we[i__ + (l * we_dim2 + 1) * we_dim1] < 
				zero) {
			    *info = 30010;
			    goto L300;
			}
/* L210: */
		    }
		    if (notzro) {
			++(*nnzw);
		    }
/* L220: */
		}
	    } else {
/*  WE CONTAINS AN ARRAY OF FULL NQ BY NQ SEMIDEFINITE MATRICES */
		i__1 = *n;
		for (i__ = 1; i__ <= i__1; ++i__) {
		    i__2 = *nq;
		    for (l1 = 1; l1 <= i__2; ++l1) {
			i__3 = *nq;
			for (l2 = l1; l2 <= i__3; ++l2) {
			    wrk0[l1 + l2 * wrk0_dim1] = we[i__ + (l1 + l2 * 
				    we_dim2) * we_dim1];
/* L230: */
			}
/* L240: */
		    }
		    dfctr_(&c_true, &wrk0[wrk0_offset], nq, nq, &inf);
		    if (inf != 0) {
			*info = 30010;
			goto L300;
		    } else {
			notzro = FALSE_;
			i__2 = *nq;
			for (l1 = 1; l1 <= i__2; ++l1) {
			    i__3 = *nq;
			    for (l2 = 1; l2 <= i__3; ++l2) {
				we1[i__ + (l1 + l2 * we1_dim2) * we1_dim1] = 
					wrk0[l1 + l2 * wrk0_dim1];
/* L250: */
			    }
			    if (we1[i__ + (l1 + l1 * we1_dim2) * we1_dim1] != 
				    zero) {
				notzro = TRUE_;
			    }
/* L260: */
			}
		    }
		    if (notzro) {
			++(*nnzw);
		    }
/* L270: */
		}
	    }
	}
    }
/*  CHECK FOR A SUFFICIENT NUMBER OF NONZERO EPSILON WEIGHTS */
    if (*nnzw < *npp) {
	*info = 30020;
    }
/*  CHECK DELTA WEIGHTS */
L300:
    if (! (*isodr) || wd[(wd_dim2 + 1) * wd_dim1 + 1] < zero) {
/*  PROBLEM IS NOT ODR, OR WD CONTAINS A SCALAR */
	return 0;
    } else {
	if (*ldwd == 1) {
	    if (*ld2wd == 1) {
/*  WD CONTAINS A DIAGONAL MATRIX */
		i__1 = *m;
		for (j = 1; j <= i__1; ++j) {
		    if (wd[(j * wd_dim2 + 1) * wd_dim1 + 1] <= zero) {
/* Computing MAX */
			i__2 = 30001, i__3 = *info + 1;
			*info = max(i__2,i__3);
			return 0;
		    }
/* L310: */
		}
	    } else {
/*  WD CONTAINS A FULL M BY M POSITIVE DEFINITE MATRIX */
		i__1 = *m;
		for (j1 = 1; j1 <= i__1; ++j1) {
		    i__2 = *m;
		    for (j2 = j1; j2 <= i__2; ++j2) {
			wrk4[j1 + j2 * wrk4_dim1] = wd[(j1 + j2 * wd_dim2) * 
				wd_dim1 + 1];
/* L320: */
		    }
/* L330: */
		}
		dfctr_(&c_false, &wrk4[wrk4_offset], m, m, &inf);
		if (inf != 0) {
/* Computing MAX */
		    i__1 = 30001, i__2 = *info + 1;
		    *info = max(i__1,i__2);
		    return 0;
		}
	    }
	} else {
	    if (*ld2wd == 1) {
/*  WD CONTAINS AN ARRAY OF DIAGONAL MATRICES */
		i__1 = *n;
		for (i__ = 1; i__ <= i__1; ++i__) {
		    i__2 = *m;
		    for (j = 1; j <= i__2; ++j) {
			if (wd[i__ + (j * wd_dim2 + 1) * wd_dim1] <= zero) {
/* Computing MAX */
			    i__3 = 30001, i__4 = *info + 1;
			    *info = max(i__3,i__4);
			    return 0;
			}
/* L410: */
		    }
/* L420: */
		}
	    } else {
/*  WD CONTAINS AN ARRAY OF FULL M BY M POSITIVE DEFINITE MATRICES */
		i__1 = *n;
		for (i__ = 1; i__ <= i__1; ++i__) {
		    i__2 = *m;
		    for (j1 = 1; j1 <= i__2; ++j1) {
			i__3 = *m;
			for (j2 = j1; j2 <= i__3; ++j2) {
			    wrk4[j1 + j2 * wrk4_dim1] = wd[i__ + (j1 + j2 * 
				    wd_dim2) * wd_dim1];
/* L430: */
			}
/* L440: */
		    }
		    dfctr_(&c_false, &wrk4[wrk4_offset], m, m, &inf);
		    if (inf != 0) {
/* Computing MAX */
			i__2 = 30001, i__3 = *info + 1;
			*info = max(i__2,i__3);
			return 0;
		    }
/* L470: */
		}
	    }
	}
    }
    return 0;
} /* dfctrw_ */

/* DFLAGS */
/* Subroutine */ int dflags_(integer *job, logical *restrt, logical *initd, 
	logical *dovcv, logical *redoj, logical *anajac, logical *cdjac, 
	logical *chkjac, logical *isodr, logical *implct)
{
    static integer j;

/* ***BEGIN PROLOGUE  DFLAGS */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  SET FLAGS INDICATING CONDITIONS SPECIFIED BY JOB */
/* ***END PROLOGUE  DFLAGS */
/* ...SCALAR ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ANAJAC:  THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE COMPUTED */
/*            BY FINITE DIFFERENCES (ANAJAC=FALSE) OR NOT (ANAJAC=TRUE). */
/*   CDJAC:   THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE COMPUTED */
/*            BY CENTRAL DIFFERENCES (CDJAC=TRUE) OR BY FORWARD */
/*            DIFFERENCES (CDJAC=FALSE). */
/*   CHKJAC:  THE VARIABLE DESIGNATING WHETHER THE USER-SUPPLIED */
/*            JACOBIANS ARE TO BE CHECKED (CHKJAC=TRUE) OR NOT */
/*            (CHKJAC=FALSE). */
/*   DOVCV:   THE VARIABLE DESIGNATING WHETHER THE COVARIANCE MATRIX IS */
/*            TO BE COMPUTED (DOVCV=TRUE) OR NOT (DOVCV=FALSE). */
/*   IMPLCT:  THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY */
/*            IMPLICIT ODR (IMPLCT=TRUE) OR EXPLICIT ODR (IMPLCT=FALSE). */
/*   INITD:   THE VARIABLE DESIGNATING WHETHER DELTA IS TO BE INITIALIZED */
/*            TO ZERO (INITD=TRUE) OR TO THE FIRST N BY M ELEMENTS OF */
/*            ARRAY WORK (INITD=FALSE). */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR BY OLS (ISODR=FALSE). */
/*   J:       THE VALUE OF A SPECIFIC DIGIT OF JOB. */
/*   JOB:     THE VARIABLE CONTROLING PROBLEM INITIALIZATION AND */
/*            COMPUTATIONAL METHOD. */
/*   REDOJ:   THE VARIABLE DESIGNATING WHETHER THE JACOBIAN MATRIX IS TO */
/*            BE RECOMPUTED FOR THE COMPUTATION OF THE COVARIANCE MATRIX */
/*            (REDOJ=TRUE) OR NOT (REDOJ=FALSE). */
/*   RESTRT:  THE VARIABLE DESIGNATING WHETHER THE CALL IS A RESTART */
/*            (RESTRT=TRUE) OR NOT (RESTRT=FALSE). */
/* ***FIRST EXECUTABLE STATEMENT  DFLAGS */
    if (*job >= 0) {
	*restrt = *job >= 10000;
	*initd = *job % 10000 / 1000 == 0;
	j = *job % 1000 / 100;
	if (j == 0) {
	    *dovcv = TRUE_;
	    *redoj = TRUE_;
	} else if (j == 1) {
	    *dovcv = TRUE_;
	    *redoj = FALSE_;
	} else {
	    *dovcv = FALSE_;
	    *redoj = FALSE_;
	}
	j = *job % 100 / 10;
	if (j == 0) {
	    *anajac = FALSE_;
	    *cdjac = FALSE_;
	    *chkjac = FALSE_;
	} else if (j == 1) {
	    *anajac = FALSE_;
	    *cdjac = TRUE_;
	    *chkjac = FALSE_;
	} else if (j == 2) {
	    *anajac = TRUE_;
	    *cdjac = FALSE_;
	    *chkjac = TRUE_;
	} else {
	    *anajac = TRUE_;
	    *cdjac = FALSE_;
	    *chkjac = FALSE_;
	}
	j = *job % 10;
	if (j == 0) {
	    *isodr = TRUE_;
	    *implct = FALSE_;
	} else if (j == 1) {
	    *isodr = TRUE_;
	    *implct = TRUE_;
	} else {
	    *isodr = FALSE_;
	    *implct = FALSE_;
	}
    } else {
	*restrt = FALSE_;
	*initd = TRUE_;
	*dovcv = TRUE_;
	*redoj = TRUE_;
	*anajac = FALSE_;
	*cdjac = FALSE_;
	*chkjac = FALSE_;
	*isodr = TRUE_;
	*implct = FALSE_;
    }
    return 0;
} /* dflags_ */

/* DHSTEP */
doublereal dhstep_(integer *itype, integer *neta, integer *i__, integer *j, 
	doublereal *stp, integer *ldstp)
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal two = 2.;
    static doublereal three = 3.;
    static doublereal ten = 10.;

    /* System generated locals */
    integer stp_dim1, stp_offset;
    doublereal ret_val, d__1;

    /* Builtin functions */
    double pow_dd(doublereal *, doublereal *);

/* ***BEGIN PROLOGUE  DHSTEP */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  SET RELATIVE STEP SIZE FOR FINITE DIFFERENCE DERIVATIVES */
/* ***END PROLOGUE  DHSTEP */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    stp_dim1 = *ldstp;
    stp_offset = 1 + stp_dim1;
    stp -= stp_offset;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   I:       AN IDENTIFIER FOR SELECTING USER SUPPLIED STEP SIZES. */
/*   ITYPE:   THE FINITE DIFFERENCE METHOD BEING USED, WHERE */
/*            ITYPE = 0 INDICATES FORWARD FINITE DIFFERENCES, AND */
/*            ITYPE = 1 INDICATES CENTRAL FINITE DIFFERENCES. */
/*   J:       AN IDENTIFIER FOR SELECTING USER SUPPLIED STEP SIZES. */
/*   LDSTP:   THE LEADING DIMENSION OF ARRAY STP. */
/*   NETA:    THE NUMBER OF GOOD DIGITS IN THE FUNCTION RESULTS. */
/*   STP:     THE STEP SIZE FOR THE FINITE DIFFERENCE DERIVATIVE. */
/*   TEN:     THE VALUE 10.0D0. */
/*   THREE:   THE VALUE 3.0D0. */
/*   TWO:     THE VALUE 2.0D0. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DHSTEP */
/*  SET DHSTEP TO RELATIVE FINITE DIFFERENCE STEP SIZE */
    if (stp[(0 + (0 + (1 + 1 * stp_dim1 << 3))) / 8] <= zero) {
	if (*itype == 0) {
/*  USE DEFAULT FORWARD FINITE DIFFERENCE STEP SIZE */
	    d__1 = -abs(*neta) / two - two;
	    ret_val = pow_dd(&ten, &d__1);
	} else {
/*  USE DEFAULT CENTRAL FINITE DIFFERENCE STEP SIZE */
	    d__1 = -abs(*neta) / three;
	    ret_val = pow_dd(&ten, &d__1);
	}
    } else if (*ldstp == 1) {
	ret_val = stp[*j * stp_dim1 + 1];
    } else {
	ret_val = stp[*i__ + *j * stp_dim1];
    }
    return ret_val;
} /* dhstep_ */

/* DIFIX */
/* Subroutine */ int difix_(integer *n, integer *m, integer *ifix, integer *
	ldifix, doublereal *t, integer *ldt, doublereal *tfix, integer *
	ldtfix)
{
    /* Initialized data */

    static doublereal zero = 0.;

    /* System generated locals */
    integer t_dim1, t_offset, tfix_dim1, tfix_offset, ifix_dim1, ifix_offset, 
	    i__1, i__2;

    /* Local variables */
    static integer i__, j;

/* ***BEGIN PROLOGUE  DIFIX */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   910612   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  SET ELEMENTS OF T TO ZERO ACCORDING TO IFIX */
/* ***END PROLOGUE  DIFIX */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    ifix_dim1 = *ldifix;
    ifix_offset = 1 + ifix_dim1;
    ifix -= ifix_offset;
    t_dim1 = *ldt;
    t_offset = 1 + t_dim1;
    t -= t_offset;
    tfix_dim1 = *ldtfix;
    tfix_offset = 1 + tfix_dim1;
    tfix -= tfix_offset;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   I:       AN INDEXING VARIABLE. */
/*   IFIX:    THE ARRAY DESIGNATING WHETHER AN ELEMENT OF T IS TO BE */
/*            SET TO ZERO. */
/*   J:       AN INDEXING VARIABLE. */
/*   LDT:     THE LEADING DIMENSION OF ARRAY T. */
/*   LDIFIX:  THE LEADING DIMENSION OF ARRAY IFIX. */
/*   LDTFIX:  THE LEADING DIMENSION OF ARRAY TFIX. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE ARRAY. */
/*   N:       THE NUMBER OF ROWS OF DATA IN THE ARRAY. */
/*   T:       THE ARRAY BEING SET TO ZERO ACCORDING TO THE ELEMENTS */
/*            OF IFIX. */
/*   TFIX:    THE RESULTING ARRAY. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DIFIX */
    if (*n == 0 || *m == 0) {
	return 0;
    }
    if ((doublereal) ifix[ifix_dim1 + 1] >= zero) {
	if (*ldifix >= *n) {
	    i__1 = *m;
	    for (j = 1; j <= i__1; ++j) {
		i__2 = *n;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    if (ifix[i__ + j * ifix_dim1] == 0) {
			tfix[i__ + j * tfix_dim1] = zero;
		    } else {
			tfix[i__ + j * tfix_dim1] = t[i__ + j * t_dim1];
		    }
/* L10: */
		}
/* L20: */
	    }
	} else {
	    i__1 = *m;
	    for (j = 1; j <= i__1; ++j) {
		if (ifix[j * ifix_dim1 + 1] == 0) {
		    i__2 = *n;
		    for (i__ = 1; i__ <= i__2; ++i__) {
			tfix[i__ + j * tfix_dim1] = zero;
/* L30: */
		    }
		} else {
		    i__2 = *n;
		    for (i__ = 1; i__ <= i__2; ++i__) {
			tfix[i__ + j * tfix_dim1] = t[i__ + j * t_dim1];
/* L90: */
		    }
		}
/* L100: */
	    }
	}
    }
    return 0;
} /* difix_ */

/* DINIWK */
/* Subroutine */ int diniwk_(integer *n, integer *m, integer *np, doublereal *
	work, integer *lwork, integer *iwork, integer *liwork, doublereal *x, 
	integer *ldx, integer *ifixx, integer *ldifx, doublereal *scld, 
	integer *ldscld, doublereal *beta, doublereal *sclb, doublereal *
	sstol, doublereal *partol, integer *maxit, doublereal *taufac, 
	integer *job, integer *iprint, integer *lunerr, integer *lunrpt, 
	integer *epsmai, integer *sstoli, integer *partli, integer *maxiti, 
	integer *taufci, integer *jobi, integer *iprini, integer *luneri, 
	integer *lunrpi, integer *ssfi, integer *tti, integer *ldtti, integer 
	*deltai)
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal one = 1.;
    static doublereal two = 2.;
    static doublereal three = 3.;

    /* System generated locals */
    integer scld_dim1, scld_offset, x_dim1, x_offset, ifixx_dim1, 
	    ifixx_offset, i__1, i__2;
    doublereal d__1;

    /* Builtin functions */
    double pow_dd(doublereal *, doublereal *), sqrt(doublereal);

    /* Local variables */
    static integer i__, j;
    static logical cdjac;
    extern /* Subroutine */ int dsclb_(integer *, doublereal *, doublereal *),
	     dscld_(integer *, integer *, doublereal *, integer *, doublereal 
	    *, integer *);
    static logical redoj, initd;
    extern /* Subroutine */ int dcopy_(integer *, doublereal *, integer *, 
	    doublereal *, integer *);
    static logical dovcv, isodr;
    extern /* Subroutine */ int dzero_(integer *, integer *, doublereal *, 
	    integer *);
    static logical anajac, chkjac;
    extern /* Subroutine */ int dflags_(integer *, logical *, logical *, 
	    logical *, logical *, logical *, logical *, logical *, logical *, 
	    logical *);
    extern doublereal dmprec_(void);
    static logical implct, restrt;

/* ***BEGIN PROLOGUE  DINIWK */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DFLAGS,DMPREC,DSCLB,DSCLD,DZERO */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  INITIALIZE WORK VECTORS AS NECESSARY */
/* ***END PROLOGUE  DINIWK */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL FUNCTIONS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --sclb;
    --beta;
    --work;
    --iwork;
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;
    scld_dim1 = *ldscld;
    scld_offset = 1 + scld_dim1;
    scld -= scld_offset;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ANAJAC:  THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE */
/*            COMPUTED BY FINITE DIFFERENCES (ANAJAC=FALSE) OR NOT */
/*            (ANAJAC=TRUE). */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   CDJAC:   THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE */
/*            COMPUTED BY CENTRAL DIFFERENCES (CDJAC=TRUE) OR BY FORWARD */
/*            DIFFERENCES (CDJAC=FALSE). */
/*   CHKJAC:  THE VARIABLE DESIGNATING WHETHER THE USER-SUPPLIED */
/*            JACOBIANS ARE TO BE CHECKED (CHKJAC=TRUE) OR NOT */
/*            (CHKJAC=FALSE). */
/*   DELTAI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY DELTA. */
/*   DOVCV:   THE VARIABLE DESIGNATING WHETHER THE COVARIANCE MATRIX IS */
/*            TO BE COMPUTED (DOVCV=TRUE) OR NOT (DOVCV=FALSE). */
/*   EPSMAI:  THE LOCATION IN ARRAY WORK OF VARIABLE EPSMAC. */
/*   I:       AN INDEXING VARIABLE. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE FIXED */
/*            AT THEIR INPUT VALUES OR NOT. */
/*   IMPLCT:  THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY */
/*            IMPLICIT ODR (IMPLCT=TRUE) OR EXPLICIT ODR (IMPLCT=FALSE). */
/*   INITD:   THE VARIABLE DESIGNATING WHETHER DELTA IS TO BE INITIALIZED */
/*            TO ZERO (INITD=TRUE) OR TO THE VALUES IN THE FIRST N BY M */
/*            ELEMENTS OF ARRAY WORK (INITD=FALSE). */
/*   IPRINI:  THE LOCATION IN ARRAY IWORK OF VARIABLE IPRINT. */
/*   IPRINT:  THE PRINT CONTROL VARIABLE. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR BY OLS (ISODR=FALSE). */
/*   IWORK:   THE INTEGER WORK SPACE. */
/*   J:       AN INDEXING VARIABLE. */
/*   JOB:     THE VARIABLE CONTROLING PROBLEM INITIALIZATION AND */
/*            COMPUTATIONAL METHOD. */
/*   JOBI:    THE LOCATION IN ARRAY IWORK OF VARIABLE JOB. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LDSCLD:  THE LEADING DIMENSION OF ARRAY SCLD. */
/*   LDTTI:   THE LEADING DIMENSION OF ARRAY TT. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   LIWORK:  THE LENGTH OF VECTOR IWORK. */
/*   LUNERI:  THE LOCATION IN ARRAY IWORK OF VARIABLE LUNERR. */
/*   LUNERR:  THE LOGICAL UNIT NUMBER USED FOR ERROR MESSAGES. */
/*   LUNRPI:  THE LOCATION IN ARRAY IWORK OF VARIABLE LUNRPT. */
/*   LUNRPT:  THE LOGICAL UNIT NUMBER USED FOR COMPUTATION REPORTS. */
/*   LWORK:   THE LENGTH OF VECTOR WORK. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE INDEPENDENT VARIABLE. */
/*   MAXIT:   THE MAXIMUM NUMBER OF ITERATIONS ALLOWED. */
/*   MAXITI:  THE LOCATION IN ARRAY IWORK OF VARIABLE MAXIT. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   ONE:     THE VALUE 1.0D0. */
/*   PARTLI:  THE LOCATION IN ARRAY WORK OF VARIABLE PARTOL. */
/*   PARTOL:  THE PARAMETER CONVERGENCE STOPPING CRITERIA. */
/*   REDOJ:   THE VARIABLE DESIGNATING WHETHER THE JACOBIAN MATRIX IS TO */
/*            BE RECOMPUTED FOR THE COMPUTATION OF THE COVARIANCE MATRIX */
/*            (REDOJ=TRUE) OR NOT (REDOJ=FALSE). */
/*   RESTRT:  THE VARIABLE DESIGNATING WHETHER THE CALL IS A RESTART */
/*            (RESTRT=TRUE) OR NOT (RESTRT=FALSE). */
/*   SCLB:    THE SCALING VALUES FOR BETA. */
/*   SCLD:    THE SCALING VALUES FOR DELTA. */
/*   SSFI:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY SSF. */
/*   SSTOL:   THE SUM-OF-SQUARES CONVERGENCE STOPPING CRITERIA. */
/*   SSTOLI:  THE LOCATION IN ARRAY WORK OF VARIABLE SSTOL. */
/*   TAUFAC:  THE FACTOR USED TO COMPUTE THE INITIAL TRUST REGION */
/*            DIAMETER. */
/*   TAUFCI:  THE LOCATION IN ARRAY WORK OF VARIABLE TAUFAC. */
/*   THREE:   THE VALUE 3.0D0. */
/*   TTI:     THE STARTING LOCATION IN ARRAY WORK OF THE ARRAY TT. */
/*   TWO:     THE VALUE 2.0D0. */
/*   WORK:    THE DOUBLE PRECISION WORK SPACE. */
/*   X:       THE INDEPENDENT VARIABLE. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DINIWK */
    dflags_(job, &restrt, &initd, &dovcv, &redoj, &anajac, &cdjac, &chkjac, &
	    isodr, &implct);
/*  STORE VALUE OF MACHINE PRECISION IN WORK VECTOR */
    work[*epsmai] = dmprec_();
/*  SET TOLERANCE FOR STOPPING CRITERIA BASED ON THE CHANGE IN THE */
/*  PARAMETERS  (SEE ALSO SUBPROGRAM DODCNT) */
    if (*partol < zero) {
	d__1 = two / three;
	work[*partli] = pow_dd(&work[*epsmai], &d__1);
    } else {
	work[*partli] = min(*partol,one);
    }
/*  SET TOLERANCE FOR STOPPING CRITERIA BASED ON THE CHANGE IN THE */
/*  SUM OF SQUARES OF THE WEIGHTED OBSERVATIONAL ERRORS */
    if (*sstol < zero) {
	work[*sstoli] = sqrt(work[*epsmai]);
    } else {
	work[*sstoli] = min(*sstol,one);
    }
/*  SET FACTOR FOR COMPUTING TRUST REGION DIAMETER AT FIRST ITERATION */
    if (*taufac <= zero) {
	work[*taufci] = one;
    } else {
	work[*taufci] = min(*taufac,one);
    }
/*  SET MAXIMUM NUMBER OF ITERATIONS */
    if (*maxit < 0) {
	iwork[*maxiti] = 50;
    } else {
	iwork[*maxiti] = *maxit;
    }
/*  STORE PROBLEM INITIALIZATION AND COMPUTATIONAL METHOD CONTROL */
/*  VARIABLE */
    if (*job <= 0) {
	iwork[*jobi] = 0;
    } else {
	iwork[*jobi] = *job;
    }
/*  SET PRINT CONTROL */
    if (*iprint < 0) {
	iwork[*iprini] = 2001;
    } else {
	iwork[*iprini] = *iprint;
    }
/*  SET LOGICAL UNIT NUMBER FOR ERROR MESSAGES */
    if (*lunerr < 0) {
	iwork[*luneri] = 6;
    } else {
	iwork[*luneri] = *lunerr;
    }
/*  SET LOGICAL UNIT NUMBER FOR COMPUTATION REPORTS */
    if (*lunrpt < 0) {
	iwork[*lunrpi] = 6;
    } else {
	iwork[*lunrpi] = *lunrpt;
    }
/*  COMPUTE SCALING FOR BETA'S AND DELTA'S */
    if (sclb[1] <= zero) {
	dsclb_(np, &beta[1], &work[*ssfi]);
    } else {
	dcopy_(np, &sclb[1], &c__1, &work[*ssfi], &c__1);
    }
    if (isodr) {
	if (scld[scld_dim1 + 1] <= zero) {
	    iwork[*ldtti] = *n;
	    dscld_(n, m, &x[x_offset], ldx, &work[*tti], &iwork[*ldtti]);
	} else {
	    if (*ldscld == 1) {
		iwork[*ldtti] = 1;
		dcopy_(m, &scld[scld_dim1 + 1], &c__1, &work[*tti], &c__1);
	    } else {
		iwork[*ldtti] = *n;
		i__1 = *m;
		for (j = 1; j <= i__1; ++j) {
		    dcopy_(n, &scld[j * scld_dim1 + 1], &c__1, &work[*tti + (
			    j - 1) * iwork[*ldtti]], &c__1);
/* L10: */
		}
	    }
	}
    }
/*  INITIALIZE DELTA'S AS NECESSARY */
    if (isodr) {
	if (initd) {
	    dzero_(n, m, &work[*deltai], n);
	} else {
	    if (ifixx[ifixx_dim1 + 1] >= 0) {
		if (*ldifx == 1) {
		    i__1 = *m;
		    for (j = 1; j <= i__1; ++j) {
			if (ifixx[j * ifixx_dim1 + 1] == 0) {
			    dzero_(n, &c__1, &work[*deltai + (j - 1) * *n], n)
				    ;
			}
/* L20: */
		    }
		} else {
		    i__1 = *m;
		    for (j = 1; j <= i__1; ++j) {
			i__2 = *n;
			for (i__ = 1; i__ <= i__2; ++i__) {
			    if (ifixx[i__ + j * ifixx_dim1] == 0) {
				work[*deltai - 1 + i__ + (j - 1) * *n] = zero;
			    }
/* L30: */
			}
/* L40: */
		    }
		}
	    }
	}
    } else {
	dzero_(n, m, &work[*deltai], n);
    }
    return 0;
} /* diniwk_ */

/* DIWINF */
/* Subroutine */ int diwinf_(integer *m, integer *np, integer *nq, integer *
	msgbi, integer *msgdi, integer *ifix2i, integer *istopi, integer *
	nnzwi, integer *nppi, integer *idfi, integer *jobi, integer *iprini, 
	integer *luneri, integer *lunrpi, integer *nrowi, integer *ntoli, 
	integer *netai, integer *maxiti, integer *niteri, integer *nfevi, 
	integer *njevi, integer *int2i, integer *iranki, integer *ldtti, 
	integer *liwkmn)
{
/* ***BEGIN PROLOGUE  DIWINF */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  SET STORAGE LOCATIONS WITHIN INTEGER WORK SPACE */
/* ***END PROLOGUE  DIWINF */
/* ...SCALAR ARGUMENTS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   IDFI:    THE LOCATION IN ARRAY IWORK OF VARIABLE IDF. */
/*   IFIX2I:  THE STARTING LOCATION IN ARRAY IWORK OF ARRAY IFIX2. */
/*   INT2I:   THE LOCATION IN ARRAY IWORK OF VARIABLE INT2. */
/*   IPRINI:  THE LOCATION IN ARRAY IWORK OF VARIABLE IPRINT. */
/*   IRANKI:  THE LOCATION IN ARRAY IWORK OF VARIABLE IRANK. */
/*   ISTOPI:  THE LOCATION IN ARRAY IWORK OF VARIABLE ISTOP. */
/*   JOBI:    THE LOCATION IN ARRAY IWORK OF VARIABLE JOB. */
/*   LDTTI:   THE LOCATION IN ARRAY IWORK OF VARIABLE LDTT. */
/*   LIWKMN:  THE MINIMUM ACCEPTABLE LENGTH OF ARRAY IWORK. */
/*   LUNERI:  THE LOCATION IN ARRAY IWORK OF VARIABLE LUNERR. */
/*   LUNRPI:  THE LOCATION IN ARRAY IWORK OF VARIABLE LUNRPT. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE INDEPENDENT VARIABLE. */
/*   MAXITI:  THE LOCATION IN ARRAY IWORK OF VARIABLE MAXIT. */
/*   MSGBI:   THE STARTING LOCATION IN ARRAY IWORK OF ARRAY MSGB. */
/*   MSGDI:   THE STARTING LOCATION IN ARRAY IWORK OF ARRAY MSGD. */
/*   NETAI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NETA. */
/*   NFEVI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NFEV. */
/*   NITERI:  THE LOCATION IN ARRAY IWORK OF VARIABEL NITER. */
/*   NJEVI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NJEV. */
/*   NNZWI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NNZW. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NPPI:    THE LOCATION IN ARRAY IWORK OF VARIABLE NPP. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   NROWI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NROW. */
/*   NTOLI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NTOL. */
/* ***FIRST EXECUTABLE STATEMENT  DIWINF */
    if (*np >= 1 && *m >= 1) {
	*msgbi = 1;
	*msgdi = *msgbi + *nq * *np + 1;
	*ifix2i = *msgdi + *nq * *m + 1;
	*istopi = *ifix2i + *np;
	*nnzwi = *istopi + 1;
	*nppi = *nnzwi + 1;
	*idfi = *nppi + 1;
	*jobi = *idfi + 1;
	*iprini = *jobi + 1;
	*luneri = *iprini + 1;
	*lunrpi = *luneri + 1;
	*nrowi = *lunrpi + 1;
	*ntoli = *nrowi + 1;
	*netai = *ntoli + 1;
	*maxiti = *netai + 1;
	*niteri = *maxiti + 1;
	*nfevi = *niteri + 1;
	*njevi = *nfevi + 1;
	*int2i = *njevi + 1;
	*iranki = *int2i + 1;
	*ldtti = *iranki + 1;
	*liwkmn = *ldtti;
    } else {
	*msgbi = 1;
	*msgdi = 1;
	*ifix2i = 1;
	*istopi = 1;
	*nnzwi = 1;
	*nppi = 1;
	*idfi = 1;
	*jobi = 1;
	*iprini = 1;
	*luneri = 1;
	*lunrpi = 1;
	*nrowi = 1;
	*ntoli = 1;
	*netai = 1;
	*maxiti = 1;
	*niteri = 1;
	*nfevi = 1;
	*njevi = 1;
	*int2i = 1;
	*iranki = 1;
	*ldtti = 1;
	*liwkmn = 1;
    }
    return 0;
} /* diwinf_ */

/* DJACCD */
/* Subroutine */ int djaccd_(S_fp fcn, integer *n, integer *m, integer *np, 
	integer *nq, doublereal *beta, doublereal *x, integer *ldx, 
	doublereal *delta, doublereal *xplusd, integer *ifixb, integer *ifixx,
	 integer *ldifx, doublereal *stpb, doublereal *stpd, integer *ldstpd, 
	doublereal *ssf, doublereal *tt, integer *ldtt, integer *neta, 
	doublereal *stp, doublereal *wrk1, doublereal *wrk2, doublereal *wrk3,
	 doublereal *wrk6, doublereal *fjacb, logical *isodr, doublereal *
	fjacd, integer *nfev, integer *istop)
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal one = 1.;

    /* System generated locals */
    integer delta_dim1, delta_offset, fjacb_dim1, fjacb_dim2, fjacb_offset, 
	    fjacd_dim1, fjacd_dim2, fjacd_offset, stpd_dim1, stpd_offset, 
	    tt_dim1, tt_offset, wrk1_dim1, wrk1_dim2, wrk1_offset, wrk2_dim1, 
	    wrk2_offset, wrk6_dim1, wrk6_dim2, wrk6_offset, x_dim1, x_offset, 
	    xplusd_dim1, xplusd_offset, ifixx_dim1, ifixx_offset, i__1, i__2, 
	    i__3;
    doublereal d__1;

    /* Builtin functions */
    double d_sign(doublereal *, doublereal *);

    /* Local variables */
    static integer i__, j, k, l;
    static logical doit;
    static doublereal typj, betak;
    extern /* Subroutine */ int dzero_(integer *, integer *, doublereal *, 
	    integer *);
    extern doublereal dhstep_(integer *, integer *, integer *, integer *, 
	    doublereal *, integer *);
    static logical setzro;

/* ***BEGIN PROLOGUE  DJACCD */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  FCN,DHSTEP,DZERO */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  COMPUTE CENTRAL DIFFERENCE APPROXIMATIONS TO THE */
/*            JACOBIAN WRT THE ESTIMATED BETAS AND WRT THE DELTAS */
/* ***END PROLOGUE  DJACCD */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...EXTERNAL FUNCTIONS */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --stp;
    xplusd_dim1 = *n;
    xplusd_offset = 1 + xplusd_dim1;
    xplusd -= xplusd_offset;
    delta_dim1 = *n;
    delta_offset = 1 + delta_dim1;
    delta -= delta_offset;
    --wrk3;
    --ssf;
    --stpb;
    --ifixb;
    --beta;
    fjacd_dim1 = *n;
    fjacd_dim2 = *m;
    fjacd_offset = 1 + fjacd_dim1 * (1 + fjacd_dim2);
    fjacd -= fjacd_offset;
    fjacb_dim1 = *n;
    fjacb_dim2 = *np;
    fjacb_offset = 1 + fjacb_dim1 * (1 + fjacb_dim2);
    fjacb -= fjacb_offset;
    wrk6_dim1 = *n;
    wrk6_dim2 = *np;
    wrk6_offset = 1 + wrk6_dim1 * (1 + wrk6_dim2);
    wrk6 -= wrk6_offset;
    wrk2_dim1 = *n;
    wrk2_offset = 1 + wrk2_dim1;
    wrk2 -= wrk2_offset;
    wrk1_dim1 = *n;
    wrk1_dim2 = *m;
    wrk1_offset = 1 + wrk1_dim1 * (1 + wrk1_dim2);
    wrk1 -= wrk1_offset;
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;
    stpd_dim1 = *ldstpd;
    stpd_offset = 1 + stpd_dim1;
    stpd -= stpd_offset;
    tt_dim1 = *ldtt;
    tt_offset = 1 + tt_dim1;
    tt -= tt_offset;

    /* Function Body */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:     THE USER SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   BETAK:   THE K-TH FUNCTION PARAMETER. */
/*   DELTA:   THE ESTIMATED ERRORS IN THE EXPLANATORY VARIABLES. */
/*   DOIT:    THE VARIABLE DESIGNATING WHETHER THE DERIVATIVE WRT A GIVEN */
/*            BETA OR DELTA NEEDS TO BE COMPUTED (DOIT=TRUE) OR NOT */
/*            (DOIT=FALSE). */
/*   FJACB:   THE JACOBIAN WITH RESPECT TO BETA. */
/*   FJACD:   THE JACOBIAN WITH RESPECT TO DELTA. */
/*   I:       AN INDEXING VARIABLE. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE FIXED */
/*            AT THEIR INPUT VALUES OR NOT. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR BY OLS (ISODR=FALSE). */
/*   ISTOP:   THE VARIABLE DESIGNATING WHETHER THERE ARE PROBLEMS */
/*            COMPUTING THE FUNCTION AT THE CURRENT BETA AND DELTA. */
/*   J:       AN INDEXING VARIABLE. */
/*   K:       AN INDEXING VARIABLE. */
/*   L:       AN INDEXING VARIABLE. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LDSTPD:  THE LEADING DIMENSION OF ARRAY STPD. */
/*   LDTT:    THE LEADING DIMENSION OF ARRAY TT. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NETA:    THE NUMBER OF GOOD DIGITS IN THE FUNCTION RESULTS. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   ONE:     THE VALUE 1.0D0. */
/*   SETZRO:  THE VARIABLE DESIGNATING WHETHER THE DERIVATIVE WRT SOME */
/*            DELTA NEEDS TO BE SET TO ZERO (SETZRO=TRUE) OR NOT */
/*            (SETZRO=FALSE). */
/*   SSF:     THE SCALING VALUES USED FOR BETA. */
/*   STP:     THE STEP USED FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO EACH DELTA. */
/*   STPB:    THE RELATIVE STEP USED FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO EACH BETA. */
/*   STPD:    THE RELATIVE STEP USED FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO EACH DELTA. */
/*   TT:      THE SCALING VALUES USED FOR DELTA. */
/*   TYPJ:    THE TYPICAL SIZE OF THE J-TH UNKNOWN BETA OR DELTA. */
/*   X:       THE EXPLANATORY VARIABLE. */
/*   XPLUSD:  THE VALUES OF X + DELTA. */
/*   WRK1:    A WORK ARRAY OF (N BY M BY NQ) ELEMENTS. */
/*   WRK2:    A WORK ARRAY OF (N BY NQ) ELEMENTS. */
/*   WRK3:    A WORK ARRAY OF (NP) ELEMENTS. */
/*   WRK6:    A WORK ARRAY OF (N BY NP BY NQ) ELEMENTS. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DJACCD */
/*  COMPUTE THE JACOBIAN WRT THE ESTIMATED BETAS */
    i__1 = *np;
    for (k = 1; k <= i__1; ++k) {
	if (ifixb[1] >= 0) {
	    if (ifixb[k] == 0) {
		doit = FALSE_;
	    } else {
		doit = TRUE_;
	    }
	} else {
	    doit = TRUE_;
	}
	if (! doit) {
	    i__2 = *nq;
	    for (l = 1; l <= i__2; ++l) {
		dzero_(n, &c__1, &fjacb[(k + l * fjacb_dim2) * fjacb_dim1 + 1]
			, n);
/* L10: */
	    }
	} else {
	    betak = beta[k];
	    if (betak == zero) {
		if (ssf[1] < zero) {
		    typj = one / abs(ssf[1]);
		} else {
		    typj = one / ssf[k];
		}
	    } else {
		typj = abs(betak);
	    }
	    wrk3[k] = betak + d_sign(&one, &betak) * typj * dhstep_(&c__1, 
		    neta, &c__1, &k, &stpb[1], &c__1);
	    wrk3[k] -= betak;
	    beta[k] = betak + wrk3[k];
	    *istop = 0;
	    (*fcn)(n, m, np, nq, n, m, np, &beta[1], &xplusd[xplusd_offset], &
		    ifixb[1], &ifixx[ifixx_offset], ldifx, &c__1, &wrk2[
		    wrk2_offset], &wrk6[wrk6_offset], &wrk1[wrk1_offset], 
		    istop);
	    if (*istop != 0) {
		return 0;
	    } else {
		++(*nfev);
		i__2 = *nq;
		for (l = 1; l <= i__2; ++l) {
		    i__3 = *n;
		    for (i__ = 1; i__ <= i__3; ++i__) {
			fjacb[i__ + (k + l * fjacb_dim2) * fjacb_dim1] = wrk2[
				i__ + l * wrk2_dim1];
/* L20: */
		    }
/* L30: */
		}
	    }
	    beta[k] = betak - wrk3[k];
	    *istop = 0;
	    (*fcn)(n, m, np, nq, n, m, np, &beta[1], &xplusd[xplusd_offset], &
		    ifixb[1], &ifixx[ifixx_offset], ldifx, &c__1, &wrk2[
		    wrk2_offset], &wrk6[wrk6_offset], &wrk1[wrk1_offset], 
		    istop);
	    if (*istop != 0) {
		return 0;
	    } else {
		++(*nfev);
	    }
	    i__2 = *nq;
	    for (l = 1; l <= i__2; ++l) {
		i__3 = *n;
		for (i__ = 1; i__ <= i__3; ++i__) {
		    fjacb[i__ + (k + l * fjacb_dim2) * fjacb_dim1] = (fjacb[
			    i__ + (k + l * fjacb_dim2) * fjacb_dim1] - wrk2[
			    i__ + l * wrk2_dim1]) / (wrk3[k] * 2);
/* L40: */
		}
/* L50: */
	    }
	    beta[k] = betak;
	}
/* L60: */
    }
/*  COMPUTE THE JACOBIAN WRT THE X'S */
    if (*isodr) {
	i__1 = *m;
	for (j = 1; j <= i__1; ++j) {
	    if (ifixx[ifixx_dim1 + 1] < 0) {
		doit = TRUE_;
		setzro = FALSE_;
	    } else if (*ldifx == 1) {
		if (ifixx[j * ifixx_dim1 + 1] == 0) {
		    doit = FALSE_;
		} else {
		    doit = TRUE_;
		}
		setzro = FALSE_;
	    } else {
		doit = FALSE_;
		setzro = FALSE_;
		i__2 = *n;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    if (ifixx[i__ + j * ifixx_dim1] != 0) {
			doit = TRUE_;
		    } else {
			setzro = TRUE_;
		    }
/* L100: */
		}
	    }
	    if (! doit) {
		i__2 = *nq;
		for (l = 1; l <= i__2; ++l) {
		    dzero_(n, &c__1, &fjacd[(j + l * fjacd_dim2) * fjacd_dim1 
			    + 1], n);
/* L110: */
		}
	    } else {
		i__2 = *n;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    if (xplusd[i__ + j * xplusd_dim1] == zero) {
			if (tt[tt_dim1 + 1] < zero) {
			    typj = one / (d__1 = tt[tt_dim1 + 1], abs(d__1));
			} else if (*ldtt == 1) {
			    typj = one / tt[j * tt_dim1 + 1];
			} else {
			    typj = one / tt[i__ + j * tt_dim1];
			}
		    } else {
			typj = (d__1 = xplusd[i__ + j * xplusd_dim1], abs(
				d__1));
		    }
		    stp[i__] = xplusd[i__ + j * xplusd_dim1] + d_sign(&one, &
			    xplusd[i__ + j * xplusd_dim1]) * typj * dhstep_(&
			    c__1, neta, &i__, &j, &stpd[stpd_offset], ldstpd);
		    stp[i__] -= xplusd[i__ + j * xplusd_dim1];
		    xplusd[i__ + j * xplusd_dim1] += stp[i__];
/* L120: */
		}
		*istop = 0;
		(*fcn)(n, m, np, nq, n, m, np, &beta[1], &xplusd[
			xplusd_offset], &ifixb[1], &ifixx[ifixx_offset], 
			ldifx, &c__1, &wrk2[wrk2_offset], &wrk6[wrk6_offset], 
			&wrk1[wrk1_offset], istop);
		if (*istop != 0) {
		    return 0;
		} else {
		    ++(*nfev);
		    i__2 = *nq;
		    for (l = 1; l <= i__2; ++l) {
			i__3 = *n;
			for (i__ = 1; i__ <= i__3; ++i__) {
			    fjacd[i__ + (j + l * fjacd_dim2) * fjacd_dim1] = 
				    wrk2[i__ + l * wrk2_dim1];
/* L130: */
			}
/* L140: */
		    }
		}
		i__2 = *n;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    xplusd[i__ + j * xplusd_dim1] = x[i__ + j * x_dim1] + 
			    delta[i__ + j * delta_dim1] - stp[i__];
/* L150: */
		}
		*istop = 0;
		(*fcn)(n, m, np, nq, n, m, np, &beta[1], &xplusd[
			xplusd_offset], &ifixb[1], &ifixx[ifixx_offset], 
			ldifx, &c__1, &wrk2[wrk2_offset], &wrk6[wrk6_offset], 
			&wrk1[wrk1_offset], istop);
		if (*istop != 0) {
		    return 0;
		} else {
		    ++(*nfev);
		}
		if (setzro) {
		    i__2 = *n;
		    for (i__ = 1; i__ <= i__2; ++i__) {
			if (ifixx[i__ + j * ifixx_dim1] == 0) {
			    i__3 = *nq;
			    for (l = 1; l <= i__3; ++l) {
				fjacd[i__ + (j + l * fjacd_dim2) * fjacd_dim1]
					 = zero;
/* L160: */
			    }
			} else {
			    i__3 = *nq;
			    for (l = 1; l <= i__3; ++l) {
				fjacd[i__ + (j + l * fjacd_dim2) * fjacd_dim1]
					 = (fjacd[i__ + (j + l * fjacd_dim2) *
					 fjacd_dim1] - wrk2[i__ + l * 
					wrk2_dim1]) / (stp[i__] * 2);
/* L170: */
			    }
			}
/* L180: */
		    }
		} else {
		    i__2 = *nq;
		    for (l = 1; l <= i__2; ++l) {
			i__3 = *n;
			for (i__ = 1; i__ <= i__3; ++i__) {
			    fjacd[i__ + (j + l * fjacd_dim2) * fjacd_dim1] = (
				    fjacd[i__ + (j + l * fjacd_dim2) * 
				    fjacd_dim1] - wrk2[i__ + l * wrk2_dim1]) /
				     (stp[i__] * 2);
/* L190: */
			}
/* L200: */
		    }
		}
		i__2 = *n;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    xplusd[i__ + j * xplusd_dim1] = x[i__ + j * x_dim1] + 
			    delta[i__ + j * delta_dim1];
/* L210: */
		}
	    }
/* L220: */
	}
    }
    return 0;
} /* djaccd_ */

/* DJACFD */
/* Subroutine */ int djacfd_(S_fp fcn, integer *n, integer *m, integer *np, 
	integer *nq, doublereal *beta, doublereal *x, integer *ldx, 
	doublereal *delta, doublereal *xplusd, integer *ifixb, integer *ifixx,
	 integer *ldifx, doublereal *stpb, doublereal *stpd, integer *ldstpd, 
	doublereal *ssf, doublereal *tt, integer *ldtt, integer *neta, 
	doublereal *fn, doublereal *stp, doublereal *wrk1, doublereal *wrk2, 
	doublereal *wrk3, doublereal *wrk6, doublereal *fjacb, logical *isodr,
	 doublereal *fjacd, integer *nfev, integer *istop)
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal one = 1.;

    /* System generated locals */
    integer delta_dim1, delta_offset, fjacb_dim1, fjacb_dim2, fjacb_offset, 
	    fjacd_dim1, fjacd_dim2, fjacd_offset, fn_dim1, fn_offset, 
	    stpd_dim1, stpd_offset, tt_dim1, tt_offset, wrk1_dim1, wrk1_dim2, 
	    wrk1_offset, wrk2_dim1, wrk2_offset, wrk6_dim1, wrk6_dim2, 
	    wrk6_offset, x_dim1, x_offset, xplusd_dim1, xplusd_offset, 
	    ifixx_dim1, ifixx_offset, i__1, i__2, i__3;
    doublereal d__1;

    /* Builtin functions */
    double d_sign(doublereal *, doublereal *);

    /* Local variables */
    static integer i__, j, k, l;
    static logical doit;
    static doublereal typj, betak;
    extern /* Subroutine */ int dzero_(integer *, integer *, doublereal *, 
	    integer *);
    extern doublereal dhstep_(integer *, integer *, integer *, integer *, 
	    doublereal *, integer *);
    static logical setzro;

/* ***BEGIN PROLOGUE  DJACFD */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  FCN,DHSTEP,DZERO */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  COMPUTE FORWARD DIFFERENCE APPROXIMATIONS TO THE */
/*            JACOBIAN WRT THE ESTIMATED BETAS AND WRT THE DELTAS */
/* ***END PROLOGUE  DJACFD */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...EXTERNAL FUNCTIONS */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --stp;
    xplusd_dim1 = *n;
    xplusd_offset = 1 + xplusd_dim1;
    xplusd -= xplusd_offset;
    delta_dim1 = *n;
    delta_offset = 1 + delta_dim1;
    delta -= delta_offset;
    --wrk3;
    --ssf;
    --stpb;
    --ifixb;
    --beta;
    fjacd_dim1 = *n;
    fjacd_dim2 = *m;
    fjacd_offset = 1 + fjacd_dim1 * (1 + fjacd_dim2);
    fjacd -= fjacd_offset;
    fjacb_dim1 = *n;
    fjacb_dim2 = *np;
    fjacb_offset = 1 + fjacb_dim1 * (1 + fjacb_dim2);
    fjacb -= fjacb_offset;
    wrk6_dim1 = *n;
    wrk6_dim2 = *np;
    wrk6_offset = 1 + wrk6_dim1 * (1 + wrk6_dim2);
    wrk6 -= wrk6_offset;
    wrk2_dim1 = *n;
    wrk2_offset = 1 + wrk2_dim1;
    wrk2 -= wrk2_offset;
    wrk1_dim1 = *n;
    wrk1_dim2 = *m;
    wrk1_offset = 1 + wrk1_dim1 * (1 + wrk1_dim2);
    wrk1 -= wrk1_offset;
    fn_dim1 = *n;
    fn_offset = 1 + fn_dim1;
    fn -= fn_offset;
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;
    stpd_dim1 = *ldstpd;
    stpd_offset = 1 + stpd_dim1;
    stpd -= stpd_offset;
    tt_dim1 = *ldtt;
    tt_offset = 1 + tt_dim1;
    tt -= tt_offset;

    /* Function Body */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:     THE USER SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   BETAK:   THE K-TH FUNCTION PARAMETER. */
/*   DELTA:   THE ESTIMATED ERRORS IN THE EXPLANATORY VARIABLES. */
/*   DOIT:    THE VARIABLE DESIGNATING WHETHER THE DERIVATIVE WRT A */
/*            GIVEN BETA OR DELTA NEEDS TO BE COMPUTED (DOIT=TRUE) */
/*            OR NOT (DOIT=FALSE). */
/*   FJACB:   THE JACOBIAN WITH RESPECT TO BETA. */
/*   FJACD:   THE JACOBIAN WITH RESPECT TO DELTA. */
/*   FN:      THE NEW PREDICTED VALUES FROM THE FUNCTION. */
/*   I:       AN INDEXING VARIABLE. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR BY OLS (ISODR=FALSE). */
/*   ISTOP:   THE VARIABLE DESIGNATING WHETHER THERE ARE PROBLEMS */
/*            COMPUTING THE FUNCTION AT THE CURRENT BETA AND DELTA. */
/*   J:       AN INDEXING VARIABLE. */
/*   K:       AN INDEXING VARIABLE. */
/*   L:       AN INDEXING VARIABLE. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LDSTPD:  THE LEADING DIMENSION OF ARRAY STPD. */
/*   LDTT:    THE LEADING DIMENSION OF ARRAY TT. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NETA:    THE NUMBER OF GOOD DIGITS IN THE FUNCTION RESULTS. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   ONE:     THE VALUE 1.0D0. */
/*   SETZRO:  THE VARIABLE DESIGNATING WHETHER THE DERIVATIVE WRT SOME */
/*            DELTA NEEDS TO BE SET TO ZERO (SETZRO=TRUE) OR NOT */
/*            (SETZRO=FALSE). */
/*   SSF:     THE SCALE USED FOR THE BETA'S. */
/*   STP:     THE STEP USED FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO DELTA. */
/*   STPB:    THE RELATIVE STEP USED FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO BETA. */
/*   STPD:    THE RELATIVE STEP USED FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO DELTA. */
/*   TT:      THE SCALING VALUES USED FOR DELTA. */
/*   TYPJ:    THE TYPICAL SIZE OF THE J-TH UNKNOWN BETA OR DELTA. */
/*   X:       THE EXPLANATORY VARIABLE. */
/*   XPLUSD:  THE VALUES OF X + DELTA. */
/*   WRK1:    A WORK ARRAY OF (N BY M BY NQ) ELEMENTS. */
/*   WRK2:    A WORK ARRAY OF (N BY NQ) ELEMENTS. */
/*   WRK3:    A WORK ARRAY OF (NP) ELEMENTS. */
/*   WRK6:    A WORK ARRAY OF (N BY NP BY NQ) ELEMENTS. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DJACFD */
/*  COMPUTE THE JACOBIAN WRT THE ESTIMATED BETAS */
    i__1 = *np;
    for (k = 1; k <= i__1; ++k) {
	if (ifixb[1] >= 0) {
	    if (ifixb[k] == 0) {
		doit = FALSE_;
	    } else {
		doit = TRUE_;
	    }
	} else {
	    doit = TRUE_;
	}
	if (! doit) {
	    i__2 = *nq;
	    for (l = 1; l <= i__2; ++l) {
		dzero_(n, &c__1, &fjacb[(k + l * fjacb_dim2) * fjacb_dim1 + 1]
			, n);
/* L10: */
	    }
	} else {
	    betak = beta[k];
	    if (betak == zero) {
		if (ssf[1] < zero) {
		    typj = one / abs(ssf[1]);
		} else {
		    typj = one / ssf[k];
		}
	    } else {
		typj = abs(betak);
	    }
	    wrk3[k] = betak + d_sign(&one, &betak) * typj * dhstep_(&c__0, 
		    neta, &c__1, &k, &stpb[1], &c__1);
	    wrk3[k] -= betak;
	    beta[k] = betak + wrk3[k];
	    *istop = 0;
	    (*fcn)(n, m, np, nq, n, m, np, &beta[1], &xplusd[xplusd_offset], &
		    ifixb[1], &ifixx[ifixx_offset], ldifx, &c__1, &wrk2[
		    wrk2_offset], &wrk6[wrk6_offset], &wrk1[wrk1_offset], 
		    istop);
	    if (*istop != 0) {
		return 0;
	    } else {
		++(*nfev);
	    }
	    i__2 = *nq;
	    for (l = 1; l <= i__2; ++l) {
		i__3 = *n;
		for (i__ = 1; i__ <= i__3; ++i__) {
		    fjacb[i__ + (k + l * fjacb_dim2) * fjacb_dim1] = (wrk2[
			    i__ + l * wrk2_dim1] - fn[i__ + l * fn_dim1]) / 
			    wrk3[k];
/* L20: */
		}
/* L30: */
	    }
	    beta[k] = betak;
	}
/* L40: */
    }
/*  COMPUTE THE JACOBIAN WRT THE X'S */
    if (*isodr) {
	i__1 = *m;
	for (j = 1; j <= i__1; ++j) {
	    if (ifixx[ifixx_dim1 + 1] < 0) {
		doit = TRUE_;
		setzro = FALSE_;
	    } else if (*ldifx == 1) {
		if (ifixx[j * ifixx_dim1 + 1] == 0) {
		    doit = FALSE_;
		} else {
		    doit = TRUE_;
		}
		setzro = FALSE_;
	    } else {
		doit = FALSE_;
		setzro = FALSE_;
		i__2 = *n;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    if (ifixx[i__ + j * ifixx_dim1] != 0) {
			doit = TRUE_;
		    } else {
			setzro = TRUE_;
		    }
/* L100: */
		}
	    }
	    if (! doit) {
		i__2 = *nq;
		for (l = 1; l <= i__2; ++l) {
		    dzero_(n, &c__1, &fjacd[(j + l * fjacd_dim2) * fjacd_dim1 
			    + 1], n);
/* L110: */
		}
	    } else {
		i__2 = *n;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    if (xplusd[i__ + j * xplusd_dim1] == zero) {
			if (tt[tt_dim1 + 1] < zero) {
			    typj = one / (d__1 = tt[tt_dim1 + 1], abs(d__1));
			} else if (*ldtt == 1) {
			    typj = one / tt[j * tt_dim1 + 1];
			} else {
			    typj = one / tt[i__ + j * tt_dim1];
			}
		    } else {
			typj = (d__1 = xplusd[i__ + j * xplusd_dim1], abs(
				d__1));
		    }
		    stp[i__] = xplusd[i__ + j * xplusd_dim1] + d_sign(&one, &
			    xplusd[i__ + j * xplusd_dim1]) * typj * dhstep_(&
			    c__0, neta, &i__, &j, &stpd[stpd_offset], ldstpd);
		    stp[i__] -= xplusd[i__ + j * xplusd_dim1];
		    xplusd[i__ + j * xplusd_dim1] += stp[i__];
/* L120: */
		}
		*istop = 0;
		(*fcn)(n, m, np, nq, n, m, np, &beta[1], &xplusd[
			xplusd_offset], &ifixb[1], &ifixx[ifixx_offset], 
			ldifx, &c__1, &wrk2[wrk2_offset], &wrk6[wrk6_offset], 
			&wrk1[wrk1_offset], istop);
		if (*istop != 0) {
		    return 0;
		} else {
		    ++(*nfev);
		    i__2 = *nq;
		    for (l = 1; l <= i__2; ++l) {
			i__3 = *n;
			for (i__ = 1; i__ <= i__3; ++i__) {
			    fjacd[i__ + (j + l * fjacd_dim2) * fjacd_dim1] = 
				    wrk2[i__ + l * wrk2_dim1];
/* L130: */
			}
/* L140: */
		    }
		}
		if (setzro) {
		    i__2 = *n;
		    for (i__ = 1; i__ <= i__2; ++i__) {
			if (ifixx[i__ + j * ifixx_dim1] == 0) {
			    i__3 = *nq;
			    for (l = 1; l <= i__3; ++l) {
				fjacd[i__ + (j + l * fjacd_dim2) * fjacd_dim1]
					 = zero;
/* L160: */
			    }
			} else {
			    i__3 = *nq;
			    for (l = 1; l <= i__3; ++l) {
				fjacd[i__ + (j + l * fjacd_dim2) * fjacd_dim1]
					 = (fjacd[i__ + (j + l * fjacd_dim2) *
					 fjacd_dim1] - fn[i__ + l * fn_dim1]) 
					/ stp[i__];
/* L170: */
			    }
			}
/* L180: */
		    }
		} else {
		    i__2 = *nq;
		    for (l = 1; l <= i__2; ++l) {
			i__3 = *n;
			for (i__ = 1; i__ <= i__3; ++i__) {
			    fjacd[i__ + (j + l * fjacd_dim2) * fjacd_dim1] = (
				    fjacd[i__ + (j + l * fjacd_dim2) * 
				    fjacd_dim1] - fn[i__ + l * fn_dim1]) / 
				    stp[i__];
/* L190: */
			}
/* L200: */
		    }
		}
		i__2 = *n;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    xplusd[i__ + j * xplusd_dim1] = x[i__ + j * x_dim1] + 
			    delta[i__ + j * delta_dim1];
/* L210: */
		}
	    }
/* L220: */
	}
    }
    return 0;
} /* djacfd_ */

/* DJCK */
/* Subroutine */ int djck_(S_fp fcn, integer *n, integer *m, integer *np, 
	integer *nq, doublereal *beta, doublereal *xplusd, integer *ifixb, 
	integer *ifixx, integer *ldifx, doublereal *stpb, doublereal *stpd, 
	integer *ldstpd, doublereal *ssf, doublereal *tt, integer *ldtt, 
	doublereal *eta, integer *neta, integer *ntol, integer *nrow, logical 
	*isodr, doublereal *epsmac, doublereal *pv0, doublereal *fjacb, 
	doublereal *fjacd, integer *msgb, integer *msgd, doublereal *diff, 
	integer *istop, integer *nfev, integer *njev, doublereal *wrk1, 
	doublereal *wrk2, doublereal *wrk6)
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal p5 = .5;
    static doublereal one = 1.;

    /* System generated locals */
    integer diff_dim1, diff_offset, fjacb_dim1, fjacb_dim2, fjacb_offset, 
	    fjacd_dim1, fjacd_dim2, fjacd_offset, pv0_dim1, pv0_offset, 
	    stpd_dim1, stpd_offset, tt_dim1, tt_offset, wrk1_dim1, wrk1_dim2, 
	    wrk1_offset, wrk2_dim1, wrk2_offset, wrk6_dim1, wrk6_dim2, 
	    wrk6_offset, xplusd_dim1, xplusd_offset, ifixx_dim1, ifixx_offset,
	     i__1, i__2;
    doublereal d__1, d__2;

    /* Builtin functions */
    double pow_dd(doublereal *, doublereal *), d_lg10(doublereal *);

    /* Local variables */
    static integer j;
    static doublereal h0;
    static integer lq;
    static doublereal pv, hc0, tol, typj;
    static integer msgb1, msgd1;
    static doublereal diffj;
    extern /* Subroutine */ int djckm_(S_fp, integer *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, doublereal *, 
	    integer *, integer *, doublereal *, doublereal *, doublereal *, 
	    logical *, doublereal *, doublereal *, doublereal *, integer *, 
	    integer *, integer *, integer *, doublereal *, doublereal *, 
	    doublereal *);
    static integer ideval;
    static logical isfixd;
    extern doublereal dhstep_(integer *, integer *, integer *, integer *, 
	    doublereal *, integer *);
    static logical iswrtb;

/* ***BEGIN PROLOGUE  DJCK */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  FCN,DHSTEP,DJCKM */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  DRIVER ROUTINE FOR THE DERIVATIVE CHECKING PROCESS */
/*            (ADAPTED FROM STARPAC SUBROUTINE DCKCNT) */
/* ***END PROLOGUE  DJCK */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...EXTERNAL FUNCTIONS */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    xplusd_dim1 = *n;
    xplusd_offset = 1 + xplusd_dim1;
    xplusd -= xplusd_offset;
    --ssf;
    --stpb;
    --ifixb;
    --beta;
    wrk6_dim1 = *n;
    wrk6_dim2 = *np;
    wrk6_offset = 1 + wrk6_dim1 * (1 + wrk6_dim2);
    wrk6 -= wrk6_offset;
    wrk2_dim1 = *n;
    wrk2_offset = 1 + wrk2_dim1;
    wrk2 -= wrk2_offset;
    wrk1_dim1 = *n;
    wrk1_dim2 = *m;
    wrk1_offset = 1 + wrk1_dim1 * (1 + wrk1_dim2);
    wrk1 -= wrk1_offset;
    diff_dim1 = *nq;
    diff_offset = 1 + diff_dim1;
    diff -= diff_offset;
    --msgd;
    --msgb;
    fjacd_dim1 = *n;
    fjacd_dim2 = *m;
    fjacd_offset = 1 + fjacd_dim1 * (1 + fjacd_dim2);
    fjacd -= fjacd_offset;
    fjacb_dim1 = *n;
    fjacb_dim2 = *np;
    fjacb_offset = 1 + fjacb_dim1 * (1 + fjacb_dim2);
    fjacb -= fjacb_offset;
    pv0_dim1 = *n;
    pv0_offset = 1 + pv0_dim1;
    pv0 -= pv0_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;
    stpd_dim1 = *ldstpd;
    stpd_offset = 1 + stpd_dim1;
    stpd -= stpd_offset;
    tt_dim1 = *ldtt;
    tt_offset = 1 + tt_dim1;
    tt -= tt_offset;

    /* Function Body */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:     THE USER SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   DIFF:    THE RELATIVE DIFFERENCES BETWEEN THE USER SUPPLIED AND */
/*            FINITE DIFFERENCE DERIVATIVES FOR EACH DERIVATIVE CHECKED. */
/*   DIFFJ:   THE RELATIVE DIFFERENCES BETWEEN THE USER SUPPLIED AND */
/*            FINITE DIFFERENCE DERIVATIVES FOR THE DERIVATIVE BEING */
/*            CHECKED. */
/*   EPSMAC:  THE VALUE OF MACHINE PRECISION. */
/*   ETA:     THE RELATIVE NOISE IN THE FUNCTION RESULTS. */
/*   FJACB:   THE JACOBIAN WITH RESPECT TO BETA. */
/*   FJACD:   THE JACOBIAN WITH RESPECT TO DELTA. */
/*   H0:      THE INITIAL RELATIVE STEP SIZE FOR FORWARD DIFFERENCES. */
/*   HC0:     THE INITIAL RELATIVE STEP SIZE FOR CENTRAL DIFFERENCES. */
/*   IDEVAL:  THE VARIABLE DESIGNATING WHAT COMPUTATIONS ARE TO BE */
/*            PERFORMED BY USER SUPPLIED SUBROUTINE FCN. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   ISFIXD:  THE VARIABLE DESIGNATING WHETHER THE PARAMETER IS FIXED */
/*            (ISFIXD=TRUE) OR NOT (ISFIXD=FALSE). */
/*   ISTOP:   THE VARIABLE DESIGNATING WHETHER THERE ARE PROBLEMS */
/*            COMPUTING THE FUNCTION AT THE CURRENT BETA AND DELTA. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=.TRUE.) OR BY OLS (ISODR=.FALSE.). */
/*   ISWRTB:  THE VARIABLE DESIGNATING WHETHER THE DERIVATIVES WRT BETA */
/*            (ISWRTB=TRUE) OR DELTA (ISWRTB=FALSE) ARE BEING CHECKED. */
/*   J:       AN INDEX VARIABLE. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LDSTPD:  THE LEADING DIMENSION OF ARRAY STPD. */
/*   LDTT:    THE LEADING DIMENSION OF ARRAY TT. */
/*   LQ:      THE RESPONSE CURRENTLY BEING EXAMINED. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   MSGB:    THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT BETA. */
/*   MSGB1:   THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT BETA. */
/*   MSGD:    THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT DELTA. */
/*   MSGD1:   THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT DELTA. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NETA:    THE NUMBER OF RELIABLE DIGITS IN THE MODEL RESULTS, EITHER */
/*            SET BY THE USER OR COMPUTED BY DETAF. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NJEV:    THE NUMBER OF JACOBIAN EVALUATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   NROW:    THE ROW NUMBER OF THE EXPLANATORY VARIABLE ARRAY AT WHICH */
/*            THE DERIVATIVE IS CHECKED. */
/*   NTOL:    THE NUMBER OF DIGITS OF AGREEMENT REQUIRED BETWEEN THE */
/*            NUMERICAL DERIVATIVES AND THE USER SUPPLIED DERIVATIVES. */
/*   ONE:     THE VALUE 1.0D0. */
/*   P5:      THE VALUE 0.5D0. */
/*   PV:      THE SCALAR IN WHICH THE PREDICTED VALUE FROM THE MODEL FOR */
/*            ROW   NROW   IS STORED. */
/*   PV0:     THE PREDICTED VALUES USING THE CURRENT PARAMETER ESTIMATES. */
/*   SSF:     THE SCALING VALUES USED FOR BETA. */
/*   STPB:    THE STEP SIZE FOR FINITE DIFFERENCE DERIVATIVES WRT BETA. */
/*   STPD:    THE STEP SIZE FOR FINITE DIFFERENCE DERIVATIVES WRT DELTA. */
/*   TOL:     THE AGREEMENT TOLERANCE. */
/*   TT:      THE SCALING VALUES USED FOR DELTA. */
/*   TYPJ:    THE TYPICAL SIZE OF THE J-TH UNKNOWN BETA OR DELTA. */
/*   WRK1:    A WORK ARRAY OF (N BY M BY NQ) ELEMENTS. */
/*   WRK2:    A WORK ARRAY OF (N BY NQ) ELEMENTS. */
/*   WRK6:    A WORK ARRAY OF (N BY NP BY NQ) ELEMENTS. */
/*   XPLUSD:  THE VALUES OF X + DELTA. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DJCK */
/*  SET TOLERANCE FOR CHECKING DERIVATIVES */
    tol = pow_dd(eta, &c_b150);
/* Computing MAX */
    d__1 = one, d__2 = p5 - d_lg10(&tol);
    *ntol = (integer) max(d__1,d__2);
/*  COMPUTE USER SUPPLIED DERIVATIVE VALUES */
    *istop = 0;
    if (*isodr) {
	ideval = 110;
    } else {
	ideval = 10;
    }
    (*fcn)(n, m, np, nq, n, m, np, &beta[1], &xplusd[xplusd_offset], &ifixb[1]
	    , &ifixx[ifixx_offset], ldifx, &ideval, &wrk2[wrk2_offset], &
	    fjacb[fjacb_offset], &fjacd[fjacd_offset], istop);
    if (*istop != 0) {
	return 0;
    } else {
	++(*njev);
    }
/*  CHECK DERIVATIVES WRT BETA FOR EACH RESPONSE OF OBSERVATION NROW */
    msgb1 = 0;
    msgd1 = 0;
    i__1 = *nq;
    for (lq = 1; lq <= i__1; ++lq) {
/*  SET PREDICTED VALUE OF MODEL AT CURRENT PARAMETER ESTIMATES */
	pv = pv0[*nrow + lq * pv0_dim1];
	iswrtb = TRUE_;
	i__2 = *np;
	for (j = 1; j <= i__2; ++j) {
	    if (ifixb[1] < 0) {
		isfixd = FALSE_;
	    } else if (ifixb[j] == 0) {
		isfixd = TRUE_;
	    } else {
		isfixd = FALSE_;
	    }
	    if (isfixd) {
		msgb[lq + 1 + (j - 1) * *nq] = -1;
	    } else {
		if (beta[j] == zero) {
		    if (ssf[1] < zero) {
			typj = one / abs(ssf[1]);
		    } else {
			typj = one / ssf[j];
		    }
		} else {
		    typj = (d__1 = beta[j], abs(d__1));
		}
		h0 = dhstep_(&c__0, neta, &c__1, &j, &stpb[1], &c__1);
		hc0 = h0;
/*  CHECK DERIVATIVE WRT THE J-TH PARAMETER AT THE NROW-TH ROW */
		djckm_((S_fp)fcn, n, m, np, nq, &beta[1], &xplusd[
			xplusd_offset], &ifixb[1], &ifixx[ifixx_offset], 
			ldifx, eta, &tol, nrow, epsmac, &j, &lq, &typj, &h0, &
			hc0, &iswrtb, &pv, &fjacb[*nrow + (j + lq * 
			fjacb_dim2) * fjacb_dim1], &diffj, &msgb1, &msgb[2], 
			istop, nfev, &wrk1[wrk1_offset], &wrk2[wrk2_offset], &
			wrk6[wrk6_offset]);
		if (*istop != 0) {
		    msgb[1] = -1;
		    return 0;
		} else {
		    diff[lq + j * diff_dim1] = diffj;
		}
	    }
/* L10: */
	}
/*  CHECK DERIVATIVES WRT X FOR EACH RESPONSE OF OBSERVATION NROW */
	if (*isodr) {
	    iswrtb = FALSE_;
	    i__2 = *m;
	    for (j = 1; j <= i__2; ++j) {
		if (ifixx[ifixx_dim1 + 1] < 0) {
		    isfixd = FALSE_;
		} else if (*ldifx == 1) {
		    if (ifixx[j * ifixx_dim1 + 1] == 0) {
			isfixd = TRUE_;
		    } else {
			isfixd = FALSE_;
		    }
		} else {
		    isfixd = FALSE_;
		}
		if (isfixd) {
		    msgd[lq + 1 + (j - 1) * *nq] = -1;
		} else {
		    if (xplusd[*nrow + j * xplusd_dim1] == zero) {
			if (tt[tt_dim1 + 1] < zero) {
			    typj = one / (d__1 = tt[tt_dim1 + 1], abs(d__1));
			} else if (*ldtt == 1) {
			    typj = one / tt[j * tt_dim1 + 1];
			} else {
			    typj = one / tt[*nrow + j * tt_dim1];
			}
		    } else {
			typj = (d__1 = xplusd[*nrow + j * xplusd_dim1], abs(
				d__1));
		    }
		    h0 = dhstep_(&c__0, neta, nrow, &j, &stpd[stpd_offset], 
			    ldstpd);
		    hc0 = dhstep_(&c__1, neta, nrow, &j, &stpd[stpd_offset], 
			    ldstpd);
/*  CHECK DERIVATIVE WRT THE J-TH COLUMN OF DELTA AT ROW NROW */
		    djckm_((S_fp)fcn, n, m, np, nq, &beta[1], &xplusd[
			    xplusd_offset], &ifixb[1], &ifixx[ifixx_offset], 
			    ldifx, eta, &tol, nrow, epsmac, &j, &lq, &typj, &
			    h0, &hc0, &iswrtb, &pv, &fjacd[*nrow + (j + lq * 
			    fjacd_dim2) * fjacd_dim1], &diffj, &msgd1, &msgd[
			    2], istop, nfev, &wrk1[wrk1_offset], &wrk2[
			    wrk2_offset], &wrk6[wrk6_offset]);
		    if (*istop != 0) {
			msgd[1] = -1;
			return 0;
		    } else {
			diff[lq + (*np + j) * diff_dim1] = diffj;
		    }
		}
/* L20: */
	    }
	}
/* L30: */
    }
    msgb[1] = msgb1;
    msgd[1] = msgd1;
    return 0;
} /* djck_ */

/* DJCKC */
/* Subroutine */ int djckc_(U_fp fcn, integer *n, integer *m, integer *np, 
	integer *nq, doublereal *beta, doublereal *xplusd, integer *ifixb, 
	integer *ifixx, integer *ldifx, doublereal *eta, doublereal *tol, 
	integer *nrow, doublereal *epsmac, integer *j, integer *lq, 
	doublereal *hc, logical *iswrtb, doublereal *fd, doublereal *typj, 
	doublereal *pvpstp, doublereal *stp0, doublereal *pv, doublereal *d__,
	 doublereal *diffj, integer *msg, integer *istop, integer *nfev, 
	doublereal *wrk1, doublereal *wrk2, doublereal *wrk6)
{
    /* Initialized data */

    static doublereal p01 = .01;
    static doublereal one = 1.;
    static doublereal two = 2.;
    static doublereal ten = 10.;

    /* System generated locals */
    integer wrk1_dim1, wrk1_dim2, wrk1_offset, wrk2_dim1, wrk2_offset, 
	    wrk6_dim1, wrk6_dim2, wrk6_offset, xplusd_dim1, xplusd_offset, 
	    ifixx_dim1, ifixx_offset, msg_dim1, msg_offset;
    doublereal d__1, d__2, d__3;

    /* Builtin functions */
    double d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal stp;
    extern /* Subroutine */ int dpvb_(U_fp, integer *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, doublereal *, integer 
	    *, integer *, doublereal *, doublereal *, doublereal *, 
	    doublereal *), dpvd_(U_fp, integer *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, doublereal *, integer 
	    *, integer *, doublereal *, doublereal *, doublereal *, 
	    doublereal *), djckf_(U_fp, integer *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, integer *, 
	    integer *, logical *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, integer *, integer *, integer *, doublereal *, 
	    doublereal *, doublereal *);
    static doublereal curve, pvmcrv, pvpcrv, stpcrv;

/* ***BEGIN PROLOGUE  DJCKC */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DJCKF,DPVB,DPVD */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  CHECK WHETHER HIGH CURVATURE COULD BE THE CAUSE OF THE */
/*            DISAGREEMENT BETWEEN THE NUMERICAL AND ANALYTIC DERVIATIVES */
/*            (ADAPTED FROM STARPAC SUBROUTINE DCKCRV) */
/* ***END PROLOGUE  DJCKC */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    xplusd_dim1 = *n;
    xplusd_offset = 1 + xplusd_dim1;
    xplusd -= xplusd_offset;
    --ifixb;
    --beta;
    wrk6_dim1 = *n;
    wrk6_dim2 = *np;
    wrk6_offset = 1 + wrk6_dim1 * (1 + wrk6_dim2);
    wrk6 -= wrk6_offset;
    wrk2_dim1 = *n;
    wrk2_offset = 1 + wrk2_dim1;
    wrk2 -= wrk2_offset;
    wrk1_dim1 = *n;
    wrk1_dim2 = *m;
    wrk1_offset = 1 + wrk1_dim1 * (1 + wrk1_dim2);
    wrk1 -= wrk1_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;
    msg_dim1 = *nq;
    msg_offset = 1 + msg_dim1;
    msg -= msg_offset;

    /* Function Body */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:     THE USER SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   CURVE:   A MEASURE OF THE CURVATURE IN THE MODEL. */
/*   D:       THE DERIVATIVE WITH RESPECT TO THE JTH UNKNOWN PARAMETER. */
/*   DIFFJ:   THE RELATIVE DIFFERENCES BETWEEN THE USER SUPPLIED AND */
/*            FINITE DIFFERENCE DERIVATIVES FOR THE DERIVATIVE BEING */
/*            CHECKED. */
/*   EPSMAC:  THE VALUE OF MACHINE PRECISION. */
/*   ETA:     THE RELATIVE NOISE IN THE MODEL */
/*   FD:      THE FORWARD DIFFERENCE DERIVATIVE WRT THE JTH PARAMETER. */
/*   HC:      THE RELATIVE STEP SIZE FOR CENTRAL FINITE DIFFERENCES. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   ISTOP:   THE VARIABLE DESIGNATING WHETHER THERE ARE PROBLEMS */
/*            COMPUTING THE FUNCTION AT THE CURRENT BETA AND DELTA. */
/*   ISWRTB:  THE VARIABLE DESIGNATING WHETHER THE DERIVATIVES WRT BETA */
/*            (ISWRTB=TRUE) OR DELTA(ISWRTB=FALSE) ARE BEING CHECKED. */
/*   J:       THE INDEX OF THE PARTIAL DERIVATIVE BEING EXAMINED. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LQ:      THE RESPONSE CURRENTLY BEING EXAMINED. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   MSG:     THE ERROR CHECKING RESULTS. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   NROW:    THE ROW NUMBER OF THE EXPLANATORY VARIABLE ARRAY AT WHICH */
/*            THE DERIVATIVE IS TO BE CHECKED. */
/*   ONE:     THE VALUE 1.0D0. */
/*   PV:      THE PREDICTED VALUE OF THE MODEL FOR ROW   NROW   . */
/*   PVMCRV:  THE PREDICTED VALUE FOR ROW    NROW   OF THE MODEL */
/*            BASED ON THE CURRENT PARAMETER ESTIMATES FOR ALL BUT THE */
/*            JTH PARAMETER VALUE, WHICH IS BETA(J)-STPCRV. */
/*   PVPCRV:  THE PREDICTED VALUE FOR ROW    NROW   OF THE MODEL */
/*            BASED ON THE CURRENT PARAMETER ESTIMATES FOR ALL BUT THE */
/*            JTH PARAMETER VALUE, WHICH IS BETA(J)+STPCRV. */
/*   PVPSTP:  THE PREDICTED VALUE FOR ROW    NROW   OF THE MODEL */
/*            BASED ON THE CURRENT PARAMETER ESTIMATES FOR ALL BUT THE */
/*            JTH PARAMETER VALUE, WHICH IS BETA(J) + STP0. */
/*   P01:     THE VALUE 0.01D0. */
/*   STP0:    THE INITIAL STEP SIZE FOR THE FINITE DIFFERENCE DERIVATIVE. */
/*   STP:     A STEP SIZE FOR THE FINITE DIFFERENCE DERIVATIVE. */
/*   STPCRV:  THE STEP SIZE SELECTED TO CHECK FOR CURVATURE IN THE MODEL. */
/*   TEN:     THE VALUE 10.0D0. */
/*   TOL:     THE AGREEMENT TOLERANCE. */
/*   TWO:     THE VALUE 2.0D0. */
/*   TYPJ:    THE TYPICAL SIZE OF THE J-TH UNKNOWN BETA OR DELTA. */
/*   WRK1:    A WORK ARRAY OF (N BY M BY NQ) ELEMENTS. */
/*   WRK2:    A WORK ARRAY OF (N BY NQ) ELEMENTS. */
/*   WRK6:    A WORK ARRAY OF (N BY NP BY NQ) ELEMENTS. */
/*   XPLUSD:  THE VALUES OF X + DELTA. */
/* ***FIRST EXECUTABLE STATEMENT  DJCKC */
    if (*iswrtb) {
/*  PERFORM CENTRAL DIFFERENCE COMPUTATIONS FOR DERIVATIVES WRT BETA */
	stpcrv = *hc * *typj * d_sign(&one, &beta[*j]) + beta[*j] - beta[*j];
	dpvb_((U_fp)fcn, n, m, np, nq, &beta[1], &xplusd[xplusd_offset], &
		ifixb[1], &ifixx[ifixx_offset], ldifx, nrow, j, lq, &stpcrv, 
		istop, nfev, &pvpcrv, &wrk1[wrk1_offset], &wrk2[wrk2_offset], 
		&wrk6[wrk6_offset]);
	if (*istop != 0) {
	    return 0;
	}
	d__1 = -stpcrv;
	dpvb_((U_fp)fcn, n, m, np, nq, &beta[1], &xplusd[xplusd_offset], &
		ifixb[1], &ifixx[ifixx_offset], ldifx, nrow, j, lq, &d__1, 
		istop, nfev, &pvmcrv, &wrk1[wrk1_offset], &wrk2[wrk2_offset], 
		&wrk6[wrk6_offset]);
	if (*istop != 0) {
	    return 0;
	}
    } else {
/*  PERFORM CENTRAL DIFFERENCE COMPUTATIONS FOR DERIVATIVES WRT DELTA */
	stpcrv = *hc * *typj * d_sign(&one, &xplusd[*nrow + *j * xplusd_dim1])
		 + xplusd[*nrow + *j * xplusd_dim1] - xplusd[*nrow + *j * 
		xplusd_dim1];
	dpvd_((U_fp)fcn, n, m, np, nq, &beta[1], &xplusd[xplusd_offset], &
		ifixb[1], &ifixx[ifixx_offset], ldifx, nrow, j, lq, &stpcrv, 
		istop, nfev, &pvpcrv, &wrk1[wrk1_offset], &wrk2[wrk2_offset], 
		&wrk6[wrk6_offset]);
	if (*istop != 0) {
	    return 0;
	}
	d__1 = -stpcrv;
	dpvd_((U_fp)fcn, n, m, np, nq, &beta[1], &xplusd[xplusd_offset], &
		ifixb[1], &ifixx[ifixx_offset], ldifx, nrow, j, lq, &d__1, 
		istop, nfev, &pvmcrv, &wrk1[wrk1_offset], &wrk2[wrk2_offset], 
		&wrk6[wrk6_offset]);
	if (*istop != 0) {
	    return 0;
	}
    }
/*  ESTIMATE CURVATURE BY SECOND DERIVATIVE OF MODEL */
    curve = (d__1 = pvpcrv - *pv + (pvmcrv - *pv), abs(d__1)) / (stpcrv * 
	    stpcrv);
/* Computing 2nd power */
    d__1 = stpcrv;
    curve += *eta * (abs(pvpcrv) + abs(pvmcrv) + two * abs(*pv)) / (d__1 * 
	    d__1);
/*  CHECK IF FINITE PRECISION ARITHMETIC COULD BE THE CULPRIT. */
    djckf_((U_fp)fcn, n, m, np, nq, &beta[1], &xplusd[xplusd_offset], &ifixb[
	    1], &ifixx[ifixx_offset], ldifx, eta, tol, nrow, j, lq, iswrtb, 
	    fd, typj, pvpstp, stp0, &curve, pv, d__, diffj, &msg[msg_offset], 
	    istop, nfev, &wrk1[wrk1_offset], &wrk2[wrk2_offset], &wrk6[
	    wrk6_offset]);
    if (*istop != 0) {
	return 0;
    }
    if (msg[*lq + *j * msg_dim1] == 0) {
	return 0;
    }
/*  CHECK IF HIGH CURVATURE COULD BE THE PROBLEM. */
/* Computing MAX */
    d__1 = *tol * abs(*d__) / curve;
    stp = two * max(d__1,*epsmac);
    if (stp < (d__1 = ten * *stp0, abs(d__1))) {
/* Computing MIN */
	d__1 = stp, d__2 = p01 * abs(*stp0);
	stp = min(d__1,d__2);
    }
    if (*iswrtb) {
/*  PERFORM COMPUTATIONS FOR DERIVATIVES WRT BETA */
	stp = stp * d_sign(&one, &beta[*j]) + beta[*j] - beta[*j];
	dpvb_((U_fp)fcn, n, m, np, nq, &beta[1], &xplusd[xplusd_offset], &
		ifixb[1], &ifixx[ifixx_offset], ldifx, nrow, j, lq, &stp, 
		istop, nfev, pvpstp, &wrk1[wrk1_offset], &wrk2[wrk2_offset], &
		wrk6[wrk6_offset]);
	if (*istop != 0) {
	    return 0;
	}
    } else {
/*  PERFORM COMPUTATIONS FOR DERIVATIVES WRT DELTA */
	stp = stp * d_sign(&one, &xplusd[*nrow + *j * xplusd_dim1]) + xplusd[*
		nrow + *j * xplusd_dim1] - xplusd[*nrow + *j * xplusd_dim1];
	dpvd_((U_fp)fcn, n, m, np, nq, &beta[1], &xplusd[xplusd_offset], &
		ifixb[1], &ifixx[ifixx_offset], ldifx, nrow, j, lq, &stp, 
		istop, nfev, pvpstp, &wrk1[wrk1_offset], &wrk2[wrk2_offset], &
		wrk6[wrk6_offset]);
	if (*istop != 0) {
	    return 0;
	}
    }
/*  COMPUTE THE NEW NUMERICAL DERIVATIVE */
    *fd = (*pvpstp - *pv) / stp;
/* Computing MIN */
    d__2 = *diffj, d__3 = (d__1 = *fd - *d__, abs(d__1)) / abs(*d__);
    *diffj = min(d__2,d__3);
/*  CHECK WHETHER THE NEW NUMERICAL DERIVATIVE IS OK */
    if ((d__1 = *fd - *d__, abs(d__1)) <= *tol * abs(*d__)) {
	msg[*lq + *j * msg_dim1] = 0;
/*  CHECK IF FINITE PRECISION MAY BE THE CULPRIT (FUDGE FACTOR = 2) */
    } else /* if(complicated condition) */ {
/* Computing 2nd power */
	d__2 = *epsmac * *typj;
	if ((d__1 = stp * (*fd - *d__), abs(d__1)) < two * *eta * (abs(*pv) + 
		abs(*pvpstp)) + curve * (d__2 * d__2)) {
	    msg[*lq + *j * msg_dim1] = 5;
	}
    }
    return 0;
} /* djckc_ */

/* DJCKF */
/* Subroutine */ int djckf_(U_fp fcn, integer *n, integer *m, integer *np, 
	integer *nq, doublereal *beta, doublereal *xplusd, integer *ifixb, 
	integer *ifixx, integer *ldifx, doublereal *eta, doublereal *tol, 
	integer *nrow, integer *j, integer *lq, logical *iswrtb, doublereal *
	fd, doublereal *typj, doublereal *pvpstp, doublereal *stp0, 
	doublereal *curve, doublereal *pv, doublereal *d__, doublereal *diffj,
	 integer *msg, integer *istop, integer *nfev, doublereal *wrk1, 
	doublereal *wrk2, doublereal *wrk6)
{
    /* Initialized data */

    static doublereal p1 = .1;
    static doublereal one = 1.;
    static doublereal two = 2.;
    static doublereal hundrd = 100.;

    /* System generated locals */
    integer wrk1_dim1, wrk1_dim2, wrk1_offset, wrk2_dim1, wrk2_offset, 
	    wrk6_dim1, wrk6_dim2, wrk6_offset, xplusd_dim1, xplusd_offset, 
	    ifixx_dim1, ifixx_offset, msg_dim1, msg_offset;
    doublereal d__1, d__2, d__3;

    /* Builtin functions */
    double d_sign(doublereal *, doublereal *);

    /* Local variables */
    static doublereal stp;
    extern /* Subroutine */ int dpvb_(U_fp, integer *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, doublereal *, integer 
	    *, integer *, doublereal *, doublereal *, doublereal *, 
	    doublereal *), dpvd_(U_fp, integer *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, doublereal *, integer 
	    *, integer *, doublereal *, doublereal *, doublereal *, 
	    doublereal *);
    static logical large;

/* ***BEGIN PROLOGUE  DJCKF */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DPVB,DPVD */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  CHECK WHETHER FINITE PRECISION ARITHMETIC COULD BE THE */
/*            CAUSE OF THE DISAGREEMENT BETWEEN THE DERIVATIVES */
/*            (ADAPTED FROM STARPAC SUBROUTINE DCKFPA) */
/* ***END PROLOGUE  DJCKF */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    xplusd_dim1 = *n;
    xplusd_offset = 1 + xplusd_dim1;
    xplusd -= xplusd_offset;
    --ifixb;
    --beta;
    wrk6_dim1 = *n;
    wrk6_dim2 = *np;
    wrk6_offset = 1 + wrk6_dim1 * (1 + wrk6_dim2);
    wrk6 -= wrk6_offset;
    wrk2_dim1 = *n;
    wrk2_offset = 1 + wrk2_dim1;
    wrk2 -= wrk2_offset;
    wrk1_dim1 = *n;
    wrk1_dim2 = *m;
    wrk1_offset = 1 + wrk1_dim1 * (1 + wrk1_dim2);
    wrk1 -= wrk1_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;
    msg_dim1 = *nq;
    msg_offset = 1 + msg_dim1;
    msg -= msg_offset;

    /* Function Body */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:     THE USER SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   CURVE:   A MEASURE OF THE CURVATURE IN THE MODEL. */
/*   D:       THE DERIVATIVE WITH RESPECT TO THE JTH UNKNOWN PARAMETER. */
/*   DIFFJ:   THE RELATIVE DIFFERENCES BETWEEN THE USER SUPPLIED AND */
/*            FINITE DIFFERENCE DERIVATIVES FOR THE DERIVATIVE BEING */
/*            CHECKED. */
/*   ETA:     THE RELATIVE NOISE IN THE MODEL */
/*   FD:      THE FORWARD DIFFERENCE DERIVATIVE WRT THE JTH PARAMETER. */
/*   HUNDRD:  THE VALUE 100.0D0. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   ISTOP:   THE VARIABLE DESIGNATING WHETHER THERE ARE PROBLEMS */
/*            COMPUTING THE FUNCTION AT THE CURRENT BETA AND DELTA. */
/*   ISWRTB:  THE VARIABLE DESIGNATING WHETHER THE DERIVATIVES WRT BETA */
/*            (ISWRTB=TRUE) OR DELTA(ISWRTB=FALSE) ARE BEING CHECKED. */
/*   J:       THE INDEX OF THE PARTIAL DERIVATIVE BEING EXAMINED. */
/*   LARGE:   THE VALUE DESIGNATING WHETHER THE RECOMMENDED INCREASE IN */
/*            THE STEP SIZE WOULD BE GREATER THAN TYPJ. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LQ:      THE RESPONSE CURRENTLY BEING EXAMINED. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   MSG:     THE ERROR CHECKING RESULTS. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   NROW:    THE ROW NUMBER OF THE EXPLANATORY VARIABLE ARRAY AT WHICH */
/*            THE DERIVATIVE IS TO BE CHECKED. */
/*   ONE:     THE VALUE 1.0D0. */
/*   PV:      THE PREDICTED VALUE FOR ROW   NROW   . */
/*   PVPSTP:  THE PREDICTED VALUE FOR ROW    NROW   OF THE MODEL */
/*            BASED ON THE CURRENT PARAMETER ESTIMATES FOR ALL BUT THE */
/*            JTH PARAMETER VALUE, WHICH IS BETA(J) + STP0. */
/*   P1:      THE VALUE 0.1D0. */
/*   STP0:     THE STEP SIZE FOR THE FINITE DIFFERENCE DERIVATIVE. */
/*   TOL:     THE AGREEMENT TOLERANCE. */
/*   TWO:     THE VALUE 2.0D0. */
/*   TYPJ:    THE TYPICAL SIZE OF THE J-TH UNKNOWN BETA OR DELTA. */
/*   WRK1:    A WORK ARRAY OF (N BY M BY NQ) ELEMENTS. */
/*   WRK2:    A WORK ARRAY OF (N BY NQ) ELEMENTS. */
/*   WRK6:    A WORK ARRAY OF (N BY NP BY NQ) ELEMENTS. */
/*   XPLUSD:  THE VALUES OF X + DELTA. */
/* ***FIRST EXECUTABLE STATEMENT  DJCKF */
/*  FINITE PRECISION ARITHMETIC COULD BE THE PROBLEM. */
/*  TRY A LARGER STEP SIZE BASED ON ESTIMATE OF CONDITION ERROR */
    stp = *eta * (abs(*pv) + abs(*pvpstp)) / (*tol * abs(*d__));
    if (stp > (d__1 = p1 * *stp0, abs(d__1))) {
/* Computing MAX */
	d__1 = stp, d__2 = hundrd * abs(*stp0);
	stp = max(d__1,d__2);
    }
    if (stp > *typj) {
	stp = *typj;
	large = TRUE_;
    } else {
	large = FALSE_;
    }
    if (*iswrtb) {
/*  PERFORM COMPUTATIONS FOR DERIVATIVES WRT BETA */
	stp = stp * d_sign(&one, &beta[*j]) + beta[*j] - beta[*j];
	dpvb_((U_fp)fcn, n, m, np, nq, &beta[1], &xplusd[xplusd_offset], &
		ifixb[1], &ifixx[ifixx_offset], ldifx, nrow, j, lq, &stp, 
		istop, nfev, pvpstp, &wrk1[wrk1_offset], &wrk2[wrk2_offset], &
		wrk6[wrk6_offset]);
    } else {
/*  PERFORM COMPUTATIONS FOR DERIVATIVES WRT DELTA */
	stp = stp * d_sign(&one, &xplusd[*nrow + *j * xplusd_dim1]) + xplusd[*
		nrow + *j * xplusd_dim1] - xplusd[*nrow + *j * xplusd_dim1];
	dpvd_((U_fp)fcn, n, m, np, nq, &beta[1], &xplusd[xplusd_offset], &
		ifixb[1], &ifixx[ifixx_offset], ldifx, nrow, j, lq, &stp, 
		istop, nfev, pvpstp, &wrk1[wrk1_offset], &wrk2[wrk2_offset], &
		wrk6[wrk6_offset]);
    }
    if (*istop != 0) {
	return 0;
    }
    *fd = (*pvpstp - *pv) / stp;
/* Computing MIN */
    d__2 = *diffj, d__3 = (d__1 = *fd - *d__, abs(d__1)) / abs(*d__);
    *diffj = min(d__2,d__3);
/*  CHECK FOR AGREEMENT */
    if ((d__1 = *fd - *d__, abs(d__1)) <= *tol * abs(*d__)) {
/*  FORWARD DIFFERENCE QUOTIENT AND ANALYTIC DERIVATIVES AGREE. */
	msg[*lq + *j * msg_dim1] = 0;
    } else if ((d__1 = *fd - *d__, abs(d__1)) <= (d__2 = two * *curve * stp, 
	    abs(d__2)) || large) {
/*  CURVATURE MAY BE THE CULPRIT (FUDGE FACTOR = 2) */
	if (large) {
	    msg[*lq + *j * msg_dim1] = 4;
	} else {
	    msg[*lq + *j * msg_dim1] = 5;
	}
    }
    return 0;
} /* djckf_ */

/* DJCKM */
/* Subroutine */ int djckm_(S_fp fcn, integer *n, integer *m, integer *np, 
	integer *nq, doublereal *beta, doublereal *xplusd, integer *ifixb, 
	integer *ifixx, integer *ldifx, doublereal *eta, doublereal *tol, 
	integer *nrow, doublereal *epsmac, integer *j, integer *lq, 
	doublereal *typj, doublereal *h0, doublereal *hc0, logical *iswrtb, 
	doublereal *pv, doublereal *d__, doublereal *diffj, integer *msg1, 
	integer *msg, integer *istop, integer *nfev, doublereal *wrk1, 
	doublereal *wrk2, doublereal *wrk6)
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal p01 = .01;
    static doublereal p1 = .1;
    static doublereal one = 1.;
    static doublereal two = 2.;
    static doublereal three = 3.;
    static doublereal ten = 10.;
    static doublereal hundrd = 100.;
    static doublereal big = 1e19;
    static doublereal tol2 = .05;

    /* System generated locals */
    integer wrk1_dim1, wrk1_dim2, wrk1_offset, wrk2_dim1, wrk2_offset, 
	    wrk6_dim1, wrk6_dim2, wrk6_offset, xplusd_dim1, xplusd_offset, 
	    ifixx_dim1, ifixx_offset, msg_dim1, msg_offset;
    doublereal d__1, d__2, d__3, d__4;

    /* Builtin functions */
    double sqrt(doublereal), pow_dd(doublereal *, doublereal *), d_sign(
	    doublereal *, doublereal *);

    /* Local variables */
    static doublereal h__;
    static integer i__;
    static doublereal h1, fd, hc, hc1, stp0;
    extern /* Subroutine */ int dpvb_(S_fp, integer *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, doublereal *, integer 
	    *, integer *, doublereal *, doublereal *, doublereal *, 
	    doublereal *), dpvd_(S_fp, integer *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, doublereal *, integer 
	    *, integer *, doublereal *, doublereal *, doublereal *, 
	    doublereal *), djckc_(S_fp, integer *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, doublereal *, 
	    integer *, integer *, doublereal *, logical *, doublereal *, 
	    doublereal *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, doublereal *, integer *, integer *, integer *, 
	    doublereal *, doublereal *, doublereal *), djckz_(S_fp, integer *,
	     integer *, integer *, integer *, doublereal *, doublereal *, 
	    integer *, integer *, integer *, integer *, doublereal *, integer 
	    *, integer *, logical *, doublereal *, doublereal *, doublereal *,
	     doublereal *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, integer *, integer *, integer *, doublereal *, 
	    doublereal *, doublereal *);
    static doublereal pvpstp;

/* ***BEGIN PROLOGUE  DJCKM */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DJCKC,DJCKZ,DPVB,DPVD */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  CHECK USER SUPPLIED ANALYTIC DERIVATIVES AGAINST NUMERICAL */
/*            DERIVATIVES */
/*            (ADAPTED FROM STARPAC SUBROUTINE DCKMN) */
/* ***END PROLOGUE  DJCKM */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    xplusd_dim1 = *n;
    xplusd_offset = 1 + xplusd_dim1;
    xplusd -= xplusd_offset;
    --ifixb;
    --beta;
    wrk6_dim1 = *n;
    wrk6_dim2 = *np;
    wrk6_offset = 1 + wrk6_dim1 * (1 + wrk6_dim2);
    wrk6 -= wrk6_offset;
    wrk2_dim1 = *n;
    wrk2_offset = 1 + wrk2_dim1;
    wrk2 -= wrk2_offset;
    wrk1_dim1 = *n;
    wrk1_dim2 = *m;
    wrk1_offset = 1 + wrk1_dim1 * (1 + wrk1_dim2);
    wrk1 -= wrk1_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;
    msg_dim1 = *nq;
    msg_offset = 1 + msg_dim1;
    msg -= msg_offset;

    /* Function Body */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:     THE USER SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   BIG:     A BIG VALUE, USED TO INITIALIZE DIFFJ. */
/*   D:       THE DERIVATIVE WITH RESPECT TO THE JTH UNKNOWN PARAMETER. */
/*   DIFFJ:   THE RELATIVE DIFFERENCES BETWEEN THE USER SUPPLIED AND */
/*            FINITE DIFFERENCE DERIVATIVES FOR THE DERIVATIVE BEING */
/*            CHECKED. */
/*   EPSMAC:  THE VALUE OF MACHINE PRECISION. */
/*   ETA:     THE RELATIVE NOISE IN THE FUNCTION RESULTS. */
/*   FD:      THE FORWARD DIFFERENCE DERIVATIVE WRT THE JTH PARAMETER. */
/*   H:       THE RELATIVE STEP SIZE FOR FORWARD DIFFERENCES. */
/*   H0:      THE INITIAL RELATIVE STEP SIZE FOR FORWARD DIFFERENCES. */
/*   H1:      THE DEFAULT RELATIVE STEP SIZE FOR FORWARD DIFFERENCES. */
/*   HC:      THE RELATIVE STEP SIZE FOR CENTRAL DIFFERENCES. */
/*   HC0:     THE INITIAL RELATIVE STEP SIZE FOR CENTRAL DIFFERENCES. */
/*   HC1:     THE DEFAULT RELATIVE STEP SIZE FOR CENTRAL DIFFERENCES. */
/*   HUNDRD:  THE VALUE 100.0D0. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   ISTOP:   THE VARIABLE DESIGNATING WHETHER THERE ARE PROBLEMS */
/*            COMPUTING THE FUNCTION AT THE CURRENT BETA AND DELTA. */
/*   ISWRTB:  THE VARIABLE DESIGNATING WHETHER THE DERIVATIVES WRT BETA */
/*            (ISWRTB=TRUE) OR DELTAS (ISWRTB=FALSE) ARE BEING CHECKED. */
/*   J:       THE INDEX OF THE PARTIAL DERIVATIVE BEING EXAMINED. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LQ:      THE RESPONSE CURRENTLY BEING EXAMINED. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   MSG:     THE ERROR CHECKING RESULTS. */
/*   MSG1:    THE ERROR CHECKING RESULTS SUMMARY. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   NROW:    THE ROW NUMBER OF THE EXPLANATORY VARIABLE ARRAY AT WHICH */
/*            THE DERIVATIVE IS TO BE CHECKED. */
/*   ONE:     THE VALUE 1.0D0. */
/*   PV:      THE PREDICTED VALUE FROM THE MODEL FOR ROW   NROW   . */
/*   PVPSTP:  THE PREDICTED VALUE FOR ROW    NROW   OF THE MODEL */
/*            USING THE CURRENT PARAMETER ESTIMATES FOR ALL BUT THE JTH */
/*            PARAMETER VALUE, WHICH IS BETA(J) + STP0. */
/*   P01:     THE VALUE 0.01D0. */
/*   P1:      THE VALUE 0.1D0. */
/*   STP0:    THE INITIAL STEP SIZE FOR THE FINITE DIFFERENCE DERIVATIVE. */
/*   TEN:     THE VALUE 10.0D0. */
/*   THREE:   THE VALUE 3.0D0. */
/*   TWO:     THE VALUE 2.0D0. */
/*   TOL:     THE AGREEMENT TOLERANCE. */
/*   TOL2:    A MINIMUM AGREEMENT TOLERANCE. */
/*   TYPJ:    THE TYPICAL SIZE OF THE J-TH UNKNOWN BETA OR DELTA. */
/*   WRK1:    A WORK ARRAY OF (N BY M BY NQ) ELEMENTS. */
/*   WRK2:    A WORK ARRAY OF (N BY NQ) ELEMENTS. */
/*   WRK6:    A WORK ARRAY OF (N BY NP BY NQ) ELEMENTS. */
/*   XPLUSD:  THE VALUES OF X + DELTA. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DJCKM */
/*  CALCULATE THE JTH PARTIAL DERIVATIVE USING FORWARD DIFFERENCE */
/*  QUOTIENTS AND DECIDE IF IT AGREES WITH USER SUPPLIED VALUES */
    h1 = sqrt(*eta);
    d__1 = one / three;
    hc1 = pow_dd(eta, &d__1);
    msg[*lq + *j * msg_dim1] = 7;
    *diffj = big;
    for (i__ = 1; i__ <= 3; ++i__) {
	if (i__ == 1) {
/*  TRY INITIAL RELATIVE STEP SIZE */
	    h__ = *h0;
	    hc = *hc0;
	} else if (i__ == 2) {
/*  TRY LARGER RELATIVE STEP SIZE */
/* Computing MAX */
/* Computing MIN */
	    d__3 = hundrd * *h0;
	    d__1 = ten * h1, d__2 = min(d__3,one);
	    h__ = max(d__1,d__2);
/* Computing MAX */
/* Computing MIN */
	    d__3 = hundrd * *hc0;
	    d__1 = ten * hc1, d__2 = min(d__3,one);
	    hc = max(d__1,d__2);
	} else if (i__ == 3) {
/*  TRY SMALLER RELATIVE STEP SIZE */
/* Computing MIN */
/* Computing MAX */
	    d__3 = p01 * h__, d__4 = two * *epsmac;
	    d__1 = p1 * h1, d__2 = max(d__3,d__4);
	    h__ = min(d__1,d__2);
/* Computing MIN */
/* Computing MAX */
	    d__3 = p01 * hc, d__4 = two * *epsmac;
	    d__1 = p1 * hc1, d__2 = max(d__3,d__4);
	    hc = min(d__1,d__2);
	}
	if (*iswrtb) {
/*  PERFORM COMPUTATIONS FOR DERIVATIVES WRT BETA */
	    stp0 = h__ * *typj * d_sign(&one, &beta[*j]) + beta[*j] - beta[*j]
		    ;
	    dpvb_((S_fp)fcn, n, m, np, nq, &beta[1], &xplusd[xplusd_offset], &
		    ifixb[1], &ifixx[ifixx_offset], ldifx, nrow, j, lq, &stp0,
		     istop, nfev, &pvpstp, &wrk1[wrk1_offset], &wrk2[
		    wrk2_offset], &wrk6[wrk6_offset]);
	} else {
/*  PERFORM COMPUTATIONS FOR DERIVATIVES WRT DELTA */
	    stp0 = h__ * *typj * d_sign(&one, &xplusd[*nrow + *j * 
		    xplusd_dim1]) + xplusd[*nrow + *j * xplusd_dim1] - xplusd[
		    *nrow + *j * xplusd_dim1];
	    dpvd_((S_fp)fcn, n, m, np, nq, &beta[1], &xplusd[xplusd_offset], &
		    ifixb[1], &ifixx[ifixx_offset], ldifx, nrow, j, lq, &stp0,
		     istop, nfev, &pvpstp, &wrk1[wrk1_offset], &wrk2[
		    wrk2_offset], &wrk6[wrk6_offset]);
	}
	if (*istop != 0) {
	    return 0;
	}
	fd = (pvpstp - *pv) / stp0;
/*  CHECK FOR AGREEMENT */
	if ((d__1 = fd - *d__, abs(d__1)) <= *tol * abs(*d__)) {
/*  NUMERICAL AND ANALYTIC DERIVATIVES AGREE */
/*  SET RELATIVE DIFFERENCE FOR DERIVATIVE CHECKING REPORT */
	    if (*d__ == zero || fd == zero) {
		*diffj = (d__1 = fd - *d__, abs(d__1));
	    } else {
		*diffj = (d__1 = fd - *d__, abs(d__1)) / abs(*d__);
	    }
/*  SET MSG FLAG. */
	    if (*d__ == zero) {
/*  JTH ANALYTIC AND NUMERICAL DERIVATIVES ARE BOTH ZERO. */
		msg[*lq + *j * msg_dim1] = 1;
	    } else {
/*  JTH ANALYTIC AND NUMERICAL DERIVATIVES ARE BOTH NONZERO. */
		msg[*lq + *j * msg_dim1] = 0;
	    }
	} else {
/*  NUMERICAL AND ANALYTIC DERIVATIVES DISAGREE.  CHECK WHY */
	    if (*d__ == zero || fd == zero) {
		djckz_((S_fp)fcn, n, m, np, nq, &beta[1], &xplusd[
			xplusd_offset], &ifixb[1], &ifixx[ifixx_offset], 
			ldifx, nrow, epsmac, j, lq, iswrtb, tol, d__, &fd, 
			typj, &pvpstp, &stp0, pv, diffj, &msg[msg_offset], 
			istop, nfev, &wrk1[wrk1_offset], &wrk2[wrk2_offset], &
			wrk6[wrk6_offset]);
	    } else {
		djckc_((S_fp)fcn, n, m, np, nq, &beta[1], &xplusd[
			xplusd_offset], &ifixb[1], &ifixx[ifixx_offset], 
			ldifx, eta, tol, nrow, epsmac, j, lq, &hc, iswrtb, &
			fd, typj, &pvpstp, &stp0, pv, d__, diffj, &msg[
			msg_offset], istop, nfev, &wrk1[wrk1_offset], &wrk2[
			wrk2_offset], &wrk6[wrk6_offset]);
	    }
	    if (msg[*lq + *j * msg_dim1] <= 2) {
		goto L20;
	    }
	}
/* L10: */
    }
/*  SET SUMMARY FLAG TO INDICATE QUESTIONABLE RESULTS */
L20:
    if (msg[*lq + *j * msg_dim1] >= 7 && *diffj <= tol2) {
	msg[*lq + *j * msg_dim1] = 6;
    }
    if (msg[*lq + *j * msg_dim1] >= 1 && msg[*lq + *j * msg_dim1] <= 6) {
	*msg1 = max(*msg1,1);
    } else if (msg[*lq + *j * msg_dim1] >= 7) {
	*msg1 = 2;
    }
    return 0;
} /* djckm_ */

/* DJCKZ */
/* Subroutine */ int djckz_(S_fp fcn, integer *n, integer *m, integer *np, 
	integer *nq, doublereal *beta, doublereal *xplusd, integer *ifixb, 
	integer *ifixx, integer *ldifx, integer *nrow, doublereal *epsmac, 
	integer *j, integer *lq, logical *iswrtb, doublereal *tol, doublereal 
	*d__, doublereal *fd, doublereal *typj, doublereal *pvpstp, 
	doublereal *stp0, doublereal *pv, doublereal *diffj, integer *msg, 
	integer *istop, integer *nfev, doublereal *wrk1, doublereal *wrk2, 
	doublereal *wrk6)
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal one = 1.;
    static doublereal two = 2.;
    static doublereal three = 3.;

    /* System generated locals */
    integer wrk1_dim1, wrk1_dim2, wrk1_offset, wrk2_dim1, wrk2_offset, 
	    wrk6_dim1, wrk6_dim2, wrk6_offset, xplusd_dim1, xplusd_offset, 
	    ifixx_dim1, ifixx_offset, msg_dim1, msg_offset;
    doublereal d__1, d__2, d__3, d__4;

    /* Builtin functions */
    double pow_dd(doublereal *, doublereal *);

    /* Local variables */
    static doublereal cd;
    extern /* Subroutine */ int dpvb_(S_fp, integer *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, doublereal *, integer 
	    *, integer *, doublereal *, doublereal *, doublereal *, 
	    doublereal *), dpvd_(S_fp, integer *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, doublereal *, integer 
	    *, integer *, doublereal *, doublereal *, doublereal *, 
	    doublereal *);
    static doublereal pvmstp;

/* ***BEGIN PROLOGUE  DJCKZ */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DPVB,DPVD */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  RECHECK THE DERIVATIVES IN THE CASE WHERE THE FINITE */
/*            DIFFERENCE DERIVATIVE DISAGREES WITH THE ANALYTIC */
/*            DERIVATIVE AND THE ANALYTIC DERIVATIVE IS ZERO */
/*            (ADAPTED FROM STARPAC SUBROUTINE DCKZRO) */
/* ***END PROLOGUE  DJCKZ */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    xplusd_dim1 = *n;
    xplusd_offset = 1 + xplusd_dim1;
    xplusd -= xplusd_offset;
    --ifixb;
    --beta;
    wrk6_dim1 = *n;
    wrk6_dim2 = *np;
    wrk6_offset = 1 + wrk6_dim1 * (1 + wrk6_dim2);
    wrk6 -= wrk6_offset;
    wrk2_dim1 = *n;
    wrk2_offset = 1 + wrk2_dim1;
    wrk2 -= wrk2_offset;
    wrk1_dim1 = *n;
    wrk1_dim2 = *m;
    wrk1_offset = 1 + wrk1_dim1 * (1 + wrk1_dim2);
    wrk1 -= wrk1_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;
    msg_dim1 = *nq;
    msg_offset = 1 + msg_dim1;
    msg -= msg_offset;

    /* Function Body */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:     THE USER SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   CD:      THE CENTRAL DIFFERENCE DERIVATIVE WRT THE JTH PARAMETER. */
/*   D:       THE DERIVATIVE WITH RESPECT TO THE JTH UNKNOWN PARAMETER. */
/*   DIFFJ:   THE RELATIVE DIFFERENCES BETWEEN THE USER SUPPLIED AND */
/*            FINITE DIFFERENCE DERIVATIVES FOR THE DERIVATIVE BEING */
/*            CHECKED. */
/*   EPSMAC:  THE VALUE OF MACHINE PRECISION. */
/*   FD:      THE FORWARD DIFFERENCE DERIVATIVE WRT THE JTH PARAMETER. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   ISTOP:   THE VARIABLE DESIGNATING WHETHER THERE ARE PROBLEMS */
/*            COMPUTING THE FUNCTION AT THE CURRENT BETA AND DELTA. */
/*   ISWRTB:  THE VARIABLE DESIGNATING WHETHER THE DERIVATIVES WRT BETA */
/*            (ISWRTB=TRUE) OR X (ISWRTB=FALSE) ARE BEING CHECKED. */
/*   J:       THE INDEX OF THE PARTIAL DERIVATIVE BEING EXAMINED. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LQ:      THE RESPONSE CURRENTLY BEING EXAMINED. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   MSG:     THE ERROR CHECKING RESULTS. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   NROW:    THE ROW NUMBER OF THE EXPLANATORY VARIABLE ARRAY AT WHICH */
/*            THE DERIVATIVE IS TO BE CHECKED. */
/*   ONE:     THE VALUE 1.0D0. */
/*   PV:      THE PREDICTED VALUE FROM THE MODEL FOR ROW   NROW   . */
/*   PVMSTP:  THE PREDICTED VALUE FOR ROW    NROW   OF THE MODEL */
/*            USING THE CURRENT PARAMETER ESTIMATES FOR ALL BUT THE */
/*            JTH PARAMETER VALUE, WHICH IS BETA(J) - STP0. */
/*   PVPSTP:  THE PREDICTED VALUE FOR ROW    NROW   OF THE MODEL */
/*            USING THE CURRENT PARAMETER ESTIMATES FOR ALL BUT THE */
/*            JTH PARAMETER VALUE, WHICH IS BETA(J) + STP0. */
/*   STP0:    THE INITIAL STEP SIZE FOR THE FINITE DIFFERENCE DERIVATIVE. */
/*   THREE:   THE VALUE 3.0D0. */
/*   TWO:     THE VALUE 2.0D0. */
/*   TOL:     THE AGREEMENT TOLERANCE. */
/*   TYPJ:    THE TYPICAL SIZE OF THE J-TH UNKNOWN BETA OR DELTA. */
/*   WRK1:    A WORK ARRAY OF (N BY M BY NQ) ELEMENTS. */
/*   WRK2:    A WORK ARRAY OF (N BY NQ) ELEMENTS. */
/*   WRK6:    A WORK ARRAY OF (N BY NP BY NQ) ELEMENTS. */
/*   XPLUSD:  THE VALUES OF X + DELTA. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DJCKZ */
/*  RECALCULATE NUMERICAL DERIVATIVE USING CENTRAL DIFFERENCE AND STEP */
/*  SIZE OF 2*STP0 */
    if (*iswrtb) {
/*  PERFORM COMPUTATIONS FOR DERIVATIVES WRT BETA */
	d__1 = -(*stp0);
	dpvb_((S_fp)fcn, n, m, np, nq, &beta[1], &xplusd[xplusd_offset], &
		ifixb[1], &ifixx[ifixx_offset], ldifx, nrow, j, lq, &d__1, 
		istop, nfev, &pvmstp, &wrk1[wrk1_offset], &wrk2[wrk2_offset], 
		&wrk6[wrk6_offset]);
    } else {
/*  PERFORM COMPUTATIONS FOR DERIVATIVES WRT DELTA */
	d__1 = -(*stp0);
	dpvd_((S_fp)fcn, n, m, np, nq, &beta[1], &xplusd[xplusd_offset], &
		ifixb[1], &ifixx[ifixx_offset], ldifx, nrow, j, lq, &d__1, 
		istop, nfev, &pvmstp, &wrk1[wrk1_offset], &wrk2[wrk2_offset], 
		&wrk6[wrk6_offset]);
    }
    if (*istop != 0) {
	return 0;
    }
    cd = (*pvpstp - pvmstp) / (two * *stp0);
/* Computing MIN */
    d__3 = (d__1 = cd - *d__, abs(d__1)), d__4 = (d__2 = *fd - *d__, abs(d__2)
	    );
    *diffj = min(d__3,d__4);
/*  CHECK FOR AGREEMENT */
    if (*diffj <= *tol * abs(*d__)) {
/*  FINITE DIFFERENCE AND ANALYTIC DERIVATIVES NOW AGREE. */
	if (*d__ == zero) {
	    msg[*lq + *j * msg_dim1] = 1;
	} else {
	    msg[*lq + *j * msg_dim1] = 0;
	}
    } else /* if(complicated condition) */ {
	d__2 = one / three;
	if (*diffj * *typj <= (d__1 = *pv * pow_dd(epsmac, &d__2), abs(d__1)))
		 {
/*  DERIVATIVES ARE BOTH CLOSE TO ZERO */
	    msg[*lq + *j * msg_dim1] = 2;
	} else {
/*  DERIVATIVES ARE NOT BOTH CLOSE TO ZERO */
	    msg[*lq + *j * msg_dim1] = 3;
	}
    }
    return 0;
} /* djckz_ */

/* DODCHK */
/* Subroutine */ int dodchk_(integer *n, integer *m, integer *np, integer *nq,
	 logical *isodr, logical *anajac, logical *implct, integer *ifixb, 
	integer *ldx, integer *ldifx, integer *ldscld, integer *ldstpd, 
	integer *ldwe, integer *ld2we, integer *ldwd, integer *ld2wd, integer 
	*ldy, integer *lwork, integer *lwkmn, integer *liwork, integer *
	liwkmn, doublereal *sclb, doublereal *scld, doublereal *stpb, 
	doublereal *stpd, integer *info)
{
    /* System generated locals */
    integer scld_dim1, scld_offset, stpd_dim1, stpd_offset, i__1, i__2;

    /* Local variables */
    static integer i__, j, k, npp, last;

/* ***BEGIN PROLOGUE  DODCHK */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  CHECK INPUT PARAMETERS, INDICATING ERRORS FOUND USING */
/*            NONZERO VALUES OF ARGUMENT INFO */
/* ***END PROLOGUE  DODCHK */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ANAJAC:  THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE */
/*            COMPUTED BY FINITE DIFFERENCES (ANAJAC=FALSE) OR NOT */
/*            (ANAJAC=TRUE). */
/*   I:       AN INDEXING VARIABLE. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IMPLCT:  THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY */
/*            IMPLICIT ODR (IMPLCT=TRUE) OR EXPLICIT ODR (IMPLCT=FALSE). */
/*   INFO:    THE VARIABLE DESIGNATING WHY THE COMPUTATIONS WERE STOPPED. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR BY OLS (ISODR=FALSE). */
/*   J:       AN INDEXING VARIABLE. */
/*   K:       AN INDEXING VARIABLE. */
/*   LAST:    THE LAST ROW OF THE ARRAY TO BE ACCESSED. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LDSCLD:  THE LEADING DIMENSION OF ARRAY SCLD. */
/*   LDSTPD:  THE LEADING DIMENSION OF ARRAY STPD. */
/*   LDWD:    THE LEADING DIMENSION OF ARRAY WD. */
/*   LDWE:    THE LEADING DIMENSION OF ARRAY WE. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   LDY:     THE LEADING DIMENSION OF ARRAY X. */
/*   LD2WD:   THE SECOND DIMENSION OF ARRAY WD. */
/*   LD2WE:   THE SECOND DIMENSION OF ARRAY WE. */
/*   LIWKMN:  THE MINIMUM ACCEPTABLE LENGTH OF ARRAY IWORK. */
/*   LIWORK:  THE LENGTH OF VECTOR IWORK. */
/*   LWKMN:   THE MINIMUM ACCEPTABLE LENGTH OF ARRAY WORK. */
/*   LWORK:   THE LENGTH OF VECTOR WORK. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NPP:     THE NUMBER OF FUNCTION PARAMETERS BEING ESTIMATED. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATIONS. */
/*   SCLB:    THE SCALING VALUES FOR BETA. */
/*   SCLD:    THE SCALING VALUE FOR DELTA. */
/*   STPB:    THE STEP FOR THE FINITE DIFFERENCE DERIVITIVE WRT BETA. */
/*   STPD:    THE STEP FOR THE FINITE DIFFERENCE DERIVITIVE WRT DELTA. */
/* ***FIRST EXECUTABLE STATEMENT  DODCHK */
/*  FIND ACTUAL NUMBER OF PARAMETERS BEING ESTIMATED */
    /* Parameter adjustments */
    --stpb;
    --sclb;
    --ifixb;
    scld_dim1 = *ldscld;
    scld_offset = 1 + scld_dim1;
    scld -= scld_offset;
    stpd_dim1 = *ldstpd;
    stpd_offset = 1 + stpd_dim1;
    stpd -= stpd_offset;

    /* Function Body */
    if (*np <= 0 || ifixb[1] < 0) {
	npp = *np;
    } else {
	npp = 0;
	i__1 = *np;
	for (k = 1; k <= i__1; ++k) {
	    if (ifixb[k] != 0) {
		++npp;
	    }
/* L10: */
	}
    }
/*  CHECK PROBLEM SPECIFICATION PARAMETERS */
    if (*n <= 0 || *m <= 0 || (npp <= 0 || npp > *n) || *nq <= 0) {
	*info = 10000;
	if (*n <= 0) {
	    *info += 1000;
	}
	if (*m <= 0) {
	    *info += 100;
	}
	if (npp <= 0 || npp > *n) {
	    *info += 10;
	}
	if (*nq <= 0) {
	    ++(*info);
	}
	return 0;
    }
/*  CHECK DIMENSION SPECIFICATION PARAMETERS */
    if (! (*implct) && *ldy < *n || *ldx < *n || *ldwe != 1 && *ldwe < *n || *
	    ld2we != 1 && *ld2we < *nq || *isodr && (*ldwd != 1 && *ldwd < *n)
	     || *isodr && (*ld2wd != 1 && *ld2wd < *m) || *isodr && (*ldifx !=
	     1 && *ldifx < *n) || *isodr && (*ldstpd != 1 && *ldstpd < *n) || 
	    *isodr && (*ldscld != 1 && *ldscld < *n) || *lwork < *lwkmn || *
	    liwork < *liwkmn) {
	*info = 20000;
	if (! (*implct) && *ldy < *n) {
	    *info += 1000;
	}
	if (*ldx < *n) {
	    *info += 2000;
	}
	if (*ldwe != 1 && *ldwe < *n || *ld2we != 1 && *ld2we < *nq) {
	    *info += 100;
	}
	if (*isodr && (*ldwd != 1 && *ldwd < *n || *ld2wd != 1 && *ld2wd < *m)
		) {
	    *info += 200;
	}
	if (*isodr && (*ldifx != 1 && *ldifx < *n)) {
	    *info += 10;
	}
	if (*isodr && (*ldstpd != 1 && *ldstpd < *n)) {
	    *info += 20;
	}
	if (*isodr && (*ldscld != 1 && *ldscld < *n)) {
	    *info += 40;
	}
	if (*lwork < *lwkmn) {
	    ++(*info);
	}
	if (*liwork < *liwkmn) {
	    *info += 2;
	}
	return 0;
    }
/*  CHECK DELTA SCALING */
    if (*isodr && scld[scld_dim1 + 1] > 0.) {
	if (*ldscld >= *n) {
	    last = *n;
	} else {
	    last = 1;
	}
	i__1 = *m;
	for (j = 1; j <= i__1; ++j) {
	    i__2 = last;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		if (scld[i__ + j * scld_dim1] <= 0.) {
		    *info = 30200;
		    goto L130;
		}
/* L110: */
	    }
/* L120: */
	}
    }
L130:
/*  CHECK BETA SCALING */
    if (sclb[1] > 0.) {
	i__1 = *np;
	for (k = 1; k <= i__1; ++k) {
	    if (sclb[k] <= 0.) {
		if (*info == 0) {
		    *info = 30100;
		} else {
		    *info += 100;
		}
		goto L220;
	    }
/* L210: */
	}
    }
L220:
/*  CHECK DELTA FINITE DIFFERENCE STEP SIZES */
    if (*anajac && *isodr && stpd[stpd_dim1 + 1] > 0.) {
	if (*ldstpd >= *n) {
	    last = *n;
	} else {
	    last = 1;
	}
	i__1 = *m;
	for (j = 1; j <= i__1; ++j) {
	    i__2 = last;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		if (stpd[i__ + j * stpd_dim1] <= 0.) {
		    if (*info == 0) {
			*info = 32000;
		    } else {
			*info += 2000;
		    }
		    goto L330;
		}
/* L310: */
	    }
/* L320: */
	}
    }
L330:
/*  CHECK BETA FINITE DIFFERENCE STEP SIZES */
    if (*anajac && stpb[1] > 0.) {
	i__1 = *np;
	for (k = 1; k <= i__1; ++k) {
	    if (stpb[k] <= 0.) {
		if (*info == 0) {
		    *info = 31000;
		} else {
		    *info += 1000;
		}
		goto L420;
	    }
/* L410: */
	}
    }
L420:
    return 0;
} /* dodchk_ */

/* DODCNT */
/* Subroutine */ int dodcnt_(logical *short__, U_fp fcn, integer *n, integer *
	m, integer *np, integer *nq, doublereal *beta, doublereal *y, integer 
	*ldy, doublereal *x, integer *ldx, doublereal *we, integer *ldwe, 
	integer *ld2we, doublereal *wd, integer *ldwd, integer *ld2wd, 
	integer *ifixb, integer *ifixx, integer *ldifx, integer *job, integer 
	*ndigit, doublereal *taufac, doublereal *sstol, doublereal *partol, 
	integer *maxit, integer *iprint, integer *lunerr, integer *lunrpt, 
	doublereal *stpb, doublereal *stpd, integer *ldstpd, doublereal *sclb,
	 doublereal *scld, integer *ldscld, doublereal *work, integer *lwork, 
	integer *iwork, integer *liwork, integer *info)
{
    /* Initialized data */

    static doublereal pcheck = 1e3;
    static doublereal pstart = 10.;
    static doublereal pfac = 10.;
    static doublereal zero = 0.;
    static doublereal one = 1.;
    static doublereal three = 3.;

    /* System generated locals */
    integer scld_dim1, scld_offset, stpd_dim1, stpd_offset, wd_dim1, wd_dim2, 
	    wd_offset, we_dim1, we_dim2, we_offset, x_dim1, x_offset, y_dim1, 
	    y_offset, ifixx_dim1, ifixx_offset;
    doublereal d__1, d__2;

    /* Builtin functions */
    double pow_dd(doublereal *, doublereal *);

    /* Local variables */
    static integer job1, job2, job3, job4, job5, ipr1, ipr2, ipr3;
    static logical head;
    static integer jobi;
    static logical done;
    static integer ipr2f;
    static doublereal pnlty[1]	/* was [1][1][1] */;
    static integer maxit1;
    extern doublereal dmprec_(void);
    extern /* Subroutine */ int doddrv_(logical *, logical *, logical *, 
	    logical *, U_fp, integer *, integer *, integer *, integer *, 
	    doublereal *, doublereal *, integer *, doublereal *, integer *, 
	    doublereal *, integer *, integer *, doublereal *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    doublereal *, doublereal *, doublereal *, integer *, integer *, 
	    integer *, integer *, doublereal *, doublereal *, integer *, 
	    doublereal *, doublereal *, integer *, doublereal *, integer *, 
	    integer *, integer *, integer *, doublereal *, integer *);
    static logical implct;
    static integer maxiti;
    static doublereal cnvtol;
    static integer iprnti;
    static logical prtpen, fstitr;
    static doublereal tstimp;

/* ***BEGIN PROLOGUE  DODCNT */
/* ***REFER TO   DODR,DODRC */
/* ***ROUTINES CALLED  DODDRV */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  DOUBLE PRECISION DRIVER ROUTINE FOR FINDING */
/*            THE WEIGHTED EXPLICIT OR IMPLICIT ORTHOGONAL DISTANCE */
/*            REGRESSION (ODR) OR ORDINARY LINEAR OR NONLINEAR LEAST */
/*            SQUARES (OLS) SOLUTION */
/* ***END PROLOGUE  DODCNT */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...LOCAL ARRAYS */
/* ...EXTERNAL SUBROUTINES */
/* ...EXTERNAL FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --sclb;
    --stpb;
    --ifixb;
    --beta;
    y_dim1 = *ldy;
    y_offset = 1 + y_dim1;
    y -= y_offset;
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    we_dim1 = *ldwe;
    we_dim2 = *ld2we;
    we_offset = 1 + we_dim1 * (1 + we_dim2);
    we -= we_offset;
    wd_dim1 = *ldwd;
    wd_dim2 = *ld2wd;
    wd_offset = 1 + wd_dim1 * (1 + wd_dim2);
    wd -= wd_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;
    stpd_dim1 = *ldstpd;
    stpd_offset = 1 + stpd_dim1;
    stpd -= stpd_offset;
    scld_dim1 = *ldscld;
    scld_offset = 1 + scld_dim1;
    scld -= scld_offset;
    --work;
    --iwork;

    /* Function Body */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:     THE USER-SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   CNVTOL:  THE CONVERGENCE TOLERANCE FOR IMPLICIT MODELS. */
/*   DONE:    THE VARIABLE DESIGNATING WHETHER THE INPLICIT SOLUTION HAS */
/*            BEEN FOUND (DONE=TRUE) OR NOT (DONE=FALSE). */
/*   FSTITR:  THE VARIABLE DESIGNATING WHETHER THIS IS THE FIRST */
/*            ITERATION (FSTITR=TRUE) OR NOT (FSTITR=FALSE). */
/*   HEAD:    THE VARIABLE DESIGNATING WHETHER THE HEADING IS TO BE */
/*            PRINTED (HEAD=TRUE) OR NOT (HEAD=FALSE). */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IMPLCT:  THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY */
/*            IMPLICIT ODR (IMPLCT=TRUE) OR EXPLICIT ODR (IMPLCT=FALSE). */
/*   INFO:    THE VARIABLE DESIGNATING WHY THE COMPUTATIONS WERE STOPPED. */
/*   IPRINT:  THE PRINT CONTROL VARIABLES. */
/*   IPRNTI:  THE PRINT CONTROL VARIABLES. */
/*   IPR1:    THE 1ST DIGIT OF THE PRINT CONTROL VARIABLE. */
/*   IPR2:    THE 2ND DIGIT OF THE PRINT CONTROL VARIABLE. */
/*   IPR3:    THE 3RD DIGIT OF THE PRINT CONTROL VARIABLE. */
/*   IPR4:    THE 4TH DIGIT OF THE PRINT CONTROL VARIABLE. */
/*   IWORK:   THE INTEGER WORK SPACE. */
/*   JOB:     THE VARIABLE CONTROLING PROBLEM INITIALIZATION AND */
/*            COMPUTATIONAL METHOD. */
/*   JOBI:    THE VARIABLE CONTROLING PROBLEM INITIALIZATION AND */
/*            COMPUTATIONAL METHOD. */
/*   JOB1:    THE 1ST DIGIT OF THE VARIABLE CONTROLING PROBLEM */
/*            INITIALIZATION AND COMPUTATIONAL METHOD. */
/*   JOB2:    THE 2ND DIGIT OF THE VARIABLE CONTROLING PROBLEM */
/*            INITIALIZATION AND COMPUTATIONAL METHOD. */
/*   JOB3:    THE 3RD DIGIT OF THE VARIABLE CONTROLING PROBLEM */
/*            INITIALIZATION AND COMPUTATIONAL METHOD. */
/*   JOB4:    THE 4TH DIGIT OF THE VARIABLE CONTROLING PROBLEM */
/*            INITIALIZATION AND COMPUTATIONAL METHOD. */
/*   JOB5:    THE 5TH DIGIT OF THE VARIABLE CONTROLING PROBLEM */
/*            INITIALIZATION AND COMPUTATIONAL METHOD. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LDSCLD:  THE LEADING DIMENSION OF ARRAY SCLD. */
/*   LDSTPD:  THE LEADING DIMENSION OF ARRAY STPD. */
/*   LDWD:    THE LEADING DIMENSION OF ARRAY WD. */
/*   LDWE:    THE LEADING DIMENSION OF ARRAY WE. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   LDY:     THE LEADING DIMENSION OF ARRAY Y. */
/*   LD2WD:   THE SECOND DIMENSION OF ARRAY WD. */
/*   LD2WE:   THE SECOND DIMENSION OF ARRAY WE. */
/*   LIWORK:  THE LENGTH OF VECTOR IWORK. */
/*   LUNERR:  THE LOGICAL UNIT NUMBER USED FOR ERROR MESSAGES. */
/*   LUNRPT:  THE LOGICAL UNIT NUMBER USED FOR COMPUTATION REPORTS. */
/*   LWORK:   THE LENGTH OF VECTOR WORK. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE INDEPENDENT VARIABLE. */
/*   MAXIT:   THE MAXIMUM NUMBER OF ITERATIONS ALLOWED. */
/*   MAXITI:  FOR IMPLICIT MODELS, THE NUMBER OF ITERATIONS ALLOWED FOR */
/*            THE CURRENT PENALTY PARAMETER VALUE. */
/*   MAXIT1:  FOR IMPLICIT MODELS, THE NUMBER OF ITERATIONS ALLOWED FOR */
/*            THE NEXT PENALTY PARAMETER VALUE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NDIGIT:  THE NUMBER OF ACCURATE DIGITS IN THE FUNCTION RESULTS, AS */
/*            SUPPLIED BY THE USER. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   ONE:     THE VALUE 1.0D0. */
/*   PARTOL:  THE USER SUPPLIED PARAMETER CONVERGENCE STOPPING TOLERANCE. */
/*   PCHECK:  THE VALUE DESIGNATING THE MINIMUM PENALTY PARAMETER ALLOWED */
/*            BEFORE THE IMPLICIT PROBLEM CAN BE CONSIDERED SOLVED. */
/*   PFAC:    THE FACTOR FOR INCREASING THE PENALTY PARAMETER. */
/*   PNLTY:   THE PENALTY PARAMETER FOR AN IMPLICIT MODEL. */
/*   PRTPEN:  THE VALUE DESIGNATING WHETHER THE PENALTY PARAMETER IS TO BE */
/*            PRINTED IN THE ITERATION REPORT (PRTPEN=TRUE) OR NOT */
/*            (PRTPEN=FALSE). */
/*   PSTART:  THE FACTOR FOR INCREASING THE PENALTY PARAMETER. */
/*   SCLB:    THE SCALING VALUES FOR BETA. */
/*   SCLD:    THE SCALING VALUES FOR DELTA. */
/*   STPB:    THE RELATIVE STEP FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO BETA. */
/*   STPD:    THE RELATIVE STEP FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO DELTA. */
/*   SHORT:   THE VARIABLE DESIGNATING WHETHER THE USER HAS INVOKED */
/*            ODRPACK BY THE SHORT-CALL (SHORT=.TRUE.) OR THE LONG-CALL */
/*            (SHORT=.FALSE.). */
/*   SSTOL:   THE SUM-OF-SQUARES CONVERGENCE STOPPING TOLERANCE. */
/*   TAUFAC:  THE FACTOR USED TO COMPUTE THE INITIAL TRUST REGION */
/*            DIAMETER. */
/*   THREE:   THE VALUE 3.0D0. */
/*   TSTIMP:  THE RELATIVE CHANGE IN THE PARAMETERS BETWEEN THE INITIAL */
/*            VALUES AND THE SOLUTION. */
/*   WD:      THE DELTA WEIGHTS. */
/*   WE:      THE EPSILON WEIGHTS. */
/*   WORK:    THE DOUBLE PRECISION WORK SPACE. */
/*   X:       THE INDEPENDENT VARIABLE. */
/*   Y:       THE DEPENDENT VARIABLE.  UNUSED WHEN THE MODEL IS IMPLICIT. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DODCNT */
    implct = *job % 10 == 1;
    fstitr = TRUE_;
    head = TRUE_;
    prtpen = FALSE_;
    if (implct) {
/*  SET UP FOR IMPLICIT PROBLEM */
	if (*iprint >= 0) {
	    ipr1 = *iprint % 10000 / 1000;
	    ipr2 = *iprint % 1000 / 100;
	    ipr2f = *iprint % 100 / 10;
	    ipr3 = *iprint % 10;
	} else {
	    ipr1 = 2;
	    ipr2 = 0;
	    ipr2f = 0;
	    ipr3 = 1;
	}
	iprnti = ipr1 * 1000 + ipr2 * 100 + ipr2f * 10;
	job5 = *job % 100000 / 10000;
	job4 = *job % 10000 / 1000;
	job3 = *job % 1000 / 100;
	job2 = *job % 100 / 10;
	job1 = *job % 10;
	jobi = job5 * 10000 + job4 * 1000 + job3 * 100 + job2 * 10 + job1;
	if (we[(we_dim2 + 1) * we_dim1 + 1] <= zero) {
	    pnlty[0] = -pstart;
	} else {
	    pnlty[0] = -we[(we_dim2 + 1) * we_dim1 + 1];
	}
	if (*partol < zero) {
	    d__1 = dmprec_();
	    d__2 = one / three;
	    cnvtol = pow_dd(&d__1, &d__2);
	} else {
	    cnvtol = min(*partol,one);
	}
	if (*maxit >= 1) {
	    maxiti = *maxit;
	} else {
	    maxiti = 100;
	}
	done = maxiti == 0;
	prtpen = TRUE_;
L10:
	doddrv_(short__, &head, &fstitr, &prtpen, (U_fp)fcn, n, m, np, nq, &
		beta[1], &y[y_offset], ldy, &x[x_offset], ldx, pnlty, &c__1, &
		c__1, &wd[wd_offset], ldwd, ld2wd, &ifixb[1], &ifixx[
		ifixx_offset], ldifx, &jobi, ndigit, taufac, sstol, &cnvtol, &
		maxiti, &iprnti, lunerr, lunrpt, &stpb[1], &stpd[stpd_offset],
		 ldstpd, &sclb[1], &scld[scld_offset], ldscld, &work[1], 
		lwork, &iwork[1], liwork, &maxit1, &tstimp, info);
	if (done) {
	    return 0;
	} else {
	    done = maxit1 <= 0 || abs(pnlty[0]) >= pcheck && tstimp <= cnvtol;
	}
	if (done) {
	    if (tstimp <= cnvtol) {
		*info = *info / 10 * 10 + 2;
	    } else {
		*info = *info / 10 * 10 + 4;
	    }
	    jobi = job3 * 100 + 11000 + job2 * 10 + job1;
	    maxiti = 0;
	    iprnti = ipr3;
	} else {
	    prtpen = TRUE_;
	    pnlty[0] = pfac * pnlty[0];
	    jobi = job2 * 10 + 11000 + job1;
	    maxiti = maxit1;
	    iprnti = ipr2 * 100 + ipr2f * 10;
	}
	goto L10;
    } else {
	doddrv_(short__, &head, &fstitr, &prtpen, (U_fp)fcn, n, m, np, nq, &
		beta[1], &y[y_offset], ldy, &x[x_offset], ldx, &we[we_offset],
		 ldwe, ld2we, &wd[wd_offset], ldwd, ld2wd, &ifixb[1], &ifixx[
		ifixx_offset], ldifx, job, ndigit, taufac, sstol, partol, 
		maxit, iprint, lunerr, lunrpt, &stpb[1], &stpd[stpd_offset], 
		ldstpd, &sclb[1], &scld[scld_offset], ldscld, &work[1], lwork,
		 &iwork[1], liwork, &maxit1, &tstimp, info);
    }
    return 0;
} /* dodcnt_ */

/* DODDRV */
/* Subroutine */ int doddrv_(logical *short__, logical *head, logical *fstitr,
	 logical *prtpen, S_fp fcn, integer *n, integer *m, integer *np, 
	integer *nq, doublereal *beta, doublereal *y, integer *ldy, 
	doublereal *x, integer *ldx, doublereal *we, integer *ldwe, integer *
	ld2we, doublereal *wd, integer *ldwd, integer *ld2wd, integer *ifixb, 
	integer *ifixx, integer *ldifx, integer *job, integer *ndigit, 
	doublereal *taufac, doublereal *sstol, doublereal *partol, integer *
	maxit, integer *iprint, integer *lunerr, integer *lunrpt, doublereal *
	stpb, doublereal *stpd, integer *ldstpd, doublereal *sclb, doublereal 
	*scld, integer *ldscld, doublereal *work, integer *lwork, integer *
	iwork, integer *liwork, integer *maxit1, doublereal *tstimp, integer *
	info)
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal p5 = .5;
    static doublereal one = 1.;
    static doublereal ten = 10.;

    /* System generated locals */
    integer scld_dim1, scld_offset, stpd_dim1, stpd_offset, we_dim1, we_dim2, 
	    we_offset, wd_dim1, wd_dim2, wd_offset, x_dim1, x_offset, y_dim1, 
	    y_offset, ifixx_dim1, ifixx_offset, i__1, i__2;
    doublereal d__1, d__2, d__3, d__4;

    /* Builtin functions */
    double sqrt(doublereal), d_lg10(doublereal *), pow_di(doublereal *, 
	    integer *);

    /* Local variables */
    static integer i__, k, fi, si, ti, ui;
    static doublereal eta;
    static integer fni, sdi, fsi, npp, ssi, tti, wrk, we1i, idfi;
    extern /* Subroutine */ int djck_(S_fp, integer *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, integer *, 
	    integer *, doublereal *, doublereal *, integer *, doublereal *, 
	    doublereal *, integer *, doublereal *, integer *, integer *, 
	    integer *, logical *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, integer *, integer *, doublereal *, integer *, 
	    integer *, integer *, doublereal *, doublereal *, doublereal *);
    static integer etai, jobi, neta, msgb, msgd, nfev;
    extern doublereal ddot_(integer *, doublereal *, integer *, doublereal *, 
	    integer *);
    static integer njev, taui, ssfi, nppi, ldtt, vcvi, ntol, lwrk;
    extern /* Subroutine */ int dxmy_(integer *, integer *, doublereal *, 
	    integer *, doublereal *, integer *, doublereal *, integer *), 
	    dxpy_(integer *, integer *, doublereal *, integer *, doublereal *,
	     integer *, doublereal *, integer *);
    static integer nrow, wssi, nnzw;
    extern doublereal dnrm2_(integer *, doublereal *, integer *);
    static integer int2i, wrk1i, wrk2i, wrk3i, wrk4i, wrk5i, wrk6i, wrk7i;
    static logical cdjac;
    static integer diffi;
    extern /* Subroutine */ int dpack_(integer *, integer *, doublereal *, 
	    doublereal *, integer *), detaf_(S_fp, integer *, integer *, 
	    integer *, integer *, doublereal *, doublereal *, doublereal *, 
	    integer *, doublereal *, doublereal *, integer *, integer *, 
	    integer *, integer *, integer *, doublereal *, integer *, 
	    doublereal *, doublereal *, doublereal *, doublereal *);
    static integer netai;
    extern /* Subroutine */ int dodmn_(logical *, logical *, logical *, S_fp, 
	    integer *, integer *, integer *, integer *, integer *, doublereal 
	    *, doublereal *, integer *, doublereal *, integer *, doublereal *,
	     doublereal *, integer *, integer *, doublereal *, integer *, 
	    integer *, integer *, integer *, integer *, doublereal *, 
	    doublereal *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, doublereal *, doublereal *, integer *, doublereal *,
	     integer *, doublereal *, doublereal *, doublereal *, integer *, 
	    doublereal *, doublereal *, integer *, doublereal *, doublereal *,
	     integer *, doublereal *, integer *, integer *, integer *, 
	    integer *);
    static logical redoj;
    static integer nfevi;
    static logical initd;
    extern /* Subroutine */ int dwinf_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, logical *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *);
    static integer njevi;
    extern /* Subroutine */ int dsetn_(integer *, integer *, doublereal *, 
	    integer *, integer *), dcopy_(integer *, doublereal *, integer *, 
	    doublereal *, integer *), dwght_(integer *, integer *, doublereal 
	    *, integer *, integer *, doublereal *, integer *, doublereal *, 
	    integer *);
    static integer ldtti;
    static logical dovcv;
    static integer rvari;
    static logical isodr;
    static integer ntoli, lwkmn, jpvti, istop, nrowi, beta0i, nnzwi;
    static logical anajac;
    static integer fjacbi, fjacdi;
    static logical chkjac;
    static integer betaci;
    extern /* Subroutine */ int dodchk_(integer *, integer *, integer *, 
	    integer *, logical *, logical *, logical *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    doublereal *, doublereal *, doublereal *, doublereal *, integer *)
	    ;
    static integer alphai;
    extern /* Subroutine */ int dflags_(integer *, logical *, logical *, 
	    logical *, logical *, logical *, logical *, logical *, logical *, 
	    logical *);
    static integer omegai, betani, deltai, betasi;
    static doublereal epsmac;
    extern /* Subroutine */ int dunpac_(integer *, doublereal *, doublereal *,
	     integer *);
    static integer taufci, iranki, deltni, epsmai, rcondi;
    extern /* Subroutine */ int diwinf_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *);
    static integer deltsi, actrsi;
    extern /* Subroutine */ int diniwk_(integer *, integer *, integer *, 
	    doublereal *, integer *, integer *, integer *, doublereal *, 
	    integer *, integer *, integer *, doublereal *, integer *, 
	    doublereal *, doublereal *, doublereal *, doublereal *, integer *,
	     doublereal *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *);
    static integer olmavi;
    static logical implct;
    extern /* Subroutine */ int dfctrw_(integer *, integer *, integer *, 
	    integer *, logical *, doublereal *, integer *, integer *, 
	    doublereal *, integer *, integer *, doublereal *, doublereal *, 
	    doublereal *, integer *, integer *);
    static integer iprini, maxiti, niteri, luneri, partli, wssdei, liwkmn;
    extern /* Subroutine */ int dodper_(integer *, integer *, logical *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    doublereal *, doublereal *, doublereal *, integer *, logical *, 
	    integer *, doublereal *, integer *, integer *, integer *);
    static integer pnormi, prersi, istopi, lunrpi, qrauxi, rnorsi, sstoli, 
	    wssepi;
    static logical restrt;
    static integer xplusi;

/* ***BEGIN PROLOGUE  DODDRV */
/* ***REFER TO DODR,DODRC */
/* ***ROUTINES CALLED  FCN,DCOPY,DDOT,DETAF,DFCTRW,DFLAGS, */
/*                    DINIWK,DIWINF,DJCK,DNRM2,DODCHK,DODMN, */
/*                    DODPER,DPACK,DSETN,DUNPAC,DWGHT,DWINF,DXMY,DXPY */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  PERFORM ERROR CHECKING AND INITIALIZATION, AND BEGIN */
/*            PROCEDURE FOR PERFORMING ORTHOGONAL DISTANCE REGRESSION */
/*            (ODR) OR ORDINARY LINEAR OR NONLINEAR LEAST SQUARES (OLS) */
/* ***END PROLOGUE  DODDRV */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL FUNCTIONS */
/* ...EXTERNAL SUBROUTINES */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --sclb;
    --stpb;
    --ifixb;
    --beta;
    y_dim1 = *ldy;
    y_offset = 1 + y_dim1;
    y -= y_offset;
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    we_dim1 = *ldwe;
    we_dim2 = *ld2we;
    we_offset = 1 + we_dim1 * (1 + we_dim2);
    we -= we_offset;
    wd_dim1 = *ldwd;
    wd_dim2 = *ld2wd;
    wd_offset = 1 + wd_dim1 * (1 + wd_dim2);
    wd -= wd_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;
    stpd_dim1 = *ldstpd;
    stpd_offset = 1 + stpd_dim1;
    stpd -= stpd_offset;
    scld_dim1 = *ldscld;
    scld_offset = 1 + scld_dim1;
    scld -= scld_offset;
    --work;
    --iwork;

    /* Function Body */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:     THE USER SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ACTRSI:  THE LOCATION IN ARRAY WORK OF VARIABLE ACTRS. */
/*   ALPHAI:  THE LOCATION IN ARRAY WORK OF VARIABLE ALPHA. */
/*   ANAJAC:  THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE */
/*            COMPUTED BY FINITE DIFFERENCES (ANAJAC=FALSE) OR NOT */
/*            (ANAJAC=TRUE). */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   BETACI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY BETAC. */
/*   BETANI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY BETAN. */
/*   BETASI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY BETAS. */
/*   BETA0I:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY BETA0. */
/*   CDJAC:   THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE */
/*            COMPUTED BY CENTRAL DIFFERENCES (CDJAC=TRUE) OR FORWARD */
/*            DIFFERENCES (CDJAC=FALSE). */
/*   CHKJAC:  THE VARIABLE DESIGNATING WHETHER THE USER SUPPLIED */
/*            JACOBIANS ARE TO BE CHECKED (CHKJAC=TRUE) OR NOT */
/*            (CHKJAC=FALSE). */
/*   DELTAI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY DELTA. */
/*   DELTNI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY DELTAN. */
/*   DELTSI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY DELTAS. */
/*   DIFFI:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY DIFF. */
/*   DOVCV:   THE VARIABLE DESIGNATING WHETHER THE COVARIANCE MATRIX IS */
/*            TO BE COMPUTED (DOVCV=TRUE) OR NOT (DOVCV=FALSE). */
/*   EPSMAI:  THE LOCATION IN ARRAY WORK OF VARIABLE EPSMAC. */
/*   ETA:     THE RELATIVE NOISE IN THE FUNCTION RESULTS. */
/*   ETAI:    THE LOCATION IN ARRAY WORK OF VARIABLE ETA. */
/*   FI:      THE STARTING LOCATION IN ARRAY WORK OF ARRAY F. */
/*   FJACBI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY FJACB. */
/*   FJACDI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY FJACD. */
/*   FNI:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY FN. */
/*   FSI:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY FS. */
/*   FSTITR:  THE VARIABLE DESIGNATING WHETHER THIS IS THE FIRST */
/*            ITERATION (FSTITR=TRUE) OR NOT (FSTITR=FALSE). */
/*   HEAD:    THE VARIABLE DESIGNATING WHETHER THE HEADING IS TO BE */
/*            PRINTED (HEAD=TRUE) OR NOT (HEAD=FALSE). */
/*   I:       AN INDEX VARIABLE. */
/*   IDFI:    THE LOCATION IN ARRAY IWORK OF VARIABLE IDF. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IMPLCT:  THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY */
/*            IMPLICIT ODR (IMPLCT=TRUE) OR EXPLICIT ODR (IMPLCT=FALSE). */
/*   INFO:    THE VARIABLE DESIGNATING WHY THE COMPUTATIONS WERE STOPPED. */
/*   INITD:   THE VARIABLE DESIGNATING WHETHER DELTA IS TO BE INITIALIZED */
/*            TO ZERO (INITD=TRUE) OR TO THE VALUES IN THE FIRST N BY M */
/*            ELEMENTS OF ARRAY WORK (INITD=FALSE). */
/*   INT2I:   THE IN ARRAY IWORK OF VARIABLE INT2. */
/*   IPRINI:  THE LOCATION IN ARRAY IWORK OF VARIABLE IPRINT. */
/*   IPRINT:  THE PRINT CONTROL VARIABLE. */
/*   IRANKI:  THE LOCATION IN ARRAY IWORK OF VARIABLE IRANK. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR BY OLS (ISODR=FALSE). */
/*   ISTOP:   THE VARIABLE DESIGNATING WHETHER THERE ARE PROBLEMS */
/*            COMPUTING THE FUNCTION AT THE CURRENT BETA AND DELTA. */
/*   ISTOPI:  THE LOCATION IN ARRAY IWORK OF VARIABLE ISTOP. */
/*   IWORK:   THE INTEGER WORK SPACE. */
/*   JOB:     THE VARIABLE CONTROLING PROBLEM INITIALIZATION AND */
/*            COMPUTATIONAL METHOD. */
/*   JOBI:    THE LOCATION IN ARRAY IWORK OF VARIABLE JOB. */
/*   JPVTI:   THE STARTING LOCATION IN ARRAY IWORK OF ARRAY JPVT. */
/*   K:       AN INDEX VARIABLE. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LDSCLD:  THE LEADING DIMENSION OF ARRAY SCLD. */
/*   LDSTPD:  THE LEADING DIMENSION OF ARRAY STPD. */
/*   LDTT:    THE LEADING DIMENSION OF ARRAY TT. */
/*   LDTTI:   THE LOCATION IN ARRAY IWORK OF VARIABLE LDTT. */
/*   LDWD:    THE LEADING DIMENSION OF ARRAY WD. */
/*   LDWE:    THE LEADING DIMENSION OF ARRAY WE. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   LDY:     THE LEADING DIMENSION OF ARRAY Y. */
/*   LD2WD:   THE SECOND DIMENSION OF ARRAY WD. */
/*   LD2WE:   THE SECOND DIMENSION OF ARRAY WE. */
/*   LIWKMN:  THE MINIMUM ACCEPTABLE LENGTH OF ARRAY IWORK. */
/*   LIWORK:  THE LENGTH OF VECTOR IWORK. */
/*   LUNERI:  THE LOCATION IN ARRAY IWORK OF VARIABLE LUNERR. */
/*   LUNERR:  THE LOGICAL UNIT NUMBER USED FOR ERROR MESSAGES. */
/*   LUNRPI:  THE LOCATION IN ARRAY IWORK OF VARIABLE LUNRPT. */
/*   LUNRPT:  THE LOGICAL UNIT NUMBER USED FOR COMPUTATION REPORTS. */
/*   LWKMN:   THE MINIMUM ACCEPTABLE LENGTH OF ARRAY WORK. */
/*   LWORK:   THE LENGTH OF VECTOR WORK. */
/*   LWRK:    THE LENGTH OF VECTOR WRK. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   MAXIT:   THE MAXIMUM NUMBER OF ITERATIONS ALLOWED. */
/*   MAXIT1:  FOR IMPLICIT MODELS, THE ITERATIONS ALLOWED FOR THE NEXT */
/*            PENALTY PARAMETER VALUE. */
/*   MAXITI:  THE LOCATION IN ARRAY IWORK OF VARIABLE MAXIT. */
/*   MSGB:    THE STARTING LOCATION IN ARRAY IWORK OF ARRAY MSGB. */
/*   MSGD:    THE STARTING LOCATION IN ARRAY IWORK OF ARRAY MSGD. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NDIGIT:  THE NUMBER OF ACCURATE DIGITS IN THE FUNCTION RESULTS, AS */
/*            SUPPLIED BY THE USER. */
/*   NETA:    THE NUMBER OF ACCURATE DIGITS IN THE FUNCTION RESULTS. */
/*   NETAI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NETA. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NFEVI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NFEV. */
/*   NITERI:  THE LOCATION IN ARRAY IWORK OF VARIABLE NITER. */
/*   NJEV:    THE NUMBER OF JACOBIAN EVALUATIONS. */
/*   NJEVI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NJEV. */
/*   NNZW:    THE NUMBER OF NONZERO OBSERVATIONAL ERROR WEIGHTS. */
/*   NNZWI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NNZW. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NPP:     THE NUMBER OF FUNCTION PARAMETERS BEING ESTIMATED. */
/*   NPPI:    THE LOCATION IN ARRAY IWORK OF VARIABLE NPP. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   NROW:    THE ROW NUMBER AT WHICH THE DERIVATIVE IS TO BE CHECKED. */
/*   NROWI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NROW. */
/*   NTOL:    THE NUMBER OF DIGITS OF AGREEMENT REQUIRED BETWEEN THE */
/*            NUMERICAL DERIVATIVES AND THE USER SUPPLIED DERIVATIVES, */
/*            SET BY DJCK. */
/*   NTOLI:   THE LOCATION IN ARRAY IWORK OF VARIABLE NTOL. */
/*   OLMAVI:  THE LOCATION IN ARRAY WORK OF VARIABLE OLMAVG. */
/*   OMEGAI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY OMEGA. */
/*   ONE:     THE VALUE 1.0D0. */
/*   PARTLI:  THE LOCATION IN ARRAY WORK OF VARIABLE PARTOL. */
/*   PARTOL:  THE PARAMETER CONVERGENCE STOPPING TOLERANCE. */
/*   PNORM:   THE NORM OF THE SCALED ESTIMATED PARAMETERS. */
/*   PNORMI:  THE LOCATION IN ARRAY WORK OF VARIABLE PNORM. */
/*   PRERSI:  THE LOCATION IN ARRAY WORK OF VARIABLE PRERS. */
/*   PRTPEN:  THE VARIABLE DESIGNATING WHETHER THE PENALTY PARAMETER IS */
/*            TO BE PRINTED IN THE ITERATION REPORT (PRTPEN=TRUE) OR NOT */
/*            (PRTPEN=FALSE). */
/*   P5:      THE VALUE 0.5D0. */
/*   QRAUXI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY QRAUX. */
/*   RCONDI:  THE LOCATION IN ARRAY WORK OF VARIABLE RCOND. */
/*   REDOJ:   THE VARIABLE DESIGNATING WHETHER THE JACOBIAN MATRIX IS TO */
/*            BE RECOMPUTED FOR THE COMPUTATION OF THE COVARIANCE MATRIX */
/*            (REDOJ=TRUE) OR NOT (REDOJ=FALSE). */
/*   RESTRT:  THE VARIABLE DESIGNATING WHETHER THE CALL IS A RESTART */
/*            (RESTRT=TRUE) OR NOT (RESTRT=FALSE). */
/*   RNORSI:  THE LOCATION IN ARRAY WORK OF VARIABLE RNORMS. */
/*   RVARI:   THE LOCATION IN ARRAY WORK OF VARIABLE RVAR. */
/*   SCLB:    THE SCALING VALUES FOR BETA. */
/*   SCLD:    THE SCALING VALUES FOR DELTA. */
/*   SDI:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY SD. */
/*   SHORT:   THE VARIABLE DESIGNATING WHETHER THE USER HAS INVOKED */
/*            ODRPACK BY THE SHORT-CALL (SHORT=TRUE) OR THE LONG-CALL */
/*            (SHORT=FALSE). */
/*   SI:      THE STARTING LOCATION IN ARRAY WORK OF ARRAY S. */
/*   SSFI:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY SSF. */
/*   SSI:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY SS. */
/*   SSTOL:   THE SUM-OF-SQUARES CONVERGENCE STOPPING TOLERANCE. */
/*   SSTOLI:  THE LOCATION IN ARRAY WORK OF VARIABLE SSTOL. */
/*   STPB:    THE STEP SIZE FOR FINITE DIFFERENCE DERIVATIVES WRT BETA. */
/*   STPD:    THE STEP SIZE FOR FINITE DIFFERENCE DERIVATIVES WRT DELTA. */
/*   TAUFAC:  THE FACTOR USED TO COMPUTE THE INITIAL TRUST REGION */
/*            DIAMETER. */
/*   TAUFCI:  THE LOCATION IN ARRAY WORK OF VARIABLE TAUFAC. */
/*   TAUI:    THE LOCATION IN ARRAY WORK OF VARIABLE TAU. */
/*   TEN:     THE VALUE 10.0D0. */
/*   TI:      THE STARTING LOCATION IN ARRAY WORK OF ARRAY T. */
/*   TSTIMP:  THE RELATIVE CHANGE IN THE PARAMETERS BETWEEN THE INITIAL */
/*            VALUES AND THE SOLUTION. */
/*   TTI:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY TT. */
/*   UI:      THE STARTING LOCATION IN ARRAY WORK OF ARRAY U. */
/*   VCVI:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY VCV. */
/*   WD:      THE DELTA WEIGHTS. */
/*   WE:      THE EPSILON WEIGHTS. */
/*   WE1I:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY WE1. */
/*   WORK:    THE DOUBLE PRECISION WORK SPACE. */
/*   WRK:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK, */
/*            EQUIVALENCED TO WRK1 AND WRK2. */
/*   WRK1I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK1. */
/*   WRK2I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK2. */
/*   WRK3I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK3. */
/*   WRK4I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK4. */
/*   WRK5I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK5. */
/*   WRK6I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK6. */
/*   WRK7I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK7. */
/*   WSSI:    THE LOCATION IN ARRAY WORK OF VARIABLE WSS. */
/*   WSSDEI:  THE LOCATION IN ARRAY WORK OF VARIABLE WSSDEL. */
/*   WSSEPI:  THE LOCATION IN ARRAY WORK OF VARIABLE WSSEPS. */
/*   X:       THE EXPLANATORY VARIABLE. */
/*   XPLUSI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY XPLUSD. */
/*   Y:       THE DEPENDENT VARIABLE.  UNUSED WHEN THE MODEL IS IMPLICIT. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DODDRV */
/*  INITIALIZE NECESSARY VARIABLES */
    dflags_(job, &restrt, &initd, &dovcv, &redoj, &anajac, &cdjac, &chkjac, &
	    isodr, &implct);
/*  SET STARTING LOCATIONS WITHIN INTEGER WORKSPACE */
/*  (INVALID VALUES OF M, NP AND/OR NQ ARE HANDLED REASONABLY BY DIWINF) */
    diwinf_(m, np, nq, &msgb, &msgd, &jpvti, &istopi, &nnzwi, &nppi, &idfi, &
	    jobi, &iprini, &luneri, &lunrpi, &nrowi, &ntoli, &netai, &maxiti, 
	    &niteri, &nfevi, &njevi, &int2i, &iranki, &ldtti, &liwkmn);
/*  SET STARTING LOCATIONS WITHIN DOUBLE PRECISION WORK SPACE */
/*  (INVALID VALUES OF N, M, NP, NQ, LDWE AND/OR LD2WE */
/*  ARE HANDLED REASONABLY BY DWINF) */
    dwinf_(n, m, np, nq, ldwe, ld2we, &isodr, &deltai, &fi, &xplusi, &fni, &
	    sdi, &vcvi, &rvari, &wssi, &wssdei, &wssepi, &rcondi, &etai, &
	    olmavi, &taui, &alphai, &actrsi, &pnormi, &rnorsi, &prersi, &
	    partli, &sstoli, &taufci, &epsmai, &beta0i, &betaci, &betasi, &
	    betani, &si, &ssi, &ssfi, &qrauxi, &ui, &fsi, &fjacbi, &we1i, &
	    diffi, &deltsi, &deltni, &ti, &tti, &omegai, &fjacdi, &wrk1i, &
	    wrk2i, &wrk3i, &wrk4i, &wrk5i, &wrk6i, &wrk7i, &lwkmn);
    if (isodr) {
	wrk = wrk1i;
	lwrk = *n * *m * *nq + *n * *nq;
    } else {
	wrk = wrk2i;
	lwrk = *n * *nq;
    }
/*  UPDATE THE PENALTY PARAMETERS */
/*  (WE(1,1,1) IS NOT A USER SUPPLIED ARRAY IN THIS CASE) */
    if (restrt && implct) {
/* Computing MAX */
/* Computing 2nd power */
	d__4 = work[we1i];
	d__2 = d__4 * d__4, d__3 = (d__1 = we[(we_dim2 + 1) * we_dim1 + 1], 
		abs(d__1));
	we[(we_dim2 + 1) * we_dim1 + 1] = max(d__2,d__3);
	work[we1i] = -sqrt((d__1 = we[(we_dim2 + 1) * we_dim1 + 1], abs(d__1))
		);
    }
    if (restrt) {
/*  RESET MAXIMUM NUMBER OF ITERATIONS */
	if (*maxit >= 0) {
	    iwork[maxiti] = iwork[niteri] + *maxit;
	} else {
	    iwork[maxiti] = iwork[niteri] + 10;
	}
	if (iwork[niteri] < iwork[maxiti]) {
	    *info = 0;
	}
	if (*job >= 0) {
	    iwork[jobi] = *job;
	}
	if (*iprint >= 0) {
	    iwork[iprini] = *iprint;
	}
	if (*partol >= zero && *partol < one) {
	    work[partli] = *partol;
	}
	if (*sstol >= zero && *sstol < one) {
	    work[sstoli] = *sstol;
	}
	work[olmavi] *= iwork[niteri];
	if (implct) {
	    i__1 = *n * *nq;
	    dcopy_(&i__1, &work[fni], &c__1, &work[fi], &c__1);
	} else {
	    dxmy_(n, nq, &work[fni], n, &y[y_offset], ldy, &work[fi], n);
	}
	dwght_(n, nq, &work[we1i], ldwe, ld2we, &work[fi], n, &work[fi], n);
	i__1 = *n * *nq;
	work[wssepi] = ddot_(&i__1, &work[fi], &c__1, &work[fi], &c__1);
	work[wssi] = work[wssepi] + work[wssdei];
    } else {
/*  PERFORM ERROR CHECKING */
	*info = 0;
	dodchk_(n, m, np, nq, &isodr, &anajac, &implct, &ifixb[1], ldx, ldifx,
		 ldscld, ldstpd, ldwe, ld2we, ldwd, ld2wd, ldy, lwork, &lwkmn,
		 liwork, &liwkmn, &sclb[1], &scld[scld_offset], &stpb[1], &
		stpd[stpd_offset], info);
	if (*info > 0) {
	    goto L50;
	}
/*  INITIALIZE WORK VECTORS AS NECESSARY */
	i__1 = *lwork;
	for (i__ = *n * *m + *n * *nq + 1; i__ <= i__1; ++i__) {
	    work[i__] = zero;
/* L10: */
	}
	i__1 = *liwork;
	for (i__ = 1; i__ <= i__1; ++i__) {
	    iwork[i__] = 0;
/* L20: */
	}
	diniwk_(n, m, np, &work[1], lwork, &iwork[1], liwork, &x[x_offset], 
		ldx, &ifixx[ifixx_offset], ldifx, &scld[scld_offset], ldscld, 
		&beta[1], &sclb[1], sstol, partol, maxit, taufac, job, iprint,
		 lunerr, lunrpt, &epsmai, &sstoli, &partli, &maxiti, &taufci, 
		&jobi, &iprini, &luneri, &lunrpi, &ssfi, &tti, &ldtti, &
		deltai);
	iwork[msgb] = -1;
	iwork[msgd] = -1;
	work[taui] = -work[taufci];
/*  SET UP FOR PARAMETER ESTIMATION - */
/*  PULL BETA'S TO BE ESTIMATED AND CORRESPONDING SCALE VALUES */
/*  AND STORE IN WORK(BETACI) AND WORK(SSI), RESPECTIVELY */
	dpack_(np, &iwork[nppi], &work[betaci], &beta[1], &ifixb[1]);
	dpack_(np, &iwork[nppi], &work[ssi], &work[ssfi], &ifixb[1]);
	npp = iwork[nppi];
/*  CHECK THAT WD IS POSITIVE DEFINITE AND WE IS POSITIVE SEMIDEFINITE, */
/*  SAVING FACTORIZATION OF WE, AND COUNTING NUMBER OF NONZERO WEIGHTS */
	dfctrw_(n, m, nq, &npp, &isodr, &we[we_offset], ldwe, ld2we, &wd[
		wd_offset], ldwd, ld2wd, &work[wrk2i], &work[wrk4i], &work[
		we1i], &nnzw, info);
	iwork[nnzwi] = nnzw;
	if (*info != 0) {
	    goto L50;
	}
/*  EVALUATE THE PREDICTED VALUES AND */
/*               WEIGHTED EPSILONS AT THE STARTING POINT */
	dunpac_(np, &work[betaci], &beta[1], &ifixb[1]);
	dxpy_(n, m, &x[x_offset], ldx, &work[deltai], n, &work[xplusi], n);
	istop = 0;
	(*fcn)(n, m, np, nq, n, m, np, &beta[1], &work[xplusi], &ifixb[1], &
		ifixx[ifixx_offset], ldifx, &c__2, &work[fni], &work[wrk6i], &
		work[wrk1i], &istop);
	iwork[istopi] = istop;
	if (istop == 0) {
	    ++iwork[nfevi];
	    if (implct) {
		i__1 = *n * *nq;
		dcopy_(&i__1, &work[fni], &c__1, &work[fi], &c__1);
	    } else {
		dxmy_(n, nq, &work[fni], n, &y[y_offset], ldy, &work[fi], n);
	    }
	    dwght_(n, nq, &work[we1i], ldwe, ld2we, &work[fi], n, &work[fi], 
		    n);
	} else {
	    *info = 52000;
	    goto L50;
	}
/*  COMPUTE NORM OF THE INITIAL ESTIMATES */
	dwght_(&npp, &c__1, &work[ssi], &npp, &c__1, &work[betaci], &npp, &
		work[wrk], &npp);
	if (isodr) {
	    dwght_(n, m, &work[tti], &iwork[ldtti], &c__1, &work[deltai], n, &
		    work[wrk + npp], n);
	    i__1 = npp + *n * *m;
	    work[pnormi] = dnrm2_(&i__1, &work[wrk], &c__1);
	} else {
	    work[pnormi] = dnrm2_(&npp, &work[wrk], &c__1);
	}
/*  COMPUTE SUM OF SQUARES OF THE WEIGHTED EPSILONS AND WEIGHTED DELTAS */
	i__1 = *n * *nq;
	work[wssepi] = ddot_(&i__1, &work[fi], &c__1, &work[fi], &c__1);
	if (isodr) {
	    dwght_(n, m, &wd[wd_offset], ldwd, ld2wd, &work[deltai], n, &work[
		    wrk], n);
	    i__1 = *n * *m;
	    work[wssdei] = ddot_(&i__1, &work[deltai], &c__1, &work[wrk], &
		    c__1);
	} else {
	    work[wssdei] = zero;
	}
	work[wssi] = work[wssepi] + work[wssdei];
/*  SELECT FIRST ROW OF X + DELTA THAT CONTAINS NO ZEROS */
	nrow = -1;
	dsetn_(n, m, &work[xplusi], n, &nrow);
	iwork[nrowi] = nrow;
/*  SET NUMBER OF GOOD DIGITS IN FUNCTION RESULTS */
	epsmac = work[epsmai];
	if (*ndigit < 2) {
	    iwork[netai] = -1;
	    nfev = iwork[nfevi];
	    detaf_((S_fp)fcn, n, m, np, nq, &work[xplusi], &beta[1], &epsmac, 
		    &nrow, &work[betani], &work[fni], &ifixb[1], &ifixx[
		    ifixx_offset], ldifx, &istop, &nfev, &eta, &neta, &work[
		    wrk1i], &work[wrk2i], &work[wrk6i], &work[wrk7i]);
	    iwork[istopi] = istop;
	    iwork[nfevi] = nfev;
	    if (istop != 0) {
		*info = 53000;
		iwork[netai] = 0;
		work[etai] = zero;
		goto L50;
	    } else {
		iwork[netai] = -neta;
		work[etai] = eta;
	    }
	} else {
/* Computing MIN */
	    i__1 = *ndigit, i__2 = (integer) (p5 - d_lg10(&epsmac));
	    iwork[netai] = min(i__1,i__2);
/* Computing MAX */
	    i__1 = -(*ndigit);
	    d__1 = epsmac, d__2 = pow_di(&ten, &i__1);
	    work[etai] = max(d__1,d__2);
	}
/*  CHECK DERIVATIVES IF NECESSARY */
	if (chkjac && anajac) {
	    ntol = -1;
	    nfev = iwork[nfevi];
	    njev = iwork[njevi];
	    neta = iwork[netai];
	    ldtt = iwork[ldtti];
	    eta = work[etai];
	    epsmac = work[epsmai];
	    djck_((S_fp)fcn, n, m, np, nq, &beta[1], &work[xplusi], &ifixb[1],
		     &ifixx[ifixx_offset], ldifx, &stpb[1], &stpd[stpd_offset]
		    , ldstpd, &work[ssfi], &work[tti], &ldtt, &eta, &neta, &
		    ntol, &nrow, &isodr, &epsmac, &work[fni], &work[fjacbi], &
		    work[fjacdi], &iwork[msgb], &iwork[msgd], &work[diffi], &
		    istop, &nfev, &njev, &work[wrk1i], &work[wrk2i], &work[
		    wrk6i]);
	    iwork[istopi] = istop;
	    iwork[nfevi] = nfev;
	    iwork[njevi] = njev;
	    iwork[ntoli] = ntol;
	    if (istop != 0) {
		*info = 54000;
	    } else if (iwork[msgb] != 0 || iwork[msgd] != 0) {
		*info = 40000;
	    }
	} else {
/*  INDICATE USER SUPPLIED DERIVATIVES WERE NOT CHECKED */
	    iwork[msgb] = -1;
	    iwork[msgd] = -1;
	}
/*  PRINT APPROPRIATE ERROR MESSAGES */
L50:
	if (*info != 0 || iwork[msgb] != -1) {
	    if (*lunerr != 0 && *iprint != 0) {
		dodper_(info, lunerr, short__, n, m, np, nq, ldscld, ldstpd, 
			ldwe, ld2we, ldwd, ld2wd, &lwkmn, &liwkmn, &work[
			fjacbi], &work[fjacdi], &work[diffi], &iwork[msgb], &
			isodr, &iwork[msgd], &work[xplusi], &iwork[nrowi], &
			iwork[netai], &iwork[ntoli]);
	    }
/*  SET INFO TO REFLECT ERRORS IN THE USER SUPPLIED JACOBIANS */
	    if (*info == 40000) {
		if (iwork[msgb] == 2 || iwork[msgd] == 2) {
		    if (iwork[msgb] == 2) {
			*info += 1000;
		    }
		    if (iwork[msgd] == 2) {
			*info += 100;
		    }
		} else {
		    *info = 0;
		}
	    }
	    if (*info != 0) {
		return 0;
	    }
	}
    }
/*  SAVE THE INITIAL VALUES OF BETA */
    dcopy_(np, &beta[1], &c__1, &work[beta0i], &c__1);
/*  FIND LEAST SQUARES SOLUTION */
    i__1 = *n * *nq;
    dcopy_(&i__1, &work[fni], &c__1, &work[fsi], &c__1);
    ldtt = iwork[ldtti];
    dodmn_(head, fstitr, prtpen, (S_fp)fcn, n, m, np, nq, job, &beta[1], &y[
	    y_offset], ldy, &x[x_offset], ldx, &we[we_offset], &work[we1i], 
	    ldwe, ld2we, &wd[wd_offset], ldwd, ld2wd, &ifixb[1], &ifixx[
	    ifixx_offset], ldifx, &work[betaci], &work[betani], &work[betasi],
	     &work[si], &work[deltai], &work[deltni], &work[deltsi], &work[ti]
	    , &work[fi], &work[fni], &work[fsi], &work[fjacbi], &iwork[msgb], 
	    &work[fjacdi], &iwork[msgd], &work[ssfi], &work[ssi], &work[tti], 
	    &ldtt, &stpb[1], &stpd[stpd_offset], ldstpd, &work[xplusi], &work[
	    wrk], &lwrk, &work[1], lwork, &iwork[1], liwork, info);
    *maxit1 = iwork[maxiti] - iwork[niteri];
    *tstimp = zero;
    i__1 = *np;
    for (k = 1; k <= i__1; ++k) {
	if (beta[k] == zero) {
/* Computing MAX */
	    d__2 = *tstimp, d__3 = (d__1 = beta[k] - work[beta0i - 1 + k], 
		    abs(d__1)) / work[ssfi - 1 + k];
	    *tstimp = max(d__2,d__3);
	} else {
/* Computing MAX */
	    d__3 = *tstimp, d__4 = (d__1 = beta[k] - work[beta0i - 1 + k], 
		    abs(d__1)) / (d__2 = beta[k], abs(d__2));
	    *tstimp = max(d__3,d__4);
	}
/* L100: */
    }
    return 0;
} /* doddrv_ */

/* DODLM */
/* Subroutine */ int dodlm_(integer *n, integer *m, integer *np, integer *nq, 
	integer *npp, doublereal *f, doublereal *fjacb, doublereal *fjacd, 
	doublereal *wd, integer *ldwd, integer *ld2wd, doublereal *ss, 
	doublereal *tt, integer *ldtt, doublereal *delta, doublereal *alpha2, 
	doublereal *tau, doublereal *epsfcn, logical *isodr, doublereal *
	tfjacb, doublereal *omega, doublereal *u, doublereal *qraux, integer *
	jpvt, doublereal *s, doublereal *t, integer *nlms, doublereal *rcond, 
	integer *irank, doublereal *wrk1, doublereal *wrk2, doublereal *wrk3, 
	doublereal *wrk4, doublereal *wrk5, doublereal *wrk, integer *lwrk, 
	integer *istopc)
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal p001 = .001;
    static doublereal p1 = .1;

    /* System generated locals */
    integer delta_dim1, delta_offset, f_dim1, f_offset, fjacb_dim1, 
	    fjacb_dim2, fjacb_offset, fjacd_dim1, fjacd_dim2, fjacd_offset, 
	    omega_dim1, omega_offset, t_dim1, t_offset, tfjacb_dim1, 
	    tfjacb_dim2, tfjacb_offset, tt_dim1, tt_offset, wd_dim1, wd_dim2, 
	    wd_offset, wrk1_dim1, wrk1_dim2, wrk1_offset, wrk2_dim1, 
	    wrk2_offset, wrk4_dim1, wrk4_offset, i__1, i__2, i__3;
    doublereal d__1, d__2;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static integer i__, j, k, l;
    static doublereal sa, bot, top, phi1, phi2;
    extern doublereal ddot_(integer *, doublereal *, integer *, doublereal *, 
	    integer *);
    static integer iwrk;
    extern doublereal dnrm2_(integer *, doublereal *, integer *);
    extern /* Subroutine */ int dwght_(integer *, integer *, doublereal *, 
	    integer *, integer *, doublereal *, integer *, doublereal *, 
	    integer *);
    static doublereal alpha1;
    extern /* Subroutine */ int dscale_(integer *, integer *, doublereal *, 
	    integer *, doublereal *, integer *, doublereal *, integer *);
    static doublereal alphan;
    extern /* Subroutine */ int dodstp_(integer *, integer *, integer *, 
	    integer *, integer *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, integer *, integer *, doublereal *, doublereal *, 
	    integer *, doublereal *, doublereal *, doublereal *, logical *, 
	    doublereal *, doublereal *, doublereal *, doublereal *, integer *,
	     doublereal *, doublereal *, doublereal *, integer *, doublereal *
	    , logical *, doublereal *, doublereal *, doublereal *, doublereal 
	    *, doublereal *, doublereal *, integer *, integer *);
    static logical forvcv;

/* ***BEGIN PROLOGUE  DODLM */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DDOT,DNRM2,DODSTP,DSCALE,DWGHT */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  COMPUTE LEVENBERG-MARQUARDT PARAMETER AND STEPS S AND T */
/*            USING ANALOG OF THE TRUST-REGION LEVENBERG-MARQUARDT */
/*            ALGORITHM */
/* ***END PROLOGUE  DODLM */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL FUNCTIONS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --wrk5;
    wrk4_dim1 = *m;
    wrk4_offset = 1 + wrk4_dim1;
    wrk4 -= wrk4_offset;
    t_dim1 = *n;
    t_offset = 1 + t_dim1;
    t -= t_offset;
    delta_dim1 = *n;
    delta_offset = 1 + delta_dim1;
    delta -= delta_offset;
    --wrk3;
    --s;
    --jpvt;
    --qraux;
    --u;
    --ss;
    wrk2_dim1 = *n;
    wrk2_offset = 1 + wrk2_dim1;
    wrk2 -= wrk2_offset;
    wrk1_dim1 = *n;
    wrk1_dim2 = *nq;
    wrk1_offset = 1 + wrk1_dim1 * (1 + wrk1_dim2);
    wrk1 -= wrk1_offset;
    omega_dim1 = *nq;
    omega_offset = 1 + omega_dim1;
    omega -= omega_offset;
    tfjacb_dim1 = *n;
    tfjacb_dim2 = *nq;
    tfjacb_offset = 1 + tfjacb_dim1 * (1 + tfjacb_dim2);
    tfjacb -= tfjacb_offset;
    fjacd_dim1 = *n;
    fjacd_dim2 = *m;
    fjacd_offset = 1 + fjacd_dim1 * (1 + fjacd_dim2);
    fjacd -= fjacd_offset;
    fjacb_dim1 = *n;
    fjacb_dim2 = *np;
    fjacb_offset = 1 + fjacb_dim1 * (1 + fjacb_dim2);
    fjacb -= fjacb_offset;
    f_dim1 = *n;
    f_offset = 1 + f_dim1;
    f -= f_offset;
    wd_dim1 = *ldwd;
    wd_dim2 = *ld2wd;
    wd_offset = 1 + wd_dim1 * (1 + wd_dim2);
    wd -= wd_offset;
    tt_dim1 = *ldtt;
    tt_offset = 1 + tt_dim1;
    tt -= tt_offset;
    --wrk;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ALPHAN:  THE NEW LEVENBERG-MARQUARDT PARAMETER. */
/*   ALPHA1:  THE PREVIOUS LEVENBERG-MARQUARDT PARAMETER. */
/*   ALPHA2:  THE CURRENT LEVENBERG-MARQUARDT PARAMETER. */
/*   BOT:     THE LOWER LIMIT FOR SETTING ALPHA. */
/*   DELTA:   THE ESTIMATED ERRORS IN THE EXPLANATORY VARIABLES. */
/*   EPSFCN:  THE FUNCTION'S PRECISION. */
/*   F:       THE (WEIGHTED) ESTIMATED VALUES OF EPSILON. */
/*   FJACB:   THE JACOBIAN WITH RESPECT TO BETA. */
/*   FJACD:   THE JACOBIAN WITH RESPECT TO DELTA. */
/*   FORVCV:  THE VARIABLE DESIGNATING WHETHER THIS SUBROUTINE WAS */
/*            CALLED TO SET UP FOR THE COVARIANCE MATRIX COMPUTATIONS */
/*            (FORVCV=TRUE) OR NOT (FORVCV=FALSE). */
/*   I:       AN INDEXING VARIABLE. */
/*   IRANK:   THE RANK DEFICIENCY OF THE JACOBIAN WRT BETA. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR BY OLS (ISODR=FALSE). */
/*   ISTOPC:  THE VARIABLE DESIGNATING WHETHER THE COMPUTATIONS WERE */
/*            STOPED DUE TO SOME NUMERICAL ERROR DETECTED WITHIN */
/*            SUBROUTINE DODSTP. */
/*   IWRK:    AN INDEXING VARIABLE. */
/*   J:       AN INDEXING VARIABLE. */
/*   K:       AN INDEXING VARIABLE. */
/*   L:       AN INDEXING VARIABLE. */
/*   JPVT:    THE PIVOT VECTOR. */
/*   LDTT:    THE LEADING DIMENSION OF ARRAY TT. */
/*   LDWD:    THE LEADING DIMENSION OF ARRAY WD. */
/*   LD2WD:   THE SECOND DIMENSION OF ARRAY WD. */
/*   LWRK:    THE LENGTH OF VECTOR WRK. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NLMS:    THE NUMBER OF LEVENBERG-MARQUARDT STEPS TAKEN. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NPP:     THE NUMBER OF FUNCTION PARAMETERS BEING ESTIMATED. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   OMEGA:   THE ARRAY (I-FJACD*INV(P)*TRANS(FJACD))**(-1/2)  WHERE */
/*            P = TRANS(FJACD)*FJACD + D**2 + ALPHA*TT**2 */
/*   P001:    THE VALUE 0.001D0 */
/*   P1:      THE VALUE 0.1D0 */
/*   PHI1:    THE PREVIOUS DIFFERENCE BETWEEN THE NORM OF THE SCALED STEP */
/*            AND THE TRUST REGION DIAMETER. */
/*   PHI2:    THE CURRENT DIFFERENCE BETWEEN THE NORM OF THE SCALED STEP */
/*            AND THE TRUST REGION DIAMETER. */
/*   QRAUX:   THE ARRAY REQUIRED TO RECOVER THE ORTHOGONAL PART OF THE */
/*            Q-R DECOMPOSITION. */
/*   RCOND:   THE APPROXIMATE RECIPROCAL CONDITION OF TFJACB. */
/*   S:       THE STEP FOR BETA. */
/*   SA:      THE SCALAR PHI2*(ALPHA1-ALPHA2)/(PHI1-PHI2). */
/*   SS:      THE SCALING VALUES USED FOR THE UNFIXED BETAS. */
/*   T:       THE STEP FOR DELTA. */
/*   TAU:     THE TRUST REGION DIAMETER. */
/*   TFJACB:  THE ARRAY OMEGA*FJACB. */
/*   TOP:     THE UPPER LIMIT FOR SETTING ALPHA. */
/*   TT:      THE SCALE USED FOR THE DELTA'S. */
/*   U:       THE APPROXIMATE NULL VECTOR FOR TFJACB. */
/*   WD:      THE DELTA WEIGHTS. */
/*   WRK:     A WORK ARRAY OF (LWRK) ELEMENTS, */
/*            EQUIVALENCED TO WRK1 AND WRK2. */
/*   WRK1:    A WORK ARRAY OF (N BY NQ BY M) ELEMENTS. */
/*   WRK2:    A WORK ARRAY OF (N BY NQ) ELEMENTS. */
/*   WRK3:    A WORK ARRAY OF (NP) ELEMENTS. */
/*   WRK4:    A WORK ARRAY OF (M BY M) ELEMENTS. */
/*   WRK5:    A WORK ARRAY OF (M) ELEMENTS. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DODLM */
    forvcv = FALSE_;
    *istopc = 0;
/*  COMPUTE FULL GAUSS-NEWTON STEP (ALPHA=0) */
    alpha1 = zero;
    dodstp_(n, m, np, nq, npp, &f[f_offset], &fjacb[fjacb_offset], &fjacd[
	    fjacd_offset], &wd[wd_offset], ldwd, ld2wd, &ss[1], &tt[tt_offset]
	    , ldtt, &delta[delta_offset], &alpha1, epsfcn, isodr, &tfjacb[
	    tfjacb_offset], &omega[omega_offset], &u[1], &qraux[1], &jpvt[1], 
	    &s[1], &t[t_offset], &phi1, irank, rcond, &forvcv, &wrk1[
	    wrk1_offset], &wrk2[wrk2_offset], &wrk3[1], &wrk4[wrk4_offset], &
	    wrk5[1], &wrk[1], lwrk, istopc);
    if (*istopc != 0) {
	return 0;
    }
/*  INITIALIZE TAU IF NECESSARY */
    if (*tau < zero) {
	*tau = abs(*tau) * phi1;
    }
/*  CHECK IF FULL GAUSS-NEWTON STEP IS OPTIMAL */
    if (phi1 - *tau <= p1 * *tau) {
	*nlms = 1;
	*alpha2 = zero;
	return 0;
    }
/*  FULL GAUSS-NEWTON STEP IS OUTSIDE TRUST REGION - */
/*  FIND LOCALLY CONSTRAINED OPTIMAL STEP */
    phi1 -= *tau;
/*  INITIALIZE UPPER AND LOWER BOUNDS FOR ALPHA */
    bot = zero;
    i__1 = *npp;
    for (k = 1; k <= i__1; ++k) {
	i__2 = *nq;
	for (l = 1; l <= i__2; ++l) {
	    i__3 = *n;
	    for (i__ = 1; i__ <= i__3; ++i__) {
		tfjacb[i__ + (l + k * tfjacb_dim2) * tfjacb_dim1] = fjacb[i__ 
			+ (k + l * fjacb_dim2) * fjacb_dim1];
/* L10: */
	    }
/* L20: */
	}
	i__2 = *n * *nq;
	wrk[k] = ddot_(&i__2, &tfjacb[(k * tfjacb_dim2 + 1) * tfjacb_dim1 + 1]
		, &c__1, &f[f_dim1 + 1], &c__1);
/* L30: */
    }
    dscale_(npp, &c__1, &ss[1], npp, &wrk[1], npp, &wrk[1], npp);
    if (*isodr) {
	dwght_(n, m, &wd[wd_offset], ldwd, ld2wd, &delta[delta_offset], n, &
		wrk[*npp + 1], n);
	iwrk = *npp;
	i__1 = *m;
	for (j = 1; j <= i__1; ++j) {
	    i__2 = *n;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		++iwrk;
		i__3 = *n * *m;
		wrk[iwrk] += ddot_(nq, &fjacd[i__ + (j + fjacd_dim2) * 
			fjacd_dim1], &i__3, &f[i__ + f_dim1], n);
/* L40: */
	    }
/* L50: */
	}
	dscale_(n, m, &tt[tt_offset], ldtt, &wrk[*npp + 1], n, &wrk[*npp + 1],
		 n);
	i__1 = *npp + *n * *m;
	top = dnrm2_(&i__1, &wrk[1], &c__1) / *tau;
    } else {
	top = dnrm2_(npp, &wrk[1], &c__1) / *tau;
    }
    if (*alpha2 > top || *alpha2 == zero) {
	*alpha2 = p001 * top;
    }
/*  MAIN LOOP */
    for (i__ = 1; i__ <= 10; ++i__) {
/*  COMPUTE LOCALLY CONSTRAINED STEPS S AND T AND PHI(ALPHA) FOR */
/*  CURRENT VALUE OF ALPHA */
	dodstp_(n, m, np, nq, npp, &f[f_offset], &fjacb[fjacb_offset], &fjacd[
		fjacd_offset], &wd[wd_offset], ldwd, ld2wd, &ss[1], &tt[
		tt_offset], ldtt, &delta[delta_offset], alpha2, epsfcn, isodr,
		 &tfjacb[tfjacb_offset], &omega[omega_offset], &u[1], &qraux[
		1], &jpvt[1], &s[1], &t[t_offset], &phi2, irank, rcond, &
		forvcv, &wrk1[wrk1_offset], &wrk2[wrk2_offset], &wrk3[1], &
		wrk4[wrk4_offset], &wrk5[1], &wrk[1], lwrk, istopc);
	if (*istopc != 0) {
	    return 0;
	}
	phi2 -= *tau;
/*  CHECK WHETHER CURRENT STEP IS OPTIMAL */
	if (abs(phi2) <= p1 * *tau || *alpha2 == bot && phi2 < zero) {
	    *nlms = i__ + 1;
	    return 0;
	}
/*  CURRENT STEP IS NOT OPTIMAL */
/*  UPDATE BOUNDS FOR ALPHA AND COMPUTE NEW ALPHA */
	if (phi1 - phi2 == zero) {
	    *nlms = 12;
	    return 0;
	}
	sa = phi2 * (alpha1 - *alpha2) / (phi1 - phi2);
	if (phi2 < zero) {
	    top = min(top,*alpha2);
	} else {
	    bot = max(bot,*alpha2);
	}
	if (phi1 * phi2 > zero) {
/* Computing MAX */
	    d__1 = bot, d__2 = *alpha2 - sa;
	    bot = max(d__1,d__2);
	} else {
/* Computing MIN */
	    d__1 = top, d__2 = *alpha2 - sa;
	    top = min(d__1,d__2);
	}
	alphan = *alpha2 - sa * (phi1 + *tau) / *tau;
	if (alphan >= top || alphan <= bot) {
/* Computing MAX */
	    d__1 = p001 * top, d__2 = sqrt(top * bot);
	    alphan = max(d__1,d__2);
	}
/*  GET READY FOR NEXT ITERATION */
	alpha1 = *alpha2;
	*alpha2 = alphan;
	phi1 = phi2;
/* L60: */
    }
/*  SET NLMS TO INDICATE AN OPTIMAL STEP COULD NOT BE FOUND IN 10 TRYS */
    *nlms = 12;
    return 0;
} /* dodlm_ */

/* DODMN */
/* Subroutine */ int dodmn_(logical *head, logical *fstitr, logical *prtpen, 
	S_fp fcn, integer *n, integer *m, integer *np, integer *nq, integer *
	job, doublereal *beta, doublereal *y, integer *ldy, doublereal *x, 
	integer *ldx, doublereal *we, doublereal *we1, integer *ldwe, integer 
	*ld2we, doublereal *wd, integer *ldwd, integer *ld2wd, integer *ifixb,
	 integer *ifixx, integer *ldifx, doublereal *betac, doublereal *betan,
	 doublereal *betas, doublereal *s, doublereal *delta, doublereal *
	deltan, doublereal *deltas, doublereal *t, doublereal *f, doublereal *
	fn, doublereal *fs, doublereal *fjacb, integer *msgb, doublereal *
	fjacd, integer *msgd, doublereal *ssf, doublereal *ss, doublereal *tt,
	 integer *ldtt, doublereal *stpb, doublereal *stpd, integer *ldstpd, 
	doublereal *xplusd, doublereal *wrk, integer *lwrk, doublereal *work, 
	integer *lwork, integer *iwork, integer *liwork, integer *info)
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal p0001 = 1e-4;
    static doublereal p1 = .1;
    static doublereal p25 = .25;
    static doublereal p5 = .5;
    static doublereal p75 = .75;
    static doublereal one = 1.;
    static integer ludflt = 6;

    /* System generated locals */
    integer delta_dim1, delta_offset, deltan_dim1, deltan_offset, deltas_dim1,
	     deltas_offset, f_dim1, f_offset, fjacb_dim1, fjacb_dim2, 
	    fjacb_offset, fjacd_dim1, fjacd_dim2, fjacd_offset, fn_dim1, 
	    fn_offset, fs_dim1, fs_offset, stpd_dim1, stpd_offset, t_dim1, 
	    t_offset, tt_dim1, tt_offset, wd_dim1, wd_dim2, wd_offset, 
	    we_dim1, we_dim2, we_offset, we1_dim1, we1_dim2, we1_offset, 
	    x_dim1, x_offset, xplusd_dim1, xplusd_offset, y_dim1, y_offset, 
	    ifixx_dim1, ifixx_offset, i__1, i__2;
    doublereal d__1, d__2;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static integer i__, j, l, u, sd, idf;
    static doublereal eta, tau;
    static integer ipr, npp, vcv, npr;
    static doublereal rss, wss[3];
    static integer ipr1, int2, ipr2, ipr3, wrk1, wrk2, wrk3, wrk4, wrk5, wrk6,
	     neta, nfev;
    extern doublereal ddot_(integer *, doublereal *, integer *, doublereal *, 
	    integer *);
    static integer njev;
    static doublereal temp;
    static integer nlms;
    static doublereal rvar;
    static integer iwrk, lunr, jpvt;
    extern /* Subroutine */ int dxmy_(integer *, integer *, doublereal *, 
	    integer *, doublereal *, integer *, doublereal *, integer *), 
	    dxpy_(integer *, integer *, doublereal *, integer *, doublereal *,
	     integer *, doublereal *, integer *);
    static integer nnzw, ipr2f;
    extern doublereal dnrm2_(integer *, doublereal *, integer *);
    static doublereal temp1, temp2;
    static logical cdjac;
    static integer iflag;
    static doublereal alpha;
    static integer omega;
    extern /* Subroutine */ int dodlm_(integer *, integer *, integer *, 
	    integer *, integer *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, integer *, integer *, doublereal *, doublereal *, 
	    integer *, doublereal *, doublereal *, doublereal *, doublereal *,
	     logical *, doublereal *, doublereal *, doublereal *, doublereal *
	    , integer *, doublereal *, doublereal *, integer *, doublereal *, 
	    integer *, doublereal *, doublereal *, doublereal *, doublereal *,
	     doublereal *, doublereal *, integer *, integer *);
    static logical redoj;
    static integer irank;
    static doublereal rcond;
    static logical initd;
    static doublereal actrs;
    extern /* Subroutine */ int dwght_(integer *, integer *, doublereal *, 
	    integer *, integer *, doublereal *, integer *, doublereal *, 
	    integer *);
    static doublereal ratio;
    extern /* Subroutine */ int dcopy_(integer *, doublereal *, integer *, 
	    doublereal *, integer *);
    static logical isodr;
    static integer niter, maxit;
    static logical dovcv, lstep;
    static doublereal pnorm, prers, rnorm;
    static integer istop, qraux;
    static logical cnvss;
    static doublereal sstol;
    static logical anajac;
    extern /* Subroutine */ int dacces_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, doublereal *, integer *, integer 
	    *, integer *, logical *, logical *, integer *, integer *, integer 
	    *, integer *, integer *, integer *, integer *, integer *, integer 
	    *, integer *, integer *, integer *, integer *, integer *, integer 
	    *, doublereal *, doublereal *, integer *, doublereal *, 
	    doublereal *, integer *, integer *, integer *, integer *, integer 
	    *, integer *, doublereal *, doublereal *, integer *, doublereal *,
	     doublereal *, integer *, integer *, integer *, integer *, 
	    doublereal *, doublereal *, integer *, doublereal *, doublereal *,
	     doublereal *, doublereal *, integer *);
    static logical chkjac;
    extern /* Subroutine */ int devjac_(S_fp, logical *, logical *, integer *,
	     integer *, integer *, integer *, doublereal *, doublereal *, 
	    doublereal *, integer *, integer *, integer *, doublereal *, 
	    integer *, doublereal *, doublereal *, doublereal *, integer *, 
	    doublereal *, doublereal *, integer *, integer *, doublereal *, 
	    doublereal *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, doublereal *, logical *, doublereal *, doublereal *,
	     integer *, integer *, integer *, integer *, integer *, integer *)
	    , dflags_(integer *, logical *, logical *, logical *, logical *, 
	    logical *, logical *, logical *, logical *, logical *);
    static logical access;
    static doublereal actred, taufac, dirder;
    extern /* Subroutine */ int dunpac_(integer *, doublereal *, doublereal *,
	     integer *), dodpcr_(integer *, integer *, logical *, logical *, 
	    logical *, logical *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, doublereal 
	    *, doublereal *, integer *, doublereal *, integer *, doublereal *,
	     doublereal *, integer *, integer *, doublereal *, integer *, 
	    integer *, integer *, integer *, integer *, doublereal *, 
	    doublereal *, integer *, doublereal *, doublereal *, integer *, 
	    integer *, integer *, doublereal *, doublereal *, doublereal *, 
	    integer *, doublereal *, doublereal *, integer *, doublereal *, 
	    integer *, integer *, integer *, doublereal *, doublereal *, 
	    doublereal *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, integer *, integer *, integer *);
    static logical intdbl, didvcv;
    static doublereal prered;
    static integer looped;
    static doublereal olmavg;
    extern /* Subroutine */ int dodvcv_(integer *, integer *, integer *, 
	    integer *, integer *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, integer *, integer *, doublereal *, doublereal *, 
	    doublereal *, integer *, doublereal *, doublereal *, logical *, 
	    doublereal *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, doublereal *, integer *, doublereal *, doublereal *,
	     integer *, doublereal *, doublereal *, integer *, doublereal *, 
	    integer *, doublereal *, doublereal *, doublereal *, doublereal *,
	     doublereal *, doublereal *, integer *, integer *);
    static logical implct, cnvpar;
    static doublereal partol;
    static integer istopc;
    static doublereal rnormn, rnorms, tsnorm;
    static logical restrt;
    static integer lunrpt;

/* ***BEGIN PROLOGUE  DODMN */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  FCN,DACCES,DCOPY,DDOT,DEVJAC,DFLAGS,DNRM2,DODLM, */
/*                    DODPCR,DODVCV,DUNPAC,DWGHT,DXMY,DXPY */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  ITERATIVELY COMPUTE LEAST SQUARES SOLUTION */
/* ***END PROLOGUE  DODMN */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...LOCAL ARRAYS */
/* ...EXTERNAL FUNCTIONS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    xplusd_dim1 = *n;
    xplusd_offset = 1 + xplusd_dim1;
    xplusd -= xplusd_offset;
    t_dim1 = *n;
    t_offset = 1 + t_dim1;
    t -= t_offset;
    deltas_dim1 = *n;
    deltas_offset = 1 + deltas_dim1;
    deltas -= deltas_offset;
    deltan_dim1 = *n;
    deltan_offset = 1 + deltan_dim1;
    deltan -= deltan_offset;
    delta_dim1 = *n;
    delta_offset = 1 + delta_dim1;
    delta -= delta_offset;
    --stpb;
    --ss;
    --ssf;
    --s;
    --betas;
    --betan;
    --betac;
    --ifixb;
    --beta;
    --msgd;
    fjacd_dim1 = *n;
    fjacd_dim2 = *m;
    fjacd_offset = 1 + fjacd_dim1 * (1 + fjacd_dim2);
    fjacd -= fjacd_offset;
    --msgb;
    fjacb_dim1 = *n;
    fjacb_dim2 = *np;
    fjacb_offset = 1 + fjacb_dim1 * (1 + fjacb_dim2);
    fjacb -= fjacb_offset;
    fs_dim1 = *n;
    fs_offset = 1 + fs_dim1;
    fs -= fs_offset;
    fn_dim1 = *n;
    fn_offset = 1 + fn_dim1;
    fn -= fn_offset;
    f_dim1 = *n;
    f_offset = 1 + f_dim1;
    f -= f_offset;
    y_dim1 = *ldy;
    y_offset = 1 + y_dim1;
    y -= y_offset;
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    we1_dim1 = *ldwe;
    we1_dim2 = *ld2we;
    we1_offset = 1 + we1_dim1 * (1 + we1_dim2);
    we1 -= we1_offset;
    we_dim1 = *ldwe;
    we_dim2 = *ld2we;
    we_offset = 1 + we_dim1 * (1 + we_dim2);
    we -= we_offset;
    wd_dim1 = *ldwd;
    wd_dim2 = *ld2wd;
    wd_offset = 1 + wd_dim1 * (1 + wd_dim2);
    wd -= wd_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;
    tt_dim1 = *ldtt;
    tt_offset = 1 + tt_dim1;
    tt -= tt_offset;
    stpd_dim1 = *ldstpd;
    stpd_offset = 1 + stpd_dim1;
    stpd -= stpd_offset;
    --wrk;
    --work;
    --iwork;

    /* Function Body */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:     THE USER SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ACCESS:  THE VARIABLE DESIGNATING WHETHER INFORMATION IS TO BE */
/*            ACCESSED FROM THE WORK ARRAYS (ACCESS=TRUE) OR STORED IN */
/*            THEM (ACCESS=FALSE). */
/*   ACTRED:  THE ACTUAL RELATIVE REDUCTION IN THE SUM-OF-SQUARES. */
/*   ACTRS:   THE SAVED ACTUAL RELATIVE REDUCTION IN THE SUM-OF-SQUARES. */
/*   ALPHA:   THE LEVENBERG-MARQUARDT PARAMETER. */
/*   ANAJAC:  THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE COMPUTED */
/*            BY FINITE DIFFERENCES (ANAJAC=FALSE) OR NOT (ANAJAC=TRUE). */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   BETAC:   THE CURRENT ESTIMATED VALUES OF THE UNFIXED BETA'S. */
/*   BETAN:   THE NEW ESTIMATED VALUES OF THE UNFIXED BETA'S. */
/*   BETAS:   THE SAVED ESTIMATED VALUES OF THE UNFIXED BETA'S. */
/*   CDJAC:   THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE COMPUTED */
/*            BY CENTRAL DIFFERENCES (CDJAC=TRUE) OR BY FORWARD */
/*            DIFFERENCES (CDJAC=FALSE). */
/*   CHKJAC:  THE VARIABLE DESIGNATING WHETHER THE USER SUPPLIED */
/*            JACOBIANS ARE TO BE CHECKED (CHKJAC=TRUE) OR NOT */
/*            (CHKJAC=FALSE). */
/*   CNVPAR:  THE VARIABLE DESIGNATING WHETHER PARAMETER CONVERGENCE WAS */
/*            ATTAINED (CNVPAR=TRUE) OR NOT (CNVPAR=FALSE). */
/*   CNVSS:   THE VARIABLE DESIGNATING WHETHER SUM-OF-SQUARES CONVERGENCE */
/*            WAS ATTAINED (CNVSS=TRUE) OR NOT (CNVSS=FALSE). */
/*   DELTA:   THE ESTIMATED ERRORS IN THE EXPLANATORY VARIABLES. */
/*   DELTAN:  THE NEW ESTIMATED ERRORS IN THE EXPLANATORY VARIABLES. */
/*   DELTAS:  THE SAVED ESTIMATED ERRORS IN THE EXPLANATORY VARIABLES. */
/*   DIDVCV:  THE VARIABLE DESIGNATING WHETHER THE COVARIANCE MATRIX WAS */
/*            COMPUTED (DIDVCV=TRUE) OR NOT (DIDVCV=FALSE). */
/*   DIRDER:  THE DIRECTIONAL DERIVATIVE. */
/*   DOVCV:   THE VARIABLE DESIGNATING WHETHER THE COVARIANCE MATRIX */
/*            SHOULD TO BE COMPUTED (DOVCV=TRUE) OR NOT (DOVCV=FALSE). */
/*   ETA:     THE RELATIVE NOISE IN THE FUNCTION RESULTS. */
/*   F:       THE (WEIGHTED) ESTIMATED VALUES OF EPSILON. */
/*   FJACB:   THE JACOBIAN WITH RESPECT TO BETA. */
/*   FJACD:   THE JACOBIAN WITH RESPECT TO DELTA. */
/*   FN:      THE NEW PREDICTED VALUES FROM THE FUNCTION. */
/*   FS:      THE SAVED PREDICTED VALUES FROM THE FUNCTION. */
/*   FSTITR:  THE VARIABLE DESIGNATING WHETHER THIS IS THE FIRST */
/*            ITERATION (FSTITR=TRUE) OR NOT (FSTITR=FALSE). */
/*   HEAD:    THE VARIABLE DESIGNATING WHETHER THE HEADING IS TO BE */
/*            PRINTED (HEAD=TRUE) OR NOT (HEAD=FALSE). */
/*   I:       AN INDEXING VARIABLE. */
/*   IDF:     THE DEGREES OF FREEDOM OF THE FIT, EQUAL TO THE NUMBER OF */
/*            OBSERVATIONS WITH NONZERO WEIGHTED DERIVATIVES MINUS THE */
/*            NUMBER OF PARAMETERS BEING ESTIMATED. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFLAG:   THE VARIABLE DESIGNATING WHICH REPORT IS TO BE PRINTED. */
/*   IMPLCT:  THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY */
/*            IMPLICIT ODR (IMPLCT=TRUE) OR EXPLICIT ODR (IMPLCT=FALSE). */
/*   INFO:    THE VARIABLE DESIGNATING WHY THE COMPUTATIONS WERE STOPPED. */
/*   INITD:   THE VARIABLE DESIGNATING WHETHER DELTA IS INITIALIZED TO */
/*            ZERO (INITD=TRUE) OR TO THE VALUES IN THE FIRST N BY M */
/*            ELEMENTS OF ARRAY WORK (INITD=FALSE). */
/*   INT2:    THE NUMBER OF INTERNAL DOUBLING STEPS TAKEN. */
/*   INTDBL:  THE VARIABLE DESIGNATING WHETHER INTERNAL DOUBLING IS TO BE */
/*            USED (INTDBL=TRUE) OR NOT (INTDBL=FALSE). */
/*   IPR:     THE VALUES DESIGNATING THE LENGTH OF THE PRINTED REPORT. */
/*   IPR1:    THE VALUE OF THE 4TH DIGIT (FROM THE RIGHT) OF IPRINT, */
/*            WHICH CONTROLS THE INITIAL SUMMARY REPORT. */
/*   IPR2:    THE VALUE OF THE 3RD DIGIT (FROM THE RIGHT) OF IPRINT, */
/*            WHICH CONTROLS THE ITERATION REPORT. */
/*   IPR2F:   THE VALUE OF THE 2ND DIGIT (FROM THE RIGHT) OF IPRINT, */
/*            WHICH CONTROLS THE FREQUENCY OF THE ITERATION REPORTS. */
/*   IPR3:    THE VALUE OF THE 1ST DIGIT (FROM THE RIGHT) OF IPRINT, */
/*            WHICH CONTROLS THE FINAL SUMMARY REPORT. */
/*   IRANK:   THE RANK DEFICIENCY OF THE JACOBIAN WRT BETA. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR OLS (ISODR=FALSE). */
/*   ISTOP:   THE VARIABLE DESIGNATING WHETHER THERE ARE PROBLEMS */
/*            COMPUTING THE FUNCTION AT THE CURRENT BETA AND DELTA. */
/*   ISTOPC:  THE VARIABLE DESIGNATING WHETHER THE COMPUTATIONS WERE */
/*            STOPED DUE TO SOME NUMERICAL ERROR WITHIN ROUTINE DODSTP. */
/*   IWORK:   THE INTEGER WORK SPACE. */
/*   IWRK:    AN INDEX VARIABLE. */
/*   J:       AN INDEX VARIABLE. */
/*   JOB:     THE VARIABLE CONTROLING PROBLEM INITIALIZATION AND */
/*            COMPUTATIONAL METHOD. */
/*   JPVT:    THE STARTING LOCATION IN IWORK OF ARRAY JPVT. */
/*   L:       AN INDEX VARIABLE. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LDTT:    THE LEADING DIMENSION OF ARRAY TT. */
/*   LDWD:    THE LEADING DIMENSION OF ARRAY WD. */
/*   LDWE:    THE LEADING DIMENSION OF ARRAY WE AND WE1. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   LDY:     THE LEADING DIMENSION OF ARRAY Y. */
/*   LD2WD:   THE SECOND DIMENSION OF ARRAY WD. */
/*   LD2WE:   THE SECOND DIMENSION OF ARRAY WE AND WE1. */
/*   LIWORK:  THE LENGTH OF VECTOR IWORK. */
/*   LOOPED:  A COUNTER USED TO DETERMINE HOW MANY TIMES THE SUBLOOP */
/*            HAS BEEN EXECUTED, WHERE IF THE COUNT BECOMES LARGE */
/*            ENOUGH THE COMPUTATIONS WILL BE STOPPED. */
/*   LSTEP:   THE VARIABLE DESIGNATING WHETHER A SUCCESSFUL STEP HAS */
/*            BEEN FOUND (LSTEP=TRUE) OR NOT (LSTEP=FALSE). */
/*   LUDFLT:  THE DEFAULT LOGICAL UNIT NUMBER, USED FOR COMPUTATION */
/*            REPORTS TO THE SCREEN. */
/*   LUNR:    THE LOGICAL UNIT NUMBER USED FOR COMPUTATION REPORTS. */
/*   LUNRPT:  THE LOGICAL UNIT NUMBER USED FOR COMPUTATION REPORTS. */
/*   LWORK:   THE LENGTH OF VECTOR WORK. */
/*   LWRK:    THE LENGTH OF VECTOR WRK. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   MAXIT:   THE MAXIMUM NUMBER OF ITERATIONS ALLOWED. */
/*   MSGB:    THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT BETA. */
/*   MSGD:    THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT DELTA. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NETA:    THE NUMBER OF ACCURATE DIGITS IN THE FUNCTION RESULTS. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NITER:   THE NUMBER OF ITERATIONS TAKEN. */
/*   NJEV:    THE NUMBER OF JACOBIAN EVALUATIONS. */
/*   NLMS:    THE NUMBER OF LEVENBERG-MARQUARDT STEPS TAKEN. */
/*   NNZW:    THE NUMBER OF NONZERO WEIGHTED OBSERVATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NPP:     THE NUMBER OF FUNCTION PARAMETERS BEING ESTIMATED. */
/*   NPR:     THE NUMBER OF TIMES THE REPORT IS TO BE WRITTEN. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   OLMAVG:  THE AVERAGE NUMBER OF LEVENBERG-MARQUARDT STEPS PER */
/*            ITERATION. */
/*   OMEGA:   THE STARTING LOCATION IN WORK OF ARRAY OMEGA. */
/*   ONE:     THE VALUE 1.0D0. */
/*   P0001:   THE VALUE 0.0001D0. */
/*   P1:      THE VALUE 0.1D0. */
/*   P25:     THE VALUE 0.25D0. */
/*   P5:      THE VALUE 0.5D0. */
/*   P75:     THE VALUE 0.75D0. */
/*   PARTOL:  THE PARAMETER CONVERGENCE STOPPING TOLERANCE. */
/*   PNORM:   THE NORM OF THE SCALED ESTIMATED PARAMETERS. */
/*   PRERED:  THE PREDICTED RELATIVE REDUCTION IN THE SUM-OF-SQUARES. */
/*   PRERS:   THE OLD PREDICTED RELATIVE REDUCTION IN THE SUM-OF-SQUARES. */
/*   PRTPEN:  THE VALUE DESIGNATING WHETHER THE PENALTY PARAMETER IS TO */
/*            BE PRINTED IN THE ITERATION REPORT (PRTPEN=TRUE) OR NOT */
/*            (PRTPEN=FALSE). */
/*   QRAUX:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY QRAUX. */
/*   RATIO:   THE RATIO OF THE ACTUAL RELATIVE REDUCTION TO THE PREDICTED */
/*            RELATIVE REDUCTION IN THE SUM-OF-SQUARES. */
/*   RCOND:   THE APPROXIMATE RECIPROCAL CONDITION OF FJACB. */
/*   REDOJ:   THE VARIABLE DESIGNATING WHETHER THE JACOBIAN MATRIX IS TO */
/*            BE RECOMPUTED FOR THE COMPUTATION OF THE COVARIANCE MATRIX */
/*            (REDOJ=TRUE) OR NOT (REDOJ=FALSE). */
/*   RESTRT:  THE VARIABLE DESIGNATING WHETHER THE CALL IS A RESTART */
/*            (RESTRT=TRUE) OR NOT (RESTRT=FALSE). */
/*   RNORM:   THE NORM OF THE WEIGHTED ERRORS. */
/*   RNORMN:  THE NEW NORM OF THE WEIGHTED ERRORS. */
/*   RNORMS:  THE SAVED NORM OF THE WEIGHTED ERRORS. */
/*   RSS:     THE RESIDUAL SUM OF SQUARES. */
/*   RVAR:    THE RESIDUAL VARIANCE. */
/*   S:       THE STEP FOR BETA. */
/*   SD:      THE STARTING LOCATION IN ARRAY WORK OF ARRAY SD. */
/*   SS:      THE SCALING VALUES USED FOR THE UNFIXED BETAS. */
/*   SSF:     THE SCALING VALUES USED FOR BETA. */
/*   SSTOL:   THE SUM-OF-SQUARES CONVERGENCE STOPPING TOLERANCE. */
/*   STPB:    THE RELATIVE STEP USED FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO EACH BETA. */
/*   STPD:    THE RELATIVE STEP USED FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO DELTA. */
/*   T:       THE STEP FOR DELTA. */
/*   TAU:     THE TRUST REGION DIAMETER. */
/*   TAUFAC:  THE FACTOR USED TO COMPUTE THE INITIAL TRUST REGION */
/*            DIAMETER. */
/*   TEMP:    A TEMPORARY STORAGE LOCATION. */
/*   TEMP1:   A TEMPORARY STORAGE LOCATION. */
/*   TEMP2:   A TEMPORARY STORAGE LOCATION. */
/*   TSNORM:  THE NORM OF THE SCALED STEP. */
/*   TT:      THE SCALING VALUES USED FOR DELTA. */
/*   U:       THE STARTING LOCATION IN ARRAY WORK OF ARRAY U. */
/*   VCV:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY VCV. */
/*   WE:      THE EPSILON WEIGHTS. */
/*   WE1:     THE SQUARE ROOT OF THE EPSILON WEIGHTS. */
/*   WD:      THE DELTA WEIGHTS. */
/*   WORK:    THE DOUBLE PRECISION WORK SPACE. */
/*   WSS:     THE SUM-OF-SQUARES OF THE WEIGHTED EPSILONS AND DELTAS, */
/*            THE SUM-OF-SQUARES OF THE WEIGHTED DELTAS, AND */
/*            THE SUM-OF-SQUARES OF THE WEIGHTED EPSILONS. */
/*   WRK:     A WORK ARRAY, EQUIVALENCED TO WRK1 AND WRK2 */
/*   WRK1:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK1. */
/*   WRK2:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK2. */
/*   WRK3:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK3. */
/*   WRK4:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK4. */
/*   WRK5:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK5. */
/*   WRK6:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK6. */
/*   X:       THE EXPLANATORY VARIABLE. */
/*   XPLUSD:  THE VALUES OF X + DELTA. */
/*   Y:       THE DEPENDENT VARIABLE.  UNUSED WHEN THE MODEL IS IMPLICIT. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DODMN */
/*  INITIALIZE NECESSARY VARIABLES */
    dflags_(job, &restrt, &initd, &dovcv, &redoj, &anajac, &cdjac, &chkjac, &
	    isodr, &implct);
    access = TRUE_;
    dacces_(n, m, np, nq, ldwe, ld2we, &work[1], lwork, &iwork[1], liwork, &
	    access, &isodr, &jpvt, &omega, &u, &qraux, &sd, &vcv, &wrk1, &
	    wrk2, &wrk3, &wrk4, &wrk5, &wrk6, &nnzw, &npp, job, &partol, &
	    sstol, &maxit, &taufac, &eta, &neta, &lunrpt, &ipr1, &ipr2, &
	    ipr2f, &ipr3, wss, &rvar, &idf, &tau, &alpha, &niter, &nfev, &
	    njev, &int2, &olmavg, &rcond, &irank, &actrs, &pnorm, &prers, &
	    rnorms, &istop);
    rnorm = sqrt(wss[0]);
    didvcv = FALSE_;
    intdbl = FALSE_;
    lstep = TRUE_;
/*  PRINT INITIAL SUMMARY IF DESIRED */
    if (ipr1 != 0 && lunrpt != 0) {
	iflag = 1;
	if (ipr1 >= 3 && lunrpt != ludflt) {
	    npr = 2;
	} else {
	    npr = 1;
	}
	if (ipr1 >= 6) {
	    ipr = 2;
	} else {
	    ipr = 2 - ipr1 % 2;
	}
	lunr = lunrpt;
	i__1 = npr;
	for (i__ = 1; i__ <= i__1; ++i__) {
	    dodpcr_(&ipr, &lunr, head, prtpen, fstitr, &didvcv, &iflag, n, m, 
		    np, nq, &npp, &nnzw, &msgb[1], &msgd[1], &beta[1], &y[
		    y_offset], ldy, &x[x_offset], ldx, &delta[delta_offset], &
		    we[we_offset], ldwe, ld2we, &wd[wd_offset], ldwd, ld2wd, &
		    ifixb[1], &ifixx[ifixx_offset], ldifx, &ssf[1], &tt[
		    tt_offset], ldtt, &stpb[1], &stpd[stpd_offset], ldstpd, 
		    job, &neta, &taufac, &sstol, &partol, &maxit, wss, &rvar, 
		    &idf, &work[sd], &niter, &nfev, &njev, &actred, &prered, &
		    tau, &pnorm, &alpha, &f[f_offset], &rcond, &irank, info, &
		    istop);
	    if (ipr1 >= 5) {
		ipr = 2;
	    } else {
		ipr = 1;
	    }
	    lunr = ludflt;
/* L10: */
	}
    }
/*  STOP IF INITIAL ESTIMATES ARE EXACT SOLUTION */
    if (rnorm == zero) {
	*info = 1;
	olmavg = zero;
	istop = 0;
	goto L150;
    }
/*  STOP IF NUMBER OF ITERATIONS ALREADY EQUALS MAXIMUM PERMITTED */
    if (restrt && niter >= maxit) {
	istop = 0;
	goto L150;
    } else if (niter >= maxit) {
	*info = 4;
	istop = 0;
	goto L150;
    }
/*  MAIN LOOP */
L100:
    ++niter;
    rnorms = rnorm;
    looped = 0;
/*  EVALUATE JACOBIAN USING BEST ESTIMATE OF FUNCTION (FS) */
    if (niter == 1 && (anajac && chkjac)) {
	istop = 0;
    } else {
	devjac_((S_fp)fcn, &anajac, &cdjac, n, m, np, nq, &betac[1], &beta[1],
		 &stpb[1], &ifixb[1], &ifixx[ifixx_offset], ldifx, &x[
		x_offset], ldx, &delta[delta_offset], &xplusd[xplusd_offset], 
		&stpd[stpd_offset], ldstpd, &ssf[1], &tt[tt_offset], ldtt, &
		neta, &fs[fs_offset], &t[t_offset], &work[wrk1], &work[wrk2], 
		&work[wrk3], &work[wrk6], &fjacb[fjacb_offset], &isodr, &
		fjacd[fjacd_offset], &we1[we1_offset], ldwe, ld2we, &njev, &
		nfev, &istop, info);
    }
    if (istop != 0) {
	*info = 51000;
	goto L200;
    } else if (*info == 50300) {
	goto L200;
    }
/*  SUB LOOP FOR */
/*     INTERNAL DOUBLING OR */
/*     COMPUTING NEW STEP WHEN OLD FAILED */
L110:
/*  COMPUTE STEPS S AND T */
    if (looped > 100) {
	*info = 60000;
	goto L200;
    } else {
	++looped;
	dodlm_(n, m, np, nq, &npp, &f[f_offset], &fjacb[fjacb_offset], &fjacd[
		fjacd_offset], &wd[wd_offset], ldwd, ld2wd, &ss[1], &tt[
		tt_offset], ldtt, &delta[delta_offset], &alpha, &tau, &eta, &
		isodr, &work[wrk6], &work[omega], &work[u], &work[qraux], &
		iwork[jpvt], &s[1], &t[t_offset], &nlms, &rcond, &irank, &
		work[wrk1], &work[wrk2], &work[wrk3], &work[wrk4], &work[wrk5]
		, &wrk[1], lwrk, &istopc);
    }
    if (istopc != 0) {
	*info = istopc;
	goto L200;
    }
    olmavg += nlms;
/*  COMPUTE BETAN = BETAC + S */
/*          DELTAN = DELTA + T */
    dxpy_(&npp, &c__1, &betac[1], &npp, &s[1], &npp, &betan[1], &npp);
    if (isodr) {
	dxpy_(n, m, &delta[delta_offset], n, &t[t_offset], n, &deltan[
		deltan_offset], n);
    }
/*  COMPUTE NORM OF SCALED STEPS S AND T (TSNORM) */
    dwght_(&npp, &c__1, &ss[1], &npp, &c__1, &s[1], &npp, &wrk[1], &npp);
    if (isodr) {
	dwght_(n, m, &tt[tt_offset], ldtt, &c__1, &t[t_offset], n, &wrk[npp + 
		1], n);
	i__1 = npp + *n * *m;
	tsnorm = dnrm2_(&i__1, &wrk[1], &c__1);
    } else {
	tsnorm = dnrm2_(&npp, &wrk[1], &c__1);
    }
/*  COMPUTE SCALED PREDICTED REDUCTION */
    iwrk = 0;
    i__1 = *nq;
    for (l = 1; l <= i__1; ++l) {
	i__2 = *n;
	for (i__ = 1; i__ <= i__2; ++i__) {
	    ++iwrk;
	    wrk[iwrk] = ddot_(&npp, &fjacb[i__ + (l * fjacb_dim2 + 1) * 
		    fjacb_dim1], n, &s[1], &c__1);
	    if (isodr) {
		wrk[iwrk] += ddot_(m, &fjacd[i__ + (l * fjacd_dim2 + 1) * 
			fjacd_dim1], n, &t[i__ + t_dim1], n);
	    }
/* L120: */
	}
/* L130: */
    }
    if (isodr) {
	dwght_(n, m, &wd[wd_offset], ldwd, ld2wd, &t[t_offset], n, &wrk[*n * *
		nq + 1], n);
	i__1 = *n * *nq;
	i__2 = *n * *m;
	temp1 = ddot_(&i__1, &wrk[1], &c__1, &wrk[1], &c__1) + ddot_(&i__2, &
		t[t_offset], &c__1, &wrk[*n * *nq + 1], &c__1);
	temp1 = sqrt(temp1) / rnorm;
    } else {
	i__1 = *n * *nq;
	temp1 = dnrm2_(&i__1, &wrk[1], &c__1) / rnorm;
    }
    temp2 = sqrt(alpha) * tsnorm / rnorm;
/* Computing 2nd power */
    d__1 = temp1;
/* Computing 2nd power */
    d__2 = temp2;
    prered = d__1 * d__1 + d__2 * d__2 / p5;
/* Computing 2nd power */
    d__1 = temp1;
/* Computing 2nd power */
    d__2 = temp2;
    dirder = -(d__1 * d__1 + d__2 * d__2);
/*  EVALUATE PREDICTED VALUES AT NEW POINT */
    dunpac_(np, &betan[1], &beta[1], &ifixb[1]);
    dxpy_(n, m, &x[x_offset], ldx, &deltan[deltan_offset], n, &xplusd[
	    xplusd_offset], n);
    istop = 0;
    (*fcn)(n, m, np, nq, n, m, np, &beta[1], &xplusd[xplusd_offset], &ifixb[1]
	    , &ifixx[ifixx_offset], ldifx, &c__2, &fn[fn_offset], &work[wrk6],
	     &work[wrk1], &istop);
    if (istop == 0) {
	++nfev;
    }
    if (istop < 0) {
/*  SET INFO TO INDICATE USER HAS STOPPED THE COMPUTATIONS IN FCN */
	*info = 51000;
	goto L200;
    } else if (istop > 0) {
/*  SET NORM TO INDICATE STEP SHOULD BE REJECTED */
	rnormn = rnorm / (p1 * p75);
    } else {
/*  COMPUTE NORM OF NEW WEIGHTED EPSILONS AND WEIGHTED DELTAS (RNORMN) */
	if (implct) {
	    i__1 = *n * *nq;
	    dcopy_(&i__1, &fn[fn_offset], &c__1, &wrk[1], &c__1);
	} else {
	    dxmy_(n, nq, &fn[fn_offset], n, &y[y_offset], ldy, &wrk[1], n);
	}
	dwght_(n, nq, &we1[we1_offset], ldwe, ld2we, &wrk[1], n, &wrk[1], n);
	if (isodr) {
	    dwght_(n, m, &wd[wd_offset], ldwd, ld2wd, &deltan[deltan_offset], 
		    n, &wrk[*n * *nq + 1], n);
	    i__1 = *n * *nq;
	    i__2 = *n * *m;
	    rnormn = sqrt(ddot_(&i__1, &wrk[1], &c__1, &wrk[1], &c__1) + 
		    ddot_(&i__2, &deltan[deltan_offset], &c__1, &wrk[*n * *nq 
		    + 1], &c__1));
	} else {
	    i__1 = *n * *nq;
	    rnormn = dnrm2_(&i__1, &wrk[1], &c__1);
	}
    }
/*  COMPUTE SCALED ACTUAL REDUCTION */
    if (p1 * rnormn < rnorm) {
/* Computing 2nd power */
	d__1 = rnormn / rnorm;
	actred = one - d__1 * d__1;
    } else {
	actred = -one;
    }
/*  COMPUTE RATIO OF ACTUAL REDUCTION TO PREDICTED REDUCTION */
    if (prered == zero) {
	ratio = zero;
    } else {
	ratio = actred / prered;
    }
/*  CHECK ON LACK OF REDUCTION IN INTERNAL DOUBLING CASE */
    if (intdbl && (ratio < p0001 || rnormn > rnorms)) {
	istop = 0;
	tau *= p5;
	alpha /= p5;
	dcopy_(&npp, &betas[1], &c__1, &betan[1], &c__1);
	i__1 = *n * *m;
	dcopy_(&i__1, &deltas[deltas_offset], &c__1, &deltan[deltan_offset], &
		c__1);
	i__1 = *n * *nq;
	dcopy_(&i__1, &fs[fs_offset], &c__1, &fn[fn_offset], &c__1);
	actred = actrs;
	prered = prers;
	rnormn = rnorms;
	ratio = p5;
    }
/*  UPDATE STEP BOUND */
    intdbl = FALSE_;
    if (ratio < p25) {
	if (actred >= zero) {
	    temp = p5;
	} else {
	    temp = p5 * dirder / (dirder + p5 * actred);
	}
	if (p1 * rnormn >= rnorm || temp < p1) {
	    temp = p1;
	}
/* Computing MIN */
	d__1 = tau, d__2 = tsnorm / p1;
	tau = temp * min(d__1,d__2);
	alpha /= temp;
    } else if (alpha == zero) {
	tau = tsnorm / p5;
    } else if (ratio >= p75 && nlms <= 11) {
/*  STEP QUALIFIES FOR INTERNAL DOUBLING */
/*     - UPDATE TAU AND ALPHA */
/*     - SAVE INFORMATION FOR CURRENT POINT */
	intdbl = TRUE_;
	tau = tsnorm / p5;
	alpha *= p5;
	dcopy_(&npp, &betan[1], &c__1, &betas[1], &c__1);
	i__1 = *n * *m;
	dcopy_(&i__1, &deltan[deltan_offset], &c__1, &deltas[deltas_offset], &
		c__1);
	i__1 = *n * *nq;
	dcopy_(&i__1, &fn[fn_offset], &c__1, &fs[fs_offset], &c__1);
	actrs = actred;
	prers = prered;
	rnorms = rnormn;
    }
/*  IF INTERNAL DOUBLING, SKIP CONVERGENCE CHECKS */
    if (intdbl && tau > zero) {
	++int2;
	goto L110;
    }
/*  CHECK ACCEPTANCE */
    if (ratio >= p0001) {
	i__1 = *n * *nq;
	dcopy_(&i__1, &fn[fn_offset], &c__1, &fs[fs_offset], &c__1);
	if (implct) {
	    i__1 = *n * *nq;
	    dcopy_(&i__1, &fs[fs_offset], &c__1, &f[f_offset], &c__1);
	} else {
	    dxmy_(n, nq, &fs[fs_offset], n, &y[y_offset], ldy, &f[f_offset], 
		    n);
	}
	dwght_(n, nq, &we1[we1_offset], ldwe, ld2we, &f[f_offset], n, &f[
		f_offset], n);
	dcopy_(&npp, &betan[1], &c__1, &betac[1], &c__1);
	i__1 = *n * *m;
	dcopy_(&i__1, &deltan[deltan_offset], &c__1, &delta[delta_offset], &
		c__1);
	rnorm = rnormn;
	dwght_(&npp, &c__1, &ss[1], &npp, &c__1, &betac[1], &npp, &wrk[1], &
		npp);
	if (isodr) {
	    dwght_(n, m, &tt[tt_offset], ldtt, &c__1, &delta[delta_offset], n,
		     &wrk[npp + 1], n);
	    i__1 = npp + *n * *m;
	    pnorm = dnrm2_(&i__1, &wrk[1], &c__1);
	} else {
	    pnorm = dnrm2_(&npp, &wrk[1], &c__1);
	}
	lstep = TRUE_;
    } else {
	lstep = FALSE_;
    }
/*  TEST CONVERGENCE */
    *info = 0;
    cnvss = rnorm == zero || abs(actred) <= sstol && prered <= sstol && p5 * 
	    ratio <= one;
    cnvpar = tau <= partol * pnorm && ! implct;
    if (cnvss) {
	*info = 1;
    }
    if (cnvpar) {
	*info = 2;
    }
    if (cnvss && cnvpar) {
	*info = 3;
    }
/*  PRINT ITERATION REPORT */
    if (*info != 0 || lstep) {
	if (ipr2 != 0 && ipr2f != 0 && lunrpt != 0) {
	    if (ipr2f == 1 || niter % ipr2f == 1) {
		iflag = 2;
		dunpac_(np, &betac[1], &beta[1], &ifixb[1]);
		wss[0] = rnorm * rnorm;
		if (ipr2 >= 3 && lunrpt != ludflt) {
		    npr = 2;
		} else {
		    npr = 1;
		}
		if (ipr2 >= 6) {
		    ipr = 2;
		} else {
		    ipr = 2 - ipr2 % 2;
		}
		lunr = lunrpt;
		i__1 = npr;
		for (i__ = 1; i__ <= i__1; ++i__) {
		    dodpcr_(&ipr, &lunr, head, prtpen, fstitr, &didvcv, &
			    iflag, n, m, np, nq, &npp, &nnzw, &msgb[1], &msgd[
			    1], &beta[1], &y[y_offset], ldy, &x[x_offset], 
			    ldx, &delta[delta_offset], &we[we_offset], ldwe, 
			    ld2we, &wd[wd_offset], ldwd, ld2wd, &ifixb[1], &
			    ifixx[ifixx_offset], ldifx, &ssf[1], &tt[
			    tt_offset], ldtt, &stpb[1], &stpd[stpd_offset], 
			    ldstpd, job, &neta, &taufac, &sstol, &partol, &
			    maxit, wss, &rvar, &idf, &work[sd], &niter, &nfev,
			     &njev, &actred, &prered, &tau, &pnorm, &alpha, &
			    f[f_offset], &rcond, &irank, info, &istop);
		    if (ipr2 >= 5) {
			ipr = 2;
		    } else {
			ipr = 1;
		    }
		    lunr = ludflt;
/* L140: */
		}
		*fstitr = FALSE_;
		*prtpen = FALSE_;
	    }
	}
    }
/*  CHECK IF FINISHED */
    if (*info == 0) {
	if (lstep) {
/*  BEGIN NEXT INTERATION UNLESS A STOPPING CRITERIA HAS BEEN MET */
	    if (niter >= maxit) {
		*info = 4;
	    } else {
		goto L100;
	    }
	} else {
/*  STEP FAILED - RECOMPUTE UNLESS A STOPPING CRITERIA HAS BEEN MET */
	    goto L110;
	}
    }
L150:
    if (istop > 0) {
	*info += 100;
    }
/*  STORE UNWEIGHTED EPSILONS AND X+DELTA TO RETURN TO USER */
    if (implct) {
	i__1 = *n * *nq;
	dcopy_(&i__1, &fs[fs_offset], &c__1, &f[f_offset], &c__1);
    } else {
	dxmy_(n, nq, &fs[fs_offset], n, &y[y_offset], ldy, &f[f_offset], n);
    }
    dunpac_(np, &betac[1], &beta[1], &ifixb[1]);
    dxpy_(n, m, &x[x_offset], ldx, &delta[delta_offset], n, &xplusd[
	    xplusd_offset], n);
/*  COMPUTE COVARIANCE MATRIX OF ESTIMATED PARAMETERS */
/*  IN UPPER NP BY NP PORTION OF WORK(VCV) IF REQUESTED */
    if (dovcv && istop == 0) {
/*  RE-EVALUATE JACOBIAN AT FINAL SOLUTION, IF REQUESTED */
/*  OTHERWISE, JACOBIAN FROM BEGINNING OF LAST ITERATION WILL BE USED */
/*  TO COMPUTE COVARIANCE MATRIX */
	if (redoj) {
	    devjac_((S_fp)fcn, &anajac, &cdjac, n, m, np, nq, &betac[1], &
		    beta[1], &stpb[1], &ifixb[1], &ifixx[ifixx_offset], ldifx,
		     &x[x_offset], ldx, &delta[delta_offset], &xplusd[
		    xplusd_offset], &stpd[stpd_offset], ldstpd, &ssf[1], &tt[
		    tt_offset], ldtt, &neta, &fs[fs_offset], &t[t_offset], &
		    work[wrk1], &work[wrk2], &work[wrk3], &work[wrk6], &fjacb[
		    fjacb_offset], &isodr, &fjacd[fjacd_offset], &we1[
		    we1_offset], ldwe, ld2we, &njev, &nfev, &istop, info);
	    if (istop != 0) {
		*info = 51000;
		goto L200;
	    } else if (*info == 50300) {
		goto L200;
	    }
	}
	if (implct) {
	    dwght_(n, m, &wd[wd_offset], ldwd, ld2wd, &delta[delta_offset], n,
		     &wrk[*n * *nq + 1], n);
	    i__1 = *n * *m;
	    rss = ddot_(&i__1, &delta[delta_offset], &c__1, &wrk[*n * *nq + 1]
		    , &c__1);
	} else {
	    rss = rnorm * rnorm;
	}
	if (redoj || niter >= 1) {
	    dodvcv_(n, m, np, nq, &npp, &f[f_offset], &fjacb[fjacb_offset], &
		    fjacd[fjacd_offset], &wd[wd_offset], ldwd, ld2wd, &ssf[1],
		     &ss[1], &tt[tt_offset], ldtt, &delta[delta_offset], &eta,
		     &isodr, &work[vcv], &work[sd], &work[wrk6], &work[omega],
		     &work[u], &work[qraux], &iwork[jpvt], &s[1], &t[t_offset]
		    , &irank, &rcond, &rss, &idf, &rvar, &ifixb[1], &work[
		    wrk1], &work[wrk2], &work[wrk3], &work[wrk4], &work[wrk5],
		     &wrk[1], lwrk, &istopc);
	    if (istopc != 0) {
		*info = istopc;
		goto L200;
	    }
	    didvcv = TRUE_;
	}
    }
/*  SET JPVT TO INDICATE DROPPED, FIXED AND ESTIMATED PARAMETERS */
L200:
    i__1 = *np - 1;
    for (i__ = 0; i__ <= i__1; ++i__) {
	work[wrk3 + i__] = (doublereal) iwork[jpvt + i__];
	iwork[jpvt + i__] = -2;
/* L210: */
    }
    if (redoj || niter >= 1) {
	i__1 = npp - 1;
	for (i__ = 0; i__ <= i__1; ++i__) {
	    j = (integer) (work[wrk3 + i__] - 1);
	    if (i__ <= npp - irank - 1) {
		iwork[jpvt + j] = 1;
	    } else {
		iwork[jpvt + j] = -1;
	    }
/* L220: */
	}
	if (npp < *np) {
	    j = npp - 1;
	    for (i__ = *np - 1; i__ >= 0; --i__) {
		if (ifixb[i__ + 1] == 0) {
		    iwork[jpvt + i__] = 0;
		} else {
		    iwork[jpvt + i__] = iwork[jpvt + j];
		    --j;
		}
/* L230: */
	    }
	}
    }
/*  STORE VARIOUS SCALARS IN WORK ARRAYS FOR RETURN TO USER */
    if (niter >= 1) {
	olmavg /= niter;
    } else {
	olmavg = zero;
    }
/*  COMPUTE WEIGHTED SUMS OF SQUARES FOR RETURN TO USER */
    dwght_(n, nq, &we1[we1_offset], ldwe, ld2we, &f[f_offset], n, &wrk[1], n);
    i__1 = *n * *nq;
    wss[2] = ddot_(&i__1, &wrk[1], &c__1, &wrk[1], &c__1);
    if (isodr) {
	dwght_(n, m, &wd[wd_offset], ldwd, ld2wd, &delta[delta_offset], n, &
		wrk[*n * *nq + 1], n);
	i__1 = *n * *m;
	wss[1] = ddot_(&i__1, &delta[delta_offset], &c__1, &wrk[*n * *nq + 1],
		 &c__1);
    } else {
	wss[1] = zero;
    }
    wss[0] = wss[1] + wss[2];
    access = FALSE_;
    dacces_(n, m, np, nq, ldwe, ld2we, &work[1], lwork, &iwork[1], liwork, &
	    access, &isodr, &jpvt, &omega, &u, &qraux, &sd, &vcv, &wrk1, &
	    wrk2, &wrk3, &wrk4, &wrk5, &wrk6, &nnzw, &npp, job, &partol, &
	    sstol, &maxit, &taufac, &eta, &neta, &lunrpt, &ipr1, &ipr2, &
	    ipr2f, &ipr3, wss, &rvar, &idf, &tau, &alpha, &niter, &nfev, &
	    njev, &int2, &olmavg, &rcond, &irank, &actrs, &pnorm, &prers, &
	    rnorms, &istop);
/*  ENCODE EXISTANCE OF QUESTIONABLE RESULTS INTO INFO */
    if (*info <= 9 || *info >= 60000) {
	if (msgb[1] == 1 || msgd[1] == 1) {
	    *info += 1000;
	}
	if (istop != 0) {
	    *info += 100;
	}
	if (irank >= 1) {
	    if (npp > irank) {
		*info += 10;
	    } else {
		*info += 20;
	    }
	}
    }
/*  PRINT FINAL SUMMARY */
    if (ipr3 != 0 && lunrpt != 0) {
	iflag = 3;
	if (ipr3 >= 3 && lunrpt != ludflt) {
	    npr = 2;
	} else {
	    npr = 1;
	}
	if (ipr3 >= 6) {
	    ipr = 2;
	} else {
	    ipr = 2 - ipr3 % 2;
	}
	lunr = lunrpt;
	i__1 = npr;
	for (i__ = 1; i__ <= i__1; ++i__) {
	    dodpcr_(&ipr, &lunr, head, prtpen, fstitr, &didvcv, &iflag, n, m, 
		    np, nq, &npp, &nnzw, &msgb[1], &msgd[1], &beta[1], &y[
		    y_offset], ldy, &x[x_offset], ldx, &delta[delta_offset], &
		    we[we_offset], ldwe, ld2we, &wd[wd_offset], ldwd, ld2wd, &
		    iwork[jpvt], &ifixx[ifixx_offset], ldifx, &ssf[1], &tt[
		    tt_offset], ldtt, &stpb[1], &stpd[stpd_offset], ldstpd, 
		    job, &neta, &taufac, &sstol, &partol, &maxit, wss, &rvar, 
		    &idf, &work[sd], &niter, &nfev, &njev, &actred, &prered, &
		    tau, &pnorm, &alpha, &f[f_offset], &rcond, &irank, info, &
		    istop);
	    if (ipr3 >= 5) {
		ipr = 2;
	    } else {
		ipr = 1;
	    }
	    lunr = ludflt;
/* L240: */
	}
    }
    return 0;
} /* dodmn_ */

/* DODPC1 */
/* Subroutine */ int dodpc1_(integer *ipr, integer *lunrpt, logical *anajac, 
	logical *cdjac, logical *chkjac, logical *initd, logical *restrt, 
	logical *isodr, logical *implct, logical *dovcv, logical *redoj, 
	integer *msgb1, integer *msgb, integer *msgd1, integer *msgd, integer 
	*n, integer *m, integer *np, integer *nq, integer *npp, integer *nnzw,
	 doublereal *x, integer *ldx, integer *ifixx, integer *ldifx, 
	doublereal *delta, doublereal *wd, integer *ldwd, integer *ld2wd, 
	doublereal *tt, integer *ldtt, doublereal *stpd, integer *ldstpd, 
	doublereal *y, integer *ldy, doublereal *we, integer *ldwe, integer *
	ld2we, doublereal *pnlty, doublereal *beta, integer *ifixb, 
	doublereal *ssf, doublereal *stpb, integer *job, integer *neta, 
	doublereal *taufac, doublereal *sstol, doublereal *partol, integer *
	maxit, doublereal *wss, doublereal *wssdel, doublereal *wsseps)
{
    /* Initialized data */

    static doublereal zero = 0.;

    /* Format strings */
    static char fmt_1000[] = "(/\002 --- PROBLEM SIZE:\002/\002            N"
	    " = \002,i5,\002          (NUMBER WITH NONZERO WEIGHT = \002,i5"
	    ",\002)\002/\002           NQ = \002,i5/\002            M = \002,"
	    "i5/\002           NP = \002,i5,\002          (NUMBER UNFIXED ="
	    " \002,i5,\002)\002)";
    static char fmt_1100[] = "(/\002 --- CONTROL VALUES:\002/\002          J"
	    "OB = \002,i5.5/\002              = ABCDE, WHERE\002)";
    static char fmt_1110[] = "(\002                       A=\002,i1,\002 ==>"
	    " FIT IS A RESTART.\002)";
    static char fmt_1111[] = "(\002                       A=\002,i1,\002 ==>"
	    " FIT IS NOT A RESTART.\002)";
    static char fmt_1120[] = "(\002                       B=\002,i1,\002 ==>"
	    " DELTAS ARE INITIALIZED\002,\002 TO ZERO.\002)";
    static char fmt_1121[] = "(\002                       B=\002,i1,\002 ==>"
	    " DELTAS ARE INITIALIZED\002,\002 BY USER.\002)";
    static char fmt_1122[] = "(\002                       B=\002,i1,\002 ==>"
	    " DELTAS ARE FIXED AT\002,\002 ZERO SINCE E=\002,i1,\002.\002)";
    static char fmt_1130[] = "(\002                       C=\002,i1,\002 ==>"
	    " COVARIANCE MATRIX WILL\002,\002 BE COMPUTED USING\002)";
    static char fmt_1131[] = "(\002                               DERIVATIVE"
	    "S RE-\002,\002EVALUATED AT THE SOLUTION.\002)";
    static char fmt_1132[] = "(\002                               DERIVATIVE"
	    "S FROM THE\002,\002 LAST ITERATION.\002)";
    static char fmt_1133[] = "(\002                       C=\002,i1,\002 ==>"
	    " COVARIANCE MATRIX WILL\002,\002 NOT BE COMPUTED.\002)";
    static char fmt_1140[] = "(\002                       D=\002,i1,\002 ==>"
	    " DERIVATIVES ARE\002,\002 SUPPLIED BY USER.\002)";
    static char fmt_1141[] = "(\002                               DERIVATIVE"
	    "S WERE CHECKED.\002/\002                               RESULTS A"
	    "PPEAR QUESTIONABLE.\002)";
    static char fmt_1142[] = "(\002                               DERIVATIVE"
	    "S WERE CHECKED.\002/\002                               RESULTS A"
	    "PPEAR CORRECT.\002)";
    static char fmt_1143[] = "(\002                               DERIVATIVE"
	    "S WERE NOT\002,\002 CHECKED.\002)";
    static char fmt_1144[] = "(\002                       D=\002,i1,\002 ==>"
	    " DERIVATIVES ARE\002,\002 ESTIMATED BY CENTRAL\002,\002 DIFFEREN"
	    "CES.\002)";
    static char fmt_1145[] = "(\002                       D=\002,i1,\002 ==>"
	    " DERIVATIVES ARE\002,\002 ESTIMATED BY FORWARD\002,\002 DIFFEREN"
	    "CES.\002)";
    static char fmt_1150[] = "(\002                       E=\002,i1,\002 ==>"
	    " METHOD IS IMPLICIT ODR.\002)";
    static char fmt_1151[] = "(\002                       E=\002,i1,\002 ==>"
	    " METHOD IS EXPLICIT ODR.\002)";
    static char fmt_1152[] = "(\002                       E=\002,i1,\002 ==>"
	    " METHOD IS EXPLICIT OLS.\002)";
    static char fmt_1200[] = "(\002       NDIGIT = \002,i5,\002          (ES"
	    "TIMATED BY ODRPACK)\002)";
    static char fmt_1210[] = "(\002       NDIGIT = \002,i5,\002          (SU"
	    "PPLIED BY USER)\002)";
    static char fmt_1300[] = "(\002       TAUFAC = \002,1p,d12.2)";
    static char fmt_1400[] = "(/\002 --- STOPPING CRITERIA:\002/\002        "
	    "SSTOL = \002,1p,d12.2,\002   (SUM OF SQUARES STOPPING TOLERANCE"
	    ")\002/\002       PARTOL = \002,1p,d12.2,\002   (PARAMETER STOPPI"
	    "NG TOLERANCE)\002/\002        MAXIT = \002,i5,\002          (MAX"
	    "IMUM NUMBER OF ITERATIONS)\002)";
    static char fmt_1500[] = "(/\002 --- INITIAL SUM OF SQUARED WEIGHTED DEL"
	    "TAS =\002,17x,1p,d17.8)";
    static char fmt_1510[] = "(\002         INITIAL PENALTY FUNCTION VALUE  "
	    "   =\002,1p,d17.8/\002                 PENALTY TERM             "
	    "  =\002,1p,d17.8/\002                 PENALTY PARAMETER         "
	    " =\002,1p,d10.1)";
    static char fmt_1600[] = "(/\002 --- INITIAL WEIGHTED SUM OF SQUARES    "
	    "    =\002,17x,1p,d17.8)";
    static char fmt_1610[] = "(\002         SUM OF SQUARED WEIGHTED DELTAS  "
	    "   =\002,1p,d17.8/\002         SUM OF SQUARED WEIGHTED EPSILONS "
	    "  =\002,1p,d17.8)";
    static char fmt_4000[] = "(/\002 --- FUNCTION PARAMETER SUMMARY:\002)";
    static char fmt_4110[] = "(/\002       INDEX         BETA(K)    FIXED   "
	    "        SCALE\002,\002    DERIVATIVE\002/\002                   "
	    "                                  \002,\002    ASSESSMENT\002/"
	    ",\002         (K)                  (IFIXB)          (SCLB)\002"
	    ",\002              \002/)";
    static char fmt_4120[] = "(/\002       INDEX         BETA(K)    FIXED   "
	    "        SCALE\002,\002              \002/\002                   "
	    "                                  \002,\002              \002/"
	    ",\002         (K)                  (IFIXB)          (SCLB)\002"
	    ",\002              \002/)";
    static char fmt_4200[] = "(/\002       INDEX         BETA(K)    FIXED   "
	    "        SCALE\002,\002    DERIVATIVE\002/\002                   "
	    "                                  \002,\002     STEP SIZE\002/"
	    ",\002         (K)                  (IFIXB)          (SCLB)\002"
	    ",\002        (STPB)\002/)";
    static char fmt_4310[] = "(7x,i5,1p,d16.8,4x,a5,d16.8,1x,a13)";
    static char fmt_4320[] = "(7x,i5,1p,d16.8,4x,a5,d16.8,1x,d13.5)";
    static char fmt_2010[] = "(/\002 --- EXPLANATORY VARIABLE AND DELTA WEIG"
	    "HT SUMMARY:\002)";
    static char fmt_2110[] = "(/\002       INDEX      X(I,J)  DELTA(I,J)    "
	    "FIXED\002,\002     SCALE    WEIGHT    DERIVATIVE\002/\002       "
	    "                                      \002,\002                 "
	    "       ASSESSMENT\002/,\002       (I,J)                         "
	    " (IFIXX)\002,\002    (SCLD)      (WD)              \002/)";
    static char fmt_2120[] = "(/\002       INDEX      X(I,J)  DELTA(I,J)    "
	    "FIXED\002,\002     SCALE    WEIGHT              \002/\002       "
	    "                                      \002,\002                 "
	    "                 \002/,\002       (I,J)                         "
	    " (IFIXX)\002,\002    (SCLD)      (WD)              \002/)";
    static char fmt_2130[] = "(/\002       INDEX      X(I,J)  DELTA(I,J)    "
	    "FIXED\002,\002     SCALE    WEIGHT    DERIVATIVE\002/\002       "
	    "                                      \002,\002                 "
	    "        STEP SIZE\002/,\002       (I,J)                         "
	    " (IFIXX)\002,\002    (SCLD)      (WD)        (STPD)\002/)";
    static char fmt_2020[] = "(/\002 --- EXPLANATORY VARIABLE SUMMARY:\002)";
    static char fmt_2140[] = "(/\002       INDEX      X(I,J)\002/\002       "
	    "(I,J)            \002/)";
    static char fmt_5110[] = "(9x,a2,i1,1p,2d12.3,4x,a5,2d10.2,1x,a13)";
    static char fmt_5120[] = "(8x,a2,i2,1p,2d12.3,4x,a5,2d10.2,1x,a13)";
    static char fmt_5210[] = "(9x,a2,i1,1p,2d12.3,4x,a5,2d10.2,1x,d13.5)";
    static char fmt_5220[] = "(8x,a2,i2,1p,2d12.3,4x,a5,2d10.2,1x,d13.5)";
    static char fmt_6000[] = "(\002 \002)";
    static char fmt_3000[] = "(/\002 --- RESPONSE VARIABLE AND EPSILON ERROR"
	    " WEIGHT\002,\002 SUMMARY:\002)";
    static char fmt_3100[] = "(/\002       INDEX      Y(I,L)      WEIGH"
	    "T\002/\002       (I,L)                    (WE)\002/)";

    /* System generated locals */
    integer delta_dim1, delta_offset, stpd_dim1, stpd_offset, tt_dim1, 
	    tt_offset, wd_dim1, wd_dim2, wd_offset, we_dim1, we_dim2, 
	    we_offset, x_dim1, x_offset, y_dim1, y_offset, ifixx_dim1, 
	    ifixx_offset, msgb_dim1, msgb_offset, msgd_dim1, msgd_offset, 
	    i__1, i__2, i__3, i__4, i__5, i__6;
    doublereal d__1;

    /* Builtin functions */
    integer s_wsfe(cilist *), do_fio(integer *, char *, ftnlen), e_wsfe(void);
    /* Subroutine */ int s_copy(char *, char *, ftnlen, ftnlen);

    /* Local variables */
    static integer i__, j, l, job1, job2, job3, job4, job5;
    static doublereal temp1, temp2, temp3;
    static integer itemp;
    static char tempc0[2], tempc1[5], tempc2[13];
    extern doublereal dhstep_(integer *, integer *, integer *, integer *, 
	    doublereal *, integer *);

    /* Fortran I/O blocks */
    static cilist io___472 = { 0, 0, 0, fmt_1000, 0 };
    static cilist io___478 = { 0, 0, 0, fmt_1100, 0 };
    static cilist io___479 = { 0, 0, 0, fmt_1110, 0 };
    static cilist io___480 = { 0, 0, 0, fmt_1111, 0 };
    static cilist io___481 = { 0, 0, 0, fmt_1120, 0 };
    static cilist io___482 = { 0, 0, 0, fmt_1121, 0 };
    static cilist io___483 = { 0, 0, 0, fmt_1122, 0 };
    static cilist io___484 = { 0, 0, 0, fmt_1130, 0 };
    static cilist io___485 = { 0, 0, 0, fmt_1131, 0 };
    static cilist io___486 = { 0, 0, 0, fmt_1132, 0 };
    static cilist io___487 = { 0, 0, 0, fmt_1133, 0 };
    static cilist io___488 = { 0, 0, 0, fmt_1140, 0 };
    static cilist io___489 = { 0, 0, 0, fmt_1141, 0 };
    static cilist io___490 = { 0, 0, 0, fmt_1142, 0 };
    static cilist io___491 = { 0, 0, 0, fmt_1143, 0 };
    static cilist io___492 = { 0, 0, 0, fmt_1144, 0 };
    static cilist io___493 = { 0, 0, 0, fmt_1145, 0 };
    static cilist io___494 = { 0, 0, 0, fmt_1150, 0 };
    static cilist io___495 = { 0, 0, 0, fmt_1151, 0 };
    static cilist io___496 = { 0, 0, 0, fmt_1152, 0 };
    static cilist io___497 = { 0, 0, 0, fmt_1200, 0 };
    static cilist io___498 = { 0, 0, 0, fmt_1210, 0 };
    static cilist io___499 = { 0, 0, 0, fmt_1300, 0 };
    static cilist io___500 = { 0, 0, 0, fmt_1400, 0 };
    static cilist io___501 = { 0, 0, 0, fmt_1500, 0 };
    static cilist io___502 = { 0, 0, 0, fmt_1510, 0 };
    static cilist io___503 = { 0, 0, 0, fmt_1600, 0 };
    static cilist io___504 = { 0, 0, 0, fmt_1610, 0 };
    static cilist io___505 = { 0, 0, 0, fmt_4000, 0 };
    static cilist io___506 = { 0, 0, 0, fmt_4110, 0 };
    static cilist io___507 = { 0, 0, 0, fmt_4120, 0 };
    static cilist io___508 = { 0, 0, 0, fmt_4200, 0 };
    static cilist io___515 = { 0, 0, 0, fmt_4310, 0 };
    static cilist io___517 = { 0, 0, 0, fmt_4320, 0 };
    static cilist io___518 = { 0, 0, 0, fmt_2010, 0 };
    static cilist io___519 = { 0, 0, 0, fmt_2110, 0 };
    static cilist io___520 = { 0, 0, 0, fmt_2120, 0 };
    static cilist io___521 = { 0, 0, 0, fmt_2130, 0 };
    static cilist io___522 = { 0, 0, 0, fmt_2020, 0 };
    static cilist io___523 = { 0, 0, 0, fmt_2140, 0 };
    static cilist io___526 = { 0, 0, 0, fmt_5110, 0 };
    static cilist io___527 = { 0, 0, 0, fmt_5120, 0 };
    static cilist io___529 = { 0, 0, 0, fmt_5210, 0 };
    static cilist io___530 = { 0, 0, 0, fmt_5220, 0 };
    static cilist io___531 = { 0, 0, 0, fmt_6000, 0 };
    static cilist io___532 = { 0, 0, 0, fmt_5110, 0 };
    static cilist io___533 = { 0, 0, 0, fmt_5120, 0 };
    static cilist io___534 = { 0, 0, 0, fmt_6000, 0 };
    static cilist io___535 = { 0, 0, 0, fmt_3000, 0 };
    static cilist io___536 = { 0, 0, 0, fmt_3100, 0 };
    static cilist io___537 = { 0, 0, 0, fmt_5110, 0 };
    static cilist io___538 = { 0, 0, 0, fmt_5120, 0 };
    static cilist io___539 = { 0, 0, 0, fmt_6000, 0 };


/* ***BEGIN PROLOGUE  DODPC1 */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DHSTEP */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  GENERATE INITIAL SUMMARY REPORT */
/* ***END PROLOGUE  DODPC1 */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...LOCAL ARRAYS */
/* ...EXTERNAL FUNCTIONS */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    delta_dim1 = *n;
    delta_offset = 1 + delta_dim1;
    delta -= delta_offset;
    --stpb;
    --ssf;
    --ifixb;
    --beta;
    msgd_dim1 = *nq;
    msgd_offset = 1 + msgd_dim1;
    msgd -= msgd_offset;
    msgb_dim1 = *nq;
    msgb_offset = 1 + msgb_dim1;
    msgb -= msgb_offset;
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;
    wd_dim1 = *ldwd;
    wd_dim2 = *ld2wd;
    wd_offset = 1 + wd_dim1 * (1 + wd_dim2);
    wd -= wd_offset;
    tt_dim1 = *ldtt;
    tt_offset = 1 + tt_dim1;
    tt -= tt_offset;
    stpd_dim1 = *ldstpd;
    stpd_offset = 1 + stpd_dim1;
    stpd -= stpd_offset;
    y_dim1 = *ldy;
    y_offset = 1 + y_dim1;
    y -= y_offset;
    we_dim1 = *ldwe;
    we_dim2 = *ld2we;
    we_offset = 1 + we_dim1 * (1 + we_dim2);
    we -= we_offset;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ANAJAC:  THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE COMPUTED */
/*            BY FINITE DIFFERENCES (ANAJAC=FALSE) OR NOT (ANAJAC=TRUE). */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   CDJAC:   THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE COMPUTED */
/*            BY CENTRAL DIFFERENCES (CDJAC=TRUE) OR FORWARD DIFFERENCES */
/*            (CDJAC=FALSE). */
/*   CHKJAC:  THE VARIABLE DESIGNATING WHETHER THE USER SUPPLIED */
/*            JACOBIANS ARE TO BE CHECKED (CHKJAC=TRUE) OR NOT */
/*            (CHKJAC=FALSE). */
/*   DELTA:   THE ESTIMATED ERRORS IN THE EXPLANATORY VARIABLES. */
/*   DOVCV:   THE VARIABLE DESIGNATING WHETHER THE COVARIANCE MATRIX IS */
/*            TO BE COMPUTED (DOVCV=TRUE) OR NOT (DOVCV=FALSE). */
/*   I:       AN INDEXING VARIABLE. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IMPLCT:  THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY */
/*            IMPLICIT ODR (IMPLCT=TRUE) OR EXPLICIT ODR (IMPLCT=FALSE). */
/*   INITD:   THE VARIABLE DESIGNATING WHETHER DELTA IS INITIALIZED TO */
/*            ZERO (INITD=TRUE) OR TO THE VALUES IN THE FIRST N BY M */
/*            ELEMENTS OF ARRAY WORK (INITD=FALSE). */
/*   IPR:     THE VALUE INDICATING THE REPORT TO BE PRINTED. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR BY OLS (ISODR=FALSE). */
/*   ITEMP:   A TEMPORARY INTEGER VALUE. */
/*   J:       AN INDEXING VARIABLE. */
/*   JOB:     THE VARIABLE CONTROLING PROBLEM INITIALIZATION AND */
/*            COMPUTATIONAL METHOD. */
/*   JOB1:    THE 1ST DIGIT (FROM THE LEFT) OF VARIABLE JOB. */
/*   JOB2:    THE 2ND DIGIT (FROM THE LEFT) OF VARIABLE JOB. */
/*   JOB3:    THE 3RD DIGIT (FROM THE LEFT) OF VARIABLE JOB. */
/*   JOB4:    THE 4TH DIGIT (FROM THE LEFT) OF VARIABLE JOB. */
/*   JOB5:    THE 5TH DIGIT (FROM THE LEFT) OF VARIABLE JOB. */
/*   L:       AN INDEXING VARIABLE. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LDTT:    THE LEADING DIMENSION OF ARRAY TT. */
/*   LDWD:    THE LEADING DIMENSION OF ARRAY WD. */
/*   LDWE:    THE LEADING DIMENSION OF ARRAY WE. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   LDY:     THE LEADING DIMENSION OF ARRAY Y. */
/*   LD2WD:   THE SECOND DIMENSION OF ARRAY WD. */
/*   LD2WE:   THE SECOND DIMENSION OF ARRAY WE. */
/*   LUNRPT:  THE LOGICAL UNIT NUMBER FOR THE COMPUTATION REPORTS. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   MAXIT:   THE MAXIMUM NUMBER OF ITERATIONS ALLOWED. */
/*   MSGB:    THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT BETA. */
/*   MSGB1:   THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT BETA. */
/*   MSGD:    THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT DELTA. */
/*   MSGD1:   THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT DELTA. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NETA:    THE NUMBER OF ACCURATE DIGITS IN THE FUNCTION RESULTS. */
/*            A NEGATIVE VALUE INDICATES THAT NETA WAS ESTIMATED BY */
/*            ODRPACK. A POSITIVE VALUE INDICTES THE VALUE WAS SUPPLIED */
/*            BY THE USER. */
/*   NNZW:    THE NUMBER OF NONZERO OBSERVATIONAL ERROR WEIGHTS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NPP:     THE NUMBER OF FUNCTION PARAMETERS BEING ESTIMATED. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   PARTOL:  THE PARAMETER CONVERGENCE STOPPING TOLERANCE. */
/*   PNLTY:   THE PENALTY PARAMETER FOR AN IMPLICIT MODEL. */
/*   REDOJ:   THE VARIABLE DESIGNATING WHETHER THE JACOBIAN MATRIX IS TO */
/*            BE RECOMPUTED FOR THE COMPUTATION OF THE COVARIANCE MATRIX */
/*            (REDOJ=TRUE) OR NOT (REDOJ=FALSE). */
/*   RESTRT:  THE VARIABLE DESIGNATING WHETHER THE CALL IS A RESTART */
/*            (RESTRT=TRUE) OR NOT (RESTRT=FALSE). */
/*   SSF:     THE SCALING VALUES FOR BETA. */
/*   SSTOL:   THE SUM-OF-SQUARES CONVERGENCE STOPPING TOLERANCE. */
/*   STPB:    THE RELATIVE STEP USED FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO BETA. */
/*   STPD:    THE RELATIVE STEP USED FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO DELTA. */
/*   TAUFAC:  THE FACTOR USED TO COMPUTE THE INITIAL TRUST REGION */
/*            DIAMETER. */
/*   TEMPC0:  A TEMPORARY CHARACTER*2 VALUE. */
/*   TEMPC1:  A TEMPORARY CHARACTER*5 VALUE. */
/*   TEMPC2:  A TEMPORARY CHARACTER*13 VALUE. */
/*   TEMP1:   A TEMPORARY DOUBLE PRECISION VALUE. */
/*   TEMP2:   A TEMPORARY DOUBLE PRECISION VALUE. */
/*   TEMP3:   A TEMPORARY DOUBLE PRECISION VALUE. */
/*   TT:      THE SCALING VALUES FOR DELTA. */
/*   WD:      THE DELTA WEIGHTS. */
/*   WE:      THE EPSILON WEIGHTS. */
/*   WSS:     THE SUM-OF-SQUARES OF THE WEIGHTED EPSILONS AND DELTAS. */
/*   WSSDEL:  THE SUM-OF-SQUARES OF THE WEIGHTED DELTAS. */
/*   WSSEPS:  THE SUM-OF-SQUARES OF THE WEIGHTED EPSILONS. */
/*   X:       THE EXPLANATORY VARIABLE. */
/*   Y:       THE RESPONSE VARIABLE.  UNUSED WHEN THE MODEL IS IMPLICIT. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DODPC1 */
/*  PRINT PROBLEM SIZE SPECIFICATION */
    io___472.ciunit = *lunrpt;
    s_wsfe(&io___472);
    do_fio(&c__1, (char *)&(*n), (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&(*nnzw), (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&(*nq), (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&(*m), (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&(*np), (ftnlen)sizeof(integer));
    do_fio(&c__1, (char *)&(*npp), (ftnlen)sizeof(integer));
    e_wsfe();
/*  PRINT CONTROL VALUES */
    job1 = *job / 10000;
    job2 = *job % 10000 / 1000;
    job3 = *job % 1000 / 100;
    job4 = *job % 100 / 10;
    job5 = *job % 10;
    io___478.ciunit = *lunrpt;
    s_wsfe(&io___478);
    do_fio(&c__1, (char *)&(*job), (ftnlen)sizeof(integer));
    e_wsfe();
    if (*restrt) {
	io___479.ciunit = *lunrpt;
	s_wsfe(&io___479);
	do_fio(&c__1, (char *)&job1, (ftnlen)sizeof(integer));
	e_wsfe();
    } else {
	io___480.ciunit = *lunrpt;
	s_wsfe(&io___480);
	do_fio(&c__1, (char *)&job1, (ftnlen)sizeof(integer));
	e_wsfe();
    }
    if (*isodr) {
	if (*initd) {
	    io___481.ciunit = *lunrpt;
	    s_wsfe(&io___481);
	    do_fio(&c__1, (char *)&job2, (ftnlen)sizeof(integer));
	    e_wsfe();
	} else {
	    io___482.ciunit = *lunrpt;
	    s_wsfe(&io___482);
	    do_fio(&c__1, (char *)&job2, (ftnlen)sizeof(integer));
	    e_wsfe();
	}
    } else {
	io___483.ciunit = *lunrpt;
	s_wsfe(&io___483);
	do_fio(&c__1, (char *)&job2, (ftnlen)sizeof(integer));
	do_fio(&c__1, (char *)&job5, (ftnlen)sizeof(integer));
	e_wsfe();
    }
    if (*dovcv) {
	io___484.ciunit = *lunrpt;
	s_wsfe(&io___484);
	do_fio(&c__1, (char *)&job3, (ftnlen)sizeof(integer));
	e_wsfe();
	if (*redoj) {
	    io___485.ciunit = *lunrpt;
	    s_wsfe(&io___485);
	    e_wsfe();
	} else {
	    io___486.ciunit = *lunrpt;
	    s_wsfe(&io___486);
	    e_wsfe();
	}
    } else {
	io___487.ciunit = *lunrpt;
	s_wsfe(&io___487);
	do_fio(&c__1, (char *)&job3, (ftnlen)sizeof(integer));
	e_wsfe();
    }
    if (*anajac) {
	io___488.ciunit = *lunrpt;
	s_wsfe(&io___488);
	do_fio(&c__1, (char *)&job4, (ftnlen)sizeof(integer));
	e_wsfe();
	if (*chkjac) {
	    if (*msgb1 >= 1 || *msgd1 >= 1) {
		io___489.ciunit = *lunrpt;
		s_wsfe(&io___489);
		e_wsfe();
	    } else {
		io___490.ciunit = *lunrpt;
		s_wsfe(&io___490);
		e_wsfe();
	    }
	} else {
	    io___491.ciunit = *lunrpt;
	    s_wsfe(&io___491);
	    e_wsfe();
	}
    } else if (*cdjac) {
	io___492.ciunit = *lunrpt;
	s_wsfe(&io___492);
	do_fio(&c__1, (char *)&job4, (ftnlen)sizeof(integer));
	e_wsfe();
    } else {
	io___493.ciunit = *lunrpt;
	s_wsfe(&io___493);
	do_fio(&c__1, (char *)&job4, (ftnlen)sizeof(integer));
	e_wsfe();
    }
    if (*isodr) {
	if (*implct) {
	    io___494.ciunit = *lunrpt;
	    s_wsfe(&io___494);
	    do_fio(&c__1, (char *)&job5, (ftnlen)sizeof(integer));
	    e_wsfe();
	} else {
	    io___495.ciunit = *lunrpt;
	    s_wsfe(&io___495);
	    do_fio(&c__1, (char *)&job5, (ftnlen)sizeof(integer));
	    e_wsfe();
	}
    } else {
	io___496.ciunit = *lunrpt;
	s_wsfe(&io___496);
	do_fio(&c__1, (char *)&job5, (ftnlen)sizeof(integer));
	e_wsfe();
    }
    if (*neta < 0) {
	io___497.ciunit = *lunrpt;
	s_wsfe(&io___497);
	i__1 = -(*neta);
	do_fio(&c__1, (char *)&i__1, (ftnlen)sizeof(integer));
	e_wsfe();
    } else {
	io___498.ciunit = *lunrpt;
	s_wsfe(&io___498);
	do_fio(&c__1, (char *)&(*neta), (ftnlen)sizeof(integer));
	e_wsfe();
    }
    io___499.ciunit = *lunrpt;
    s_wsfe(&io___499);
    do_fio(&c__1, (char *)&(*taufac), (ftnlen)sizeof(doublereal));
    e_wsfe();
/*  PRINT STOPPING CRITERIA */
    io___500.ciunit = *lunrpt;
    s_wsfe(&io___500);
    do_fio(&c__1, (char *)&(*sstol), (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&(*partol), (ftnlen)sizeof(doublereal));
    do_fio(&c__1, (char *)&(*maxit), (ftnlen)sizeof(integer));
    e_wsfe();
/*  PRINT INITIAL SUM OF SQUARES */
    if (*implct) {
	io___501.ciunit = *lunrpt;
	s_wsfe(&io___501);
	do_fio(&c__1, (char *)&(*wssdel), (ftnlen)sizeof(doublereal));
	e_wsfe();
	if (*isodr) {
	    io___502.ciunit = *lunrpt;
	    s_wsfe(&io___502);
	    do_fio(&c__1, (char *)&(*wss), (ftnlen)sizeof(doublereal));
	    do_fio(&c__1, (char *)&(*wsseps), (ftnlen)sizeof(doublereal));
	    do_fio(&c__1, (char *)&(*pnlty), (ftnlen)sizeof(doublereal));
	    e_wsfe();
	}
    } else {
	io___503.ciunit = *lunrpt;
	s_wsfe(&io___503);
	do_fio(&c__1, (char *)&(*wss), (ftnlen)sizeof(doublereal));
	e_wsfe();
	if (*isodr) {
	    io___504.ciunit = *lunrpt;
	    s_wsfe(&io___504);
	    do_fio(&c__1, (char *)&(*wssdel), (ftnlen)sizeof(doublereal));
	    do_fio(&c__1, (char *)&(*wsseps), (ftnlen)sizeof(doublereal));
	    e_wsfe();
	}
    }
    if (*ipr >= 2) {
/*  PRINT FUNCTION PARAMETER DATA */
	io___505.ciunit = *lunrpt;
	s_wsfe(&io___505);
	e_wsfe();
	if (*chkjac && (*msgb1 >= 1 || *msgd1 >= 1)) {
	    io___506.ciunit = *lunrpt;
	    s_wsfe(&io___506);
	    e_wsfe();
	} else if (*anajac) {
	    io___507.ciunit = *lunrpt;
	    s_wsfe(&io___507);
	    e_wsfe();
	} else {
	    io___508.ciunit = *lunrpt;
	    s_wsfe(&io___508);
	    e_wsfe();
	}
	i__1 = *np;
	for (j = 1; j <= i__1; ++j) {
	    if (ifixb[1] < 0) {
		s_copy(tempc1, "   NO", (ftnlen)5, (ftnlen)5);
	    } else {
		if (ifixb[j] != 0) {
		    s_copy(tempc1, "   NO", (ftnlen)5, (ftnlen)5);
		} else {
		    s_copy(tempc1, "  YES", (ftnlen)5, (ftnlen)5);
		}
	    }
	    if (*anajac) {
		if (*chkjac && (*msgb1 >= 1 || *msgd1 >= 1)) {
		    itemp = -1;
		    i__2 = *nq;
		    for (l = 1; l <= i__2; ++l) {
/* Computing MAX */
			i__3 = itemp, i__4 = msgb[l + j * msgb_dim1];
			itemp = max(i__3,i__4);
/* L110: */
		    }
		    if (itemp <= -1) {
			s_copy(tempc2, "    UNCHECKED", (ftnlen)13, (ftnlen)
				13);
		    } else if (itemp == 0) {
			s_copy(tempc2, "     VERIFIED", (ftnlen)13, (ftnlen)
				13);
		    } else if (itemp >= 1) {
			s_copy(tempc2, " QUESTIONABLE", (ftnlen)13, (ftnlen)
				13);
		    }
		} else {
		    s_copy(tempc2, "             ", (ftnlen)13, (ftnlen)13);
		}
	    } else {
		s_copy(tempc2, "             ", (ftnlen)13, (ftnlen)13);
	    }
	    if (ssf[1] < zero) {
		temp1 = abs(ssf[1]);
	    } else {
		temp1 = ssf[j];
	    }
	    if (*anajac) {
		io___515.ciunit = *lunrpt;
		s_wsfe(&io___515);
		do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
		do_fio(&c__1, (char *)&beta[j], (ftnlen)sizeof(doublereal));
		do_fio(&c__1, tempc1, (ftnlen)5);
		do_fio(&c__1, (char *)&temp1, (ftnlen)sizeof(doublereal));
		do_fio(&c__1, tempc2, (ftnlen)13);
		e_wsfe();
	    } else {
		if (*cdjac) {
		    temp2 = dhstep_(&c__1, neta, &c__1, &j, &stpb[1], &c__1);
		} else {
		    temp2 = dhstep_(&c__0, neta, &c__1, &j, &stpb[1], &c__1);
		}
		io___517.ciunit = *lunrpt;
		s_wsfe(&io___517);
		do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
		do_fio(&c__1, (char *)&beta[j], (ftnlen)sizeof(doublereal));
		do_fio(&c__1, tempc1, (ftnlen)5);
		do_fio(&c__1, (char *)&temp1, (ftnlen)sizeof(doublereal));
		do_fio(&c__1, (char *)&temp2, (ftnlen)sizeof(doublereal));
		e_wsfe();
	    }
/* L130: */
	}
/*  PRINT EXPLANATORY VARIABLE DATA */
	if (*isodr) {
	    io___518.ciunit = *lunrpt;
	    s_wsfe(&io___518);
	    e_wsfe();
	    if (*chkjac && (*msgb1 >= 1 || *msgd1 >= 1)) {
		io___519.ciunit = *lunrpt;
		s_wsfe(&io___519);
		e_wsfe();
	    } else if (*anajac) {
		io___520.ciunit = *lunrpt;
		s_wsfe(&io___520);
		e_wsfe();
	    } else {
		io___521.ciunit = *lunrpt;
		s_wsfe(&io___521);
		e_wsfe();
	    }
	} else {
	    io___522.ciunit = *lunrpt;
	    s_wsfe(&io___522);
	    e_wsfe();
	    io___523.ciunit = *lunrpt;
	    s_wsfe(&io___523);
	    e_wsfe();
	}
	if (*isodr) {
	    i__1 = *m;
	    for (j = 1; j <= i__1; ++j) {
		s_copy(tempc0, "1,", (ftnlen)2, (ftnlen)2);
		i__2 = *n;
		i__3 = *n - 1;
		for (i__ = 1; i__3 < 0 ? i__ >= i__2 : i__ <= i__2; i__ += 
			i__3) {
		    if (ifixx[ifixx_dim1 + 1] < 0) {
			s_copy(tempc1, "   NO", (ftnlen)5, (ftnlen)5);
		    } else {
			if (*ldifx == 1) {
			    if (ifixx[j * ifixx_dim1 + 1] == 0) {
				s_copy(tempc1, "  YES", (ftnlen)5, (ftnlen)5);
			    } else {
				s_copy(tempc1, "   NO", (ftnlen)5, (ftnlen)5);
			    }
			} else {
			    if (ifixx[i__ + j * ifixx_dim1] == 0) {
				s_copy(tempc1, "  YES", (ftnlen)5, (ftnlen)5);
			    } else {
				s_copy(tempc1, "   NO", (ftnlen)5, (ftnlen)5);
			    }
			}
		    }
		    if (tt[tt_dim1 + 1] < zero) {
			temp1 = (d__1 = tt[tt_dim1 + 1], abs(d__1));
		    } else {
			if (*ldtt == 1) {
			    temp1 = tt[j * tt_dim1 + 1];
			} else {
			    temp1 = tt[i__ + j * tt_dim1];
			}
		    }
		    if (wd[(wd_dim2 + 1) * wd_dim1 + 1] < zero) {
			temp2 = (d__1 = wd[(wd_dim2 + 1) * wd_dim1 + 1], abs(
				d__1));
		    } else {
			if (*ldwd == 1) {
			    if (*ld2wd == 1) {
				temp2 = wd[(j * wd_dim2 + 1) * wd_dim1 + 1];
			    } else {
				temp2 = wd[(j + j * wd_dim2) * wd_dim1 + 1];
			    }
			} else {
			    if (*ld2wd == 1) {
				temp2 = wd[i__ + (j * wd_dim2 + 1) * wd_dim1];
			    } else {
				temp2 = wd[i__ + (j + j * wd_dim2) * wd_dim1];
			    }
			}
		    }
		    if (*anajac) {
			if (*chkjac && ((*msgb1 >= 1 || *msgd1 >= 1) && i__ ==
				 1)) {
			    itemp = -1;
			    i__4 = *nq;
			    for (l = 1; l <= i__4; ++l) {
/* Computing MAX */
				i__5 = itemp, i__6 = msgd[l + j * msgd_dim1];
				itemp = max(i__5,i__6);
/* L210: */
			    }
			    if (itemp <= -1) {
				s_copy(tempc2, "    UNCHECKED", (ftnlen)13, (
					ftnlen)13);
			    } else if (itemp == 0) {
				s_copy(tempc2, "     VERIFIED", (ftnlen)13, (
					ftnlen)13);
			    } else if (itemp >= 1) {
				s_copy(tempc2, " QUESTIONABLE", (ftnlen)13, (
					ftnlen)13);
			    }
			} else {
			    s_copy(tempc2, "             ", (ftnlen)13, (
				    ftnlen)13);
			}
			if (*m <= 9) {
			    io___526.ciunit = *lunrpt;
			    s_wsfe(&io___526);
			    do_fio(&c__1, tempc0, (ftnlen)2);
			    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer))
				    ;
			    do_fio(&c__1, (char *)&x[i__ + j * x_dim1], (
				    ftnlen)sizeof(doublereal));
			    do_fio(&c__1, (char *)&delta[i__ + j * delta_dim1]
				    , (ftnlen)sizeof(doublereal));
			    do_fio(&c__1, tempc1, (ftnlen)5);
			    do_fio(&c__1, (char *)&temp1, (ftnlen)sizeof(
				    doublereal));
			    do_fio(&c__1, (char *)&temp2, (ftnlen)sizeof(
				    doublereal));
			    do_fio(&c__1, tempc2, (ftnlen)13);
			    e_wsfe();
			} else {
			    io___527.ciunit = *lunrpt;
			    s_wsfe(&io___527);
			    do_fio(&c__1, tempc0, (ftnlen)2);
			    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer))
				    ;
			    do_fio(&c__1, (char *)&x[i__ + j * x_dim1], (
				    ftnlen)sizeof(doublereal));
			    do_fio(&c__1, (char *)&delta[i__ + j * delta_dim1]
				    , (ftnlen)sizeof(doublereal));
			    do_fio(&c__1, tempc1, (ftnlen)5);
			    do_fio(&c__1, (char *)&temp1, (ftnlen)sizeof(
				    doublereal));
			    do_fio(&c__1, (char *)&temp2, (ftnlen)sizeof(
				    doublereal));
			    do_fio(&c__1, tempc2, (ftnlen)13);
			    e_wsfe();
			}
		    } else {
			s_copy(tempc2, "             ", (ftnlen)13, (ftnlen)
				13);
			if (*cdjac) {
			    temp3 = dhstep_(&c__1, neta, &i__, &j, &stpd[
				    stpd_offset], ldstpd);
			} else {
			    temp3 = dhstep_(&c__0, neta, &i__, &j, &stpd[
				    stpd_offset], ldstpd);
			}
			if (*m <= 9) {
			    io___529.ciunit = *lunrpt;
			    s_wsfe(&io___529);
			    do_fio(&c__1, tempc0, (ftnlen)2);
			    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer))
				    ;
			    do_fio(&c__1, (char *)&x[i__ + j * x_dim1], (
				    ftnlen)sizeof(doublereal));
			    do_fio(&c__1, (char *)&delta[i__ + j * delta_dim1]
				    , (ftnlen)sizeof(doublereal));
			    do_fio(&c__1, tempc1, (ftnlen)5);
			    do_fio(&c__1, (char *)&temp1, (ftnlen)sizeof(
				    doublereal));
			    do_fio(&c__1, (char *)&temp2, (ftnlen)sizeof(
				    doublereal));
			    do_fio(&c__1, (char *)&temp3, (ftnlen)sizeof(
				    doublereal));
			    e_wsfe();
			} else {
			    io___530.ciunit = *lunrpt;
			    s_wsfe(&io___530);
			    do_fio(&c__1, tempc0, (ftnlen)2);
			    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer))
				    ;
			    do_fio(&c__1, (char *)&x[i__ + j * x_dim1], (
				    ftnlen)sizeof(doublereal));
			    do_fio(&c__1, (char *)&delta[i__ + j * delta_dim1]
				    , (ftnlen)sizeof(doublereal));
			    do_fio(&c__1, tempc1, (ftnlen)5);
			    do_fio(&c__1, (char *)&temp1, (ftnlen)sizeof(
				    doublereal));
			    do_fio(&c__1, (char *)&temp2, (ftnlen)sizeof(
				    doublereal));
			    do_fio(&c__1, (char *)&temp3, (ftnlen)sizeof(
				    doublereal));
			    e_wsfe();
			}
		    }
		    s_copy(tempc0, "N,", (ftnlen)2, (ftnlen)2);
/* L230: */
		}
		if (j < *m) {
		    io___531.ciunit = *lunrpt;
		    s_wsfe(&io___531);
		    e_wsfe();
		}
/* L240: */
	    }
	} else {
	    i__1 = *m;
	    for (j = 1; j <= i__1; ++j) {
		s_copy(tempc0, "1,", (ftnlen)2, (ftnlen)2);
		i__3 = *n;
		i__2 = *n - 1;
		for (i__ = 1; i__2 < 0 ? i__ >= i__3 : i__ <= i__3; i__ += 
			i__2) {
		    if (*m <= 9) {
			io___532.ciunit = *lunrpt;
			s_wsfe(&io___532);
			do_fio(&c__1, tempc0, (ftnlen)2);
			do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
			do_fio(&c__1, (char *)&x[i__ + j * x_dim1], (ftnlen)
				sizeof(doublereal));
			e_wsfe();
		    } else {
			io___533.ciunit = *lunrpt;
			s_wsfe(&io___533);
			do_fio(&c__1, tempc0, (ftnlen)2);
			do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
			do_fio(&c__1, (char *)&x[i__ + j * x_dim1], (ftnlen)
				sizeof(doublereal));
			e_wsfe();
		    }
		    s_copy(tempc0, "N,", (ftnlen)2, (ftnlen)2);
/* L250: */
		}
		if (j < *m) {
		    io___534.ciunit = *lunrpt;
		    s_wsfe(&io___534);
		    e_wsfe();
		}
/* L260: */
	    }
	}
/*  PRINT RESPONSE VARIABLE DATA AND OBSERVATION ERROR WEIGHTS */
	if (! (*implct)) {
	    io___535.ciunit = *lunrpt;
	    s_wsfe(&io___535);
	    e_wsfe();
	    io___536.ciunit = *lunrpt;
	    s_wsfe(&io___536);
	    e_wsfe();
	    i__1 = *nq;
	    for (l = 1; l <= i__1; ++l) {
		s_copy(tempc0, "1,", (ftnlen)2, (ftnlen)2);
		i__2 = *n;
		i__3 = *n - 1;
		for (i__ = 1; i__3 < 0 ? i__ >= i__2 : i__ <= i__2; i__ += 
			i__3) {
		    if (we[(we_dim2 + 1) * we_dim1 + 1] < zero) {
			temp1 = (d__1 = we[(we_dim2 + 1) * we_dim1 + 1], abs(
				d__1));
		    } else if (*ldwe == 1) {
			if (*ld2we == 1) {
			    temp1 = we[(l * we_dim2 + 1) * we_dim1 + 1];
			} else {
			    temp1 = we[(l + l * we_dim2) * we_dim1 + 1];
			}
		    } else {
			if (*ld2we == 1) {
			    temp1 = we[i__ + (l * we_dim2 + 1) * we_dim1];
			} else {
			    temp1 = we[i__ + (l + l * we_dim2) * we_dim1];
			}
		    }
		    if (*nq <= 9) {
			io___537.ciunit = *lunrpt;
			s_wsfe(&io___537);
			do_fio(&c__1, tempc0, (ftnlen)2);
			do_fio(&c__1, (char *)&l, (ftnlen)sizeof(integer));
			do_fio(&c__1, (char *)&y[i__ + l * y_dim1], (ftnlen)
				sizeof(doublereal));
			do_fio(&c__1, (char *)&temp1, (ftnlen)sizeof(
				doublereal));
			e_wsfe();
		    } else {
			io___538.ciunit = *lunrpt;
			s_wsfe(&io___538);
			do_fio(&c__1, tempc0, (ftnlen)2);
			do_fio(&c__1, (char *)&l, (ftnlen)sizeof(integer));
			do_fio(&c__1, (char *)&y[i__ + l * y_dim1], (ftnlen)
				sizeof(doublereal));
			do_fio(&c__1, (char *)&temp1, (ftnlen)sizeof(
				doublereal));
			e_wsfe();
		    }
		    s_copy(tempc0, "N,", (ftnlen)2, (ftnlen)2);
/* L300: */
		}
		if (l < *nq) {
		    io___539.ciunit = *lunrpt;
		    s_wsfe(&io___539);
		    e_wsfe();
		}
/* L310: */
	    }
	}
    }
    return 0;
/*  FORMAT STATEMENTS */
} /* dodpc1_ */

/* DODPC2 */
/* Subroutine */ int dodpc2_(integer *ipr, integer *lunrpt, logical *fstitr, 
	logical *implct, logical *prtpen, doublereal *pnlty, integer *niter, 
	integer *nfev, doublereal *wss, doublereal *actred, doublereal *
	prered, doublereal *alpha, doublereal *tau, doublereal *pnorm, 
	integer *np, doublereal *beta)
{
    /* Initialized data */

    static doublereal zero = 0.;

    /* Format strings */
    static char fmt_1121[] = "(//\002         CUM.      PENALTY    ACT. REL."
	    "   PRED. REL.\002/\002  IT.  NO. FN     FUNCTION   SUM-OF-SQS   "
	    "SUM-OF-SQS\002,\002              G-N\002/\002 NUM.   EVALS      "
	    "  VALUE    REDUCTION    REDUCTION\002,\002  TAU/PNORM  STEP\002"
	    "/\002 ----  ------  -----------  -----------  -----------\002"
	    ",\002  ---------  ----\002)";
    static char fmt_1122[] = "(//\002         CUM.                 ACT. REL."
	    "   PRED. REL.\002/\002  IT.  NO. FN     WEIGHTED   SUM-OF-SQS   "
	    "SUM-OF-SQS\002,\002              G-N\002/\002 NUM.   EVALS   SUM"
	    "-OF-SQS    REDUCTION    REDUCTION\002,\002  TAU/PNORM  STEP\002"
	    "/\002 ----  ------  -----------  -----------  -----------\002"
	    ",\002  ---------  ----\002/)";
    static char fmt_1131[] = "(//\002         CUM.      PENALTY    ACT. REL."
	    "   PRED. REL.\002/\002  IT.  NO. FN     FUNCTION   SUM-OF-SQS   "
	    "SUM-OF-SQS\002,\002              G-N      BETA --------------"
	    ">\002/\002 NUM.   EVALS        VALUE    REDUCTION    REDUCTIO"
	    "N\002,\002  TAU/PNORM  STEP     INDEX           VALUE\002/\002 -"
	    "---  ------  -----------  -----------  -----------\002,\002  ---"
	    "------  ----     -----           -----\002)";
    static char fmt_1132[] = "(//\002         CUM.                 ACT. REL."
	    "   PRED. REL.\002/\002  IT.  NO. FN     WEIGHTED   SUM-OF-SQS   "
	    "SUM-OF-SQS\002,\002              G-N      BETA --------------"
	    ">\002/\002 NUM.   EVALS   SUM-OF-SQS    REDUCTION    REDUCTIO"
	    "N\002,\002  TAU/PNORM  STEP     INDEX           VALUE\002/\002 -"
	    "---  ------  -----------  -----------  -----------\002,\002  ---"
	    "------  ----     -----           -----\002/)";
    static char fmt_1133[] = "(/\002 PENALTY PARAMETER VALUE = \002,1p,e10.1)"
	    ;
    static char fmt_1141[] = "(1x,i4,i8,1x,1p,d12.5,2d13.4,d11.3,3x,a3,7x,i3"
	    ",3d16.8)";
    static char fmt_1142[] = "(1x,i4,i8,1x,1p,d12.5,2d13.4,d11.3,3x,a3,1x,"
	    "i3,\002 TO\002,i3,3d16.8)";
    static char fmt_1151[] = "(76x,i3,1p,d16.8)";
    static char fmt_1152[] = "(70x,i3,\002 TO\002,i3,1p,3d16.8)";

    /* System generated locals */
    integer i__1, i__2;

    /* Builtin functions */
    integer s_wsfe(cilist *), e_wsfe(void), do_fio(integer *, char *, ftnlen);
    /* Subroutine */ int s_copy(char *, char *, ftnlen, ftnlen);

    /* Local variables */
    static integer j, k, l;
    static char gn[3];
    static doublereal ratio;

    /* Fortran I/O blocks */
    static cilist io___541 = { 0, 0, 0, fmt_1121, 0 };
    static cilist io___542 = { 0, 0, 0, fmt_1122, 0 };
    static cilist io___543 = { 0, 0, 0, fmt_1131, 0 };
    static cilist io___544 = { 0, 0, 0, fmt_1132, 0 };
    static cilist io___545 = { 0, 0, 0, fmt_1133, 0 };
    static cilist io___548 = { 0, 0, 0, fmt_1141, 0 };
    static cilist io___551 = { 0, 0, 0, fmt_1141, 0 };
    static cilist io___552 = { 0, 0, 0, fmt_1142, 0 };
    static cilist io___554 = { 0, 0, 0, fmt_1151, 0 };
    static cilist io___555 = { 0, 0, 0, fmt_1152, 0 };


/* ***BEGIN PROLOGUE  DODPC2 */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  GENERATE ITERATION REPORTS */
/* ***END PROLOGUE  DODPC2 */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --beta;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ACTRED:  THE ACTUAL RELATIVE REDUCTION IN THE SUM-OF-SQUARES. */
/*   ALPHA:   THE LEVENBERG-MARQUARDT PARAMETER. */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   FSTITR:  THE VARIABLE DESIGNATING WHETHER THIS IS THE FIRST */
/*            ITERATION (FSTITR=.TRUE.) OR NOT (FSTITR=.FALSE.). */
/*   GN:      THE CHARACTER*3 VARIABLE INDICATING WHETHER A GAUSS-NEWTON */
/*            STEP WAS TAKEN. */
/*   IMPLCT:  THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY */
/*            IMPLICIT ODR (IMPLCT=TRUE) OR EXPLICIT ODR (IMPLCT=FALSE). */
/*   IPR:     THE VALUE INDICATING THE REPORT TO BE PRINTED. */
/*   J:       AN INDEXING VARIABLE. */
/*   K:       AN INDEXING VARIABLE. */
/*   L:       AN INDEXING VARIABLE. */
/*   LUNRPT:  THE LOGICAL UNIT NUMBER USED FOR COMPUTATION REPORTS. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NITER:   THE NUMBER OF ITERATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   PNLTY:   THE PENALTY PARAMETER FOR AN IMPLICIT MODEL. */
/*   PNORM:   THE NORM OF THE SCALED ESTIMATED PARAMETERS. */
/*   PRERED:  THE PREDICTED RELATIVE REDUCTION IN THE SUM-OF-SQUARES. */
/*   PRTPEN:  THE VARIABLE DESIGNATING WHETHER THE PENALTY PARAMETER IS */
/*            TO BE PRINTED IN THE ITERATION REPORT (PRTPEN=TRUE) OR NOT */
/*            (PRTPEN=FALSE). */
/*   RATIO:   THE RATIO OF TAU TO PNORM. */
/*   TAU:     THE TRUST REGION DIAMETER. */
/*   WSS:     THE SUM-OF-SQUARES OF THE WEIGHTED EPSILONS AND DELTAS. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DODPC2 */
    if (*fstitr) {
	if (*ipr == 1) {
	    if (*implct) {
		io___541.ciunit = *lunrpt;
		s_wsfe(&io___541);
		e_wsfe();
	    } else {
		io___542.ciunit = *lunrpt;
		s_wsfe(&io___542);
		e_wsfe();
	    }
	} else {
	    if (*implct) {
		io___543.ciunit = *lunrpt;
		s_wsfe(&io___543);
		e_wsfe();
	    } else {
		io___544.ciunit = *lunrpt;
		s_wsfe(&io___544);
		e_wsfe();
	    }
	}
    }
    if (*prtpen) {
	io___545.ciunit = *lunrpt;
	s_wsfe(&io___545);
	do_fio(&c__1, (char *)&(*pnlty), (ftnlen)sizeof(doublereal));
	e_wsfe();
    }
    if (*alpha == zero) {
	s_copy(gn, "YES", (ftnlen)3, (ftnlen)3);
    } else {
	s_copy(gn, " NO", (ftnlen)3, (ftnlen)3);
    }
    if (*pnorm != zero) {
	ratio = *tau / *pnorm;
    } else {
	ratio = zero;
    }
    if (*ipr == 1) {
	io___548.ciunit = *lunrpt;
	s_wsfe(&io___548);
	do_fio(&c__1, (char *)&(*niter), (ftnlen)sizeof(integer));
	do_fio(&c__1, (char *)&(*nfev), (ftnlen)sizeof(integer));
	do_fio(&c__1, (char *)&(*wss), (ftnlen)sizeof(doublereal));
	do_fio(&c__1, (char *)&(*actred), (ftnlen)sizeof(doublereal));
	do_fio(&c__1, (char *)&(*prered), (ftnlen)sizeof(doublereal));
	do_fio(&c__1, (char *)&ratio, (ftnlen)sizeof(doublereal));
	do_fio(&c__1, gn, (ftnlen)3);
	e_wsfe();
    } else {
	j = 1;
	k = min(3,*np);
	if (j == k) {
	    io___551.ciunit = *lunrpt;
	    s_wsfe(&io___551);
	    do_fio(&c__1, (char *)&(*niter), (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&(*nfev), (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&(*wss), (ftnlen)sizeof(doublereal));
	    do_fio(&c__1, (char *)&(*actred), (ftnlen)sizeof(doublereal));
	    do_fio(&c__1, (char *)&(*prered), (ftnlen)sizeof(doublereal));
	    do_fio(&c__1, (char *)&ratio, (ftnlen)sizeof(doublereal));
	    do_fio(&c__1, gn, (ftnlen)3);
	    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&beta[j], (ftnlen)sizeof(doublereal));
	    e_wsfe();
	} else {
	    io___552.ciunit = *lunrpt;
	    s_wsfe(&io___552);
	    do_fio(&c__1, (char *)&(*niter), (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&(*nfev), (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&(*wss), (ftnlen)sizeof(doublereal));
	    do_fio(&c__1, (char *)&(*actred), (ftnlen)sizeof(doublereal));
	    do_fio(&c__1, (char *)&(*prered), (ftnlen)sizeof(doublereal));
	    do_fio(&c__1, (char *)&ratio, (ftnlen)sizeof(doublereal));
	    do_fio(&c__1, gn, (ftnlen)3);
	    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
	    do_fio(&c__1, (char *)&k, (ftnlen)sizeof(integer));
	    i__1 = k;
	    for (l = j; l <= i__1; ++l) {
		do_fio(&c__1, (char *)&beta[l], (ftnlen)sizeof(doublereal));
	    }
	    e_wsfe();
	}
	if (*np > 3) {
	    i__1 = *np;
	    for (j = 4; j <= i__1; j += 3) {
/* Computing MIN */
		i__2 = j + 2;
		k = min(i__2,*np);
		if (j == k) {
		    io___554.ciunit = *lunrpt;
		    s_wsfe(&io___554);
		    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
		    do_fio(&c__1, (char *)&beta[j], (ftnlen)sizeof(doublereal)
			    );
		    e_wsfe();
		} else {
		    io___555.ciunit = *lunrpt;
		    s_wsfe(&io___555);
		    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
		    do_fio(&c__1, (char *)&k, (ftnlen)sizeof(integer));
		    i__2 = k;
		    for (l = j; l <= i__2; ++l) {
			do_fio(&c__1, (char *)&beta[l], (ftnlen)sizeof(
				doublereal));
		    }
		    e_wsfe();
		}
/* L10: */
	    }
	}
    }
    return 0;
/*  FORMAT STATEMENTS */
} /* dodpc2_ */

/* DODPC3 */
/* Subroutine */ int dodpc3_(integer *ipr, integer *lunrpt, logical *isodr, 
	logical *implct, logical *didvcv, logical *dovcv, logical *redoj, 
	logical *anajac, integer *n, integer *m, integer *np, integer *nq, 
	integer *npp, integer *info, integer *niter, integer *nfev, integer *
	njev, integer *irank, doublereal *rcond, integer *istop, doublereal *
	wss, doublereal *wssdel, doublereal *wsseps, doublereal *pnlty, 
	doublereal *rvar, integer *idf, doublereal *beta, doublereal *sdbeta, 
	integer *ifixb2, doublereal *f, doublereal *delta)
{
    /* Format strings */
    static char fmt_1000[] = "(/\002 --- STOPPING CONDITIONS:\002)";
    static char fmt_1011[] = "(\002         INFO = \002,i5,\002 ==> SUM OF S"
	    "QUARES CONVERGENCE.\002)";
    static char fmt_1012[] = "(\002         INFO = \002,i5,\002 ==> PARAMETE"
	    "R CONVERGENCE.\002)";
    static char fmt_1013[] = "(\002         INFO = \002,i5,\002 ==> SUM OF S"
	    "QUARES CONVERGENCE AND\002,\002 PARAMETER CONVERGENCE.\002)";
    static char fmt_1014[] = "(\002         INFO = \002,i5,\002 ==> ITERATIO"
	    "N LIMIT REACHED.\002)";
    static char fmt_1015[] = "(\002         INFO = \002,i5,\002 ==> UNEXPECT"
	    "ED VALUE,\002,\002 PROBABLY INDICATING\002/\002                 "
	    "          INCORRECTLY SPECIFIED\002,\002 USER INPUT.\002)";
    static char fmt_1020[] = "(\002         INFO = \002,i5.4/\002           "
	    "   =  ABCD, WHERE A NONZERO VALUE FOR DIGIT A,\002,\002 B, OR C "
	    "INDICATES WHY\002/\002                       THE RESULTS MIGHT B"
	    "E QUESTIONABLE,\002,\002 AND DIGIT D INDICATES\002/\002         "
	    "              THE ACTUAL STOPPING CONDITION.\002)";
    static char fmt_1021[] = "(\002                       A=1 ==> DERIVATIVE"
	    "S ARE\002,\002 QUESTIONABLE.\002)";
    static char fmt_1022[] = "(\002                       B=1 ==> USER SET I"
	    "STOP TO\002,\002 NONZERO VALUE DURING LAST\002/\002             "
	    "                  CALL TO SUBROUTINE FCN.\002)";
    static char fmt_1023[] = "(\002                       C=1 ==> DERIVATIVE"
	    "S ARE NOT\002,\002 FULL RANK AT THE SOLUTION.\002)";
    static char fmt_1024[] = "(\002                       C=2 ==> DERIVATIVE"
	    "S ARE ZERO\002,\002 RANK AT THE SOLUTION.\002)";
    static char fmt_1031[] = "(\002                       D=1 ==> SUM OF SQU"
	    "ARES CONVERGENCE.\002)";
    static char fmt_1032[] = "(\002                       D=2 ==> PARAMETER "
	    "CONVERGENCE.\002)";
    static char fmt_1033[] = "(\002                       D=3 ==> SUM OF SQU"
	    "ARES CONVERGENCE\002,\002 AND PARAMETER CONVERGENCE.\002)";
    static char fmt_1034[] = "(\002                       D=4 ==> ITERATION "
	    "LIMIT REACHED.\002)";
    static char fmt_1035[] = "(\002                       D=\002,i1,\002 ==>"
	    " UNEXPECTED VALUE,\002,\002 PROBABLY INDICATING\002/\002        "
	    "                       INCORRECTLY SPECIFIED\002,\002 USER INPUT."
	    "\002)";
    static char fmt_1040[] = "(\002         INFO = \002,i5.5/\002           "
	    "   = ABCDE, WHERE A NONZERO VALUE FOR A GIVEN\002,\002 DIGIT IND"
	    "ICATES AN\002/\002                       ABNORMAL STOPPING CONDI"
	    "TION.\002)";
    static char fmt_1042[] = "(\002                       A=5 ==> USER STOPP"
	    "ED COMPUTATIONS\002,\002 IN SUBROUTINE FCN.\002)";
    static char fmt_1043[] = "(\002                       B=\002,i1,\002 ==>"
	    " COMPUTATIONS WERE\002,\002 STOPPED DURING THE\002/\002         "
	    "                           FUNCTION EVALUATION.\002)";
    static char fmt_1044[] = "(\002                       C=\002,i1,\002 ==>"
	    " COMPUTATIONS WERE\002,\002 STOPPED BECAUSE\002/\002            "
	    "                        DERIVATIVES WITH\002,\002 RESPECT TO DEL"
	    "TA WERE\002/\002                                    COMPUTED B"
	    "Y\002,\002 SUBROUTINE FCN WHEN\002/\002                         "
	    "           FIT IS OLS.\002)";
    static char fmt_1045[] = "(\002                       C=\002,i1,\002 ==>"
	    " COMPUTATIONS WERE\002,\002 STOPPED DURING THE\002/\002         "
	    "                           JACOBIAN EVALUATION.\002)";
    static char fmt_1050[] = "(\002                       A=6 ==> NUMERICAL "
	    "INSTABILITIES\002,\002 HAVE BEEN DETECTED,\002/\002             "
	    "                  POSSIBLY INDICATING\002,\002 A DISCONTINUITY I"
	    "N THE\002/\002                               DERIVATIVES OR A PO"
	    "OR\002,\002 POOR CHOICE OF PROBLEM\002/\002                     "
	    "          SCALE OR WEIGHTS.\002)";
    static char fmt_1060[] = "(\002                       A=\002,i1,\002 ==>"
	    " UNEXPECTED VALUE,\002,\002 PROBABLY INDICATING\002/\002        "
	    "                       INCORRECTLY SPECIFIED\002,\002 USER INPUT."
	    "\002)";
    static char fmt_1300[] = "(\002        NITER = \002,i5,\002          (NU"
	    "MBER OF ITERATIONS)\002)";
    static char fmt_1310[] = "(\002         NFEV = \002,i5,\002          (NU"
	    "MBER OF FUNCTION EVALUATIONS)\002)";
    static char fmt_1320[] = "(\002         NJEV = \002,i5,\002          (NU"
	    "MBER OF JACOBIAN EVALUATIONS)\002)";
    static char fmt_1330[] = "(\002        IRANK = \002,i5,\002          (RA"
	    "NK DEFICIENCY)\002)";
    static char fmt_1340[] = "(\002        RCOND = \002,1p,d12.2,\002   (INV"
	    "ERSE CONDITION NUMBER)\002)";
    static char fmt_1350[] = "(\002        ISTOP = \002,i5,\002          (RE"
	    "TURNED BY USER FROM\002,\002 SUBROUTINE FCN)\002)";
    static char fmt_2000[] = "(/\002 --- FINAL SUM OF SQUARED WEIGHTED DELTA"
	    "S = \002,17x,1p,d17.8)";
    static char fmt_2010[] = "(\002         FINAL PENALTY FUNCTION VALUE    "
	    " = \002,1p,d17.8/\002               PENALTY TERM               = "
	    "\002,1p,d17.8/\002               PENALTY PARAMETER          ="
	    " \002,1p,d10.1)";
    static char fmt_2100[] = "(/\002 --- FINAL WEIGHTED SUMS OF SQUARES     "
	    "  = \002,17x,1p,d17.8)";
    static char fmt_2110[] = "(\002         SUM OF SQUARED WEIGHTED DELTAS  "
	    " = \002,1p,d17.8/\002         SUM OF SQUARED WEIGHTED EPSILONS = "
	    "\002,1p,d17.8)";
    static char fmt_2200[] = "(/\002 --- RESIDUAL STANDARD DEVIATION        "
	    "  = \002,17x,1p,d17.8/\002         DEGREES OF FREEDOM           "
	    "    =\002,i5)";
    static char fmt_3000[] = "(/\002 --- ESTIMATED BETA(J), J = 1, ..., NP"
	    ":\002)";
    static char fmt_7300[] = "(/\002                     BETA      S.D. BET"
	    "A\002,\002    ---- 95%  CONFIDENCE INTERVAL ----\002/)";
    static char fmt_8400[] = "(3x,i5,1x,1p,d16.8,3x,d12.4,3x,d16.8,1x,\002T"
	    "O\002,d16.8)";
    static char fmt_8600[] = "(3x,i5,1x,1p,d16.8,6x,\002    FIXED\002)";
    static char fmt_8700[] = "(3x,i5,1x,1p,d16.8,6x,\002  DROPPED\002)";
    static char fmt_7310[] = "(/\002     N.B. STANDARD ERRORS AND CONFIDENCE"
	    " INTERVALS ARE\002,\002 COMPUTED USING\002/\002          DERIVAT"
	    "IVES CALCULATED AT THE BEGINNING\002,\002 OF THE LAST ITERATION"
	    ",\002/\002          AND NOT USING DERIVATIVES RE-EVALUATED AT THE"
	    "\002,\002 FINAL SOLUTION.\002)";
    static char fmt_7410[] = "(/\002     N.B. THE STANDARD ERRORS OF THE EST"
	    "IMATED BETAS WERE\002,\002 NOT COMPUTED BECAUSE\002/\002        "
	    "  THE DERIVATIVES WERE NOT AVAILABLE.  EITHER MAXIT\002,\002 IS "
	    "0 AND THE THIRD\002/\002          DIGIT OF JOB IS GREATER THAN 1"
	    ", OR THE MOST\002,\002 RECENTLY TRIED VALUES OF\002/\002        "
	    "  BETA AND/OR X+DELTA WERE IDENTIFIED AS\002,\002 UNACCEPTABLE B"
	    "Y USER SUPPLIED\002/\002          SUBROUTINE FCN.\002)";
    static char fmt_7420[] = "(/\002     N.B. THE STANDARD ERRORS OF THE EST"
	    "IMATED BETAS WERE\002,\002 NOT COMPUTED.\002/\002          (SEE "
	    "INFO ABOVE.)\002)";
    static char fmt_7100[] = "(/\002           INDEX           VALUE\002/)";
    static char fmt_7200[] = "(/\002           INDEX           VALUE -------"
	    "------->\002/)";
    static char fmt_8100[] = "(11x,i5,1p,d16.8)";
    static char fmt_8200[] = "(3x,i5,\002 TO\002,i5,1p,7d16.8)";
    static char fmt_8800[] = "(/\002     N.B. NO PARAMETERS WERE FIXED BY TH"
	    "E USER OR\002,\002 DROPPED AT THE LAST\002/\002          ITERATI"
	    "ON BECAUSE THEY CAUSED THE MODEL TO BE\002,\002 RANK DEFICIENT"
	    ".\002)";
    static char fmt_8900[] = "(/\002     N.B. NO CHANGE WAS MADE TO THE USER"
	    " SUPPLIED PARAMETER\002,\002 VALUES BECAUSE\002/\002          MA"
	    "XIT=0.\002)";
    static char fmt_7500[] = "(/\002                     BETA         STATU"
	    "S\002)";
    static char fmt_8500[] = "(3x,i5,1x,1p,d16.8,6x,\002ESTIMATED\002)";
    static char fmt_4100[] = "(/\002 --- ESTIMATED DELTA(I,*), I = 1, ...,"
	    " N:\002)";
    static char fmt_9110[] = "(\002(/'         I',\002,i2,\002('      DELTA("
	    "I,',I1,')')/)\002)";
    static char fmt_4130[] = "(5x,i5,1p,5d16.8)";
    static char fmt_4110[] = "(/\002 --- ESTIMATED EPSILON(I) AND DELTA(I,*)"
	    ", I = 1, ..., N:\002)";
    static char fmt_9120[] = "(\002(/'         I',\002,i2,\002('    EPSILON("
	    "I,',I1,')'),\002,i2,\002('      DELTA(I,',I1,')')/)\002)";
    static char fmt_4120[] = "(/\002 --- ESTIMATED EPSILON(I), I = 1, ...,"
	    " N:\002)";
    static char fmt_9130[] = "(\002(/'         I',\002,i2,\002('    EPSILON("
	    "I,',I1,')')/)\002)";
    static char fmt_4200[] = "(/\002 --- ESTIMATED EPSILON(I,\002,i3,\002), "
	    "I = 1, ..., N:\002)";
    static char fmt_4300[] = "(/\002 --- ESTIMATED DELTA(I,\002,i3,\002), I "
	    "= 1, ..., N:\002)";

    /* System generated locals */
    integer delta_dim1, delta_offset, f_dim1, f_offset, i__1, i__2, i__3, 
	    i__4;
    doublereal d__1, d__2;

    /* Builtin functions */
    integer s_wsfe(cilist *), e_wsfe(void), do_fio(integer *, char *, ftnlen);
    double sqrt(doublereal);
    integer s_wsfi(icilist *), e_wsfi(void);

    /* Local variables */
    static integer i__, j, k, l, d1, d2, d3, d4, d5;
    static char fmt1[90];
    static doublereal tval;
    extern doublereal dppt_(doublereal *, integer *);
    static integer nplm1;

    /* Fortran I/O blocks */
    static cilist io___561 = { 0, 0, 0, fmt_1000, 0 };
    static cilist io___562 = { 0, 0, 0, fmt_1011, 0 };
    static cilist io___563 = { 0, 0, 0, fmt_1012, 0 };
    static cilist io___564 = { 0, 0, 0, fmt_1013, 0 };
    static cilist io___565 = { 0, 0, 0, fmt_1014, 0 };
    static cilist io___566 = { 0, 0, 0, fmt_1015, 0 };
    static cilist io___567 = { 0, 0, 0, fmt_1020, 0 };
    static cilist io___568 = { 0, 0, 0, fmt_1021, 0 };
    static cilist io___569 = { 0, 0, 0, fmt_1022, 0 };
    static cilist io___570 = { 0, 0, 0, fmt_1023, 0 };
    static cilist io___571 = { 0, 0, 0, fmt_1024, 0 };
    static cilist io___572 = { 0, 0, 0, fmt_1031, 0 };
    static cilist io___573 = { 0, 0, 0, fmt_1032, 0 };
    static cilist io___574 = { 0, 0, 0, fmt_1033, 0 };
    static cilist io___575 = { 0, 0, 0, fmt_1034, 0 };
    static cilist io___576 = { 0, 0, 0, fmt_1035, 0 };
    static cilist io___577 = { 0, 0, 0, fmt_1040, 0 };
    static cilist io___578 = { 0, 0, 0, fmt_1042, 0 };
    static cilist io___579 = { 0, 0, 0, fmt_1043, 0 };
    static cilist io___580 = { 0, 0, 0, fmt_1044, 0 };
    static cilist io___581 = { 0, 0, 0, fmt_1045, 0 };
    static cilist io___582 = { 0, 0, 0, fmt_1050, 0 };
    static cilist io___583 = { 0, 0, 0, fmt_1060, 0 };
    static cilist io___584 = { 0, 0, 0, fmt_1300, 0 };
    static cilist io___585 = { 0, 0, 0, fmt_1310, 0 };
    static cilist io___586 = { 0, 0, 0, fmt_1320, 0 };
    static cilist io___587 = { 0, 0, 0, fmt_1330, 0 };
    static cilist io___588 = { 0, 0, 0, fmt_1340, 0 };
    static cilist io___589 = { 0, 0, 0, fmt_1350, 0 };
    static cilist io___590 = { 0, 0, 0, fmt_2000, 0 };
    static cilist io___591 = { 0, 0, 0, fmt_2010, 0 };
    static cilist io___592 = { 0, 0, 0, fmt_2100, 0 };
    static cilist io___593 = { 0, 0, 0, fmt_2110, 0 };
    static cilist io___594 = { 0, 0, 0, fmt_2200, 0 };
    static cilist io___596 = { 0, 0, 0, fmt_3000, 0 };
    static cilist io___597 = { 0, 0, 0, fmt_7300, 0 };
    static cilist io___600 = { 0, 0, 0, fmt_8400, 0 };
    static cilist io___601 = { 0, 0, 0, fmt_8600, 0 };
    static cilist io___602 = { 0, 0, 0, fmt_8700, 0 };
    static cilist io___603 = { 0, 0, 0, fmt_7310, 0 };
    static cilist io___604 = { 0, 0, 0, fmt_7410, 0 };
    static cilist io___605 = { 0, 0, 0, fmt_7420, 0 };
    static cilist io___606 = { 0, 0, 0, fmt_7100, 0 };
    static cilist io___607 = { 0, 0, 0, fmt_7200, 0 };
    static cilist io___609 = { 0, 0, 0, fmt_8100, 0 };
    static cilist io___610 = { 0, 0, 0, fmt_8200, 0 };
    static cilist io___612 = { 0, 0, 0, fmt_8800, 0 };
    static cilist io___613 = { 0, 0, 0, fmt_8900, 0 };
    static cilist io___614 = { 0, 0, 0, fmt_7500, 0 };
    static cilist io___615 = { 0, 0, 0, fmt_8500, 0 };
    static cilist io___616 = { 0, 0, 0, fmt_8600, 0 };
    static cilist io___617 = { 0, 0, 0, fmt_8700, 0 };
    static cilist io___618 = { 0, 0, 0, fmt_4100, 0 };
    static icilist io___620 = { 0, fmt1, 0, fmt_9110, 90, 1 };
    static cilist io___621 = { 0, 0, 0, fmt1, 0 };
    static cilist io___623 = { 0, 0, 0, fmt_4130, 0 };
    static cilist io___624 = { 0, 0, 0, fmt_4110, 0 };
    static icilist io___625 = { 0, fmt1, 0, fmt_9120, 90, 1 };
    static cilist io___626 = { 0, 0, 0, fmt1, 0 };
    static cilist io___627 = { 0, 0, 0, fmt_4130, 0 };
    static cilist io___628 = { 0, 0, 0, fmt_4120, 0 };
    static icilist io___629 = { 0, fmt1, 0, fmt_9130, 90, 1 };
    static cilist io___630 = { 0, 0, 0, fmt1, 0 };
    static cilist io___631 = { 0, 0, 0, fmt_4130, 0 };
    static cilist io___632 = { 0, 0, 0, fmt_4200, 0 };
    static cilist io___633 = { 0, 0, 0, fmt_7100, 0 };
    static cilist io___634 = { 0, 0, 0, fmt_7200, 0 };
    static cilist io___635 = { 0, 0, 0, fmt_8100, 0 };
    static cilist io___636 = { 0, 0, 0, fmt_8200, 0 };
    static cilist io___637 = { 0, 0, 0, fmt_4300, 0 };
    static cilist io___638 = { 0, 0, 0, fmt_7100, 0 };
    static cilist io___639 = { 0, 0, 0, fmt_7200, 0 };
    static cilist io___640 = { 0, 0, 0, fmt_8100, 0 };
    static cilist io___641 = { 0, 0, 0, fmt_8200, 0 };


/* ***BEGIN PROLOGUE  DODPC3 */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DPPT */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  GENERATE FINAL SUMMARY REPORT */
/* ***END PROLOGUE  DODPC3 */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL FUNCTIONS */
/* ...INTRINSIC FUNCTIONS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ANAJAC:  THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE COMPUTED */
/*            BY FINITE DIFFERENCES (ANAJAC=FALSE) OR NOT (ANAJAC=TRUE). */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   D1:      THE FIRST DIGIT OF INFO. */
/*   D2:      THE SECOND DIGIT OF INFO. */
/*   D3:      THE THIRD DIGIT OF INFO. */
/*   D4:      THE FOURTH DIGIT OF INFO. */
/*   D5:      THE FIFTH DIGIT OF INFO. */
/*   DELTA:   THE ESTIMATED ERRORS IN THE EXPLANATORY VARIABLES. */
/*   DIDVCV:  THE VARIABLE DESIGNATING WHETHER THE COVARIANCE MATRIX WAS */
/*            COMPUTED (DIDVCV=TRUE) OR NOT (DIDVCV=FALSE). */
/*   DOVCV:   THE VARIABLE DESIGNATING WHETHER THE COVARIANCE MATRIX WAS */
/*            TO BE COMPUTED (DOVCV=TRUE) OR NOT (DOVCV=FALSE). */
/*   F:       THE ESTIMATED VALUES OF EPSILON. */
/*   FMT1:    A CHARACTER*90 VARIABLE USED FOR FORMATS. */
/*   I:       AN INDEXING VARIABLE. */
/*   IDF:     THE DEGREES OF FREEDOM OF THE FIT, EQUAL TO THE NUMBER OF */
/*            OBSERVATIONS WITH NONZERO WEIGHTED DERIVATIVES MINUS THE */
/*            NUMBER OF PARAMETERS BEING ESTIMATED. */
/*   IFIXB2:  THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA WERE */
/*            ESTIMATED, FIXED, OR DROPPED BECAUSE THEY CAUSED RANK */
/*            DEFICIENCY, CORRESPONDING TO VALUES OF IFIXB2 EQUALING 1, */
/*            0, AND -1, RESPECTIVELY.  IF IFIXB2 IS -2, THEN NO ATTEMPT */
/*            WAS MADE TO ESTIMATE THE PARAMETERS BECAUSE MAXIT = 0. */
/*   IMPLCT:  THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY */
/*            IMPLICIT ODR (IMPLCT=TRUE) OR EXPLICIT ODR (IMPLCT=FALSE). */
/*   INFO:    THE VARIABLE DESIGNATING WHY THE COMPUTATIONS WERE STOPPED. */
/*   IPR:     THE VARIABLE INDICATING WHAT IS TO BE PRINTED. */
/*   IRANK:   THE RANK DEFICIENCY OF THE JACOBIAN WRT BETA. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR BY OLS (ISODR=FALSE). */
/*   ISTOP:   THE VARIABLE DESIGNATING WHETHER THERE ARE PROBLEMS */
/*            COMPUTING THE FUNCTION AT THE CURRENT BETA AND DELTA. */
/*   J:       AN INDEXING VARIABLE. */
/*   K:       AN INDEXING VARIABLE. */
/*   L:       AN INDEXING VARIABLE. */
/*   LUNRPT:  THE LOGICAL UNIT NUMBER USED FOR COMPUTATION REPORTS. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NITER:   THE NUMBER OF ITERATIONS. */
/*   NJEV:    THE NUMBER OF JACOBIAN EVALUATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NPLM1:   THE NUMBER OF ITEMS TO BE PRINTED PER LINE, MINUS ONE. */
/*   NPP:     THE NUMBER OF FUNCTION PARAMETERS BEING ESTIMATED. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   PNLTY:   THE PENALTY PARAMETER FOR AN IMPLICIT MODEL. */
/*   RCOND:   THE APPROXIMATE RECIPROCAL CONDITION OF TFJACB. */
/*   REDOJ:   THE VARIABLE DESIGNATING WHETHER THE JACOBIAN MATRIX IS */
/*            TO BE RECOMPUTED FOR THE COMPUTATION OF THE COVARIANCE */
/*            MATRIX (REDOJ=TRUE) OR NOT (REDOJ=FALSE). */
/*   RVAR:    THE RESIDUAL VARIANCE. */
/*   SDBETA:  THE STANDARD ERRORS OF THE ESTIMATED PARAMETERS. */
/*   TVAL:    THE VALUE OF THE 97.5 PERCENT POINT FUNCTION FOR THE */
/*            T DISTRIBUTION. */
/*   WSS:     THE SUM-OF-SQUARES OF THE WEIGHTED EPSILONS AND DELTAS. */
/*   WSSDEL:  THE SUM-OF-SQUARES OF THE WEIGHTED DELTAS. */
/*   WSSEPS:  THE SUM-OF-SQUARES OF THE WEIGHTED EPSILONS. */
/* ***FIRST EXECUTABLE STATEMENT  DODPC3 */
    /* Parameter adjustments */
    delta_dim1 = *n;
    delta_offset = 1 + delta_dim1;
    delta -= delta_offset;
    --ifixb2;
    --sdbeta;
    --beta;
    f_dim1 = *n;
    f_offset = 1 + f_dim1;
    f -= f_offset;

    /* Function Body */
    d1 = *info / 10000;
    d2 = *info % 10000 / 1000;
    d3 = *info % 1000 / 100;
    d4 = *info % 100 / 10;
    d5 = *info % 10;
/*  PRINT STOPPING CONDITIONS */
    io___561.ciunit = *lunrpt;
    s_wsfe(&io___561);
    e_wsfe();
    if (*info <= 9) {
	if (*info == 1) {
	    io___562.ciunit = *lunrpt;
	    s_wsfe(&io___562);
	    do_fio(&c__1, (char *)&(*info), (ftnlen)sizeof(integer));
	    e_wsfe();
	} else if (*info == 2) {
	    io___563.ciunit = *lunrpt;
	    s_wsfe(&io___563);
	    do_fio(&c__1, (char *)&(*info), (ftnlen)sizeof(integer));
	    e_wsfe();
	} else if (*info == 3) {
	    io___564.ciunit = *lunrpt;
	    s_wsfe(&io___564);
	    do_fio(&c__1, (char *)&(*info), (ftnlen)sizeof(integer));
	    e_wsfe();
	} else if (*info == 4) {
	    io___565.ciunit = *lunrpt;
	    s_wsfe(&io___565);
	    do_fio(&c__1, (char *)&(*info), (ftnlen)sizeof(integer));
	    e_wsfe();
	} else if (*info <= 9) {
	    io___566.ciunit = *lunrpt;
	    s_wsfe(&io___566);
	    do_fio(&c__1, (char *)&(*info), (ftnlen)sizeof(integer));
	    e_wsfe();
	}
    } else if (*info <= 9999) {
/*  PRINT WARNING DIAGNOSTICS */
	io___567.ciunit = *lunrpt;
	s_wsfe(&io___567);
	do_fio(&c__1, (char *)&(*info), (ftnlen)sizeof(integer));
	e_wsfe();
	if (d2 == 1) {
	    io___568.ciunit = *lunrpt;
	    s_wsfe(&io___568);
	    e_wsfe();
	}
	if (d3 == 1) {
	    io___569.ciunit = *lunrpt;
	    s_wsfe(&io___569);
	    e_wsfe();
	}
	if (d4 == 1) {
	    io___570.ciunit = *lunrpt;
	    s_wsfe(&io___570);
	    e_wsfe();
	}
	if (d4 == 2) {
	    io___571.ciunit = *lunrpt;
	    s_wsfe(&io___571);
	    e_wsfe();
	}
	if (d5 == 1) {
	    io___572.ciunit = *lunrpt;
	    s_wsfe(&io___572);
	    e_wsfe();
	} else if (d5 == 2) {
	    io___573.ciunit = *lunrpt;
	    s_wsfe(&io___573);
	    e_wsfe();
	} else if (d5 == 3) {
	    io___574.ciunit = *lunrpt;
	    s_wsfe(&io___574);
	    e_wsfe();
	} else if (d5 == 4) {
	    io___575.ciunit = *lunrpt;
	    s_wsfe(&io___575);
	    e_wsfe();
	} else if (d5 <= 9) {
	    io___576.ciunit = *lunrpt;
	    s_wsfe(&io___576);
	    do_fio(&c__1, (char *)&d5, (ftnlen)sizeof(integer));
	    e_wsfe();
	}
    } else {
/*  PRINT ERROR MESSAGES */
	io___577.ciunit = *lunrpt;
	s_wsfe(&io___577);
	do_fio(&c__1, (char *)&(*info), (ftnlen)sizeof(integer));
	e_wsfe();
	if (d1 == 5) {
	    io___578.ciunit = *lunrpt;
	    s_wsfe(&io___578);
	    e_wsfe();
	    if (d2 != 0) {
		io___579.ciunit = *lunrpt;
		s_wsfe(&io___579);
		do_fio(&c__1, (char *)&d2, (ftnlen)sizeof(integer));
		e_wsfe();
	    }
	    if (d3 == 3) {
		io___580.ciunit = *lunrpt;
		s_wsfe(&io___580);
		do_fio(&c__1, (char *)&d3, (ftnlen)sizeof(integer));
		e_wsfe();
	    } else if (d3 != 0) {
		io___581.ciunit = *lunrpt;
		s_wsfe(&io___581);
		do_fio(&c__1, (char *)&d3, (ftnlen)sizeof(integer));
		e_wsfe();
	    }
	} else if (d1 == 6) {
	    io___582.ciunit = *lunrpt;
	    s_wsfe(&io___582);
	    e_wsfe();
	} else {
	    io___583.ciunit = *lunrpt;
	    s_wsfe(&io___583);
	    do_fio(&c__1, (char *)&d1, (ftnlen)sizeof(integer));
	    e_wsfe();
	}
    }
/*  PRINT MISC. STOPPING INFO */
    io___584.ciunit = *lunrpt;
    s_wsfe(&io___584);
    do_fio(&c__1, (char *)&(*niter), (ftnlen)sizeof(integer));
    e_wsfe();
    io___585.ciunit = *lunrpt;
    s_wsfe(&io___585);
    do_fio(&c__1, (char *)&(*nfev), (ftnlen)sizeof(integer));
    e_wsfe();
    if (*anajac) {
	io___586.ciunit = *lunrpt;
	s_wsfe(&io___586);
	do_fio(&c__1, (char *)&(*njev), (ftnlen)sizeof(integer));
	e_wsfe();
    }
    io___587.ciunit = *lunrpt;
    s_wsfe(&io___587);
    do_fio(&c__1, (char *)&(*irank), (ftnlen)sizeof(integer));
    e_wsfe();
    io___588.ciunit = *lunrpt;
    s_wsfe(&io___588);
    do_fio(&c__1, (char *)&(*rcond), (ftnlen)sizeof(doublereal));
    e_wsfe();
    io___589.ciunit = *lunrpt;
    s_wsfe(&io___589);
    do_fio(&c__1, (char *)&(*istop), (ftnlen)sizeof(integer));
    e_wsfe();
/*  PRINT FINAL SUM OF SQUARES */
    if (*implct) {
	io___590.ciunit = *lunrpt;
	s_wsfe(&io___590);
	do_fio(&c__1, (char *)&(*wssdel), (ftnlen)sizeof(doublereal));
	e_wsfe();
	if (*isodr) {
	    io___591.ciunit = *lunrpt;
	    s_wsfe(&io___591);
	    do_fio(&c__1, (char *)&(*wss), (ftnlen)sizeof(doublereal));
	    do_fio(&c__1, (char *)&(*wsseps), (ftnlen)sizeof(doublereal));
	    do_fio(&c__1, (char *)&(*pnlty), (ftnlen)sizeof(doublereal));
	    e_wsfe();
	}
    } else {
	io___592.ciunit = *lunrpt;
	s_wsfe(&io___592);
	do_fio(&c__1, (char *)&(*wss), (ftnlen)sizeof(doublereal));
	e_wsfe();
	if (*isodr) {
	    io___593.ciunit = *lunrpt;
	    s_wsfe(&io___593);
	    do_fio(&c__1, (char *)&(*wssdel), (ftnlen)sizeof(doublereal));
	    do_fio(&c__1, (char *)&(*wsseps), (ftnlen)sizeof(doublereal));
	    e_wsfe();
	}
    }
    if (*didvcv) {
	io___594.ciunit = *lunrpt;
	s_wsfe(&io___594);
	d__1 = sqrt(*rvar);
	do_fio(&c__1, (char *)&d__1, (ftnlen)sizeof(doublereal));
	do_fio(&c__1, (char *)&(*idf), (ftnlen)sizeof(integer));
	e_wsfe();
    }
    nplm1 = 3;
/*  PRINT ESTIMATED BETA'S, AND, */
/*  IF, FULL RANK, THEIR STANDARD ERRORS */
    io___596.ciunit = *lunrpt;
    s_wsfe(&io___596);
    e_wsfe();
    if (*didvcv) {
	io___597.ciunit = *lunrpt;
	s_wsfe(&io___597);
	e_wsfe();
	tval = dppt_(&c_b666, idf);
	i__1 = *np;
	for (j = 1; j <= i__1; ++j) {
	    if (ifixb2[j] >= 1) {
		io___600.ciunit = *lunrpt;
		s_wsfe(&io___600);
		do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
		do_fio(&c__1, (char *)&beta[j], (ftnlen)sizeof(doublereal));
		do_fio(&c__1, (char *)&sdbeta[j], (ftnlen)sizeof(doublereal));
		d__1 = beta[j] - tval * sdbeta[j];
		do_fio(&c__1, (char *)&d__1, (ftnlen)sizeof(doublereal));
		d__2 = beta[j] + tval * sdbeta[j];
		do_fio(&c__1, (char *)&d__2, (ftnlen)sizeof(doublereal));
		e_wsfe();
	    } else if (ifixb2[j] == 0) {
		io___601.ciunit = *lunrpt;
		s_wsfe(&io___601);
		do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
		do_fio(&c__1, (char *)&beta[j], (ftnlen)sizeof(doublereal));
		e_wsfe();
	    } else {
		io___602.ciunit = *lunrpt;
		s_wsfe(&io___602);
		do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
		do_fio(&c__1, (char *)&beta[j], (ftnlen)sizeof(doublereal));
		e_wsfe();
	    }
/* L10: */
	}
	if (! (*redoj)) {
	    io___603.ciunit = *lunrpt;
	    s_wsfe(&io___603);
	    e_wsfe();
	}
    } else {
	if (*dovcv) {
	    if (d1 <= 5) {
		io___604.ciunit = *lunrpt;
		s_wsfe(&io___604);
		e_wsfe();
	    } else {
		io___605.ciunit = *lunrpt;
		s_wsfe(&io___605);
		e_wsfe();
	    }
	}
	if (*irank == 0 && *npp == *np || *niter == 0) {
	    if (*np == 1) {
		io___606.ciunit = *lunrpt;
		s_wsfe(&io___606);
		e_wsfe();
	    } else {
		io___607.ciunit = *lunrpt;
		s_wsfe(&io___607);
		e_wsfe();
	    }
	    i__1 = *np;
	    i__2 = nplm1 + 1;
	    for (j = 1; i__2 < 0 ? j >= i__1 : j <= i__1; j += i__2) {
/* Computing MIN */
		i__3 = j + nplm1;
		k = min(i__3,*np);
		if (k == j) {
		    io___609.ciunit = *lunrpt;
		    s_wsfe(&io___609);
		    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
		    do_fio(&c__1, (char *)&beta[j], (ftnlen)sizeof(doublereal)
			    );
		    e_wsfe();
		} else {
		    io___610.ciunit = *lunrpt;
		    s_wsfe(&io___610);
		    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
		    do_fio(&c__1, (char *)&k, (ftnlen)sizeof(integer));
		    i__3 = k;
		    for (l = j; l <= i__3; ++l) {
			do_fio(&c__1, (char *)&beta[l], (ftnlen)sizeof(
				doublereal));
		    }
		    e_wsfe();
		}
/* L20: */
	    }
	    if (*niter >= 1) {
		io___612.ciunit = *lunrpt;
		s_wsfe(&io___612);
		e_wsfe();
	    } else {
		io___613.ciunit = *lunrpt;
		s_wsfe(&io___613);
		e_wsfe();
	    }
	} else {
	    io___614.ciunit = *lunrpt;
	    s_wsfe(&io___614);
	    e_wsfe();
	    i__2 = *np;
	    for (j = 1; j <= i__2; ++j) {
		if (ifixb2[j] >= 1) {
		    io___615.ciunit = *lunrpt;
		    s_wsfe(&io___615);
		    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
		    do_fio(&c__1, (char *)&beta[j], (ftnlen)sizeof(doublereal)
			    );
		    e_wsfe();
		} else if (ifixb2[j] == 0) {
		    io___616.ciunit = *lunrpt;
		    s_wsfe(&io___616);
		    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
		    do_fio(&c__1, (char *)&beta[j], (ftnlen)sizeof(doublereal)
			    );
		    e_wsfe();
		} else {
		    io___617.ciunit = *lunrpt;
		    s_wsfe(&io___617);
		    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
		    do_fio(&c__1, (char *)&beta[j], (ftnlen)sizeof(doublereal)
			    );
		    e_wsfe();
		}
/* L30: */
	    }
	}
    }
    if (*ipr == 1) {
	return 0;
    }
/*  PRINT EPSILON'S AND DELTA'S TOGETHER IN A COLUMN IF THE NUMBER OF */
/*  COLUMNS OF DATA IN EPSILON AND DELTA IS LESS THAN OR EQUAL TO THREE. */
    if (*implct && *m <= 4) {
	io___618.ciunit = *lunrpt;
	s_wsfe(&io___618);
	e_wsfe();
	s_wsfi(&io___620);
	do_fio(&c__1, (char *)&(*m), (ftnlen)sizeof(integer));
	e_wsfi();
	io___621.ciunit = *lunrpt;
	s_wsfe(&io___621);
	i__2 = *m;
	for (j = 1; j <= i__2; ++j) {
	    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
	}
	e_wsfe();
	i__2 = *n;
	for (i__ = 1; i__ <= i__2; ++i__) {
	    io___623.ciunit = *lunrpt;
	    s_wsfe(&io___623);
	    do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
	    i__1 = *m;
	    for (j = 1; j <= i__1; ++j) {
		do_fio(&c__1, (char *)&delta[i__ + j * delta_dim1], (ftnlen)
			sizeof(doublereal));
	    }
	    e_wsfe();
/* L40: */
	}
    } else if (*isodr && *nq + *m <= 4) {
	io___624.ciunit = *lunrpt;
	s_wsfe(&io___624);
	e_wsfe();
	s_wsfi(&io___625);
	do_fio(&c__1, (char *)&(*nq), (ftnlen)sizeof(integer));
	do_fio(&c__1, (char *)&(*m), (ftnlen)sizeof(integer));
	e_wsfi();
	io___626.ciunit = *lunrpt;
	s_wsfe(&io___626);
	i__2 = *nq;
	for (l = 1; l <= i__2; ++l) {
	    do_fio(&c__1, (char *)&l, (ftnlen)sizeof(integer));
	}
	i__1 = *m;
	for (j = 1; j <= i__1; ++j) {
	    do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
	}
	e_wsfe();
	i__2 = *n;
	for (i__ = 1; i__ <= i__2; ++i__) {
	    io___627.ciunit = *lunrpt;
	    s_wsfe(&io___627);
	    do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
	    i__1 = *nq;
	    for (l = 1; l <= i__1; ++l) {
		do_fio(&c__1, (char *)&f[i__ + l * f_dim1], (ftnlen)sizeof(
			doublereal));
	    }
	    i__3 = *m;
	    for (j = 1; j <= i__3; ++j) {
		do_fio(&c__1, (char *)&delta[i__ + j * delta_dim1], (ftnlen)
			sizeof(doublereal));
	    }
	    e_wsfe();
/* L50: */
	}
    } else if (! (*isodr) && (*nq >= 2 && *nq <= 4)) {
	io___628.ciunit = *lunrpt;
	s_wsfe(&io___628);
	e_wsfe();
	s_wsfi(&io___629);
	do_fio(&c__1, (char *)&(*nq), (ftnlen)sizeof(integer));
	e_wsfi();
	io___630.ciunit = *lunrpt;
	s_wsfe(&io___630);
	i__2 = *nq;
	for (l = 1; l <= i__2; ++l) {
	    do_fio(&c__1, (char *)&l, (ftnlen)sizeof(integer));
	}
	e_wsfe();
	i__2 = *n;
	for (i__ = 1; i__ <= i__2; ++i__) {
	    io___631.ciunit = *lunrpt;
	    s_wsfe(&io___631);
	    do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
	    i__1 = *nq;
	    for (l = 1; l <= i__1; ++l) {
		do_fio(&c__1, (char *)&f[i__ + l * f_dim1], (ftnlen)sizeof(
			doublereal));
	    }
	    e_wsfe();
/* L60: */
	}
    } else {
/*  PRINT EPSILON'S AND DELTA'S SEPARATELY */
	if (! (*implct)) {
/*  PRINT EPSILON'S */
	    i__2 = *nq;
	    for (j = 1; j <= i__2; ++j) {
		io___632.ciunit = *lunrpt;
		s_wsfe(&io___632);
		do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
		e_wsfe();
		if (*n == 1) {
		    io___633.ciunit = *lunrpt;
		    s_wsfe(&io___633);
		    e_wsfe();
		} else {
		    io___634.ciunit = *lunrpt;
		    s_wsfe(&io___634);
		    e_wsfe();
		}
		i__1 = *n;
		i__3 = nplm1 + 1;
		for (i__ = 1; i__3 < 0 ? i__ >= i__1 : i__ <= i__1; i__ += 
			i__3) {
/* Computing MIN */
		    i__4 = i__ + nplm1;
		    k = min(i__4,*n);
		    if (i__ == k) {
			io___635.ciunit = *lunrpt;
			s_wsfe(&io___635);
			do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
			do_fio(&c__1, (char *)&f[i__ + j * f_dim1], (ftnlen)
				sizeof(doublereal));
			e_wsfe();
		    } else {
			io___636.ciunit = *lunrpt;
			s_wsfe(&io___636);
			do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
			do_fio(&c__1, (char *)&k, (ftnlen)sizeof(integer));
			i__4 = k;
			for (l = i__; l <= i__4; ++l) {
			    do_fio(&c__1, (char *)&f[l + j * f_dim1], (ftnlen)
				    sizeof(doublereal));
			}
			e_wsfe();
		    }
/* L70: */
		}
/* L80: */
	    }
	}
/*  PRINT DELTA'S */
	if (*isodr) {
	    i__2 = *m;
	    for (j = 1; j <= i__2; ++j) {
		io___637.ciunit = *lunrpt;
		s_wsfe(&io___637);
		do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
		e_wsfe();
		if (*n == 1) {
		    io___638.ciunit = *lunrpt;
		    s_wsfe(&io___638);
		    e_wsfe();
		} else {
		    io___639.ciunit = *lunrpt;
		    s_wsfe(&io___639);
		    e_wsfe();
		}
		i__3 = *n;
		i__1 = nplm1 + 1;
		for (i__ = 1; i__1 < 0 ? i__ >= i__3 : i__ <= i__3; i__ += 
			i__1) {
/* Computing MIN */
		    i__4 = i__ + nplm1;
		    k = min(i__4,*n);
		    if (i__ == k) {
			io___640.ciunit = *lunrpt;
			s_wsfe(&io___640);
			do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
			do_fio(&c__1, (char *)&delta[i__ + j * delta_dim1], (
				ftnlen)sizeof(doublereal));
			e_wsfe();
		    } else {
			io___641.ciunit = *lunrpt;
			s_wsfe(&io___641);
			do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
			do_fio(&c__1, (char *)&k, (ftnlen)sizeof(integer));
			i__4 = k;
			for (l = i__; l <= i__4; ++l) {
			    do_fio(&c__1, (char *)&delta[l + j * delta_dim1], 
				    (ftnlen)sizeof(doublereal));
			}
			e_wsfe();
		    }
/* L90: */
		}
/* L100: */
	    }
	}
    }
    return 0;
/*  FORMAT STATEMENTS */
/* 1341 FORMAT */
/*    +  ('                      ==> POSSIBLY FEWER THAN 2 SIGNIFICANT', */
/*    +                        ' DIGITS IN RESULTS;'/ */
/*    +   '                          SEE ODRPACK REFERENCE', */
/*    +                        ' GUIDE, SECTION 4.C.') */
} /* dodpc3_ */

/* DODPCR */
/* Subroutine */ int dodpcr_(integer *ipr, integer *lunrpt, logical *head, 
	logical *prtpen, logical *fstitr, logical *didvcv, integer *iflag, 
	integer *n, integer *m, integer *np, integer *nq, integer *npp, 
	integer *nnzw, integer *msgb, integer *msgd, doublereal *beta, 
	doublereal *y, integer *ldy, doublereal *x, integer *ldx, doublereal *
	delta, doublereal *we, integer *ldwe, integer *ld2we, doublereal *wd, 
	integer *ldwd, integer *ld2wd, integer *ifixb, integer *ifixx, 
	integer *ldifx, doublereal *ssf, doublereal *tt, integer *ldtt, 
	doublereal *stpb, doublereal *stpd, integer *ldstpd, integer *job, 
	integer *neta, doublereal *taufac, doublereal *sstol, doublereal *
	partol, integer *maxit, doublereal *wss, doublereal *rvar, integer *
	idf, doublereal *sdbeta, integer *niter, integer *nfev, integer *njev,
	 doublereal *actred, doublereal *prered, doublereal *tau, doublereal *
	pnorm, doublereal *alpha, doublereal *f, doublereal *rcond, integer *
	irank, integer *info, integer *istop)
{
    /* Format strings */
    static char fmt_1200[] = "(/\002 *** INITIAL SUMMARY FOR FIT BY METHOD O"
	    "F \002,a3,\002 ***\002)";
    static char fmt_1300[] = "(/\002 *** ITERATION REPORTS FOR FIT BY METHOD"
	    " OF \002,a3,\002 ***\002)";
    static char fmt_1400[] = "(/\002 *** FINAL SUMMARY FOR FIT BY METHOD OF"
	    " \002,a3,\002 ***\002)";

    /* System generated locals */
    integer delta_dim1, delta_offset, f_dim1, f_offset, stpd_dim1, 
	    stpd_offset, tt_dim1, tt_offset, wd_dim1, wd_dim2, wd_offset, 
	    we_dim1, we_dim2, we_offset, x_dim1, x_offset, y_dim1, y_offset, 
	    ifixx_dim1, ifixx_offset;
    doublereal d__1;

    /* Builtin functions */
    /* Subroutine */ int s_copy(char *, char *, ftnlen, ftnlen);
    integer s_wsfe(cilist *), do_fio(integer *, char *, ftnlen), e_wsfe(void);

    /* Local variables */
    static char typ[3];
    static logical cdjac, redoj, initd, isodr, dovcv;
    static doublereal pnlty;
    extern /* Subroutine */ int dodpc1_(integer *, integer *, logical *, 
	    logical *, logical *, logical *, logical *, logical *, logical *, 
	    logical *, logical *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    doublereal *, integer *, integer *, integer *, doublereal *, 
	    doublereal *, integer *, integer *, doublereal *, integer *, 
	    doublereal *, integer *, doublereal *, integer *, doublereal *, 
	    integer *, integer *, doublereal *, doublereal *, integer *, 
	    doublereal *, doublereal *, integer *, integer *, doublereal *, 
	    doublereal *, doublereal *, integer *, doublereal *, doublereal *,
	     doublereal *), dodpc2_(integer *, integer *, logical *, logical *
	    , logical *, doublereal *, integer *, integer *, doublereal *, 
	    doublereal *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, integer *, doublereal *), dodpc3_(integer *, 
	    integer *, logical *, logical *, logical *, logical *, logical *, 
	    logical *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, doublereal 
	    *, integer *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, doublereal *, integer *, doublereal *, doublereal *,
	     integer *, doublereal *, doublereal *);
    static logical anajac, chkjac;
    extern /* Subroutine */ int dflags_(integer *, logical *, logical *, 
	    logical *, logical *, logical *, logical *, logical *, logical *, 
	    logical *), dodphd_(logical *, integer *);
    static logical implct, restrt;

    /* Fortran I/O blocks */
    static cilist io___653 = { 0, 0, 0, fmt_1200, 0 };
    static cilist io___654 = { 0, 0, 0, fmt_1300, 0 };
    static cilist io___655 = { 0, 0, 0, fmt_1400, 0 };


/* ***BEGIN PROLOGUE  DODPCR */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DFLAGS,DODPC1,DODPC2,DODPC3,DODPHD */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  GENERATE COMPUTATION REPORTS */
/* ***END PROLOGUE  DODPCR */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ACTRED:  THE ACTUAL RELATIVE REDUCTION IN THE SUM-OF-SQUARES. */
/*   ALPHA:   THE LEVENBERG-MARQUARDT PARAMETER. */
/*   ANAJAC:  THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE COMPUTED */
/*            BY FINITE DIFFERENCES (ANAJAC=FALSE) OR NOT (ANAJAC=TRUE). */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   CDJAC:   THE VARIABLE DESIGNATING WHETHER THE JACOBIANS ARE COMPUTED */
/*            BY CENTRAL DIFFERENCES (CDJAC=TRUE) OR BY FORWARD */
/*            DIFFERENCES (CDJAC=FALSE). */
/*   CHKJAC:  THE VARIABLE DESIGNATING WHETHER THE USER SUPPLIED */
/*            JACOBIANS ARE TO BE CHECKED (CHKJAC=TRUE) OR NOT */
/*            (CHKJAC=FALSE). */
/*   DELTA:   THE ESTIMATED ERRORS IN THE EXPLANATORY VARIABLES. */
/*   DIDVCV:  THE VARIABLE DESIGNATING WHETHER THE COVARIANCE MATRIX WAS */
/*            COMPUTED (DIDVCV=TRUE) OR NOT (DIDVCV=FALSE). */
/*   DOVCV:   THE VARIABLE DESIGNATING WHETHER THE COVARIANCE MATRIX IS */
/*            TO BE COMPUTED (DOVCV=TRUE) OR NOT (DOVCV=FALSE). */
/*   F:       THE (WEIGHTED) ESTIMATED VALUES OF EPSILON. */
/*   FSTITR:  THE VARIABLE DESIGNATING WHETHER THIS IS THE FIRST */
/*            ITERATION (FSTITR=TRUE) OR NOT (FSTITR=FALSE). */
/*   HEAD:    THE VARIABLE DESIGNATING WHETHER THE HEADING IS TO BE */
/*            PRINTED (HEAD=TRUE) OR NOT (HEAD=FALSE). */
/*   IDF:     THE DEGREES OF FREEDOM OF THE FIT, EQUAL TO THE NUMBER OF */
/*            OBSERVATIONS WITH NONZERO WEIGHTED DERIVATIVES MINUS THE */
/*            NUMBER OF PARAMETERS BEING ESTIMATED. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFLAG:   THE VARIABLE DESIGNATING WHAT IS TO BE PRINTED. */
/*   IMPLCT:  THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY */
/*            IMPLICIT ODR (IMPLCT=TRUE) OR EXPLICIT ODR (IMPLCT=FALSE). */
/*   INFO:    THE VARIABLE DESIGNATING WHY THE COMPUTATIONS WERE STOPPED. */
/*   INITD:   THE VARIABLE DESIGNATING WHETHER DELTA IS INITIALIZED TO */
/*            ZERO (INITD=TRUE) OR TO THE VALUES IN THE FIRST N BY M */
/*            ELEMENTS OF ARRAY WORK (INITD=FALSE). */
/*   IPR:     THE VALUE INDICATING THE REPORT TO BE PRINTED. */
/*   IRANK:   THE RANK DEFICIENCY OF THE JACOBIAN WRT BETA. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR BY OLS (ISODR=FALSE). */
/*   ISTOP:   THE VARIABLE DESIGNATING WHETHER THERE ARE PROBLEMS */
/*            COMPUTING THE FUNCTION AT THE CURRENT BETA AND DELTA. */
/*   JOB:     THE VARIABLE CONTROLING PROBLEM INITIALIZATION AND */
/*            COMPUTATIONAL METHOD. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LDSTPD:  THE LEADING DIMENSION OF ARRAY STPD. */
/*   LDTT:    THE LEADING DIMENSION OF ARRAY TT. */
/*   LDWD:    THE LEADING DIMENSION OF ARRAY WD. */
/*   LDWE:    THE LEADING DIMENSION OF ARRAY WE. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   LDY:     THE LEADING DIMENSION OF ARRAY Y. */
/*   LD2WD:   THE SECOND DIMENSION OF ARRAY WD. */
/*   LD2WE:   THE SECOND DIMENSION OF ARRAY WE. */
/*   LUNRPT:  THE LOGICAL UNIT NUMBER FOR COMPUTATION REPORTS. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   MAXIT:   THE MAXIMUM NUMBER OF ITERATIONS ALLOWED. */
/*   MSGB:    THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT BETA. */
/*   MSGD:    THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT DELTA. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NETA:    THE NUMBER OF ACCURATE DIGITS IN THE FUNCTION RESULTS. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NITER:   THE NUMBER OF ITERATIONS. */
/*   NJEV:    THE NUMBER OF JACOBIAN EVALUATIONS. */
/*   NNZW:    THE NUMBER OF NONZERO WEIGHTED OBSERVATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   NPP:     THE NUMBER OF FUNCTION PARAMETERS BEING ESTIMATED. */
/*   PARTOL:  THE PARAMETER CONVERGENCE STOPPING TOLERANCE. */
/*   PNLTY:   THE PENALTY PARAMETER FOR AN IMPLICIT MODEL. */
/*   PNORM:   THE NORM OF THE SCALED ESTIMATED PARAMETERS. */
/*   PRERED:  THE PREDICTED RELATIVE REDUCTION IN THE SUM-OF-SQUARES. */
/*   PRTPEN:  THE VARIABLE DESIGNATING WHETHER THE PENALTY PARAMETER IS */
/*            TO BE PRINTED IN THE ITERATION REPORT (PRTPEN=TRUE) OR NOT */
/*            (PRTPEN=FALSE). */
/*   RCOND:   THE APPROXIMATE RECIPROCAL CONDITION NUMBER OF TFJACB. */
/*   REDOJ:   THE VARIABLE DESIGNATING WHETHER THE JACOBIAN MATRIX IS TO */
/*            BE RECOMPUTED FOR THE COMPUTATION OF THE COVARIANCE MATRIX */
/*            (REDOJ=TRUE) OR NOT (REDOJ=FALSE). */
/*   RESTRT:  THE VARIABLE DESIGNATING WHETHER THE CALL IS A RESTART */
/*            (RESTRT=TRUE) OR NOT (RESTRT=FALSE). */
/*   RVAR:    THE RESIDUAL VARIANCE. */
/*   SDBETA:  THE STANDARD DEVIATIONS OF THE ESTIMATED BETA'S. */
/*   SSF:     THE SCALING VALUES FOR BETA. */
/*   SSTOL:   THE SUM-OF-SQUARES CONVERGENCE STOPPING TOLERANCE. */
/*   STPB:    THE RELATIVE STEP FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO BETA. */
/*   STPD:    THE RELATIVE STEP FOR COMPUTING FINITE DIFFERENCE */
/*            DERIVATIVES WITH RESPECT TO DELTA. */
/*   TAU:     THE TRUST REGION DIAMETER. */
/*   TAUFAC:  THE FACTOR USED TO COMPUTE THE INITIAL TRUST REGION */
/*            DIAMETER. */
/*   TT:      THE SCALING VALUES FOR DELTA. */
/*   TYP:     THE CHARACTER*3 STRING "ODR" OR "OLS". */
/*   WE:      THE EPSILON WEIGHTS. */
/*   WD:      THE DELTA WEIGHTS. */
/*   WSS:     THE SUM-OF-SQUARES OF THE WEIGHTED EPSILONS AND DELTAS, */
/*            THE SUM-OF-SQUARES OF THE WEIGHTED DELTAS, AND */
/*            THE SUM-OF-SQUARES OF THE WEIGHTED EPSILONS. */
/*   X:       THE EXPLANATORY VARIABLE. */
/*   Y:       THE DEPENDENT VARIABLE.  UNUSED WHEN THE MODEL IS IMPLICIT. */
/* ***FIRST EXECUTABLE STATEMENT  DODPCR */
    /* Parameter adjustments */
    delta_dim1 = *n;
    delta_offset = 1 + delta_dim1;
    delta -= delta_offset;
    --sdbeta;
    --stpb;
    --ssf;
    --ifixb;
    --beta;
    f_dim1 = *n;
    f_offset = 1 + f_dim1;
    f -= f_offset;
    --msgd;
    --msgb;
    y_dim1 = *ldy;
    y_offset = 1 + y_dim1;
    y -= y_offset;
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    we_dim1 = *ldwe;
    we_dim2 = *ld2we;
    we_offset = 1 + we_dim1 * (1 + we_dim2);
    we -= we_offset;
    wd_dim1 = *ldwd;
    wd_dim2 = *ld2wd;
    wd_offset = 1 + wd_dim1 * (1 + wd_dim2);
    wd -= wd_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;
    tt_dim1 = *ldtt;
    tt_offset = 1 + tt_dim1;
    tt -= tt_offset;
    stpd_dim1 = *ldstpd;
    stpd_offset = 1 + stpd_dim1;
    stpd -= stpd_offset;
    --wss;

    /* Function Body */
    dflags_(job, &restrt, &initd, &dovcv, &redoj, &anajac, &cdjac, &chkjac, &
	    isodr, &implct);
    pnlty = (d__1 = we[(we_dim2 + 1) * we_dim1 + 1], abs(d__1));
    if (*head) {
	dodphd_(head, lunrpt);
    }
    if (isodr) {
	s_copy(typ, "ODR", (ftnlen)3, (ftnlen)3);
    } else {
	s_copy(typ, "OLS", (ftnlen)3, (ftnlen)3);
    }
/*  PRINT INITIAL SUMMARY */
    if (*iflag == 1) {
	io___653.ciunit = *lunrpt;
	s_wsfe(&io___653);
	do_fio(&c__1, typ, (ftnlen)3);
	e_wsfe();
	dodpc1_(ipr, lunrpt, &anajac, &cdjac, &chkjac, &initd, &restrt, &
		isodr, &implct, &dovcv, &redoj, &msgb[1], &msgb[2], &msgd[1], 
		&msgd[2], n, m, np, nq, npp, nnzw, &x[x_offset], ldx, &ifixx[
		ifixx_offset], ldifx, &delta[delta_offset], &wd[wd_offset], 
		ldwd, ld2wd, &tt[tt_offset], ldtt, &stpd[stpd_offset], ldstpd,
		 &y[y_offset], ldy, &we[we_offset], ldwe, ld2we, &pnlty, &
		beta[1], &ifixb[1], &ssf[1], &stpb[1], job, neta, taufac, 
		sstol, partol, maxit, &wss[1], &wss[2], &wss[3]);
/*  PRINT ITERATION REPORTS */
    } else if (*iflag == 2) {
	if (*fstitr) {
	    io___654.ciunit = *lunrpt;
	    s_wsfe(&io___654);
	    do_fio(&c__1, typ, (ftnlen)3);
	    e_wsfe();
	}
	dodpc2_(ipr, lunrpt, fstitr, &implct, prtpen, &pnlty, niter, nfev, &
		wss[1], actred, prered, alpha, tau, pnorm, np, &beta[1]);
/*  PRINT FINAL SUMMARY */
    } else if (*iflag == 3) {
	io___655.ciunit = *lunrpt;
	s_wsfe(&io___655);
	do_fio(&c__1, typ, (ftnlen)3);
	e_wsfe();
	dodpc3_(ipr, lunrpt, &isodr, &implct, didvcv, &dovcv, &redoj, &anajac,
		 n, m, np, nq, npp, info, niter, nfev, njev, irank, rcond, 
		istop, &wss[1], &wss[2], &wss[3], &pnlty, rvar, idf, &beta[1],
		 &sdbeta[1], &ifixb[1], &f[f_offset], &delta[delta_offset]);
    }
    return 0;
/*  FORMAT STATEMENTS */
} /* dodpcr_ */

/* DODPE1 */
/* Subroutine */ int dodpe1_(integer *unit, integer *d1, integer *d2, integer 
	*d3, integer *d4, integer *d5, integer *n, integer *m, integer *nq, 
	integer *ldscld, integer *ldstpd, integer *ldwe, integer *ld2we, 
	integer *ldwd, integer *ld2wd, integer *lwkmn, integer *liwkmn)
{
    /* Format strings */
    static char fmt_1100[] = "(/\002 ERROR :  N IS LESS THAN ONE.\002)";
    static char fmt_1200[] = "(/\002 ERROR :  M IS LESS THAN ONE.\002)";
    static char fmt_1300[] = "(/\002 ERROR :  NP IS LESS THAN ONE\002/\002  "
	    "        OR NP IS GREATER THAN N.\002)";
    static char fmt_1400[] = "(/\002 ERROR :  NQ IS LESS THAN ONE.\002)";
    static char fmt_2110[] = "(/\002 ERROR :  LDX IS LESS THAN N.\002)";
    static char fmt_2120[] = "(/\002 ERROR :  LDY IS LESS THAN N.\002)";
    static char fmt_2210[] = "(/\002 ERROR :  LDIFX IS LESS THAN N\002/\002 "
	    "         AND LDIFX IS NOT EQUAL TO ONE.\002)";
    static char fmt_2220[] = "(/\002 ERROR :  LDSCLD IS LESS THAN N\002/\002"
	    "          AND LDSCLD IS NOT EQUAL TO ONE.\002)";
    static char fmt_2230[] = "(/\002 ERROR :  LDSTPD IS LESS THAN N\002/\002"
	    "          AND LDSTPD IS NOT EQUAL TO ONE.\002)";
    static char fmt_2310[] = "(/\002 ERROR :  LDWE IS LESS THAN N\002/\002  "
	    "        AND LDWE IS NOT EQUAL TO ONE OR\002/\002          OR\002/"
	    "\002          LD2WE IS LESS THAN NQ\002/\002          AND LD2WE "
	    "IS NOT EQUAL TO ONE.\002)";
    static char fmt_2320[] = "(/\002 ERROR :  LDWD IS LESS THAN N\002/\002  "
	    "        AND LDWD IS NOT EQUAL TO ONE.\002)";
    static char fmt_2410[] = "(/\002 ERROR :  LWORK IS LESS THAN \002,i7,"
	    "\002,\002/\002          THE SMALLEST ACCEPTABLE DIMENSION OF ARR"
	    "AY WORK.\002)";
    static char fmt_2420[] = "(/\002 ERROR :  LIWORK IS LESS THAN \002,i7"
	    ",\002,\002/\002          THE SMALLEST ACCEPTABLE DIMENSION OF AR"
	    "RAY\002,\002 IWORK.\002)";
    static char fmt_3110[] = "(/\002 ERROR :  SCLD(I,J) IS LESS THAN OR EQUA"
	    "L TO ZERO\002/\002          FOR SOME I = 1, ..., N AND J = 1, .."
	    "., M.\002//\002          WHEN SCLD(1,1) IS GREATER THAN ZERO\002/"
	    "\002          AND LDSCLD IS GREATER THAN OR EQUAL TO N THEN\002"
	    "/\002          EACH OF THE N BY M ELEMENTS OF\002/\002          "
	    "SCLD MUST BE GREATER THAN ZERO.\002)";
    static char fmt_3120[] = "(/\002 ERROR :  SCLD(1,J) IS LESS THAN OR EQUA"
	    "L TO ZERO\002/\002          FOR SOME J = 1, ..., M.\002//\002   "
	    "       WHEN SCLD(1,1) IS GREATER THAN ZERO\002/\002          AND"
	    " LDSCLD IS EQUAL TO ONE THEN\002/\002          EACH OF THE 1 BY "
	    "M ELEMENTS OF\002/\002          SCLD MUST BE GREATER THAN ZERO"
	    ".\002)";
    static char fmt_3130[] = "(/\002 ERROR :  SCLB(K) IS LESS THAN OR EQUAL "
	    "TO ZERO\002/\002          FOR SOME K = 1, ..., NP.\002//\002    "
	    "      ALL NP ELEMENTS OF\002,\002 SCLB MUST BE GREATER THAN ZERO."
	    "\002)";
    static char fmt_3210[] = "(/\002 ERROR :  STPD(I,J) IS LESS THAN OR EQUA"
	    "L TO ZERO\002/\002          FOR SOME I = 1, ..., N AND J = 1, .."
	    "., M.\002//\002          WHEN STPD(1,1) IS GREATER THAN ZERO\002/"
	    "\002          AND LDSTPD IS GREATER THAN OR EQUAL TO N THEN\002"
	    "/\002          EACH OF THE N BY M ELEMENTS OF\002/\002          "
	    "STPD MUST BE GREATER THAN ZERO.\002)";
    static char fmt_3220[] = "(/\002 ERROR :  STPD(1,J) IS LESS THAN OR EQUA"
	    "L TO ZERO\002/\002          FOR SOME J = 1, ..., M.\002//\002   "
	    "       WHEN STPD(1,1) IS GREATER THAN ZERO\002/\002          AND"
	    " LDSTPD IS EQUAL TO ONE THEN\002/\002          EACH OF THE 1 BY "
	    "M ELEMENTS OF\002/\002          STPD MUST BE GREATER THAN ZERO"
	    ".\002)";
    static char fmt_3230[] = "(/\002 ERROR :  STPB(K) IS LESS THAN OR EQUAL "
	    "TO ZERO\002/\002          FOR SOME K = 1, ..., NP.\002//\002    "
	    "      ALL NP ELEMENTS OF\002,\002 STPB MUST BE GREATER THAN ZERO."
	    "\002)";
    static char fmt_3310[] = "(/\002 ERROR :  AT LEAST ONE OF THE (NQ BY NQ)"
	    " ARRAYS STARTING\002/\002          IN WE(I,1,1), I = 1, ..., N, "
	    "IS NOT POSITIVE\002/\002          SEMIDEFINITE.  WHEN WE(1,1,1) "
	    "IS GREATER THAN\002/\002          OR EQUAL TO ZERO, AND LDWE IS "
	    "GREATER THAN OR\002/\002          EQUAL TO N, AND LD2WE IS GREAT"
	    "ER THAN OR EQUAL\002/\002          TO NQ, THEN EACH OF THE (NQ B"
	    "Y NQ) ARRAYS IN WE\002/\002          MUST BE POSITIVE SEMIDEFINI"
	    "TE.\002)";
    static char fmt_3320[] = "(/\002 ERROR :  AT LEAST ONE OF THE (1 BY NQ) "
	    "ARRAYS STARTING\002/\002          IN WE(I,1,1), I = 1, ..., N, H"
	    "AS A NEGATIVE\002/\002          ELEMENT.  WHEN WE(1,1,1) IS GREA"
	    "TER THAN OR\002/\002          EQUAL TO ZERO, AND LDWE IS GREATER"
	    " THAN OR EQUAL\002/\002          TO N, AND LD2WE IS EQUAL TO 1, "
	    "THEN EACH OF THE\002/\002          (1 BY NQ) ARRAYS IN WE MUST H"
	    "AVE ONLY NON-\002/\002          NEGATIVE ELEMENTS.\002)";
    static char fmt_3410[] = "(/\002 ERROR :  THE (NQ BY NQ) ARRAY STARTING "
	    "IN WE(1,1,1) IS\002/\002          NOT POSITIVE SEMIDEFINITE.  WH"
	    "EN WE(1,1,1) IS\002/\002          GREATER THAN OR EQUAL TO ZERO,"
	    " AND LDWE IS EQUAL\002/\002          TO 1, AND LD2WE IS GREATER "
	    "THAN OR EQUAL TO NQ,\002/\002          THEN THE (NQ BY NQ) ARRAY"
	    " IN WE MUST BE POSITIVE\002/\002          SEMIDEFINITE.\002)";
    static char fmt_3420[] = "(/\002 ERROR :  THE (1 BY NQ) ARRAY STARTING I"
	    "N WE(1,1,1) HAS\002/\002          A NEGATIVE ELEMENT.  WHEN WE(1"
	    ",1,1) IS GREATER\002/\002          THAN OR EQUAL TO ZERO, AND LD"
	    "WE IS EQUAL TO 1,\002/\002          AND LD2WE IS EQUAL TO 1, THE"
	    "N THE (1 BY NQ)\002/\002          ARRAY IN WE MUST HAVE ONLY NON"
	    "NEGATIVE ELEMENTS.\002)";
    static char fmt_3500[] = "(/\002 ERROR :  THE NUMBER OF NONZERO ARRAYS I"
	    "N ARRAY WE IS\002/\002          LESS THAN NP.\002)";
    static char fmt_4310[] = "(/\002 ERROR :  AT LEAST ONE OF THE (M BY M) A"
	    "RRAYS STARTING\002/\002          IN WD(I,1,1), I = 1, ..., N, IS"
	    " NOT POSITIVE\002/\002          DEFINITE.  WHEN WD(1,1,1) IS GRE"
	    "ATER THAN ZERO,\002/\002          AND LDWD IS GREATER THAN OR EQ"
	    "UAL TO N, AND\002/\002          LD2WD IS GREATER THAN OR EQUAL T"
	    "O M, THEN EACH\002/\002          OF THE (M BY M) ARRAYS IN WD MU"
	    "ST BE POSITIVE\002/\002          DEFINITE.\002)";
    static char fmt_4320[] = "(/\002 ERROR :  AT LEAST ONE OF THE (1 BY M) A"
	    "RRAYS STARTING\002/\002          IN WD(I,1,1), I = 1, ..., N, HA"
	    "S A NONPOSITIVE\002/\002          ELEMENT.  WHEN WD(1,1,1) IS GR"
	    "EATER THAN ZERO,\002/\002          AND LDWD IS GREATER THAN OR E"
	    "QUAL TO N, AND\002/\002          LD2WD IS EQUAL TO 1, THEN EACH "
	    "OF THE (1 BY M)\002/\002          ARRAYS IN WD MUST HAVE ONLY PO"
	    "SITIVE ELEMENTS.\002)";
    static char fmt_4410[] = "(/\002 ERROR :  THE (M BY M) ARRAY STARTING IN"
	    " WD(1,1,1) IS\002/\002          NOT POSITIVE DEFINITE.  WHEN WD("
	    "1,1,1) IS\002/\002          GREATER THAN ZERO, AND LDWD IS EQUAL"
	    " TO 1, AND\002/\002          LD2WD IS GREATER THAN OR EQUAL TO M"
	    ", THEN THE\002/\002          (M BY M) ARRAY IN WD MUST BE POSITI"
	    "VE DEFINITE.\002)";
    static char fmt_4420[] = "(/\002 ERROR :  THE (1 BY M) ARRAY STARTING IN"
	    " WD(1,1,1) HAS A\002/\002          NONPOSITIVE ELEMENT.  WHEN WD"
	    "(1,1,1) IS GREATER\002/\002          THAN ZERO, AND LDWD IS EQUA"
	    "L TO 1, AND LD2WD IS\002/\002          EQUAL TO 1, THEN THE (1 B"
	    "Y M) ARRAY IN WD MUST\002/\002          HAVE ONLY POSITIVE ELEME"
	    "NTS.\002)";

    /* Builtin functions */
    integer s_wsfe(cilist *), e_wsfe(void), do_fio(integer *, char *, ftnlen);

    /* Fortran I/O blocks */
    static cilist io___656 = { 0, 0, 0, fmt_1100, 0 };
    static cilist io___657 = { 0, 0, 0, fmt_1200, 0 };
    static cilist io___658 = { 0, 0, 0, fmt_1300, 0 };
    static cilist io___659 = { 0, 0, 0, fmt_1400, 0 };
    static cilist io___660 = { 0, 0, 0, fmt_2110, 0 };
    static cilist io___661 = { 0, 0, 0, fmt_2120, 0 };
    static cilist io___662 = { 0, 0, 0, fmt_2210, 0 };
    static cilist io___663 = { 0, 0, 0, fmt_2220, 0 };
    static cilist io___664 = { 0, 0, 0, fmt_2230, 0 };
    static cilist io___665 = { 0, 0, 0, fmt_2310, 0 };
    static cilist io___666 = { 0, 0, 0, fmt_2320, 0 };
    static cilist io___667 = { 0, 0, 0, fmt_2410, 0 };
    static cilist io___668 = { 0, 0, 0, fmt_2420, 0 };
    static cilist io___669 = { 0, 0, 0, fmt_3110, 0 };
    static cilist io___670 = { 0, 0, 0, fmt_3120, 0 };
    static cilist io___671 = { 0, 0, 0, fmt_3130, 0 };
    static cilist io___672 = { 0, 0, 0, fmt_3210, 0 };
    static cilist io___673 = { 0, 0, 0, fmt_3220, 0 };
    static cilist io___674 = { 0, 0, 0, fmt_3230, 0 };
    static cilist io___675 = { 0, 0, 0, fmt_3310, 0 };
    static cilist io___676 = { 0, 0, 0, fmt_3320, 0 };
    static cilist io___677 = { 0, 0, 0, fmt_3410, 0 };
    static cilist io___678 = { 0, 0, 0, fmt_3420, 0 };
    static cilist io___679 = { 0, 0, 0, fmt_3500, 0 };
    static cilist io___680 = { 0, 0, 0, fmt_4310, 0 };
    static cilist io___681 = { 0, 0, 0, fmt_4320, 0 };
    static cilist io___682 = { 0, 0, 0, fmt_4410, 0 };
    static cilist io___683 = { 0, 0, 0, fmt_4420, 0 };


/* ***BEGIN PROLOGUE  DODPE1 */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  PRINT ERROR REPORTS */
/* ***END PROLOGUE  DODPE1 */
/* ...SCALAR ARGUMENTS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   D1:      THE 1ST DIGIT (FROM THE LEFT) OF INFO. */
/*   D2:      THE 2ND DIGIT (FROM THE LEFT) OF INFO. */
/*   D3:      THE 3RD DIGIT (FROM THE LEFT) OF INFO. */
/*   D4:      THE 4TH DIGIT (FROM THE LEFT) OF INFO. */
/*   D5:      THE 5TH DIGIT (FROM THE LEFT) OF INFO. */
/*   LDSCLD:  THE LEADING DIMENSION OF ARRAY SCLD. */
/*   LDSTPD:  THE LEADING DIMENSION OF ARRAY STPD. */
/*   LDWD:    THE LEADING DIMENSION OF ARRAY WD. */
/*   LDWE:    THE LEADING DIMENSION OF ARRAY WE. */
/*   LIWKMN:  THE MINIMUM ACCEPTABLE LENGTH OF ARRAY IWORK. */
/*   LWKMN:   THE MINIMUM ACCEPTABLE LENGTH OF ARRAY WORK. */
/*   LD2WD:   THE SECOND DIMENSION OF ARRAY WD. */
/*   LD2WE:   THE SECOND DIMENSION OF ARRAY WE. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   UNIT:    THE LOGICAL UNIT NUMBER USED FOR ERROR MESSAGES. */
/* ***FIRST EXECUTABLE STATEMENT  DODPE1 */
/*  PRINT APPROPRIATE MESSAGES FOR ERRORS IN PROBLEM SPECIFICATION */
/*  PARAMETERS */
    if (*d1 == 1) {
	if (*d2 != 0) {
	    io___656.ciunit = *unit;
	    s_wsfe(&io___656);
	    e_wsfe();
	}
	if (*d3 != 0) {
	    io___657.ciunit = *unit;
	    s_wsfe(&io___657);
	    e_wsfe();
	}
	if (*d4 != 0) {
	    io___658.ciunit = *unit;
	    s_wsfe(&io___658);
	    e_wsfe();
	}
	if (*d5 != 0) {
	    io___659.ciunit = *unit;
	    s_wsfe(&io___659);
	    e_wsfe();
	}
/*  PRINT APPROPRIATE MESSAGES FOR ERRORS IN DIMENSION SPECIFICATION */
/*  PARAMETERS */
    } else if (*d1 == 2) {
	if (*d2 != 0) {
	    if (*d2 == 1 || *d2 == 3) {
		io___660.ciunit = *unit;
		s_wsfe(&io___660);
		e_wsfe();
	    }
	    if (*d2 == 2 || *d2 == 3) {
		io___661.ciunit = *unit;
		s_wsfe(&io___661);
		e_wsfe();
	    }
	}
	if (*d3 != 0) {
	    if (*d3 == 1 || *d3 == 3 || *d3 == 5 || *d3 == 7) {
		io___662.ciunit = *unit;
		s_wsfe(&io___662);
		e_wsfe();
	    }
	    if (*d3 == 2 || *d3 == 3 || *d3 == 6 || *d3 == 7) {
		io___663.ciunit = *unit;
		s_wsfe(&io___663);
		e_wsfe();
	    }
	    if (*d3 == 4 || *d3 == 5 || *d3 == 6 || *d3 == 7) {
		io___664.ciunit = *unit;
		s_wsfe(&io___664);
		e_wsfe();
	    }
	}
	if (*d4 != 0) {
	    if (*d4 == 1 || *d4 == 3) {
		io___665.ciunit = *unit;
		s_wsfe(&io___665);
		e_wsfe();
	    }
	    if (*d4 == 2 || *d4 == 3) {
		io___666.ciunit = *unit;
		s_wsfe(&io___666);
		e_wsfe();
	    }
	}
	if (*d5 != 0) {
	    if (*d5 == 1 || *d5 == 3) {
		io___667.ciunit = *unit;
		s_wsfe(&io___667);
		do_fio(&c__1, (char *)&(*lwkmn), (ftnlen)sizeof(integer));
		e_wsfe();
	    }
	    if (*d5 == 2 || *d5 == 3) {
		io___668.ciunit = *unit;
		s_wsfe(&io___668);
		do_fio(&c__1, (char *)&(*liwkmn), (ftnlen)sizeof(integer));
		e_wsfe();
	    }
	}
    } else if (*d1 == 3) {
/*  PRINT APPROPRIATE MESSAGES FOR ERRORS IN SCALE VALUES */
	if (*d2 != 0) {
	    if (*d2 == 1 || *d2 == 3) {
		if (*ldscld >= *n) {
		    io___669.ciunit = *unit;
		    s_wsfe(&io___669);
		    e_wsfe();
		} else {
		    io___670.ciunit = *unit;
		    s_wsfe(&io___670);
		    e_wsfe();
		}
	    }
	    if (*d2 == 2 || *d2 == 3) {
		io___671.ciunit = *unit;
		s_wsfe(&io___671);
		e_wsfe();
	    }
	}
/*  PRINT APPROPRIATE MESSAGES FOR ERRORS IN DERIVATIVE STEP VALUES */
	if (*d3 != 0) {
	    if (*d3 == 1 || *d3 == 3) {
		if (*ldstpd >= *n) {
		    io___672.ciunit = *unit;
		    s_wsfe(&io___672);
		    e_wsfe();
		} else {
		    io___673.ciunit = *unit;
		    s_wsfe(&io___673);
		    e_wsfe();
		}
	    }
	    if (*d3 == 2 || *d3 == 3) {
		io___674.ciunit = *unit;
		s_wsfe(&io___674);
		e_wsfe();
	    }
	}
/*  PRINT APPROPRIATE MESSAGES FOR ERRORS IN OBSERVATIONAL ERROR WEIGHTS */
	if (*d4 != 0) {
	    if (*d4 == 1) {
		if (*ldwe >= *n) {
		    if (*ld2we >= *nq) {
			io___675.ciunit = *unit;
			s_wsfe(&io___675);
			e_wsfe();
		    } else {
			io___676.ciunit = *unit;
			s_wsfe(&io___676);
			e_wsfe();
		    }
		} else {
		    if (*ld2we >= *nq) {
			io___677.ciunit = *unit;
			s_wsfe(&io___677);
			e_wsfe();
		    } else {
			io___678.ciunit = *unit;
			s_wsfe(&io___678);
			e_wsfe();
		    }
		}
	    }
	    if (*d4 == 2) {
		io___679.ciunit = *unit;
		s_wsfe(&io___679);
		e_wsfe();
	    }
	}
/*  PRINT APPROPRIATE MESSAGES FOR ERRORS IN DELTA WEIGHTS */
	if (*d5 != 0) {
	    if (*ldwd >= *n) {
		if (*ld2wd >= *m) {
		    io___680.ciunit = *unit;
		    s_wsfe(&io___680);
		    e_wsfe();
		} else {
		    io___681.ciunit = *unit;
		    s_wsfe(&io___681);
		    e_wsfe();
		}
	    } else {
		if (*ld2wd >= *m) {
		    io___682.ciunit = *unit;
		    s_wsfe(&io___682);
		    e_wsfe();
		} else {
		    io___683.ciunit = *unit;
		    s_wsfe(&io___683);
		    e_wsfe();
		}
	    }
	}
    }
/*  FORMAT STATEMENTS */
    return 0;
} /* dodpe1_ */

/* DODPE2 */
/* Subroutine */ int dodpe2_(integer *unit, integer *n, integer *m, integer *
	np, integer *nq, doublereal *fjacb, doublereal *fjacd, doublereal *
	diff, integer *msgb1, integer *msgb, logical *isodr, integer *msgd1, 
	integer *msgd, doublereal *xplusd, integer *nrow, integer *neta, 
	integer *ntol)
{
    /* Format strings */
    static char fmt_1000[] = "(//\002 *** DERIVATIVE CHECKING REPORT FOR FIT"
	    " BY METHOD OF \002,a3,\002 ***\002/)";
    static char fmt_2100[] = "(/\002     FOR RESPONSE \002,i2,\002 OF OBSERV"
	    "ATION \002,i5/)";
    static char fmt_2200[] = "(\002                      \002,\002         U"
	    "SER\002,\002               \002,\002                \002/\002   "
	    "                   \002,\002     SUPPLIED\002,\002     RELATIV"
	    "E\002,\002    DERIVATIVE \002/\002        DERIVATIVE WRT\002,"
	    "\002        VALUE\002,\002   DIFFERENCE\002,\002    ASSESSMENT"
	    " \002/)";
    static char fmt_3100[] = "(\002             BETA(\002,i3,\002)\002,\002 "
	    "      ---   \002,\002       ---   \002,\002    UNCHECKED\002)";
    static char fmt_3200[] = "(\002             BETA(\002,i3,\002)\002,1p,2d"
	    "13.2,3x,a1,\002VERIFIED\002)";
    static char fmt_3300[] = "(\002             BETA(\002,i3,\002)\002,1p,2d"
	    "13.2,3x,a1,\002QUESTIONABLE (SEE NOTE \002,i1,\002)\002)";
    static char fmt_4100[] = "(\002          DELTA(\002,i2,\002,\002,i2"
	    ",\002)\002,\002       ---   \002,\002       ---   \002,\002    U"
	    "NCHECKED\002)";
    static char fmt_4200[] = "(\002          DELTA(\002,i2,\002,\002,i2"
	    ",\002)\002,1p,2d13.2,3x,a1,\002VERIFIED\002)";
    static char fmt_4300[] = "(\002          DELTA(\002,i2,\002,\002,i2"
	    ",\002)\002,1p,2d13.2,3x,a1,\002QUESTIONABLE (SEE NOTE \002,i1"
	    ",\002)\002)";
    static char fmt_5000[] = "(/\002     NOTES:\002)";
    static char fmt_5100[] = "(/\002      (1) USER SUPPLIED AND FINITE DIFFE"
	    "RENCE DERIVATIVES\002,\002 AGREE, BUT\002/\002          RESULTS "
	    "ARE QUESTIONABLE BECAUSE BOTH ARE ZERO.\002)";
    static char fmt_5200[] = "(/\002      (2) USER SUPPLIED AND FINITE DIFFE"
	    "RENCE DERIVATIVES\002,\002 AGREE, BUT\002/\002          RESULTS "
	    "ARE QUESTIONABLE BECAUSE ONE IS\002,\002 IDENTICALLY ZERO\002"
	    "/\002          AND THE OTHER IS ONLY APPROXIMATELY ZERO.\002)";
    static char fmt_5300[] = "(/\002      (3) USER SUPPLIED AND FINITE DIFFE"
	    "RENCE DERIVATIVES\002,\002 DISAGREE, BUT\002/\002          RESUL"
	    "TS ARE QUESTIONABLE BECAUSE ONE IS\002,\002 IDENTICALLY ZERO\002/"
	    "\002          AND THE OTHER IS NOT.\002)";
    static char fmt_5400[] = "(/\002      (4) USER SUPPLIED AND FINITE DIFFE"
	    "RENCE DERIVATIVES\002,\002 DISAGREE, BUT\002/\002          FINIT"
	    "E DIFFERENCE DERIVATIVE IS QUESTIONABLE\002,\002 BECAUSE EITHE"
	    "R\002/\002          THE RATIO OF RELATIVE CURVATURE TO RELATIV"
	    "E\002,\002 SLOPE IS TOO HIGH\002/\002          OR THE SCALE IS W"
	    "RONG.\002)";
    static char fmt_5500[] = "(/\002      (5) USER SUPPLIED AND FINITE DIFFE"
	    "RENCE DERIVATIVES\002,\002 DISAGREE, BUT\002/\002          FINIT"
	    "E DIFFERENCE DERIVATIVE IS QUESTIONABLE\002,\002 BECAUSE THE\002/"
	    "\002          RATIO OF RELATIVE CURVATURE TO RELATIVE SLOPE I"
	    "S\002,\002 TOO HIGH.\002)";
    static char fmt_5600[] = "(/\002      (6) USER SUPPLIED AND FINITE DIFFE"
	    "RENCE DERIVATIVES\002,\002 DISAGREE, BUT\002/\002          HAVE "
	    "AT LEAST 2 DIGITS IN COMMON.\002)";
    static char fmt_5700[] = "(/\002      (7) USER SUPPLIED AND FINITE DIFFE"
	    "RENCE DERIVATIVES\002,\002 DISAGREE, AND\002/\002          HAVE "
	    "FEWER THAN 2 DIGITS IN COMMON.  DERIVATIVE\002,\002 CHECKING MUST"
	    "\002/\002          BE TURNED OFF IN ORDER TO PROCEED.\002)";
    static char fmt_6000[] = "(/\002     NUMBER OF RELIABLE DIGITS IN FUNCTI"
	    "ON RESULTS       \002,i5/\002        (ESTIMATED BY ODRPACK)\002)";
    static char fmt_6100[] = "(/\002     NUMBER OF RELIABLE DIGITS IN FUNCTI"
	    "ON RESULTS       \002,i5/\002        (SUPPLIED BY USER)\002)";
    static char fmt_7000[] = "(/\002     NUMBER OF DIGITS OF AGREEMENT REQUI"
	    "RED BETWEEN      \002/\002     USER SUPPLIED AND FINITE DIFFEREN"
	    "CE DERIVATIVE FOR  \002/\002     USER SUPPLIED DERIVATIVE TO BE "
	    "CONSIDERED VERIFIED  \002,i5)";
    static char fmt_8100[] = "(/\002     ROW NUMBER AT WHICH DERIVATIVES WER"
	    "E CHECKED        \002,i5//\002       -VALUES OF THE EXPLANATORY "
	    "VARIABLES AT THIS ROW\002/)";
    static char fmt_8110[] = "(10x,\002X(\002,i2,\002,\002,i2,\002)\002,1x,1"
	    "p,3d16.8)";

    /* System generated locals */
    integer diff_dim1, diff_offset, fjacb_dim1, fjacb_dim2, fjacb_offset, 
	    fjacd_dim1, fjacd_dim2, fjacd_offset, xplusd_dim1, xplusd_offset, 
	    msgb_dim1, msgb_offset, msgd_dim1, msgd_offset, i__1, i__2;

    /* Builtin functions */
    /* Subroutine */ int s_copy(char *, char *, ftnlen, ftnlen);
    integer s_wsfe(cilist *), do_fio(integer *, char *, ftnlen), e_wsfe(void);

    /* Local variables */
    static integer i__, j, k, l;
    static char typ[3], flag__[1];
    static logical ftnote[8];

    /* Fortran I/O blocks */
    static cilist io___688 = { 0, 0, 0, fmt_1000, 0 };
    static cilist io___689 = { 0, 0, 0, fmt_2100, 0 };
    static cilist io___690 = { 0, 0, 0, fmt_2200, 0 };
    static cilist io___693 = { 0, 0, 0, fmt_3100, 0 };
    static cilist io___694 = { 0, 0, 0, fmt_3200, 0 };
    static cilist io___695 = { 0, 0, 0, fmt_3300, 0 };
    static cilist io___696 = { 0, 0, 0, fmt_4100, 0 };
    static cilist io___697 = { 0, 0, 0, fmt_4200, 0 };
    static cilist io___698 = { 0, 0, 0, fmt_4300, 0 };
    static cilist io___699 = { 0, 0, 0, fmt_5000, 0 };
    static cilist io___700 = { 0, 0, 0, fmt_5100, 0 };
    static cilist io___701 = { 0, 0, 0, fmt_5200, 0 };
    static cilist io___702 = { 0, 0, 0, fmt_5300, 0 };
    static cilist io___703 = { 0, 0, 0, fmt_5400, 0 };
    static cilist io___704 = { 0, 0, 0, fmt_5500, 0 };
    static cilist io___705 = { 0, 0, 0, fmt_5600, 0 };
    static cilist io___706 = { 0, 0, 0, fmt_5700, 0 };
    static cilist io___707 = { 0, 0, 0, fmt_6000, 0 };
    static cilist io___708 = { 0, 0, 0, fmt_6100, 0 };
    static cilist io___709 = { 0, 0, 0, fmt_7000, 0 };
    static cilist io___710 = { 0, 0, 0, fmt_8100, 0 };
    static cilist io___712 = { 0, 0, 0, fmt_8110, 0 };


/* ***BEGIN PROLOGUE  DODPE2 */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  GENERATE THE DERIVATIVE CHECKING REPORT */
/* ***END PROLOGUE  DODPE2 */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...LOCAL ARRAYS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   DIFF:    THE RELATIVE DIFFERENCES BETWEEN THE USER SUPPLIED AND */
/*            FINITE DIFFERENCE DERIVATIVES FOR EACH DERIVATIVE CHECKED. */
/*   FJACB:   THE JACOBIAN WITH RESPECT TO BETA. */
/*   FJACD:   THE JACOBIAN WITH RESPECT TO DELTA. */
/*   FLAG:    THE CHARACTER STRING INDICATING HIGHLY QUESTIONABLE RESULTS. */
/*   FTNOTE:  THE ARRAY CONTROLING FOOTNOTES. */
/*   I:       AN INDEX VARIABLE. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=.TRUE.) OR BY OLS (ISODR=.FALSE.). */
/*   J:       AN INDEX VARIABLE. */
/*   K:       AN INDEX VARIABLE. */
/*   L:       AN INDEX VARIABLE. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   MSGB:    THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT BETA. */
/*   MSGB1:   THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT BETA. */
/*   MSGD:    THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT DELTA. */
/*   MSGD1:   THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT DELTA. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NETA:    THE NUMBER OF RELIABLE DIGITS IN THE MODEL. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   NROW:    THE ROW NUMBER OF THE EXPLANATORY VARIABLE ARRAY AT */
/*            WHICH THE DERIVATIVE IS TO BE CHECKED. */
/*   NTOL:    THE NUMBER OF DIGITS OF AGREEMENT REQUIRED BETWEEN THE */
/*            FINITE DIFFERENCE AND THE USER SUPPLIED DERIVATIVES. */
/*   TYP:     THE CHARACTER STRING INDICATING SOLUTION TYPE, ODR OR OLS. */
/*   UNIT:    THE LOGICAL UNIT NUMBER USED FOR ERROR MESSAGES. */
/*   XPLUSD:  THE VALUES OF X + DELTA. */
/* ***FIRST EXECUTABLE STATEMENT  DODPE2 */
/*  SET UP FOR FOOTNOTES */
    /* Parameter adjustments */
    xplusd_dim1 = *n;
    xplusd_offset = 1 + xplusd_dim1;
    xplusd -= xplusd_offset;
    msgd_dim1 = *nq;
    msgd_offset = 1 + msgd_dim1;
    msgd -= msgd_offset;
    msgb_dim1 = *nq;
    msgb_offset = 1 + msgb_dim1;
    msgb -= msgb_offset;
    diff_dim1 = *nq;
    diff_offset = 1 + diff_dim1;
    diff -= diff_offset;
    fjacd_dim1 = *n;
    fjacd_dim2 = *m;
    fjacd_offset = 1 + fjacd_dim1 * (1 + fjacd_dim2);
    fjacd -= fjacd_offset;
    fjacb_dim1 = *n;
    fjacb_dim2 = *np;
    fjacb_offset = 1 + fjacb_dim1 * (1 + fjacb_dim2);
    fjacb -= fjacb_offset;

    /* Function Body */
    for (i__ = 0; i__ <= 7; ++i__) {
	ftnote[i__] = FALSE_;
/* L10: */
    }
    i__1 = *nq;
    for (l = 1; l <= i__1; ++l) {
	if (*msgb1 >= 1) {
	    i__2 = *np;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		if (msgb[l + i__ * msgb_dim1] >= 1) {
		    ftnote[0] = TRUE_;
		    ftnote[msgb[l + i__ * msgb_dim1]] = TRUE_;
		}
/* L20: */
	    }
	}
	if (*msgd1 >= 1) {
	    i__2 = *m;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		if (msgd[l + i__ * msgd_dim1] >= 1) {
		    ftnote[0] = TRUE_;
		    ftnote[msgd[l + i__ * msgd_dim1]] = TRUE_;
		}
/* L30: */
	    }
	}
/* L40: */
    }
/*     PRINT REPORT */
    if (*isodr) {
	s_copy(typ, "ODR", (ftnlen)3, (ftnlen)3);
    } else {
	s_copy(typ, "OLS", (ftnlen)3, (ftnlen)3);
    }
    io___688.ciunit = *unit;
    s_wsfe(&io___688);
    do_fio(&c__1, typ, (ftnlen)3);
    e_wsfe();
    i__1 = *nq;
    for (l = 1; l <= i__1; ++l) {
	io___689.ciunit = *unit;
	s_wsfe(&io___689);
	do_fio(&c__1, (char *)&l, (ftnlen)sizeof(integer));
	do_fio(&c__1, (char *)&(*nrow), (ftnlen)sizeof(integer));
	e_wsfe();
	io___690.ciunit = *unit;
	s_wsfe(&io___690);
	e_wsfe();
	i__2 = *np;
	for (i__ = 1; i__ <= i__2; ++i__) {
	    k = msgb[l + i__ * msgb_dim1];
	    if (k >= 7) {
		*(unsigned char *)flag__ = '*';
	    } else {
		*(unsigned char *)flag__ = ' ';
	    }
	    if (k <= -1) {
		io___693.ciunit = *unit;
		s_wsfe(&io___693);
		do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
		e_wsfe();
	    } else if (k == 0) {
		io___694.ciunit = *unit;
		s_wsfe(&io___694);
		do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
		do_fio(&c__1, (char *)&fjacb[*nrow + (i__ + l * fjacb_dim2) * 
			fjacb_dim1], (ftnlen)sizeof(doublereal));
		do_fio(&c__1, (char *)&diff[l + i__ * diff_dim1], (ftnlen)
			sizeof(doublereal));
		do_fio(&c__1, flag__, (ftnlen)1);
		e_wsfe();
	    } else if (k >= 1) {
		io___695.ciunit = *unit;
		s_wsfe(&io___695);
		do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
		do_fio(&c__1, (char *)&fjacb[*nrow + (i__ + l * fjacb_dim2) * 
			fjacb_dim1], (ftnlen)sizeof(doublereal));
		do_fio(&c__1, (char *)&diff[l + i__ * diff_dim1], (ftnlen)
			sizeof(doublereal));
		do_fio(&c__1, flag__, (ftnlen)1);
		do_fio(&c__1, (char *)&k, (ftnlen)sizeof(integer));
		e_wsfe();
	    }
/* L50: */
	}
	if (*isodr) {
	    i__2 = *m;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		k = msgd[l + i__ * msgd_dim1];
		if (k >= 7) {
		    *(unsigned char *)flag__ = '*';
		} else {
		    *(unsigned char *)flag__ = ' ';
		}
		if (k <= -1) {
		    io___696.ciunit = *unit;
		    s_wsfe(&io___696);
		    do_fio(&c__1, (char *)&(*nrow), (ftnlen)sizeof(integer));
		    do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
		    e_wsfe();
		} else if (k == 0) {
		    io___697.ciunit = *unit;
		    s_wsfe(&io___697);
		    do_fio(&c__1, (char *)&(*nrow), (ftnlen)sizeof(integer));
		    do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
		    do_fio(&c__1, (char *)&fjacd[*nrow + (i__ + l * 
			    fjacd_dim2) * fjacd_dim1], (ftnlen)sizeof(
			    doublereal));
		    do_fio(&c__1, (char *)&diff[l + (*np + i__) * diff_dim1], 
			    (ftnlen)sizeof(doublereal));
		    do_fio(&c__1, flag__, (ftnlen)1);
		    e_wsfe();
		} else if (k >= 1) {
		    io___698.ciunit = *unit;
		    s_wsfe(&io___698);
		    do_fio(&c__1, (char *)&(*nrow), (ftnlen)sizeof(integer));
		    do_fio(&c__1, (char *)&i__, (ftnlen)sizeof(integer));
		    do_fio(&c__1, (char *)&fjacd[*nrow + (i__ + l * 
			    fjacd_dim2) * fjacd_dim1], (ftnlen)sizeof(
			    doublereal));
		    do_fio(&c__1, (char *)&diff[l + (*np + i__) * diff_dim1], 
			    (ftnlen)sizeof(doublereal));
		    do_fio(&c__1, flag__, (ftnlen)1);
		    do_fio(&c__1, (char *)&k, (ftnlen)sizeof(integer));
		    e_wsfe();
		}
/* L60: */
	    }
	}
/* L70: */
    }
/*     PRINT FOOTNOTES */
    if (ftnote[0]) {
	io___699.ciunit = *unit;
	s_wsfe(&io___699);
	e_wsfe();
	if (ftnote[1]) {
	    io___700.ciunit = *unit;
	    s_wsfe(&io___700);
	    e_wsfe();
	}
	if (ftnote[2]) {
	    io___701.ciunit = *unit;
	    s_wsfe(&io___701);
	    e_wsfe();
	}
	if (ftnote[3]) {
	    io___702.ciunit = *unit;
	    s_wsfe(&io___702);
	    e_wsfe();
	}
	if (ftnote[4]) {
	    io___703.ciunit = *unit;
	    s_wsfe(&io___703);
	    e_wsfe();
	}
	if (ftnote[5]) {
	    io___704.ciunit = *unit;
	    s_wsfe(&io___704);
	    e_wsfe();
	}
	if (ftnote[6]) {
	    io___705.ciunit = *unit;
	    s_wsfe(&io___705);
	    e_wsfe();
	}
	if (ftnote[7]) {
	    io___706.ciunit = *unit;
	    s_wsfe(&io___706);
	    e_wsfe();
	}
    }
    if (*neta < 0) {
	io___707.ciunit = *unit;
	s_wsfe(&io___707);
	i__1 = -(*neta);
	do_fio(&c__1, (char *)&i__1, (ftnlen)sizeof(integer));
	e_wsfe();
    } else {
	io___708.ciunit = *unit;
	s_wsfe(&io___708);
	do_fio(&c__1, (char *)&(*neta), (ftnlen)sizeof(integer));
	e_wsfe();
    }
    io___709.ciunit = *unit;
    s_wsfe(&io___709);
    do_fio(&c__1, (char *)&(*ntol), (ftnlen)sizeof(integer));
    e_wsfe();
/*  PRINT OUT ROW OF EXPLANATORY VARIABLE WHICH WAS CHECKED. */
    io___710.ciunit = *unit;
    s_wsfe(&io___710);
    do_fio(&c__1, (char *)&(*nrow), (ftnlen)sizeof(integer));
    e_wsfe();
    i__1 = *m;
    for (j = 1; j <= i__1; ++j) {
	io___712.ciunit = *unit;
	s_wsfe(&io___712);
	do_fio(&c__1, (char *)&(*nrow), (ftnlen)sizeof(integer));
	do_fio(&c__1, (char *)&j, (ftnlen)sizeof(integer));
	do_fio(&c__1, (char *)&xplusd[*nrow + j * xplusd_dim1], (ftnlen)
		sizeof(doublereal));
	e_wsfe();
/* L80: */
    }
    return 0;
/*     FORMAT STATEMENTS */
} /* dodpe2_ */

/* DODPE3 */
/* Subroutine */ int dodpe3_(integer *unit, integer *d2, integer *d3)
{
    /* Format strings */
    static char fmt_1100[] = "(//\002 VARIABLE ISTOP HAS BEEN RETURNED WITH "
	    "A NONZERO VALUE  \002/\002 FROM USER SUPPLIED SUBROUTINE FCN WHE"
	    "N INVOKED USING THE\002/\002 INITIAL ESTIMATES OF BETA AND DELTA"
	    " SUPPLIED BY THE     \002/\002 USER.  THE INITIAL ESTIMATES MUST"
	    " BE ADJUSTED TO ALLOW  \002/\002 PROPER EVALUATION OF SUBROUTINE"
	    " FCN BEFORE THE          \002/\002 REGRESSION PROCEDURE CAN CONT"
	    "INUE.\002)";
    static char fmt_1200[] = "(//\002 VARIABLE ISTOP HAS BEEN RETURNED WITH "
	    "A NONZERO VALUE  \002/\002 FROM USER SUPPLIED SUBROUTINE FCN.  T"
	    "HIS OCCURRED DURING\002/\002 THE COMPUTATION OF THE NUMBER OF RE"
	    "LIABLE DIGITS IN THE \002/\002 PREDICTED VALUES (F) RETURNED FRO"
	    "M SUBROUTINE FCN, INDI-\002/\002 CATING THAT CHANGES IN THE INIT"
	    "IAL ESTIMATES OF BETA(K),\002/\002 K=1,NP, AS SMALL AS 2*BETA(K)"
	    "*SQRT(MACHINE PRECISION),  \002/\002 WHERE MACHINE PRECISION IS "
	    "DEFINED AS THE SMALLEST VALUE\002/\002 E SUCH THAT 1+E>1 ON THE "
	    "COMPUTER BEING USED, PREVENT   \002/\002 SUBROUTINE FCN FROM BEI"
	    "NG PROPERLY EVALUATED.  THE      \002/\002 INITIAL ESTIMATES MUS"
	    "T BE ADJUSTED TO ALLOW PROPER      \002/\002 EVALUATION OF SUBRO"
	    "UTINE FCN DURING THESE COMPUTATIONS  \002/\002 BEFORE THE REGRES"
	    "SION PROCEDURE CAN CONTINUE.\002)";
    static char fmt_1300[] = "(//\002 VARIABLE ISTOP HAS BEEN RETURNED WITH "
	    "A NONZERO VALUE  \002/\002 FROM USER SUPPLIED SUBROUTINE FCN.  T"
	    "HIS OCCURRED DURING\002/\002 THE DERIVATIVE CHECKING PROCEDURE, "
	    "INDICATING THAT      \002/\002 CHANGES IN THE INITIAL ESTIMATES "
	    "OF BETA(K), K=1,NP, AS \002/\002 SMALL AS MAX[BETA(K),1/SCLB(K)]"
	    "*10**(-NETA/2), AND/OR   \002/\002 OF DELTA(I,J), I=1,N AND J=1,"
	    "M, AS SMALL AS             \002/\002 MAX[DELTA(I,J),1/SCLD(I,J)]"
	    "*10**(-NETA/2), WHERE NETA   \002/\002 IS DEFINED TO BE THE NUMB"
	    "ER OF RELIABLE DIGITS IN       \002/\002 PREDICTED VALUES (F) RE"
	    "TURNED FROM SUBROUTINE FCN,      \002/\002 PREVENT SUBROUTINE FC"
	    "N FROM BEING PROPERLY EVALUATED.   \002/\002 THE INITIAL ESTIMAT"
	    "ES MUST BE ADJUSTED TO ALLOW PROPER  \002/\002 EVALUATION OF SUB"
	    "ROUTINE FCN DURING THESE COMPUTATIONS  \002/\002 BEFORE THE REGR"
	    "ESSION PROCEDURE CAN CONTINUE.\002)";
    static char fmt_1400[] = "(//\002 VARIABLE ISTOP HAS BEEN RETURNED WITH "
	    "A NONZERO VALUE  \002/\002 FROM USER SUPPLIED SUBROUTINE FCN WHE"
	    "N INVOKED FOR \002/\002 DERIVATIVE EVALUATIONS USING THE INITIAL"
	    " ESTIMATES OF \002/\002 BETA AND DELTA SUPPLIED BY THE USER.  TH"
	    "E INITIAL \002/\002 ESTIMATES MUST BE ADJUSTED TO ALLOW PROPER E"
	    "VALUATION \002/\002 OF SUBROUTINE FCN BEFORE THE REGRESSION PROC"
	    "EDURE CAN \002/\002 CONTINUE.\002)";

    /* Builtin functions */
    integer s_wsfe(cilist *), e_wsfe(void);

    /* Fortran I/O blocks */
    static cilist io___713 = { 0, 0, 0, fmt_1100, 0 };
    static cilist io___714 = { 0, 0, 0, fmt_1200, 0 };
    static cilist io___715 = { 0, 0, 0, fmt_1300, 0 };
    static cilist io___716 = { 0, 0, 0, fmt_1400, 0 };


/* ***BEGIN PROLOGUE  DODPE3 */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  PRINT ERROR REPORTS INDICATING THAT COMPUTATIONS WERE */
/*            STOPPED IN USER SUPPLIED SUBROUTINES FCN */
/* ***END PROLOGUE  DODPE3 */
/* ...SCALAR ARGUMENTS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   D2:      THE 2ND DIGIT (FROM THE LEFT) OF INFO. */
/*   D3:      THE 3RD DIGIT (FROM THE LEFT) OF INFO. */
/*   UNIT:    THE LOGICAL UNIT NUMBER USED FOR ERROR MESSAGES. */
/* ***FIRST EXECUTABLE STATEMENT  DODPE3 */
/*  PRINT APPROPRIATE MESSAGES TO INDICATE WHERE COMPUTATIONS WERE */
/*  STOPPED */
    if (*d2 == 2) {
	io___713.ciunit = *unit;
	s_wsfe(&io___713);
	e_wsfe();
    } else if (*d2 == 3) {
	io___714.ciunit = *unit;
	s_wsfe(&io___714);
	e_wsfe();
    } else if (*d2 == 4) {
	io___715.ciunit = *unit;
	s_wsfe(&io___715);
	e_wsfe();
    }
    if (*d3 == 2) {
	io___716.ciunit = *unit;
	s_wsfe(&io___716);
	e_wsfe();
    }
/*  FORMAT STATEMENTS */
    return 0;
} /* dodpe3_ */

/* DODPER */
/* Subroutine */ int dodper_(integer *info, integer *lunerr, logical *short__,
	 integer *n, integer *m, integer *np, integer *nq, integer *ldscld, 
	integer *ldstpd, integer *ldwe, integer *ld2we, integer *ldwd, 
	integer *ld2wd, integer *lwkmn, integer *liwkmn, doublereal *fjacb, 
	doublereal *fjacd, doublereal *diff, integer *msgb, logical *isodr, 
	integer *msgd, doublereal *xplusd, integer *nrow, integer *neta, 
	integer *ntol)
{
    /* Format strings */
    static char fmt_1100[] = "(//\002 THE CORRECT FORM OF THE CALL STATEMENT"
	    " IS \002//\002       CALL DODR\002/\002      +     (FCN,\002/"
	    "\002      +     N,M,NP,NQ,\002/\002      +     BETA,\002/\002   "
	    "   +     Y,LDY,X,LDX,\002/\002      +     WE,LDWE,LD2WE,WD,LDWD,"
	    "LD2WD,\002/\002      +     JOB,\002/\002      +     IPRINT,LUNER"
	    "R,LUNRPT,\002/\002      +     WORK,LWORK,IWORK,LIWORK,\002/\002 "
	    "     +     INFO)\002)";
    static char fmt_1200[] = "(//\002 THE CORRECT FORM OF THE CALL STATEMENT"
	    " IS \002//\002       CALL DODRC\002/\002      +     (FCN,\002"
	    "/\002      +     N,M,NP,NQ,\002/\002      +     BETA,\002/\002  "
	    "    +     Y,LDY,X,LDX,\002/\002      +     WE,LDWE,LD2WE,WD,LDWD"
	    ",LD2WD,\002/\002      +     IFIXB,IFIXX,LDIFX,\002/\002      +  "
	    "   JOB,NDIGIT,TAUFAC,\002/\002      +     SSTOL,PARTOL,MAXIT,"
	    "\002/\002      +     IPRINT,LUNERR,LUNRPT,\002/\002      +     S"
	    "TPB,STPD,LDSTPD,\002/\002      +     SCLB,SCLD,LDSCLD,\002/\002 "
	    "     +     WORK,LWORK,IWORK,LIWORK,\002/\002      +     INFO)"
	    "\002)";

    /* System generated locals */
    integer diff_dim1, diff_offset, fjacb_dim1, fjacb_dim2, fjacb_offset, 
	    fjacd_dim1, fjacd_dim2, fjacd_offset, xplusd_dim1, xplusd_offset;

    /* Builtin functions */
    integer s_wsfe(cilist *), e_wsfe(void);

    /* Local variables */
    static integer d1, d2, d3, d4, d5;
    static logical head;
    static integer unit;
    extern /* Subroutine */ int dodpe1_(integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *, integer *, integer *, integer *, integer *, 
	    integer *, integer *), dodpe2_(integer *, integer *, integer *, 
	    integer *, integer *, doublereal *, doublereal *, doublereal *, 
	    integer *, integer *, logical *, integer *, integer *, doublereal 
	    *, integer *, integer *, integer *), dodpe3_(integer *, integer *,
	     integer *), dodphd_(logical *, integer *);

    /* Fortran I/O blocks */
    static cilist io___724 = { 0, 0, 0, fmt_1100, 0 };
    static cilist io___725 = { 0, 0, 0, fmt_1200, 0 };


/* ***BEGIN PROLOGUE  DODPER */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DODPE1,DODPE2,DODPE3,DODPHD */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  CONTROLLING ROUTINE FOR PRINTING ERROR REPORTS */
/* ***END PROLOGUE  DODPER */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   D1:      THE 1ST DIGIT (FROM THE LEFT) OF INFO. */
/*   D2:      THE 2ND DIGIT (FROM THE LEFT) OF INFO. */
/*   D3:      THE 3RD DIGIT (FROM THE LEFT) OF INFO. */
/*   D4:      THE 4TH DIGIT (FROM THE LEFT) OF INFO. */
/*   D5:      THE 5TH DIGIT (FROM THE LEFT) OF INFO. */
/*   DIFF:    THE RELATIVE DIFFERENCES BETWEEN THE USER SUPPLIED AND */
/*            FINITE DIFFERENCE DERIVATIVES FOR EACH DERIVATIVE CHECKED. */
/*   FJACB:   THE JACOBIAN WITH RESPECT TO BETA. */
/*   FJACD:   THE JACOBIAN WITH RESPECT TO DELTA. */
/*   HEAD:    THE VARIABLE DESIGNATING WHETHER THE HEADING IS TO BE */
/*            PRINTED (HEAD=.TRUE.) OR NOT (HEAD=.FALSE.). */
/*   INFO:    THE VARIABLE DESIGNATING WHY THE COMPUTATIONS WERE STOPPED. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=.TRUE.) OR BY OLS (ISODR=.FALSE.). */
/*   LDSCLD:  THE LEADING DIMENSION OF ARRAY SCLD. */
/*   LDSTPD:  THE LEADING DIMENSION OF ARRAY STPD. */
/*   LDWD:    THE LEADING DIMENSION OF ARRAY WD. */
/*   LDWE:    THE LEADING DIMENSION OF ARRAY WE. */
/*   LD2WD:   THE SECOND DIMENSION OF ARRAY WD. */
/*   LD2WE:   THE SECOND DIMENSION OF ARRAY WE. */
/*   LIWKMN:  THE MINIMUM ACCEPTABLE LENGTH OF ARRAY IWORK. */
/*   LUNERR:  THE LOGICAL UNIT NUMBER USED FOR ERROR MESSAGES. */
/*   LWKMN:   THE MINIMUM ACCEPTABLE LENGTH OF ARRAY WORK. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   MSGB:    THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT BETA. */
/*   MSGD:    THE ERROR CHECKING RESULTS FOR THE JACOBIAN WRT DELTA. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NETA:    THE NUMBER OF RELIABLE DIGITS IN THE MODEL. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   NROW:    THE ROW NUMBER OF THE EXPLANATORY VARIABLE ARRAY AT */
/*            WHICH THE DERIVATIVE IS TO BE CHECKED. */
/*   NTOL:    THE NUMBER OF DIGITS OF AGREEMENT REQUIRED BETWEEN THE */
/*            FINITE DIFFERENCE AND THE USER SUPPLIED DERIVATIVES. */
/*   SHORT:   THE VARIABLE DESIGNATING WHETHER THE USER HAS INVOKED */
/*            ODRPACK BY THE SHORT-CALL (SHORT=.TRUE.) OR THE LONG-CALL */
/*            (SHORT=.FALSE.). */
/*   UNIT:    THE LOGICAL UNIT NUMBER FOR ERROR MESSAGES. */
/*   XPLUSD:  THE VALUES X + DELTA. */
/* ***FIRST EXECUTABLE STATEMENT  DODPER */
/*  SET LOGICAL UNIT NUMBER FOR ERROR REPORT */
    /* Parameter adjustments */
    xplusd_dim1 = *n;
    xplusd_offset = 1 + xplusd_dim1;
    xplusd -= xplusd_offset;
    --msgd;
    --msgb;
    diff_dim1 = *nq;
    diff_offset = 1 + diff_dim1;
    diff -= diff_offset;
    fjacd_dim1 = *n;
    fjacd_dim2 = *m;
    fjacd_offset = 1 + fjacd_dim1 * (1 + fjacd_dim2);
    fjacd -= fjacd_offset;
    fjacb_dim1 = *n;
    fjacb_dim2 = *np;
    fjacb_offset = 1 + fjacb_dim1 * (1 + fjacb_dim2);
    fjacb -= fjacb_offset;

    /* Function Body */
    if (*lunerr == 0) {
	return 0;
    } else if (*lunerr < 0) {
	unit = 6;
    } else {
	unit = *lunerr;
    }
/*  PRINT HEADING */
    head = TRUE_;
    dodphd_(&head, &unit);
/*  EXTRACT INDIVIDUAL DIGITS FROM VARIABLE INFO */
    d1 = *info % 100000 / 10000;
    d2 = *info % 10000 / 1000;
    d3 = *info % 1000 / 100;
    d4 = *info % 100 / 10;
    d5 = *info % 10;
/*  PRINT APPROPRIATE ERROR MESSAGES FOR ODRPACK INVOKED STOP */
    if (d1 >= 1 && d1 <= 3) {
/*  PRINT APPROPRIATE MESSAGES FOR ERRORS IN */
/*     PROBLEM SPECIFICATION PARAMETERS */
/*     DIMENSION SPECIFICATION PARAMETERS */
/*     NUMBER OF GOOD DIGITS IN X */
/*     WEIGHTS */
	dodpe1_(&unit, &d1, &d2, &d3, &d4, &d5, n, m, nq, ldscld, ldstpd, 
		ldwe, ld2we, ldwd, ld2wd, lwkmn, liwkmn);
    } else if (d1 == 4 || msgb[1] >= 0) {
/*  PRINT APPROPRIATE MESSAGES FOR DERIVATIVE CHECKING */
	dodpe2_(&unit, n, m, np, nq, &fjacb[fjacb_offset], &fjacd[
		fjacd_offset], &diff[diff_offset], &msgb[1], &msgb[2], isodr, 
		&msgd[1], &msgd[2], &xplusd[xplusd_offset], nrow, neta, ntol);
    } else if (d1 == 5) {
/*  PRINT APPROPRIATE ERROR MESSAGE FOR USER INVOKED STOP FROM FCN */
	dodpe3_(&unit, &d2, &d3);
    }
/*  PRINT CORRECT FORM OF CALL STATEMENT */
    if (d1 >= 1 && d1 <= 3 || d1 == 4 && (d2 == 2 || d3 == 2) || d1 == 5) {
	if (*short__) {
	    io___724.ciunit = unit;
	    s_wsfe(&io___724);
	    e_wsfe();
	} else {
	    io___725.ciunit = unit;
	    s_wsfe(&io___725);
	    e_wsfe();
	}
    }
    return 0;
/*  FORMAT STATEMENTS */
} /* dodper_ */

/* DODPHD */
/* Subroutine */ int dodphd_(logical *head, integer *unit)
{
    /* Format strings */
    static char fmt_1000[] = "(\002 ****************************************"
	    "*************** \002/\002 * ODRPACK VERSION 2.01 OF 06-19-92 (DO"
	    "UBLE PRECISION) * \002/\002 ************************************"
	    "******************* \002/)";

    /* Builtin functions */
    integer s_wsfe(cilist *), e_wsfe(void);

    /* Fortran I/O blocks */
    static cilist io___726 = { 0, 0, 0, fmt_1000, 0 };


/* ***BEGIN PROLOGUE  DODPHD */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  PRINT ODRPACK HEADING */
/* ***END PROLOGUE  DODPHD */
/* ...SCALAR ARGUMENTS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   HEAD:    THE VARIABLE DESIGNATING WHETHER THE HEADING IS TO BE */
/*            PRINTED (HEAD=.TRUE.) OR NOT (HEAD=.FALSE.). */
/*   UNIT:    THE LOGICAL UNIT NUMBER TO WHICH THE HEADING IS WRITTEN. */
/* ***FIRST EXECUTABLE STATEMENT  DODPHD */
    if (*head) {
	io___726.ciunit = *unit;
	s_wsfe(&io___726);
	e_wsfe();
	*head = FALSE_;
    }
    return 0;
/*   FORMAT STATEMENTS */
} /* dodphd_ */

/* DODSTP */
/* Subroutine */ int dodstp_(integer *n, integer *m, integer *np, integer *nq,
	 integer *npp, doublereal *f, doublereal *fjacb, doublereal *fjacd, 
	doublereal *wd, integer *ldwd, integer *ld2wd, doublereal *ss, 
	doublereal *tt, integer *ldtt, doublereal *delta, doublereal *alpha, 
	doublereal *epsfcn, logical *isodr, doublereal *tfjacb, doublereal *
	omega, doublereal *u, doublereal *qraux, integer *kpvt, doublereal *s,
	 doublereal *t, doublereal *phi, integer *irank, doublereal *rcond, 
	logical *forvcv, doublereal *wrk1, doublereal *wrk2, doublereal *wrk3,
	 doublereal *wrk4, doublereal *wrk5, doublereal *wrk, integer *lwrk, 
	integer *istopc)
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal one = 1.;

    /* System generated locals */
    integer delta_dim1, delta_offset, f_dim1, f_offset, fjacb_dim1, 
	    fjacb_dim2, fjacb_offset, fjacd_dim1, fjacd_dim2, fjacd_offset, 
	    omega_dim1, omega_offset, t_dim1, t_offset, tfjacb_dim1, 
	    tfjacb_dim2, tfjacb_offset, tt_dim1, tt_offset, wd_dim1, wd_dim2, 
	    wd_offset, wrk1_dim1, wrk1_dim2, wrk1_offset, wrk2_dim1, 
	    wrk2_offset, wrk4_dim1, wrk4_offset, i__1, i__2, i__3, i__4;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static integer i__, j, k, l, k1, k2;
    static doublereal co, si;
    static integer kp, inf;
    static doublereal dum[2];
    static logical elim;
    static integer imax;
    static doublereal temp;
    extern /* Subroutine */ int drot_(integer *, doublereal *, integer *, 
	    doublereal *, integer *, doublereal *, doublereal *);
    static integer ipvt;
    extern doublereal dnrm2_(integer *, doublereal *, integer *);
    extern /* Subroutine */ int dchex_(doublereal *, integer *, integer *, 
	    integer *, integer *, doublereal *, integer *, integer *, 
	    doublereal *, doublereal *, integer *), dqrdc_(doublereal *, 
	    integer *, integer *, integer *, doublereal *, integer *, 
	    doublereal *, integer *), dfctr_(logical *, doublereal *, integer 
	    *, integer *, integer *), dtrco_(doublereal *, integer *, integer 
	    *, doublereal *, doublereal *, integer *), dwght_(integer *, 
	    integer *, doublereal *, integer *, integer *, doublereal *, 
	    integer *, doublereal *, integer *), drotg_(doublereal *, 
	    doublereal *, doublereal *, doublereal *), dzero_(integer *, 
	    integer *, doublereal *, integer *), dqrsl_(doublereal *, integer 
	    *, integer *, integer *, doublereal *, doublereal *, doublereal *,
	     doublereal *, doublereal *, doublereal *, doublereal *, integer *
	    , integer *), dtrsl_(doublereal *, integer *, integer *, 
	    doublereal *, integer *, integer *);
    extern integer idamax_(integer *, doublereal *, integer *);
    extern /* Subroutine */ int desubi_(integer *, integer *, doublereal *, 
	    integer *, integer *, doublereal *, doublereal *, integer *, 
	    integer *, doublereal *), dsolve_(integer *, doublereal *, 
	    integer *, doublereal *, integer *, integer *), dvevtr_(integer *,
	     integer *, integer *, doublereal *, integer *, integer *, 
	    doublereal *, integer *, doublereal *, integer *, integer *, 
	    doublereal *, integer *, doublereal *);

/* ***BEGIN PROLOGUE  DODSTP */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  IDAMAX,DCHEX,DESUBI,DFCTR,DNRM2,DQRDC,DQRSL,DROT, */
/*                    DROTG,DSOLVE,DTRCO,DTRSL,DVEVTR,DWGHT,DZERO */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  COMPUTE LOCALLY CONSTRAINED STEPS S AND T, AND PHI(ALPHA) */
/* ***END PROLOGUE  DODSTP */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...LOCAL ARRAYS */
/* ...EXTERNAL FUNCTIONS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --wrk5;
    wrk4_dim1 = *m;
    wrk4_offset = 1 + wrk4_dim1;
    wrk4 -= wrk4_offset;
    t_dim1 = *n;
    t_offset = 1 + t_dim1;
    t -= t_offset;
    delta_dim1 = *n;
    delta_offset = 1 + delta_dim1;
    delta -= delta_offset;
    --wrk3;
    --s;
    --kpvt;
    --qraux;
    --u;
    --ss;
    wrk2_dim1 = *n;
    wrk2_offset = 1 + wrk2_dim1;
    wrk2 -= wrk2_offset;
    wrk1_dim1 = *n;
    wrk1_dim2 = *nq;
    wrk1_offset = 1 + wrk1_dim1 * (1 + wrk1_dim2);
    wrk1 -= wrk1_offset;
    omega_dim1 = *nq;
    omega_offset = 1 + omega_dim1;
    omega -= omega_offset;
    tfjacb_dim1 = *n;
    tfjacb_dim2 = *nq;
    tfjacb_offset = 1 + tfjacb_dim1 * (1 + tfjacb_dim2);
    tfjacb -= tfjacb_offset;
    fjacd_dim1 = *n;
    fjacd_dim2 = *m;
    fjacd_offset = 1 + fjacd_dim1 * (1 + fjacd_dim2);
    fjacd -= fjacd_offset;
    fjacb_dim1 = *n;
    fjacb_dim2 = *np;
    fjacb_offset = 1 + fjacb_dim1 * (1 + fjacb_dim2);
    fjacb -= fjacb_offset;
    f_dim1 = *n;
    f_offset = 1 + f_dim1;
    f -= f_offset;
    wd_dim1 = *ldwd;
    wd_dim2 = *ld2wd;
    wd_offset = 1 + wd_dim1 * (1 + wd_dim2);
    wd -= wd_offset;
    tt_dim1 = *ldtt;
    tt_offset = 1 + tt_dim1;
    tt -= tt_offset;
    --wrk;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ALPHA:   THE LEVENBERG-MARQUARDT PARAMETER. */
/*   CO:      THE COSINE FROM THE PLANE ROTATION. */
/*   DELTA:   THE ESTIMATED ERRORS IN THE EXPLANATORY VARIABLES. */
/*   DUM:     A DUMMY ARRAY. */
/*   ELIM:    THE VARIABLE DESIGNATING WHETHER COLUMNS OF THE JACOBIAN */
/*            WRT BETA HAVE BEEN ELIMINATED (ELIM=TRUE) OR NOT */
/*            (ELIM=FALSE). */
/*   EPSFCN:  THE FUNCTION'S PRECISION. */
/*   F:       THE (WEIGHTED) ESTIMATED VALUES OF EPSILON. */
/*   FJACB:   THE JACOBIAN WITH RESPECT TO BETA. */
/*   FJACD:   THE JACOBIAN WITH RESPECT TO DELTA. */
/*   FORVCV:  THE VARIABLE DESIGNATING WHETHER THIS SUBROUTINE WAS */
/*            CALLED TO SET UP FOR THE COVARIANCE MATRIX COMPUTATIONS */
/*            (FORVCV=TRUE) OR NOT (FORVCV=FALSE). */
/*   I:       AN INDEXING VARIABLE. */
/*   IMAX:    THE INDEX OF THE ELEMENT OF U HAVING THE LARGEST ABSOLUTE */
/*            VALUE. */
/*   INF:     THE RETURN CODE FROM LINPACK ROUTINES. */
/*   IPVT:    THE VARIABLE DESIGNATING WHETHER PIVOTING IS TO BE DONE. */
/*   IRANK:   THE RANK DEFICIENCY OF THE JACOBIAN WRT BETA. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR BY OLS (ISODR=FALSE). */
/*   ISTOPC:  THE VARIABLE DESIGNATING WHETHER THE COMPUTATIONS WERE */
/*            STOPED DUE TO A NUMERICAL ERROR WITHIN SUBROUTINE DODSTP. */
/*   J:       AN INDEXING VARIABLE. */
/*   K:       AN INDEXING VARIABLE. */
/*   K1:      AN INDEXING VARIABLE. */
/*   K2:      AN INDEXING VARIABLE. */
/*   KP:      THE RANK OF THE JACOBIAN WRT BETA. */
/*   KPVT:    THE PIVOT VECTOR. */
/*   L:       AN INDEXING VARIABLE. */
/*   LDTT:    THE LEADING DIMENSION OF ARRAY TT. */
/*   LDWD:    THE LEADING DIMENSION OF ARRAY WD. */
/*   LD2WD:   THE SECOND DIMENSION OF ARRAY WD. */
/*   LWRK:    THE LENGTH OF VECTOR WRK. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NPP:     THE NUMBER OF FUNCTION PARAMETERS BEING ESTIMATED. */
/*   OMEGA:   THE ARRAY DEFINED S.T. */
/*            OMEGA*TRANS(OMEGA) = INV(I+FJACD*INV(E)*TRANS(FJACD)) */
/*                               = (I-FJACD*INV(P)*TRANS(FJACD)) */
/*            WHERE E = D**2 + ALPHA*TT**2 */
/*                  P = TRANS(FJACD)*FJACD + D**2 + ALPHA*TT**2 */
/*   ONE:     THE VALUE 1.0D0. */
/*   PHI:     THE DIFFERENCE BETWEEN THE NORM OF THE SCALED STEP */
/*            AND THE TRUST REGION DIAMETER. */
/*   QRAUX:   THE ARRAY REQUIRED TO RECOVER THE ORTHOGONAL PART OF THE */
/*            Q-R DECOMPOSITION. */
/*   RCOND:   THE APPROXIMATE RECIPROCAL CONDITION NUMBER OF TFJACB. */
/*   S:       THE STEP FOR BETA. */
/*   SI:      THE SINE FROM THE PLANE ROTATION. */
/*   SS:      THE SCALING VALUES FOR THE UNFIXED BETAS. */
/*   T:       THE STEP FOR DELTA. */
/*   TEMP:    A TEMPORARY STORAGE LOCATION. */
/*   TFJACB:  THE ARRAY OMEGA*FJACB. */
/*   TT:      THE SCALING VALUES FOR DELTA. */
/*   U:       THE APPROXIMATE NULL VECTOR FOR TFJACB. */
/*   WD:      THE (SQUARED) DELTA WEIGHTS. */
/*   WRK:     A WORK ARRAY OF (LWRK) ELEMENTS, */
/*            EQUIVALENCED TO WRK1 AND WRK2. */
/*   WRK1:    A WORK ARRAY OF (N BY NQ BY M) ELEMENTS. */
/*   WRK2:    A WORK ARRAY OF (N BY NQ) ELEMENTS. */
/*   WRK3:    A WORK ARRAY OF (NP) ELEMENTS. */
/*   WRK4:    A WORK ARRAY OF (M BY M) ELEMENTS. */
/*   WRK5:    A WORK ARRAY OF (M) ELEMENTS. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DODSTP */
/*  COMPUTE LOOP PARAMETERS WHICH DEPEND ON WEIGHT STRUCTURE */
/*  SET UP KPVT IF ALPHA = 0 */
    if (*alpha == zero) {
	kp = *npp;
	i__1 = *np;
	for (k = 1; k <= i__1; ++k) {
	    kpvt[k] = k;
/* L10: */
	}
    } else {
	if (*npp >= 1) {
	    kp = *npp - *irank;
	} else {
	    kp = *npp;
	}
    }
    if (*isodr) {
/*  T = WD * DELTA = D*G2 */
	dwght_(n, m, &wd[wd_offset], ldwd, ld2wd, &delta[delta_offset], n, &t[
		t_offset], n);
	i__1 = *n;
	for (i__ = 1; i__ <= i__1; ++i__) {
/*  COMPUTE WRK4, SUCH THAT */
/*                TRANS(WRK4)*WRK4 = E = (D**2 + ALPHA*TT**2) */
	    desubi_(n, m, &wd[wd_offset], ldwd, ld2wd, alpha, &tt[tt_offset], 
		    ldtt, &i__, &wrk4[wrk4_offset]);
	    dfctr_(&c_false, &wrk4[wrk4_offset], m, m, &inf);
	    if (inf != 0) {
		*istopc = 60000;
		return 0;
	    }
/*  COMPUTE OMEGA, SUCH THAT */
/*                 TRANS(OMEGA)*OMEGA = I+FJACD*INV(E)*TRANS(FJACD) */
/*                 INV(TRANS(OMEGA)*OMEGA) = I-FJACD*INV(P)*TRANS(FJACD) */
	    dvevtr_(m, nq, &i__, &fjacd[fjacd_offset], n, m, &wrk4[
		    wrk4_offset], m, &wrk1[wrk1_offset], n, nq, &omega[
		    omega_offset], nq, &wrk5[1]);
	    i__2 = *nq;
	    for (l = 1; l <= i__2; ++l) {
		omega[l + l * omega_dim1] = one + omega[l + l * omega_dim1];
/* L110: */
	    }
	    dfctr_(&c_false, &omega[omega_offset], nq, nq, &inf);
	    if (inf != 0) {
		*istopc = 60000;
		return 0;
	    }
/*  COMPUTE WRK1 = TRANS(FJACD)*(I-FJACD*INV(P)*TRANS(JFACD)) */
/*               = TRANS(FJACD)*INV(TRANS(OMEGA)*OMEGA) */
	    i__2 = *m;
	    for (j = 1; j <= i__2; ++j) {
		i__3 = *nq;
		for (l = 1; l <= i__3; ++l) {
		    wrk1[i__ + (l + j * wrk1_dim2) * wrk1_dim1] = fjacd[i__ + 
			    (j + l * fjacd_dim2) * fjacd_dim1];
/* L120: */
		}
		dsolve_(nq, &omega[omega_offset], nq, &wrk1[i__ + (j * 
			wrk1_dim2 + 1) * wrk1_dim1], n, &c__4);
		dsolve_(nq, &omega[omega_offset], nq, &wrk1[i__ + (j * 
			wrk1_dim2 + 1) * wrk1_dim1], n, &c__2);
/* L130: */
	    }
/*  COMPUTE WRK5 = INV(E)*D*G2 */
	    i__2 = *m;
	    for (j = 1; j <= i__2; ++j) {
		wrk5[j] = t[i__ + j * t_dim1];
/* L140: */
	    }
	    dsolve_(m, &wrk4[wrk4_offset], m, &wrk5[1], &c__1, &c__4);
	    dsolve_(m, &wrk4[wrk4_offset], m, &wrk5[1], &c__1, &c__2);
/*  COMPUTE TFJACB = INV(TRANS(OMEGA))*FJACB */
	    i__2 = kp;
	    for (k = 1; k <= i__2; ++k) {
		i__3 = *nq;
		for (l = 1; l <= i__3; ++l) {
		    tfjacb[i__ + (l + k * tfjacb_dim2) * tfjacb_dim1] = fjacb[
			    i__ + (kpvt[k] + l * fjacb_dim2) * fjacb_dim1];
/* L150: */
		}
		dsolve_(nq, &omega[omega_offset], nq, &tfjacb[i__ + (k * 
			tfjacb_dim2 + 1) * tfjacb_dim1], n, &c__4);
		i__3 = *nq;
		for (l = 1; l <= i__3; ++l) {
		    if (ss[1] > zero) {
			tfjacb[i__ + (l + k * tfjacb_dim2) * tfjacb_dim1] /= 
				ss[kpvt[k]];
		    } else {
			tfjacb[i__ + (l + k * tfjacb_dim2) * tfjacb_dim1] /= 
				abs(ss[1]);
		    }
/* L160: */
		}
/* L170: */
	    }
/*  COMPUTE WRK2 = (V*INV(E)*D**2*G2 - G1) */
	    i__2 = *nq;
	    for (l = 1; l <= i__2; ++l) {
		wrk2[i__ + l * wrk2_dim1] = zero;
		i__3 = *m;
		for (j = 1; j <= i__3; ++j) {
		    wrk2[i__ + l * wrk2_dim1] += fjacd[i__ + (j + l * 
			    fjacd_dim2) * fjacd_dim1] * wrk5[j];
/* L180: */
		}
		wrk2[i__ + l * wrk2_dim1] -= f[i__ + l * f_dim1];
/* L190: */
	    }
/*  COMPUTE WRK2 = INV(TRANS(OMEGA))*(V*INV(E)*D**2*G2 - G1) */
	    dsolve_(nq, &omega[omega_offset], nq, &wrk2[i__ + wrk2_dim1], n, &
		    c__4);
/* L300: */
	}
    } else {
	i__1 = *n;
	for (i__ = 1; i__ <= i__1; ++i__) {
	    i__2 = *nq;
	    for (l = 1; l <= i__2; ++l) {
		i__3 = kp;
		for (k = 1; k <= i__3; ++k) {
		    tfjacb[i__ + (l + k * tfjacb_dim2) * tfjacb_dim1] = fjacb[
			    i__ + (kpvt[k] + l * fjacb_dim2) * fjacb_dim1];
		    if (ss[1] > zero) {
			tfjacb[i__ + (l + k * tfjacb_dim2) * tfjacb_dim1] /= 
				ss[kpvt[k]];
		    } else {
			tfjacb[i__ + (l + k * tfjacb_dim2) * tfjacb_dim1] /= 
				abs(ss[1]);
		    }
/* L340: */
		}
		wrk2[i__ + l * wrk2_dim1] = -f[i__ + l * f_dim1];
/* L350: */
	    }
/* L360: */
	}
    }
/*  COMPUTE S */
/*  DO QR FACTORIZATION (WITH COLUMN PIVOTING OF TRJACB IF ALPHA = 0) */
    if (*alpha == zero) {
	ipvt = 1;
	i__1 = *np;
	for (k = 1; k <= i__1; ++k) {
	    kpvt[k] = 0;
/* L410: */
	}
    } else {
	ipvt = 0;
    }
    i__1 = *n * *nq;
    i__2 = *n * *nq;
    dqrdc_(&tfjacb[tfjacb_offset], &i__1, &i__2, &kp, &qraux[1], &kpvt[1], &
	    wrk3[1], &ipvt);
    i__1 = *n * *nq;
    i__2 = *n * *nq;
    dqrsl_(&tfjacb[tfjacb_offset], &i__1, &i__2, &kp, &qraux[1], &wrk2[
	    wrk2_offset], dum, &wrk2[wrk2_offset], dum, dum, dum, &c__1000, &
	    inf);
    if (inf != 0) {
	*istopc = 60000;
	return 0;
    }
/*  ELIMINATE ALPHA PART USING GIVENS ROTATIONS */
    if (*alpha != zero) {
	dzero_(npp, &c__1, &s[1], npp);
	i__1 = kp;
	for (k1 = 1; k1 <= i__1; ++k1) {
	    dzero_(&kp, &c__1, &wrk3[1], &kp);
	    wrk3[k1] = sqrt(*alpha);
	    i__2 = kp;
	    for (k2 = k1; k2 <= i__2; ++k2) {
		drotg_(&tfjacb[k2 + (k2 * tfjacb_dim2 + 1) * tfjacb_dim1], &
			wrk3[k2], &co, &si);
		if (kp - k2 >= 1) {
		    i__3 = kp - k2;
		    i__4 = *n * *nq;
		    drot_(&i__3, &tfjacb[k2 + ((k2 + 1) * tfjacb_dim2 + 1) * 
			    tfjacb_dim1], &i__4, &wrk3[k2 + 1], &c__1, &co, &
			    si);
		}
		temp = co * wrk2[k2 + wrk2_dim1] + si * s[kpvt[k1]];
		s[kpvt[k1]] = -si * wrk2[k2 + wrk2_dim1] + co * s[kpvt[k1]];
		wrk2[k2 + wrk2_dim1] = temp;
/* L420: */
	    }
/* L430: */
	}
    }
/*  COMPUTE SOLUTION - ELIMINATE VARIABLES IF NECESSARY */
    if (*npp >= 1) {
	if (*alpha == zero) {
	    kp = *npp;
/*  ESTIMATE RCOND - U WILL CONTAIN APPROX NULL VECTOR */
L440:
	    i__1 = *n * *nq;
	    dtrco_(&tfjacb[tfjacb_offset], &i__1, &kp, rcond, &u[1], &c__1);
	    if (*rcond <= *epsfcn) {
		elim = TRUE_;
		imax = idamax_(&kp, &u[1], &c__1);
/* IMAX IS THE COLUMN TO REMOVE - USE DCHEX AND FIX KPVT */
		if (imax != kp) {
		    i__1 = *n * *nq;
		    i__2 = *n * *nq;
		    dchex_(&tfjacb[tfjacb_offset], &i__1, &kp, &imax, &kp, &
			    wrk2[wrk2_offset], &i__2, &c__1, &qraux[1], &wrk3[
			    1], &c__2);
		    k = kpvt[imax];
		    i__1 = kp - 1;
		    for (i__ = imax; i__ <= i__1; ++i__) {
			kpvt[i__] = kpvt[i__ + 1];
/* L450: */
		    }
		    kpvt[kp] = k;
		}
		--kp;
	    } else {
		elim = FALSE_;
	    }
	    if (elim && kp >= 1) {
		goto L440;
	    } else {
		*irank = *npp - kp;
	    }
	}
    }
    if (*forvcv) {
	return 0;
    }
/*  BACKSOLVE AND UNSCRAMBLE */
    if (*npp >= 1) {
	i__1 = *npp;
	for (i__ = kp + 1; i__ <= i__1; ++i__) {
	    wrk2[i__ + wrk2_dim1] = zero;
/* L510: */
	}
	if (kp >= 1) {
	    i__1 = *n * *nq;
	    dtrsl_(&tfjacb[tfjacb_offset], &i__1, &kp, &wrk2[wrk2_offset], &
		    c__1, &inf);
	    if (inf != 0) {
		*istopc = 60000;
		return 0;
	    }
	}
	i__1 = *npp;
	for (i__ = 1; i__ <= i__1; ++i__) {
	    if (ss[1] > zero) {
		s[kpvt[i__]] = wrk2[i__ + wrk2_dim1] / ss[kpvt[i__]];
	    } else {
		s[kpvt[i__]] = wrk2[i__ + wrk2_dim1] / abs(ss[1]);
	    }
/* L520: */
	}
    }
    if (*isodr) {
/*  NOTE: T AND WRK1 HAVE BEEN INITIALIZED ABOVE, */
/*        WHERE T    = WD * DELTA = D*G2 */
/*              WRK1 = TRANS(FJACD)*(I-FJACD*INV(P)*TRANS(JFACD)) */
	i__1 = *n;
	for (i__ = 1; i__ <= i__1; ++i__) {
/*  COMPUTE WRK4, SUCH THAT */
/*                TRANS(WRK4)*WRK4 = E = (D**2 + ALPHA*TT**2) */
	    desubi_(n, m, &wd[wd_offset], ldwd, ld2wd, alpha, &tt[tt_offset], 
		    ldtt, &i__, &wrk4[wrk4_offset]);
	    dfctr_(&c_false, &wrk4[wrk4_offset], m, m, &inf);
	    if (inf != 0) {
		*istopc = 60000;
		return 0;
	    }
/*  COMPUTE WRK5 = INV(E)*D*G2 */
	    i__2 = *m;
	    for (j = 1; j <= i__2; ++j) {
		wrk5[j] = t[i__ + j * t_dim1];
/* L610: */
	    }
	    dsolve_(m, &wrk4[wrk4_offset], m, &wrk5[1], &c__1, &c__4);
	    dsolve_(m, &wrk4[wrk4_offset], m, &wrk5[1], &c__1, &c__2);
	    i__2 = *nq;
	    for (l = 1; l <= i__2; ++l) {
		wrk2[i__ + l * wrk2_dim1] = f[i__ + l * f_dim1];
		i__3 = *npp;
		for (k = 1; k <= i__3; ++k) {
		    wrk2[i__ + l * wrk2_dim1] += fjacb[i__ + (k + l * 
			    fjacb_dim2) * fjacb_dim1] * s[k];
/* L620: */
		}
		i__3 = *m;
		for (j = 1; j <= i__3; ++j) {
		    wrk2[i__ + l * wrk2_dim1] -= fjacd[i__ + (j + l * 
			    fjacd_dim2) * fjacd_dim1] * wrk5[j];
/* L630: */
		}
/* L640: */
	    }
	    i__2 = *m;
	    for (j = 1; j <= i__2; ++j) {
		wrk5[j] = zero;
		i__3 = *nq;
		for (l = 1; l <= i__3; ++l) {
		    wrk5[j] += wrk1[i__ + (l + j * wrk1_dim2) * wrk1_dim1] * 
			    wrk2[i__ + l * wrk2_dim1];
/* L650: */
		}
		t[i__ + j * t_dim1] = -(wrk5[j] + t[i__ + j * t_dim1]);
/* L660: */
	    }
	    dsolve_(m, &wrk4[wrk4_offset], m, &t[i__ + t_dim1], n, &c__4);
	    dsolve_(m, &wrk4[wrk4_offset], m, &t[i__ + t_dim1], n, &c__2);
/* L670: */
	}
    }
/*  COMPUTE PHI(ALPHA) FROM SCALED S AND T */
    dwght_(npp, &c__1, &ss[1], npp, &c__1, &s[1], npp, &wrk[1], npp);
    if (*isodr) {
	dwght_(n, m, &tt[tt_offset], ldtt, &c__1, &t[t_offset], n, &wrk[*npp 
		+ 1], n);
	i__1 = *npp + *n * *m;
	*phi = dnrm2_(&i__1, &wrk[1], &c__1);
    } else {
	*phi = dnrm2_(npp, &wrk[1], &c__1);
    }
    return 0;
} /* dodstp_ */

/* DODVCV */
/* Subroutine */ int dodvcv_(integer *n, integer *m, integer *np, integer *nq,
	 integer *npp, doublereal *f, doublereal *fjacb, doublereal *fjacd, 
	doublereal *wd, integer *ldwd, integer *ld2wd, doublereal *ssf, 
	doublereal *ss, doublereal *tt, integer *ldtt, doublereal *delta, 
	doublereal *epsfcn, logical *isodr, doublereal *vcv, doublereal *sd, 
	doublereal *wrk6, doublereal *omega, doublereal *u, doublereal *qraux,
	 integer *jpvt, doublereal *s, doublereal *t, integer *irank, 
	doublereal *rcond, doublereal *rss, integer *idf, doublereal *rvar, 
	integer *ifixb, doublereal *wrk1, doublereal *wrk2, doublereal *wrk3, 
	doublereal *wrk4, doublereal *wrk5, doublereal *wrk, integer *lwrk, 
	integer *istopc)
{
    /* Initialized data */

    static doublereal zero = 0.;

    /* System generated locals */
    integer delta_dim1, delta_offset, f_dim1, f_offset, fjacb_dim1, 
	    fjacb_dim2, fjacb_offset, fjacd_dim1, fjacd_dim2, fjacd_offset, 
	    omega_dim1, omega_offset, t_dim1, t_offset, tt_dim1, tt_offset, 
	    vcv_dim1, vcv_offset, wd_dim1, wd_dim2, wd_offset, wrk1_dim1, 
	    wrk1_dim2, wrk1_offset, wrk2_dim1, wrk2_offset, wrk4_dim1, 
	    wrk4_offset, wrk6_dim1, wrk6_offset, i__1, i__2, i__3;

    /* Builtin functions */
    double sqrt(doublereal);

    /* Local variables */
    static integer i__, j, l, kp;
    static doublereal temp;
    extern /* Subroutine */ int dpodi_(doublereal *, integer *, integer *, 
	    doublereal *, integer *), dodstp_(integer *, integer *, integer *,
	     integer *, integer *, doublereal *, doublereal *, doublereal *, 
	    doublereal *, integer *, integer *, doublereal *, doublereal *, 
	    integer *, doublereal *, doublereal *, doublereal *, logical *, 
	    doublereal *, doublereal *, doublereal *, doublereal *, integer *,
	     doublereal *, doublereal *, doublereal *, integer *, doublereal *
	    , logical *, doublereal *, doublereal *, doublereal *, doublereal 
	    *, doublereal *, doublereal *, integer *, integer *);
    static integer iunfix, junfix;
    static logical forvcv;

/* ***BEGIN PROLOGUE  DODVCV */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DPODI,DODSTP */
/* ***DATE WRITTEN   901207   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  COMPUTE COVARIANCE MATRIX OF ESTIMATED PARAMETERS */
/* ***END PROLOGUE  DODVCV */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --wrk5;
    wrk4_dim1 = *m;
    wrk4_offset = 1 + wrk4_dim1;
    wrk4 -= wrk4_offset;
    t_dim1 = *n;
    t_offset = 1 + t_dim1;
    t -= t_offset;
    delta_dim1 = *n;
    delta_offset = 1 + delta_dim1;
    delta -= delta_offset;
    --wrk3;
    --ifixb;
    --s;
    --jpvt;
    --qraux;
    --u;
    --sd;
    vcv_dim1 = *np;
    vcv_offset = 1 + vcv_dim1;
    vcv -= vcv_offset;
    --ss;
    --ssf;
    wrk2_dim1 = *n;
    wrk2_offset = 1 + wrk2_dim1;
    wrk2 -= wrk2_offset;
    wrk1_dim1 = *n;
    wrk1_dim2 = *nq;
    wrk1_offset = 1 + wrk1_dim1 * (1 + wrk1_dim2);
    wrk1 -= wrk1_offset;
    omega_dim1 = *nq;
    omega_offset = 1 + omega_dim1;
    omega -= omega_offset;
    wrk6_dim1 = *n * *nq;
    wrk6_offset = 1 + wrk6_dim1;
    wrk6 -= wrk6_offset;
    fjacd_dim1 = *n;
    fjacd_dim2 = *m;
    fjacd_offset = 1 + fjacd_dim1 * (1 + fjacd_dim2);
    fjacd -= fjacd_offset;
    fjacb_dim1 = *n;
    fjacb_dim2 = *np;
    fjacb_offset = 1 + fjacb_dim1 * (1 + fjacb_dim2);
    fjacb -= fjacb_offset;
    f_dim1 = *n;
    f_offset = 1 + f_dim1;
    f -= f_offset;
    wd_dim1 = *ldwd;
    wd_dim2 = *ld2wd;
    wd_offset = 1 + wd_dim1 * (1 + wd_dim2);
    wd -= wd_offset;
    tt_dim1 = *ldtt;
    tt_offset = 1 + tt_dim1;
    tt -= tt_offset;
    --wrk;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   DELTA:   THE ESTIMATED ERRORS IN THE EXPLANATORY VARIABLES. */
/*   EPSFCN:  THE FUNCTION'S PRECISION. */
/*   F:       THE (WEIGHTED) ESTIMATED VALUES OF EPSILON. */
/*   FJACB:   THE JACOBIAN WITH RESPECT TO BETA. */
/*   FJACD:   THE JACOBIAN WITH RESPECT TO DELTA. */
/*   FORVCV:  THE VARIABLE DESIGNATING WHETHER SUBROUTINE DODSTP IS */
/*            CALLED TO SET UP FOR THE COVARIANCE MATRIX COMPUTATIONS */
/*            (FORVCV=TRUE) OR NOT (FORVCV=FALSE). */
/*   I:       AN INDEXING VARIABLE. */
/*   IDF:     THE DEGREES OF FREEDOM OF THE FIT, EQUAL TO THE NUMBER OF */
/*            OBSERVATIONS WITH NONZERO WEIGHTED DERIVATIVES MINUS THE */
/*            NUMBER OF PARAMETERS BEING ESTIMATED. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IMAX:    THE INDEX OF THE ELEMENT OF U HAVING THE LARGEST ABSOLUTE */
/*            VALUE. */
/*   IRANK:   THE RANK DEFICIENCY OF THE JACOBIAN WRT BETA. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR BY OLS (ISODR=FALSE). */
/*   ISTOPC:  THE VARIABLE DESIGNATING WHETHER THE COMPUTATIONS WERE */
/*            STOPED DUE TO A NUMERICAL ERROR WITHIN SUBROUTINE DODSTP. */
/*   IUNFIX:  THE INDEX OF THE NEXT UNFIXED PARAMETER. */
/*   J:       AN INDEXING VARIABLE. */
/*   JPVT:    THE PIVOT VECTOR. */
/*   JUNFIX:  THE INDEX OF THE NEXT UNFIXED PARAMETER. */
/*   KP:      THE RANK OF THE JACOBIAN WRT BETA. */
/*   L:       AN INDEXING VARIABLE. */
/*   LDTT:    THE LEADING DIMENSION OF ARRAY TT. */
/*   LDWD:    THE LEADING DIMENSION OF ARRAY WD. */
/*   LD2WD:   THE SECOND DIMENSION OF ARRAY WD. */
/*   LWRK:    THE LENGTH OF VECTOR WRK. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NPP:     THE NUMBER OF FUNCTION PARAMETERS BEING ESTIMATED. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   OMEGA:   THE ARRAY DEFINED S.T. */
/*            OMEGA*TRANS(OMEGA) = INV(I+FJACD*INV(E)*TRANS(FJACD)) */
/*                               = (I-FJACD*INV(P)*TRANS(FJACD)) */
/*            WHERE E = D**2 + ALPHA*TT**2 */
/*                  P = TRANS(FJACD)*FJACD + D**2 + ALPHA*TT**2 */
/*   QRAUX:   THE ARRAY REQUIRED TO RECOVER THE ORTHOGONAL PART OF THE */
/*            Q-R DECOMPOSITION. */
/*   RCOND:   THE APPROXIMATE RECIPROCAL CONDITION OF FJACB. */
/*   RSS:     THE RESIDUAL SUM OF SQUARES. */
/*   RVAR:    THE RESIDUAL VARIANCE. */
/*   S:       THE STEP FOR BETA. */
/*   SD:      THE STANDARD DEVIATIONS OF THE ESTIMATED BETAS. */
/*   SS:      THE SCALING VALUES FOR THE UNFIXED BETAS. */
/*   SSF:     THE SCALING VALUES USED FOR BETA. */
/*   T:       THE STEP FOR DELTA. */
/*   TEMP:    A TEMPORARY STORAGE LOCATION */
/*   TT:      THE SCALING VALUES FOR DELTA. */
/*   U:       THE APPROXIMATE NULL VECTOR FOR FJACB. */
/*   VCV:     THE COVARIANCE MATRIX OF THE ESTIMATED BETAS. */
/*   WD:      THE DELTA WEIGHTS. */
/*   WRK:     A WORK ARRAY OF (LWRK) ELEMENTS, */
/*            EQUIVALENCED TO WRK1 AND WRK2. */
/*   WRK1:    A WORK ARRAY OF (N BY NQ BY M) ELEMENTS. */
/*   WRK2:    A WORK ARRAY OF (N BY NQ) ELEMENTS. */
/*   WRK3:    A WORK ARRAY OF (NP) ELEMENTS. */
/*   WRK4:    A WORK ARRAY OF (M BY M) ELEMENTS. */
/*   WRK5:    A WORK ARRAY OF (M) ELEMENTS. */
/*   WRK6:    A WORK ARRAY OF (N*NQ BY P) ELEMENTS. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DODVCV */
    forvcv = TRUE_;
    *istopc = 0;
    dodstp_(n, m, np, nq, npp, &f[f_offset], &fjacb[fjacb_offset], &fjacd[
	    fjacd_offset], &wd[wd_offset], ldwd, ld2wd, &ss[1], &tt[tt_offset]
	    , ldtt, &delta[delta_offset], &zero, epsfcn, isodr, &wrk6[
	    wrk6_offset], &omega[omega_offset], &u[1], &qraux[1], &jpvt[1], &
	    s[1], &t[t_offset], &temp, irank, rcond, &forvcv, &wrk1[
	    wrk1_offset], &wrk2[wrk2_offset], &wrk3[1], &wrk4[wrk4_offset], &
	    wrk5[1], &wrk[1], lwrk, istopc);
    if (*istopc != 0) {
	return 0;
    }
    kp = *npp - *irank;
    i__1 = *n * *nq;
    dpodi_(&wrk6[wrk6_offset], &i__1, &kp, &wrk3[1], &c__1);
    *idf = 0;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	i__2 = *npp;
	for (j = 1; j <= i__2; ++j) {
	    i__3 = *nq;
	    for (l = 1; l <= i__3; ++l) {
		if (fjacb[i__ + (j + l * fjacb_dim2) * fjacb_dim1] != zero) {
		    ++(*idf);
		    goto L150;
		}
/* L110: */
	    }
/* L120: */
	}
	if (*isodr) {
	    i__2 = *m;
	    for (j = 1; j <= i__2; ++j) {
		i__3 = *nq;
		for (l = 1; l <= i__3; ++l) {
		    if (fjacd[i__ + (j + l * fjacd_dim2) * fjacd_dim1] != 
			    zero) {
			++(*idf);
			goto L150;
		    }
/* L130: */
		}
/* L140: */
	    }
	}
L150:
	;
    }
    if (*idf > kp) {
	*idf -= kp;
	*rvar = *rss / *idf;
    } else {
	*idf = 0;
	*rvar = *rss;
    }
/*  STORE VARIANCES IN SD, RESTORING ORIGINAL ORDER */
    i__1 = *np;
    for (i__ = 1; i__ <= i__1; ++i__) {
	sd[i__] = zero;
/* L200: */
    }
    i__1 = kp;
    for (i__ = 1; i__ <= i__1; ++i__) {
	sd[jpvt[i__]] = wrk6[i__ + i__ * wrk6_dim1];
/* L210: */
    }
    if (*np > *npp) {
	junfix = *npp;
	for (j = *np; j >= 1; --j) {
	    if (ifixb[j] == 0) {
		sd[j] = zero;
	    } else {
		sd[j] = sd[junfix];
		--junfix;
	    }
/* L220: */
	}
    }
/*  STORE COVARIANCE MATRIX IN VCV, RESTORING ORIGINAL ORDER */
    i__1 = *np;
    for (i__ = 1; i__ <= i__1; ++i__) {
	i__2 = i__;
	for (j = 1; j <= i__2; ++j) {
	    vcv[i__ + j * vcv_dim1] = zero;
/* L300: */
	}
/* L310: */
    }
    i__1 = kp;
    for (i__ = 1; i__ <= i__1; ++i__) {
	i__2 = kp;
	for (j = i__ + 1; j <= i__2; ++j) {
	    if (jpvt[i__] > jpvt[j]) {
		vcv[jpvt[i__] + jpvt[j] * vcv_dim1] = wrk6[i__ + j * 
			wrk6_dim1];
	    } else {
		vcv[jpvt[j] + jpvt[i__] * vcv_dim1] = wrk6[i__ + j * 
			wrk6_dim1];
	    }
/* L320: */
	}
/* L330: */
    }
    if (*np > *npp) {
	iunfix = *npp;
	for (i__ = *np; i__ >= 1; --i__) {
	    if (ifixb[i__] == 0) {
		for (j = i__; j >= 1; --j) {
		    vcv[i__ + j * vcv_dim1] = zero;
/* L340: */
		}
	    } else {
		junfix = *npp;
		for (j = *np; j >= 1; --j) {
		    if (ifixb[j] == 0) {
			vcv[i__ + j * vcv_dim1] = zero;
		    } else {
			vcv[i__ + j * vcv_dim1] = vcv[iunfix + junfix * 
				vcv_dim1];
			--junfix;
		    }
/* L350: */
		}
		--iunfix;
	    }
/* L360: */
	}
    }
    i__1 = *np;
    for (i__ = 1; i__ <= i__1; ++i__) {
	vcv[i__ + i__ * vcv_dim1] = sd[i__];
	sd[i__] = sqrt(*rvar * sd[i__]);
	i__2 = i__;
	for (j = 1; j <= i__2; ++j) {
	    vcv[j + i__ * vcv_dim1] = vcv[i__ + j * vcv_dim1];
/* L370: */
	}
/* L380: */
    }
/*  UNSCALE STANDARD ERRORS AND COVARIANCE MATRIX */
    i__1 = *np;
    for (i__ = 1; i__ <= i__1; ++i__) {
	if (ssf[1] > zero) {
	    sd[i__] /= ssf[i__];
	} else {
	    sd[i__] /= abs(ssf[1]);
	}
	i__2 = *np;
	for (j = 1; j <= i__2; ++j) {
	    if (ssf[1] > zero) {
		vcv[i__ + j * vcv_dim1] /= ssf[i__] * ssf[j];
	    } else {
		vcv[i__ + j * vcv_dim1] /= ssf[1] * ssf[1];
	    }
/* L400: */
	}
/* L410: */
    }
    return 0;
} /* dodvcv_ */

/* DPACK */
/* Subroutine */ int dpack_(integer *n2, integer *n1, doublereal *v1, 
	doublereal *v2, integer *ifix)
{
    /* System generated locals */
    integer i__1;

    /* Local variables */
    static integer i__;
    extern /* Subroutine */ int dcopy_(integer *, doublereal *, integer *, 
	    doublereal *, integer *);

/* ***BEGIN PROLOGUE  DPACK */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DCOPY */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  SELECT THE UNFIXED ELEMENTS OF V2 AND RETURN THEM IN V1 */
/* ***END PROLOGUE  DPACK */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   I:       AN INDEXING VARIABLE. */
/*   IFIX:    THE VALUES DESIGNATING WHETHER THE ELEMENTS OF V2 ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   N1:      THE NUMBER OF ITEMS IN V1. */
/*   N2:      THE NUMBER OF ITEMS IN V2. */
/*   V1:      THE VECTOR OF THE UNFIXED ITEMS FROM V2. */
/*   V2:      THE VECTOR OF THE FIXED AND UNFIXED ITEMS FROM WHICH THE */
/*            UNFIXED ELEMENTS ARE TO BE EXTRACTED. */
/* ***FIRST EXECUTABLE STATEMENT  DPACK */
    /* Parameter adjustments */
    --ifix;
    --v2;
    --v1;

    /* Function Body */
    *n1 = 0;
    if (ifix[1] >= 0) {
	i__1 = *n2;
	for (i__ = 1; i__ <= i__1; ++i__) {
	    if (ifix[i__] != 0) {
		++(*n1);
		v1[*n1] = v2[i__];
	    }
/* L10: */
	}
    } else {
	*n1 = *n2;
	dcopy_(n2, &v2[1], &c__1, &v1[1], &c__1);
    }
    return 0;
} /* dpack_ */

/* DPPNML */
doublereal dppnml_(doublereal *p)
{
    /* Initialized data */

    static doublereal p0 = -.322232431088;
    static doublereal p1 = -1.;
    static doublereal p2 = -.342242088547;
    static doublereal p3 = -.0204231210245;
    static doublereal p4 = -4.53642210148e-5;
    static doublereal q0 = .099348462606;
    static doublereal q1 = .588581570495;
    static doublereal q2 = .531103462366;
    static doublereal q3 = .10353775285;
    static doublereal q4 = .0038560700634;
    static doublereal zero = 0.;
    static doublereal half = .5;
    static doublereal one = 1.;
    static doublereal two = 2.;

    /* System generated locals */
    doublereal ret_val;

    /* Builtin functions */
    double log(doublereal), sqrt(doublereal);

    /* Local variables */
    static doublereal r__, t, aden, anum;

/* ***BEGIN PROLOGUE  DPPNML */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   901207   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***AUTHOR  FILLIBEN, JAMES J., */
/*             STATISTICAL ENGINEERING DIVISION */
/*             NATIONAL BUREAU OF STANDARDS */
/*             WASHINGTON, D. C. 20234 */
/*             (ORIGINAL VERSION--JUNE      1972. */
/*             (UPDATED         --SEPTEMBER 1975, */
/*                                NOVEMBER  1975, AND */
/*                                OCTOBER   1976. */
/* ***PURPOSE  COMPUTE THE PERCENT POINT FUNCTION VALUE FOR THE */
/*            NORMAL (GAUSSIAN) DISTRIBUTION WITH MEAN 0 AND STANDARD */
/*            DEVIATION 1, AND WITH PROBABILITY DENSITY FUNCTION */
/*            F(X) = (1/SQRT(2*PI))*EXP(-X*X/2). */
/*            (ADAPTED FROM DATAPAC SUBROUTINE TPPF, WITH MODIFICATIONS */
/*            TO FACILITATE CONVERSION TO DOUBLE PRECISION AUTOMATICALLY) */
/* ***DESCRIPTION */
/*               --THE CODING AS PRESENTED BELOW IS ESSENTIALLY */
/*                 IDENTICAL TO THAT PRESENTED BY ODEH AND EVANS */
/*                 AS ALGORTIHM 70 OF APPLIED STATISTICS. */
/*               --AS POINTED OUT BY ODEH AND EVANS IN APPLIED */
/*                 STATISTICS, THEIR ALGORITHM REPRESENTES A */
/*                 SUBSTANTIAL IMPROVEMENT OVER THE PREVIOUSLY EMPLOYED */
/*                 HASTINGS APPROXIMATION FOR THE NORMAL PERCENT POINT */
/*                 FUNCTION, WITH ACCURACY IMPROVING FROM 4.5*(10**-4) */
/*                 TO 1.5*(10**-8). */
/* ***REFERENCES  ODEH AND EVANS, THE PERCENTAGE POINTS OF THE NORMAL */
/*                 DISTRIBUTION, ALGORTIHM 70, APPLIED STATISTICS, 1974, */
/*                 PAGES 96-97. */
/*               EVANS, ALGORITHMS FOR MINIMAL DEGREE POLYNOMIAL AND */
/*                 RATIONAL APPROXIMATION, M. SC. THESIS, 1972, */
/*                 UNIVERSITY OF VICTORIA, B. C., CANADA. */
/*               HASTINGS, APPROXIMATIONS FOR DIGITAL COMPUTERS, 1955, */
/*                 PAGES 113, 191, 192. */
/*               NATIONAL BUREAU OF STANDARDS APPLIED MATHEMATICS */
/*                 SERIES 55, 1964, PAGE 933, FORMULA 26.2.23. */
/*               FILLIBEN, SIMPLE AND ROBUST LINEAR ESTIMATION OF THE */
/*                 LOCATION PARAMETER OF A SYMMETRIC DISTRIBUTION */
/*                 (UNPUBLISHED PH.D. DISSERTATION, PRINCETON */
/*                 UNIVERSITY), 1969, PAGES 21-44, 229-231. */
/*               FILLIBEN, "THE PERCENT POINT FUNCTION", */
/*                 (UNPUBLISHED MANUSCRIPT), 1970, PAGES 28-31. */
/*               JOHNSON AND KOTZ, CONTINUOUS UNIVARIATE DISTRIBUTIONS, */
/*                 VOLUME 1, 1970, PAGES 40-111. */
/*               KELLEY STATISTICAL TABLES, 1948. */
/*               OWEN, HANDBOOK OF STATISTICAL TABLES, 1962, PAGES 3-16. */
/*               PEARSON AND HARTLEY, BIOMETRIKA TABLES FOR */
/*                 STATISTICIANS, VOLUME 1, 1954, PAGES 104-113. */
/* ***END PROLOGUE  DPPNML */
/* ...SCALAR ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ADEN:    A VALUE USED IN THE APPROXIMATION. */
/*   ANUM:    A VALUE USED IN THE APPROXIMATION. */
/*   HALF:    THE VALUE 0.5D0. */
/*   ONE:     THE VALUE 1.0D0. */
/*   P:       THE PROBABILITY AT WHICH THE PERCENT POINT IS TO BE */
/*            EVALUATED.  P MUST BE BETWEEN 0.0D0 AND 1.0D0, EXCLUSIVE. */
/*   P0:      A PARAMETER USED IN THE APPROXIMATION. */
/*   P1:      A PARAMETER USED IN THE APPROXIMATION. */
/*   P2:      A PARAMETER USED IN THE APPROXIMATION. */
/*   P3:      A PARAMETER USED IN THE APPROXIMATION. */
/*   P4:      A PARAMETER USED IN THE APPROXIMATION. */
/*   Q0:      A PARAMETER USED IN THE APPROXIMATION. */
/*   Q1:      A PARAMETER USED IN THE APPROXIMATION. */
/*   Q2:      A PARAMETER USED IN THE APPROXIMATION. */
/*   Q3:      A PARAMETER USED IN THE APPROXIMATION. */
/*   Q4:      A PARAMETER USED IN THE APPROXIMATION. */
/*   R:       THE PROBABILITY AT WHICH THE PERCENT POINT IS EVALUATED. */
/*   T:       A VALUE USED IN THE APPROXIMATION. */
/*   TWO:     THE VALUE 2.0D0. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DPPT */
    if (*p == half) {
	ret_val = zero;
    } else {
	r__ = *p;
	if (*p > half) {
	    r__ = one - r__;
	}
	t = sqrt(-two * log(r__));
	anum = (((t * p4 + p3) * t + p2) * t + p1) * t + p0;
	aden = (((t * q4 + q3) * t + q2) * t + q1) * t + q0;
	ret_val = t + anum / aden;
	if (*p < half) {
	    ret_val = -ret_val;
	}
    }
    return ret_val;
} /* dppnml_ */

/* DPPT */
doublereal dppt_(doublereal *p, integer *idf)
{
    /* Initialized data */

    static doublereal b21 = 4.;
    static doublereal b31 = 96.;
    static doublereal b32 = 5.;
    static doublereal b33 = 16.;
    static doublereal b34 = 3.;
    static doublereal b41 = 384.;
    static doublereal b42 = 3.;
    static doublereal b43 = 19.;
    static doublereal b44 = 17.;
    static doublereal b45 = -15.;
    static doublereal b51 = 9216.;
    static doublereal b52 = 79.;
    static doublereal b53 = 776.;
    static doublereal b54 = 1482.;
    static doublereal b55 = -1920.;
    static doublereal b56 = -945.;
    static doublereal zero = 0.;
    static doublereal half = .5;
    static doublereal one = 1.;
    static doublereal two = 2.;
    static doublereal three = 3.;
    static doublereal eight = 8.;
    static doublereal fiftn = 15.;

    /* System generated locals */
    integer i__1;
    doublereal ret_val, d__1, d__2, d__3, d__4;

    /* Builtin functions */
    double cos(doublereal), sin(doublereal), sqrt(doublereal), atan(
	    doublereal);

    /* Local variables */
    static doublereal c__, s, z__, d1, d3, d5, d7, d9, df, pi, arg, con, ppfn,
	     term1, term2, term3, term4, term5;
    static integer ipass, maxit;
    extern doublereal dppnml_(doublereal *);

/* ***BEGIN PROLOGUE  DPPT */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DPPNML */
/* ***DATE WRITTEN   901207   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***AUTHOR  FILLIBEN, JAMES J., */
/*             STATISTICAL ENGINEERING DIVISION */
/*             NATIONAL BUREAU OF STANDARDS */
/*             WASHINGTON, D. C. 20234 */
/*             (ORIGINAL VERSION--OCTOBER   1975.) */
/*             (UPDATED         --NOVEMBER  1975.) */
/* ***PURPOSE  COMPUTE THE PERCENT POINT FUNCTION VALUE FOR THE */
/*            STUDENT'S T DISTRIBUTION WITH IDF DEGREES OF FREEDOM. */
/*            (ADAPTED FROM DATAPAC SUBROUTINE TPPF, WITH MODIFICATIONS */
/*            TO FACILITATE CONVERSION TO DOUBLE PRECISION AUTOMATICALLY) */
/* ***DESCRIPTION */
/*              --FOR IDF = 1 AND IDF = 2, THE PERCENT POINT FUNCTION */
/*                FOR THE T DISTRIBUTION EXISTS IN SIMPLE CLOSED FORM */
/*                AND SO THE COMPUTED PERCENT POINTS ARE EXACT. */
/*              --FOR IDF BETWEEN 3 AND 6, INCLUSIVELY, THE APPROXIMATION */
/*                IS AUGMENTED BY 3 ITERATIONS OF NEWTON'S METHOD TO */
/*                IMPROVE THE ACCURACY, ESPECIALLY FOR P NEAR 0 OR 1. */
/* ***REFERENCES  NATIONAL BUREAU OF STANDARDS APPLIED MATHMATICS */
/*                 SERIES 55, 1964, PAGE 949, FORMULA 26.7.5. */
/*               JOHNSON AND KOTZ, CONTINUOUS UNIVARIATE DISTRIBUTIONS, */
/*                 VOLUME 2, 1970, PAGE 102, FORMULA 11. */
/*               FEDERIGHI, "EXTENDED TABLES OF THE PERCENTAGE POINTS */
/*                 OF STUDENT"S T DISTRIBUTION, JOURNAL OF THE AMERICAN */
/*                 STATISTICAL ASSOCIATION, 1969, PAGES 683-688. */
/*               HASTINGS AND PEACOCK, STATISTICAL DISTRIBUTIONS, A */
/*                 HANDBOOK FOR STUDENTS AND PRACTITIONERS, 1975, */
/*                 PAGES 120-123. */
/* ***END PROLOGUE  DPPT */
/* ...SCALAR ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL FUNCTIONS */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ARG:    A VALUE USED IN THE APPROXIMATION. */
/*   B21:    A PARAMETER USED IN THE APPROXIMATION. */
/*   B31:    A PARAMETER USED IN THE APPROXIMATION. */
/*   B32:    A PARAMETER USED IN THE APPROXIMATION. */
/*   B33:    A PARAMETER USED IN THE APPROXIMATION. */
/*   B34:    A PARAMETER USED IN THE APPROXIMATION. */
/*   B41:    A PARAMETER USED IN THE APPROXIMATION. */
/*   B42:    A PARAMETER USED IN THE APPROXIMATION. */
/*   B43:    A PARAMETER USED IN THE APPROXIMATION. */
/*   B44:    A PARAMETER USED IN THE APPROXIMATION. */
/*   B45:    A PARAMETER USED IN THE APPROXIMATION. */
/*   B51:    A PARAMETER USED IN THE APPROXIMATION. */
/*   B52:    A PARAMETER USED IN THE APPROXIMATION. */
/*   B53:    A PARAMETER USED IN THE APPROXIMATION. */
/*   B54:    A PARAMETER USED IN THE APPROXIMATION. */
/*   B55:    A PARAMETER USED IN THE APPROXIMATION. */
/*   B56:    A PARAMETER USED IN THE APPROXIMATION. */
/*   C:      A VALUE USED IN THE APPROXIMATION. */
/*   CON:    A VALUE USED IN THE APPROXIMATION. */
/*   DF:     THE DEGREES OF FREEDOM. */
/*   D1:     A VALUE USED IN THE APPROXIMATION. */
/*   D3:     A VALUE USED IN THE APPROXIMATION. */
/*   D5:     A VALUE USED IN THE APPROXIMATION. */
/*   D7:     A VALUE USED IN THE APPROXIMATION. */
/*   D9:     A VALUE USED IN THE APPROXIMATION. */
/*   EIGHT:  THE VALUE 8.0D0. */
/*   FIFTN:  THE VALUE 15.0D0. */
/*   HALF:   THE VALUE 0.5D0. */
/*   IDF:    THE (POSITIVE INTEGER) DEGREES OF FREEDOM. */
/*   IPASS:  A VALUE USED IN THE APPROXIMATION. */
/*   MAXIT:  THE MAXIMUM NUMBER OF ITERATIONS ALLOWED FOR THE APPROX. */
/*   ONE:    THE VALUE 1.0D0. */
/*   P:      THE PROBABILITY AT WHICH THE PERCENT POINT IS TO BE */
/*           EVALUATED.  P MUST LIE BETWEEN 0.0DO AND 1.0D0, EXCLUSIVE. */
/*   PI:     THE VALUE OF PI. */
/*   PPFN:   THE NORMAL PERCENT POINT VALUE. */
/*   S:      A VALUE USED IN THE APPROXIMATION. */
/*   TERM1:  A VALUE USED IN THE APPROXIMATION. */
/*   TERM2:  A VALUE USED IN THE APPROXIMATION. */
/*   TERM3:  A VALUE USED IN THE APPROXIMATION. */
/*   TERM4:  A VALUE USED IN THE APPROXIMATION. */
/*   TERM5:  A VALUE USED IN THE APPROXIMATION. */
/*   THREE:  THE VALUE 3.0D0. */
/*   TWO:    THE VALUE 2.0D0. */
/*   Z:      A VALUE USED IN THE APPROXIMATION. */
/*   ZERO:   THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DPPT */
    pi = 3.141592653589793238462643383279;
    df = (doublereal) (*idf);
    maxit = 5;
    if (*idf <= 0) {
/*  TREAT THE IDF < 1 CASE */
	ret_val = zero;
    } else if (*idf == 1) {
/*  TREAT THE IDF = 1 (CAUCHY) CASE */
	arg = pi * *p;
	ret_val = -cos(arg) / sin(arg);
    } else if (*idf == 2) {
/*  TREAT THE IDF = 2 CASE */
	term1 = sqrt(two) / two;
	term2 = two * *p - one;
	term3 = sqrt(*p * (one - *p));
	ret_val = term1 * term2 / term3;
    } else if (*idf >= 3) {
/*  TREAT THE IDF GREATER THAN OR EQUAL TO 3 CASE */
	ppfn = dppnml_(p);
	d1 = ppfn;
/* Computing 3rd power */
	d__1 = ppfn;
	d3 = d__1 * (d__1 * d__1);
/* Computing 5th power */
	d__1 = ppfn, d__2 = d__1, d__1 *= d__1;
	d5 = d__2 * (d__1 * d__1);
/* Computing 7th power */
	d__1 = ppfn, d__2 = d__1, d__1 *= d__1, d__2 *= d__1;
	d7 = d__2 * (d__1 * d__1);
/* Computing 9th power */
	d__1 = ppfn, d__2 = d__1, d__1 *= d__1, d__1 *= d__1;
	d9 = d__2 * (d__1 * d__1);
	term1 = d1;
	term2 = one / b21 * (d3 + d1) / df;
/* Computing 2nd power */
	d__1 = df;
	term3 = one / b31 * (b32 * d5 + b33 * d3 + b34 * d1) / (d__1 * d__1);
/* Computing 3rd power */
	d__1 = df;
	term4 = one / b41 * (b42 * d7 + b43 * d5 + b44 * d3 + b45 * d1) / (
		d__1 * (d__1 * d__1));
/* Computing 4th power */
	d__1 = df, d__1 *= d__1;
	term5 = one / b51 * (b52 * d9 + b53 * d7 + b54 * d5 + b55 * d3 + b56 *
		 d1) / (d__1 * d__1);
	ret_val = term1 + term2 + term3 + term4 + term5;
	if (*idf == 3) {
/*  AUGMENT THE RESULTS FOR THE IDF = 3 CASE */
	    con = pi * (*p - half);
	    arg = ret_val / sqrt(df);
	    z__ = atan(arg);
	    i__1 = maxit;
	    for (ipass = 1; ipass <= i__1; ++ipass) {
		s = sin(z__);
		c__ = cos(z__);
/* Computing 2nd power */
		d__1 = c__;
		z__ -= (z__ + s * c__ - con) / (two * (d__1 * d__1));
/* L70: */
	    }
	    ret_val = sqrt(df) * s / c__;
	} else if (*idf == 4) {
/*  AUGMENT THE RESULTS FOR THE IDF = 4 CASE */
	    con = two * (*p - half);
	    arg = ret_val / sqrt(df);
	    z__ = atan(arg);
	    i__1 = maxit;
	    for (ipass = 1; ipass <= i__1; ++ipass) {
		s = sin(z__);
		c__ = cos(z__);
/* Computing 2nd power */
		d__1 = c__;
/* Computing 3rd power */
		d__2 = c__;
		z__ -= ((one + half * (d__1 * d__1)) * s - con) / ((one + 
			half) * (d__2 * (d__2 * d__2)));
/* L90: */
	    }
	    ret_val = sqrt(df) * s / c__;
	} else if (*idf == 5) {
/*  AUGMENT THE RESULTS FOR THE IDF = 5 CASE */
	    con = pi * (*p - half);
	    arg = ret_val / sqrt(df);
	    z__ = atan(arg);
	    i__1 = maxit;
	    for (ipass = 1; ipass <= i__1; ++ipass) {
		s = sin(z__);
		c__ = cos(z__);
/* Computing 3rd power */
		d__1 = c__;
/* Computing 4th power */
		d__2 = c__, d__2 *= d__2;
		z__ -= (z__ + (c__ + two / three * (d__1 * (d__1 * d__1))) * 
			s - con) / (eight / three * (d__2 * d__2));
/* L110: */
	    }
	    ret_val = sqrt(df) * s / c__;
	} else if (*idf == 6) {
/*  AUGMENT THE RESULTS FOR THE IDF = 6 CASE */
	    con = two * (*p - half);
	    arg = ret_val / sqrt(df);
	    z__ = atan(arg);
	    i__1 = maxit;
	    for (ipass = 1; ipass <= i__1; ++ipass) {
		s = sin(z__);
		c__ = cos(z__);
/* Computing 2nd power */
		d__1 = c__;
/* Computing 4th power */
		d__2 = c__, d__2 *= d__2;
/* Computing 5th power */
		d__3 = c__, d__4 = d__3, d__3 *= d__3;
		z__ -= ((one + half * (d__1 * d__1) + three / eight * (d__2 * 
			d__2)) * s - con) / (fiftn / eight * (d__4 * (d__3 * 
			d__3)));
/* L130: */
	    }
	    ret_val = sqrt(df) * s / c__;
	}
    }
    return ret_val;
} /* dppt_ */

/* DPVB */
/* Subroutine */ int dpvb_(S_fp fcn, integer *n, integer *m, integer *np, 
	integer *nq, doublereal *beta, doublereal *xplusd, integer *ifixb, 
	integer *ifixx, integer *ldifx, integer *nrow, integer *j, integer *
	lq, doublereal *stp, integer *istop, integer *nfev, doublereal *pvb, 
	doublereal *wrk1, doublereal *wrk2, doublereal *wrk6)
{
    /* System generated locals */
    integer wrk1_dim1, wrk1_dim2, wrk1_offset, wrk2_dim1, wrk2_offset, 
	    wrk6_dim1, wrk6_dim2, wrk6_offset, xplusd_dim1, xplusd_offset, 
	    ifixx_dim1, ifixx_offset;

    /* Local variables */
    static doublereal betaj;

/* ***BEGIN PROLOGUE  DPVB */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  FCN */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  COMPUTE THE NROW-TH FUNCTION VALUE USING BETA(J) + STP */
/* ***END PROLOGUE  DPVB */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:     THE USER-SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   BETAJ:   THE CURRENT ESTIMATE OF THE JTH PARAMETER. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   ISTOP:   THE VARIABLE DESIGNATING WHETHER THERE ARE PROBLEMS */
/*            COMPUTING THE FUNCTION AT THE CURRENT BETA AND DELTA. */
/*   J:       THE INDEX OF THE PARTIAL DERIVATIVE BEING EXAMINED. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LQ:      THE RESPONSE CURRENTLY BEING EXAMINED. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE INDEPENDENT VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   NROW:    THE ROW NUMBER OF THE INDEPENDENT VARIABLE ARRAY AT */
/*            WHICH THE DERIVATIVE IS TO BE CHECKED. */
/*   PVB:     THE FUNCTION VALUE FOR THE SELECTED OBSERVATION & RESPONSE. */
/*   STP:     THE STEP SIZE FOR THE FINITE DIFFERENCE DERIVATIVE. */
/*   XPLUSD:  THE VALUES OF X + DELTA. */
/* ***FIRST EXECUTABLE STATEMENT  DPVB */
/*  COMPUTE PREDICTED VALUES */
    /* Parameter adjustments */
    xplusd_dim1 = *n;
    xplusd_offset = 1 + xplusd_dim1;
    xplusd -= xplusd_offset;
    --ifixb;
    --beta;
    wrk6_dim1 = *n;
    wrk6_dim2 = *np;
    wrk6_offset = 1 + wrk6_dim1 * (1 + wrk6_dim2);
    wrk6 -= wrk6_offset;
    wrk2_dim1 = *n;
    wrk2_offset = 1 + wrk2_dim1;
    wrk2 -= wrk2_offset;
    wrk1_dim1 = *n;
    wrk1_dim2 = *m;
    wrk1_offset = 1 + wrk1_dim1 * (1 + wrk1_dim2);
    wrk1 -= wrk1_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;

    /* Function Body */
    betaj = beta[*j];
    beta[*j] += *stp;
    *istop = 0;
    (*fcn)(n, m, np, nq, n, m, np, &beta[1], &xplusd[xplusd_offset], &ifixb[1]
	    , &ifixx[ifixx_offset], ldifx, &c__3, &wrk2[wrk2_offset], &wrk6[
	    wrk6_offset], &wrk1[wrk1_offset], istop);
    if (*istop == 0) {
	++(*nfev);
    } else {
	return 0;
    }
    beta[*j] = betaj;
    *pvb = wrk2[*nrow + *lq * wrk2_dim1];
    return 0;
} /* dpvb_ */

/* DPVD */
/* Subroutine */ int dpvd_(S_fp fcn, integer *n, integer *m, integer *np, 
	integer *nq, doublereal *beta, doublereal *xplusd, integer *ifixb, 
	integer *ifixx, integer *ldifx, integer *nrow, integer *j, integer *
	lq, doublereal *stp, integer *istop, integer *nfev, doublereal *pvd, 
	doublereal *wrk1, doublereal *wrk2, doublereal *wrk6)
{
    /* System generated locals */
    integer wrk1_dim1, wrk1_dim2, wrk1_offset, wrk2_dim1, wrk2_offset, 
	    wrk6_dim1, wrk6_dim2, wrk6_offset, xplusd_dim1, xplusd_offset, 
	    ifixx_dim1, ifixx_offset;

    /* Local variables */
    static doublereal xpdj;

/* ***BEGIN PROLOGUE  DPVD */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  FCN */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  COMPUTE NROW-TH FUNCTION VALUE USING */
/*            X(NROW,J) + DELTA(NROW,J) + STP */
/* ***END PROLOGUE  DPVD */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...SUBROUTINE ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...ROUTINE NAMES USED AS SUBPROGRAM ARGUMENTS */
/*   FCN:     THE USER-SUPPLIED SUBROUTINE FOR EVALUATING THE MODEL. */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   IFIXB:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF BETA ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   IFIXX:   THE VALUES DESIGNATING WHETHER THE ELEMENTS OF X ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*   ISTOP:   THE VARIABLE DESIGNATING WHETHER THERE ARE PROBLEMS */
/*            COMPUTING THE FUNCTION AT THE CURRENT BETA AND DELTA. */
/*   J:       THE INDEX OF THE PARTIAL DERIVATIVE BEING EXAMINED. */
/*   LDIFX:   THE LEADING DIMENSION OF ARRAY IFIXX. */
/*   LQ:      THE RESPONSE CURRENTLY BEING EXAMINED. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE INDEPENDENT VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NFEV:    THE NUMBER OF FUNCTION EVALUATIONS. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   NROW:    THE ROW NUMBER OF THE INDEPENDENT VARIABLE ARRAY AT */
/*            WHICH THE DERIVATIVE IS TO BE CHECKED. */
/*   PVD:     THE FUNCTION VALUE FOR THE SELECTED OBSERVATION & RESPONSE. */
/*   STP:     THE STEP SIZE FOR THE FINITE DIFFERENCE DERIVATIVE. */
/*   XPDJ:    THE (NROW,J)TH ELEMENT OF XPLUSD. */
/*   XPLUSD:  THE VALUES OF X + DELTA. */
/* ***FIRST EXECUTABLE STATEMENT  DPVD */
/*  COMPUTE PREDICTED VALUES */
    /* Parameter adjustments */
    xplusd_dim1 = *n;
    xplusd_offset = 1 + xplusd_dim1;
    xplusd -= xplusd_offset;
    --ifixb;
    --beta;
    wrk6_dim1 = *n;
    wrk6_dim2 = *np;
    wrk6_offset = 1 + wrk6_dim1 * (1 + wrk6_dim2);
    wrk6 -= wrk6_offset;
    wrk2_dim1 = *n;
    wrk2_offset = 1 + wrk2_dim1;
    wrk2 -= wrk2_offset;
    wrk1_dim1 = *n;
    wrk1_dim2 = *m;
    wrk1_offset = 1 + wrk1_dim1 * (1 + wrk1_dim2);
    wrk1 -= wrk1_offset;
    ifixx_dim1 = *ldifx;
    ifixx_offset = 1 + ifixx_dim1;
    ifixx -= ifixx_offset;

    /* Function Body */
    xpdj = xplusd[*nrow + *j * xplusd_dim1];
    xplusd[*nrow + *j * xplusd_dim1] += *stp;
    *istop = 0;
    (*fcn)(n, m, np, nq, n, m, np, &beta[1], &xplusd[xplusd_offset], &ifixb[1]
	    , &ifixx[ifixx_offset], ldifx, &c__3, &wrk2[wrk2_offset], &wrk6[
	    wrk6_offset], &wrk1[wrk1_offset], istop);
    if (*istop == 0) {
	++(*nfev);
    } else {
	return 0;
    }
    xplusd[*nrow + *j * xplusd_dim1] = xpdj;
    *pvd = wrk2[*nrow + *lq * wrk2_dim1];
    return 0;
} /* dpvd_ */

/* DSCALE */
/* Subroutine */ int dscale_(integer *n, integer *m, doublereal *scl, integer 
	*ldscl, doublereal *t, integer *ldt, doublereal *sclt, integer *
	ldsclt)
{
    /* Initialized data */

    static doublereal one = 1.;
    static doublereal zero = 0.;

    /* System generated locals */
    integer t_dim1, t_offset, scl_dim1, scl_offset, sclt_dim1, sclt_offset, 
	    i__1, i__2;
    doublereal d__1;

    /* Local variables */
    static integer i__, j;
    static doublereal temp;

/* ***BEGIN PROLOGUE  DSCALE */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  SCALE T BY THE INVERSE OF SCL, I.E., COMPUTE T/SCL */
/* ***END PROLOGUE  DSCALE */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    scl_dim1 = *ldscl;
    scl_offset = 1 + scl_dim1;
    scl -= scl_offset;
    t_dim1 = *ldt;
    t_offset = 1 + t_dim1;
    t -= t_offset;
    sclt_dim1 = *ldsclt;
    sclt_offset = 1 + sclt_dim1;
    sclt -= sclt_offset;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   I:       AN INDEXING VARIABLE. */
/*   J:       AN INDEXING VARIABLE. */
/*   LDSCL:   THE LEADING DIMENSION OF ARRAY SCL. */
/*   LDSCLT:  THE LEADING DIMENSION OF ARRAY SCLT. */
/*   LDT:     THE LEADING DIMENSION OF ARRAY T. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN T. */
/*   N:       THE NUMBER OF ROWS OF DATA IN T. */
/*   ONE:     THE VALUE 1.0D0. */
/*   SCL:     THE SCALE VALUES. */
/*   SCLT:    THE INVERSELY SCALED MATRIX. */
/*   T:       THE ARRAY TO BE INVERSELY SCALED BY SCL. */
/*   TEMP:    A TEMPORARY SCALAR. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DSCALE */
    if (*n == 0 || *m == 0) {
	return 0;
    }
    if (scl[scl_dim1 + 1] >= zero) {
	if (*ldscl >= *n) {
	    i__1 = *m;
	    for (j = 1; j <= i__1; ++j) {
		i__2 = *n;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    sclt[i__ + j * sclt_dim1] = t[i__ + j * t_dim1] / scl[i__ 
			    + j * scl_dim1];
/* L70: */
		}
/* L80: */
	    }
	} else {
	    i__1 = *m;
	    for (j = 1; j <= i__1; ++j) {
		temp = one / scl[j * scl_dim1 + 1];
		i__2 = *n;
		for (i__ = 1; i__ <= i__2; ++i__) {
		    sclt[i__ + j * sclt_dim1] = t[i__ + j * t_dim1] * temp;
/* L90: */
		}
/* L100: */
	    }
	}
    } else {
	temp = one / (d__1 = scl[scl_dim1 + 1], abs(d__1));
	i__1 = *m;
	for (j = 1; j <= i__1; ++j) {
	    i__2 = *n;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		sclt[i__ + j * sclt_dim1] = t[i__ + j * t_dim1] * temp;
/* L110: */
	    }
/* L120: */
	}
    }
    return 0;
} /* dscale_ */

/* DSCLB */
/* Subroutine */ int dsclb_(integer *np, doublereal *beta, doublereal *ssf)
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal one = 1.;
    static doublereal ten = 10.;

    /* System generated locals */
    integer i__1;
    doublereal d__1, d__2, d__3;

    /* Builtin functions */
    double d_lg10(doublereal *);

    /* Local variables */
    static integer k;
    static doublereal bmin, bmax;
    static logical bigdif;

/* ***BEGIN PROLOGUE  DSCLB */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  SELECT SCALING VALUES FOR BETA ACCORDING TO THE */
/*            ALGORITHM GIVEN IN THE ODRPACK REFERENCE GUIDE */
/* ***END PROLOGUE  DSCLB */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --ssf;
    --beta;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   BETA:    THE FUNCTION PARAMETERS. */
/*   BIGDIF:  THE VARIABLE DESIGNATING WHETHER THERE IS A SIGNIFICANT */
/*            DIFFERENCE IN THE MAGNITUDES OF THE NONZERO ELEMENTS OF */
/*            BETA (BIGDIF=.TRUE.) OR NOT (BIGDIF=.FALSE.). */
/*   BMAX:    THE LARGEST NONZERO MAGNITUDE. */
/*   BMIN:    THE SMALLEST NONZERO MAGNITUDE. */
/*   K:       AN INDEXING VARIABLE. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   ONE:     THE VALUE 1.0D0. */
/*   SSF:     THE SCALING VALUES FOR BETA. */
/*   TEN:     THE VALUE 10.0D0. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DSCLB */
    bmax = abs(beta[1]);
    i__1 = *np;
    for (k = 2; k <= i__1; ++k) {
/* Computing MAX */
	d__2 = bmax, d__3 = (d__1 = beta[k], abs(d__1));
	bmax = max(d__2,d__3);
/* L10: */
    }
    if (bmax == zero) {
/*  ALL INPUT VALUES OF BETA ARE ZERO */
	i__1 = *np;
	for (k = 1; k <= i__1; ++k) {
	    ssf[k] = one;
/* L20: */
	}
    } else {
/*  SOME OF THE INPUT VALUES ARE NONZERO */
	bmin = bmax;
	i__1 = *np;
	for (k = 1; k <= i__1; ++k) {
	    if (beta[k] != zero) {
/* Computing MIN */
		d__2 = bmin, d__3 = (d__1 = beta[k], abs(d__1));
		bmin = min(d__2,d__3);
	    }
/* L30: */
	}
	bigdif = d_lg10(&bmax) - d_lg10(&bmin) >= one;
	i__1 = *np;
	for (k = 1; k <= i__1; ++k) {
	    if (beta[k] == zero) {
		ssf[k] = ten / bmin;
	    } else {
		if (bigdif) {
		    ssf[k] = one / (d__1 = beta[k], abs(d__1));
		} else {
		    ssf[k] = one / bmax;
		}
	    }
/* L40: */
	}
    }
    return 0;
} /* dsclb_ */

/* DSCLD */
/* Subroutine */ int dscld_(integer *n, integer *m, doublereal *x, integer *
	ldx, doublereal *tt, integer *ldtt)
{
    /* Initialized data */

    static doublereal zero = 0.;
    static doublereal one = 1.;
    static doublereal ten = 10.;

    /* System generated locals */
    integer tt_dim1, tt_offset, x_dim1, x_offset, i__1, i__2;
    doublereal d__1, d__2, d__3;

    /* Builtin functions */
    double d_lg10(doublereal *);

    /* Local variables */
    static integer i__, j;
    static doublereal xmin, xmax;
    static logical bigdif;

/* ***BEGIN PROLOGUE  DSCLD */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  SELECT SCALING VALUES FOR DELTA ACCORDING TO THE */
/*            ALGORITHM GIVEN IN THE ODRPACK REFERENCE GUIDE */
/* ***END PROLOGUE  DSCLD */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    tt_dim1 = *ldtt;
    tt_offset = 1 + tt_dim1;
    tt -= tt_offset;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   BIGDIF:  THE VARIABLE DESIGNATING WHETHER THERE IS A SIGNIFICANT */
/*            DIFFERENCE IN THE MAGNITUDES OF THE NONZERO ELEMENTS OF */
/*            X (BIGDIF=.TRUE.) OR NOT (BIGDIF=.FALSE.). */
/*   I:       AN INDEXING VARIABLE. */
/*   J:       AN INDEXING VARIABLE. */
/*   LDTT:    THE LEADING DIMENSION OF ARRAY TT. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE INDEPENDENT VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   ONE:     THE VALUE 1.0D0. */
/*   TT:      THE SCALING VALUES FOR DELTA. */
/*   X:       THE INDEPENDENT VARIABLE. */
/*   XMAX:    THE LARGEST NONZERO MAGNITUDE. */
/*   XMIN:    THE SMALLEST NONZERO MAGNITUDE. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DSCLD */
    i__1 = *m;
    for (j = 1; j <= i__1; ++j) {
	xmax = (d__1 = x[j * x_dim1 + 1], abs(d__1));
	i__2 = *n;
	for (i__ = 2; i__ <= i__2; ++i__) {
/* Computing MAX */
	    d__2 = xmax, d__3 = (d__1 = x[i__ + j * x_dim1], abs(d__1));
	    xmax = max(d__2,d__3);
/* L10: */
	}
	if (xmax == zero) {
/*  ALL INPUT VALUES OF X(I,J), I=1,...,N, ARE ZERO */
	    i__2 = *n;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		tt[i__ + j * tt_dim1] = one;
/* L20: */
	    }
	} else {
/*  SOME OF THE INPUT VALUES ARE NONZERO */
	    xmin = xmax;
	    i__2 = *n;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		if (x[i__ + j * x_dim1] != zero) {
/* Computing MIN */
		    d__2 = xmin, d__3 = (d__1 = x[i__ + j * x_dim1], abs(d__1)
			    );
		    xmin = min(d__2,d__3);
		}
/* L30: */
	    }
	    bigdif = d_lg10(&xmax) - d_lg10(&xmin) >= one;
	    i__2 = *n;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		if (x[i__ + j * x_dim1] != zero) {
		    if (bigdif) {
			tt[i__ + j * tt_dim1] = one / (d__1 = x[i__ + j * 
				x_dim1], abs(d__1));
		    } else {
			tt[i__ + j * tt_dim1] = one / xmax;
		    }
		} else {
		    tt[i__ + j * tt_dim1] = ten / xmin;
		}
/* L40: */
	    }
	}
/* L50: */
    }
    return 0;
} /* dscld_ */

/* DSETN */
/* Subroutine */ int dsetn_(integer *n, integer *m, doublereal *x, integer *
	ldx, integer *nrow)
{
    /* System generated locals */
    integer x_dim1, x_offset, i__1, i__2;

    /* Local variables */
    static integer i__, j;

/* ***BEGIN PROLOGUE  DSETN */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  SELECT THE ROW AT WHICH THE DERIVATIVE WILL BE CHECKED */
/* ***END PROLOGUE  DSETN */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   I:       AN INDEX VARIABLE. */
/*   J:       AN INDEX VARIABLE. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE INDEPENDENT VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NROW:    THE SELECTED ROW NUMBER OF THE INDEPENDENT VARIABLE. */
/*   X:       THE INDEPENDENT VARIABLE. */
/* ***FIRST EXECUTABLE STATEMENT  DSETN */
    /* Parameter adjustments */
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;

    /* Function Body */
    if (*nrow >= 1 && *nrow <= *n) {
	return 0;
    }
/*     SELECT FIRST ROW OF INDEPENDENT VARIABLES WHICH CONTAINS NO ZEROS */
/*     IF THERE IS ONE, OTHERWISE FIRST ROW IS USED. */
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	i__2 = *m;
	for (j = 1; j <= i__2; ++j) {
	    if (x[i__ + j * x_dim1] == 0.f) {
		goto L20;
	    }
/* L10: */
	}
	*nrow = i__;
	return 0;
L20:
	;
    }
    *nrow = 1;
    return 0;
} /* dsetn_ */

/* DSOLVE */
/* Subroutine */ int dsolve_(integer *n, doublereal *t, integer *ldt, 
	doublereal *b, integer *ldb, integer *job)
{
    /* Initialized data */

    static doublereal zero = 0.;

    /* System generated locals */
    integer b_dim1, b_offset, t_dim1, t_offset, i__1, i__2;

    /* Local variables */
    static integer j, j1, jn;
    extern doublereal ddot_(integer *, doublereal *, integer *, doublereal *, 
	    integer *);
    static doublereal temp;
    extern /* Subroutine */ int daxpy_(integer *, doublereal *, doublereal *, 
	    integer *, doublereal *, integer *);

/* ***BEGIN PROLOGUE  DSOLVE */
/* ***REFER TO DODR,DODRC */
/* ***ROUTINES CALLED  DAXPY,DDOT */
/* ***DATE WRITTEN   920220   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  SOLVE SYSTEMS OF THE FORM */
/*                   T * X = B  OR  TRANS(T) * X = B */
/*            WHERE T IS AN UPPER OR LOWER TRIANGULAR MATRIX OF ORDER N, */
/*            AND THE SOLUTION X OVERWRITES THE RHS B. */
/*            (ADAPTED FROM LINPACK SUBROUTINE DTRSL) */
/* ***REFERENCES  DONGARRA J.J., BUNCH J.R., MOLER C.B., STEWART G.W., */
/*                 *LINPACK USERS GUIDE*, SIAM, 1979. */
/* ***END PROLOGUE  DSOLVE */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL FUNCTIONS */
/* ...EXTERNAL SUBROUTINES */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    t_dim1 = *ldt;
    t_offset = 1 + t_dim1;
    t -= t_offset;
    b_dim1 = *ldb;
    b_offset = 1 + b_dim1;
    b -= b_offset;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   B:       ON INPUT:  THE RIGHT HAND SIDE;  ON EXIT:  THE SOLUTION */
/*   J1:      THE FIRST NONZERO ENTRY IN T. */
/*   J:       AN INDEXING VARIABLE. */
/*   JN:      THE LAST NONZERO ENTRY IN T. */
/*   JOB:     WHAT KIND OF SYSTEM IS TO BE SOLVED, WHERE IF JOB IS */
/*            1   SOLVE T*X=B, T LOWER TRIANGULAR, */
/*            2   SOLVE T*X=B, T UPPER TRIANGULAR, */
/*            3   SOLVE TRANS(T)*X=B, T LOWER TRIANGULAR, */
/*            4   SOLVE TRANS(T)*X=B, T UPPER TRIANGULAR. */
/*   LDB:     THE LEADING DIMENSION OF ARRAY B. */
/*   LDT:     THE LEADING DIMENSION OF ARRAY T. */
/*   N:       THE NUMBER OF ROWS AND COLUMNS OF DATA IN ARRAY T. */
/*   T:       THE UPPER OR LOWER TRIDIAGONAL SYSTEM. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DSOLVE */
/*  FIND FIRST NONZERO DIAGONAL ENTRY IN T */
    j1 = 0;
    i__1 = *n;
    for (j = 1; j <= i__1; ++j) {
	if (j1 == 0 && t[j + j * t_dim1] != zero) {
	    j1 = j;
	} else if (t[j + j * t_dim1] == zero) {
	    b[j * b_dim1 + 1] = zero;
	}
/* L10: */
    }
    if (j1 == 0) {
	return 0;
    }
/*  FIND LAST NONZERO DIAGONAL ENTRY IN T */
    jn = 0;
    i__1 = j1;
    for (j = *n; j >= i__1; --j) {
	if (jn == 0 && t[j + j * t_dim1] != zero) {
	    jn = j;
	} else if (t[j + j * t_dim1] == zero) {
	    b[j * b_dim1 + 1] = zero;
	}
/* L20: */
    }
    if (*job == 1) {
/*  SOLVE T*X=B FOR T LOWER TRIANGULAR */
	b[j1 * b_dim1 + 1] /= t[j1 + j1 * t_dim1];
	i__1 = jn;
	for (j = j1 + 1; j <= i__1; ++j) {
	    temp = -b[(j - 1) * b_dim1 + 1];
	    i__2 = jn - j + 1;
	    daxpy_(&i__2, &temp, &t[j + (j - 1) * t_dim1], &c__1, &b[j * 
		    b_dim1 + 1], ldb);
	    if (t[j + j * t_dim1] != zero) {
		b[j * b_dim1 + 1] /= t[j + j * t_dim1];
	    } else {
		b[j * b_dim1 + 1] = zero;
	    }
/* L30: */
	}
    } else if (*job == 2) {
/*  SOLVE T*X=B FOR T UPPER TRIANGULAR. */
	b[jn * b_dim1 + 1] /= t[jn + jn * t_dim1];
	i__1 = j1;
	for (j = jn - 1; j >= i__1; --j) {
	    temp = -b[(j + 1) * b_dim1 + 1];
	    daxpy_(&j, &temp, &t[(j + 1) * t_dim1 + 1], &c__1, &b[b_dim1 + 1],
		     ldb);
	    if (t[j + j * t_dim1] != zero) {
		b[j * b_dim1 + 1] /= t[j + j * t_dim1];
	    } else {
		b[j * b_dim1 + 1] = zero;
	    }
/* L40: */
	}
    } else if (*job == 3) {
/*  SOLVE TRANS(T)*X=B FOR T LOWER TRIANGULAR. */
	b[jn * b_dim1 + 1] /= t[jn + jn * t_dim1];
	i__1 = j1;
	for (j = jn - 1; j >= i__1; --j) {
	    i__2 = jn - j + 1;
	    b[j * b_dim1 + 1] -= ddot_(&i__2, &t[j + 1 + j * t_dim1], &c__1, &
		    b[(j + 1) * b_dim1 + 1], ldb);
	    if (t[j + j * t_dim1] != zero) {
		b[j * b_dim1 + 1] /= t[j + j * t_dim1];
	    } else {
		b[j * b_dim1 + 1] = zero;
	    }
/* L50: */
	}
    } else if (*job == 4) {
/*  SOLVE TRANS(T)*X=B FOR T UPPER TRIANGULAR. */
	b[j1 * b_dim1 + 1] /= t[j1 + j1 * t_dim1];
	i__1 = jn;
	for (j = j1 + 1; j <= i__1; ++j) {
	    i__2 = j - 1;
	    b[j * b_dim1 + 1] -= ddot_(&i__2, &t[j * t_dim1 + 1], &c__1, &b[
		    b_dim1 + 1], ldb);
	    if (t[j + j * t_dim1] != zero) {
		b[j * b_dim1 + 1] /= t[j + j * t_dim1];
	    } else {
		b[j * b_dim1 + 1] = zero;
	    }
/* L60: */
	}
    }
    return 0;
} /* dsolve_ */

/* DUNPAC */
/* Subroutine */ int dunpac_(integer *n2, doublereal *v1, doublereal *v2, 
	integer *ifix)
{
    /* System generated locals */
    integer i__1;

    /* Local variables */
    static integer i__, n1;
    extern /* Subroutine */ int dcopy_(integer *, doublereal *, integer *, 
	    doublereal *, integer *);

/* ***BEGIN PROLOGUE  DUNPAC */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DCOPY */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  COPY THE ELEMENTS OF V1 INTO THE LOCATIONS OF V2 WHICH ARE */
/*            UNFIXED */
/* ***END PROLOGUE  DUNPAC */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   I:       AN INDEXING VARIABLE. */
/*   IFIX:    THE VALUES DESIGNATING WHETHER THE ELEMENTS OF V2 ARE */
/*            FIXED AT THEIR INPUT VALUES OR NOT. */
/*            ODRPACK REFERENCE GUIDE.) */
/*   N1:      THE NUMBER OF ITEMS IN V1. */
/*   N2:      THE NUMBER OF ITEMS IN V2. */
/*   V1:      THE VECTOR OF THE UNFIXED ITEMS. */
/*   V2:      THE VECTOR OF THE FIXED AND UNFIXED ITEMS INTO WHICH THE */
/*            ELEMENTS OF V1 ARE TO BE INSERTED. */
/* ***FIRST EXECUTABLE STATEMENT  DUNPAC */
    /* Parameter adjustments */
    --ifix;
    --v2;
    --v1;

    /* Function Body */
    n1 = 0;
    if (ifix[1] >= 0) {
	i__1 = *n2;
	for (i__ = 1; i__ <= i__1; ++i__) {
	    if (ifix[i__] != 0) {
		++n1;
		v2[i__] = v1[n1];
	    }
/* L10: */
	}
    } else {
	n1 = *n2;
	dcopy_(n2, &v1[1], &c__1, &v2[1], &c__1);
    }
    return 0;
} /* dunpac_ */

/* DVEVTR */
/* Subroutine */ int dvevtr_(integer *m, integer *nq, integer *indx, 
	doublereal *v, integer *ldv, integer *ld2v, doublereal *e, integer *
	lde, doublereal *ve, integer *ldve, integer *ld2ve, doublereal *vev, 
	integer *ldvev, doublereal *wrk5)
{
    /* Initialized data */

    static doublereal zero = 0.;

    /* System generated locals */
    integer e_dim1, e_offset, v_dim1, v_dim2, v_offset, ve_dim1, ve_dim2, 
	    ve_offset, vev_dim1, vev_offset, i__1, i__2, i__3;

    /* Local variables */
    static integer j, l1, l2;
    extern /* Subroutine */ int dsolve_(integer *, doublereal *, integer *, 
	    doublereal *, integer *, integer *);

/* ***BEGIN PROLOGUE  DVEVTR */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  DSOLVE */
/* ***DATE WRITTEN   910613   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  COMPUTE  V*E*TRANS(V) FOR THE (INDX)TH M BY NQ ARRAY IN V */
/* ***END PROLOGUE  DVEVTR */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --wrk5;
    v_dim1 = *ldv;
    v_dim2 = *ld2v;
    v_offset = 1 + v_dim1 * (1 + v_dim2);
    v -= v_offset;
    e_dim1 = *lde;
    e_offset = 1 + e_dim1;
    e -= e_offset;
    ve_dim1 = *ldve;
    ve_dim2 = *ld2ve;
    ve_offset = 1 + ve_dim1 * (1 + ve_dim2);
    ve -= ve_offset;
    vev_dim1 = *ldvev;
    vev_offset = 1 + vev_dim1;
    vev -= vev_offset;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   INDX:    THE ROW IN V IN WHICH THE M BY NQ ARRAY IS STORED. */
/*   J:       AN INDEXING VARIABLE. */
/*   LDE:     THE LEADING DIMENSION OF ARRAY E. */
/*   LDV:     THE LEADING DIMENSION OF ARRAY V. */
/*   LDVE:    THE LEADING DIMENSION OF ARRAY VE. */
/*   LDVEV:   THE LEADING DIMENSION OF ARRAY VEV. */
/*   LD2V:    THE SECOND DIMENSION OF ARRAY V. */
/*   L1:      AN INDEXING VARIABLE. */
/*   L2:      AN INDEXING VARIABLE. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE INDEPENDENT VARIABLE. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   E:       THE M BY M MATRIX OF THE FACTORS SO ETE = (D**2 + ALPHA*T**2). */
/*   V:       AN ARRAY OF NQ BY M MATRICES. */
/*   VE:      THE NQ BY M ARRAY VE = V * INV(E) */
/*   VEV:     THE NQ BY NQ ARRAY VEV = V * INV(ETE) * TRANS(V). */
/*   WRK5:    AN M WORK VECTOR. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DVEVTR */
    if (*nq == 0 || *m == 0) {
	return 0;
    }
    i__1 = *nq;
    for (l1 = 1; l1 <= i__1; ++l1) {
	i__2 = *m;
	for (j = 1; j <= i__2; ++j) {
	    wrk5[j] = v[*indx + (j + l1 * v_dim2) * v_dim1];
/* L110: */
	}
	dsolve_(m, &e[e_offset], lde, &wrk5[1], &c__1, &c__4);
	i__2 = *m;
	for (j = 1; j <= i__2; ++j) {
	    ve[*indx + (l1 + j * ve_dim2) * ve_dim1] = wrk5[j];
/* L120: */
	}
/* L140: */
    }
    i__1 = *nq;
    for (l1 = 1; l1 <= i__1; ++l1) {
	i__2 = l1;
	for (l2 = 1; l2 <= i__2; ++l2) {
	    vev[l1 + l2 * vev_dim1] = zero;
	    i__3 = *m;
	    for (j = 1; j <= i__3; ++j) {
		vev[l1 + l2 * vev_dim1] += ve[*indx + (l1 + j * ve_dim2) * 
			ve_dim1] * ve[*indx + (l2 + j * ve_dim2) * ve_dim1];
/* L210: */
	    }
	    vev[l2 + l1 * vev_dim1] = vev[l1 + l2 * vev_dim1];
/* L220: */
	}
/* L230: */
    }
    return 0;
} /* dvevtr_ */

/* DWGHT */
/* Subroutine */ int dwght_(integer *n, integer *m, doublereal *wt, integer *
	ldwt, integer *ld2wt, doublereal *t, integer *ldt, doublereal *wtt, 
	integer *ldwtt)
{
    /* Initialized data */

    static doublereal zero = 0.;

    /* System generated locals */
    integer t_dim1, t_offset, wt_dim1, wt_dim2, wt_offset, wtt_dim1, 
	    wtt_offset, i__1, i__2, i__3;
    doublereal d__1;

    /* Local variables */
    static integer i__, j, k;
    static doublereal temp;

/* ***BEGIN PROLOGUE  DWGHT */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  SCALE MATRIX T USING WT, I.E., COMPUTE WTT = WT*T */
/* ***END PROLOGUE  DWGHT */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    wt_dim1 = *ldwt;
    wt_dim2 = *ld2wt;
    wt_offset = 1 + wt_dim1 * (1 + wt_dim2);
    wt -= wt_offset;
    t_dim1 = *ldt;
    t_offset = 1 + t_dim1;
    t -= t_offset;
    wtt_dim1 = *ldwtt;
    wtt_offset = 1 + wtt_dim1;
    wtt -= wtt_offset;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   I:       AN INDEXING VARIABLE. */
/*   J:       AN INDEXING VARIABLE. */
/*   K:       AN INDEXING VARIABLE. */
/*   LDT:     THE LEADING DIMENSION OF ARRAY T. */
/*   LDWT:    THE LEADING DIMENSION OF ARRAY WT. */
/*   LDWTT:   THE LEADING DIMENSION OF ARRAY WTT. */
/*   LD2WT:   THE SECOND DIMENSION OF ARRAY WT. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN T. */
/*   N:       THE NUMBER OF ROWS OF DATA IN T. */
/*   T:       THE ARRAY BEING SCALED BY WT. */
/*   TEMP:    A TEMPORARY SCALAR. */
/*   WT:      THE WEIGHTS. */
/*   WTT:     THE RESULTS OF WEIGHTING ARRAY T BY WT. */
/*            ARRAY WTT CAN BE THE SAME AS T ONLY IF THE ARRAYS IN WT */
/*            ARE UPPER TRIANGULAR WITH ZEROS BELOW THE DIAGONAL. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DWGHT */
    if (*n == 0 || *m == 0) {
	return 0;
    }
    if (wt[(wt_dim2 + 1) * wt_dim1 + 1] >= zero) {
	if (*ldwt >= *n) {
	    if (*ld2wt >= *m) {
/*  WT IS AN N-ARRAY OF M BY M MATRICES */
		i__1 = *n;
		for (i__ = 1; i__ <= i__1; ++i__) {
		    i__2 = *m;
		    for (j = 1; j <= i__2; ++j) {
			temp = zero;
			i__3 = *m;
			for (k = 1; k <= i__3; ++k) {
			    temp += wt[i__ + (j + k * wt_dim2) * wt_dim1] * t[
				    i__ + k * t_dim1];
/* L110: */
			}
			wtt[i__ + j * wtt_dim1] = temp;
/* L120: */
		    }
/* L130: */
		}
	    } else {
/*  WT IS AN N-ARRAY OF DIAGONAL MATRICES */
		i__1 = *n;
		for (i__ = 1; i__ <= i__1; ++i__) {
		    i__2 = *m;
		    for (j = 1; j <= i__2; ++j) {
			wtt[i__ + j * wtt_dim1] = wt[i__ + (j * wt_dim2 + 1) *
				 wt_dim1] * t[i__ + j * t_dim1];
/* L220: */
		    }
/* L230: */
		}
	    }
	} else {
	    if (*ld2wt >= *m) {
/*  WT IS AN M BY M MATRIX */
		i__1 = *n;
		for (i__ = 1; i__ <= i__1; ++i__) {
		    i__2 = *m;
		    for (j = 1; j <= i__2; ++j) {
			temp = zero;
			i__3 = *m;
			for (k = 1; k <= i__3; ++k) {
			    temp += wt[(j + k * wt_dim2) * wt_dim1 + 1] * t[
				    i__ + k * t_dim1];
/* L310: */
			}
			wtt[i__ + j * wtt_dim1] = temp;
/* L320: */
		    }
/* L330: */
		}
	    } else {
/*  WT IS A DIAGONAL MATRICE */
		i__1 = *n;
		for (i__ = 1; i__ <= i__1; ++i__) {
		    i__2 = *m;
		    for (j = 1; j <= i__2; ++j) {
			wtt[i__ + j * wtt_dim1] = wt[(j * wt_dim2 + 1) * 
				wt_dim1 + 1] * t[i__ + j * t_dim1];
/* L420: */
		    }
/* L430: */
		}
	    }
	}
    } else {
/*  WT IS A SCALAR */
	i__1 = *m;
	for (j = 1; j <= i__1; ++j) {
	    i__2 = *n;
	    for (i__ = 1; i__ <= i__2; ++i__) {
		wtt[i__ + j * wtt_dim1] = (d__1 = wt[(wt_dim2 + 1) * wt_dim1 
			+ 1], abs(d__1)) * t[i__ + j * t_dim1];
/* L510: */
	    }
/* L520: */
	}
    }
    return 0;
} /* dwght_ */

/* DWINF */
/* Subroutine */ int dwinf_(integer *n, integer *m, integer *np, integer *nq, 
	integer *ldwe, integer *ld2we, logical *isodr, integer *deltai, 
	integer *epsi, integer *xplusi, integer *fni, integer *sdi, integer *
	vcvi, integer *rvari, integer *wssi, integer *wssdei, integer *wssepi,
	 integer *rcondi, integer *etai, integer *olmavi, integer *taui, 
	integer *alphai, integer *actrsi, integer *pnormi, integer *rnorsi, 
	integer *prersi, integer *partli, integer *sstoli, integer *taufci, 
	integer *epsmai, integer *beta0i, integer *betaci, integer *betasi, 
	integer *betani, integer *si, integer *ssi, integer *ssfi, integer *
	qrauxi, integer *ui, integer *fsi, integer *fjacbi, integer *we1i, 
	integer *diffi, integer *deltsi, integer *deltni, integer *ti, 
	integer *tti, integer *omegai, integer *fjacdi, integer *wrk1i, 
	integer *wrk2i, integer *wrk3i, integer *wrk4i, integer *wrk5i, 
	integer *wrk6i, integer *wrk7i, integer *lwkmn)
{
    static integer next;

/* ***BEGIN PROLOGUE  DWINF */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920619   (YYMMDD) */
/* ***PURPOSE  SET STORAGE LOCATIONS WITHIN DOUBLE PRECISION WORK SPACE */
/* ***END PROLOGUE  DWINF */
/* ...SCALAR ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   ACTRSI:  THE LOCATION IN ARRAY WORK OF VARIABLE ACTRS. */
/*   ALPHAI:  THE LOCATION IN ARRAY WORK OF VARIABLE ALPHA. */
/*   BETACI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY BETAC. */
/*   BETANI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY BETAN. */
/*   BETASI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY BETAS. */
/*   BETA0I:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY BETA0. */
/*   DELTAI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY DELTA. */
/*   DELTNI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY DELTAN. */
/*   DELTSI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY DELTAS. */
/*   DIFFI:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY DIFF. */
/*   EPSI:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY EPS. */
/*   EPSMAI:  THE LOCATION IN ARRAY WORK OF VARIABLE EPSMAC. */
/*   ETAI:    THE LOCATION IN ARRAY WORK OF VARIABLE ETA. */
/*   FJACBI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY FJACB. */
/*   FJACDI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY FJACD. */
/*   FNI:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY FN. */
/*   FSI:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY FS. */
/*   ISODR:   THE VARIABLE DESIGNATING WHETHER THE SOLUTION IS BY ODR */
/*            (ISODR=TRUE) OR BY OLS (ISODR=FALSE). */
/*   LDWE:    THE LEADING DIMENSION OF ARRAY WE. */
/*   LD2WE:   THE SECOND DIMENSION OF ARRAY WE. */
/*   LWKMN:   THE MINIMUM ACCEPTABLE LENGTH OF VECTOR WORK. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN THE EXPLANATORY VARIABLE. */
/*   N:       THE NUMBER OF OBSERVATIONS. */
/*   NEXT:    THE NEXT AVAILABLE LOCATION WITH WORK. */
/*   NP:      THE NUMBER OF FUNCTION PARAMETERS. */
/*   NQ:      THE NUMBER OF RESPONSES PER OBSERVATION. */
/*   OLMAVI:  THE LOCATION IN ARRAY WORK OF VARIABLE OLMAVG. */
/*   OMEGAI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY OMEGA. */
/*   PARTLI:  THE LOCATION IN ARRAY WORK OF VARIABLE PARTOL. */
/*   PNORMI:  THE LOCATION IN ARRAY WORK OF VARIABLE PNORM. */
/*   PRERSI:  THE LOCATION IN ARRAY WORK OF VARIABLE PRERS. */
/*   QRAUXI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY QRAUX. */
/*   RCONDI:  THE LOCATION IN ARRAY WORK OF VARIABLE RCONDI. */
/*   RNORSI:  THE LOCATION IN ARRAY WORK OF VARIABLE RNORMS. */
/*   RVARI:   THE LOCATION IN ARRAY WORK OF VARIABLE RVAR. */
/*   SDI:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY SD. */
/*   SI:      THE STARTING LOCATION IN ARRAY WORK OF ARRAY S. */
/*   SSFI:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY SSF. */
/*   SSI:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY SS. */
/*   SSTOLI:  THE LOCATION IN ARRAY WORK OF VARIABLE SSTOL. */
/*   TAUFCI:  THE LOCATION IN ARRAY WORK OF VARIABLE TAUFAC. */
/*   TAUI:    THE LOCATION IN ARRAY WORK OF VARIABLE TAU. */
/*   TI:      THE STARTING LOCATION IN ARRAY WORK OF ARRAY T. */
/*   TTI:     THE STARTING LOCATION IN ARRAY WORK OF ARRAY TT. */
/*   UI:      THE STARTING LOCATION IN ARRAY WORK OF ARRAY U. */
/*   VCVI:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY VCV. */
/*   WE1I:    THE STARTING LOCATION IN ARRAY WORK OF ARRAY WE1. */
/*   WRK1I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK1. */
/*   WRK2I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK2. */
/*   WRK3I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK3. */
/*   WRK4I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK4. */
/*   WRK5I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK5. */
/*   WRK6I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK6. */
/*   WRK7I:   THE STARTING LOCATION IN ARRAY WORK OF ARRAY WRK7. */
/*   WSSI:    THE LOCATION IN ARRAY WORK OF VARIABLE WSS. */
/*   WSSDEI:  THE LOCATION IN ARRAY WORK OF VARIABLE WSSDEL. */
/*   WSSEPI:  THE LOCATION IN ARRAY WORK OF VARIABLE WSSEPS. */
/*   XPLUSI:  THE STARTING LOCATION IN ARRAY WORK OF ARRAY XPLUSD. */
/* ***FIRST EXECUTABLE STATEMENT  DWINF */
    if (*n >= 1 && *m >= 1 && *np >= 1 && *nq >= 1 && *ldwe >= 1 && *ld2we >= 
	    1) {
	*deltai = 1;
	*epsi = *deltai + *n * *m;
	*xplusi = *epsi + *n * *nq;
	*fni = *xplusi + *n * *m;
	*sdi = *fni + *n * *nq;
	*vcvi = *sdi + *np;
	*rvari = *vcvi + *np * *np;
	*wssi = *rvari + 1;
	*wssdei = *wssi + 1;
	*wssepi = *wssdei + 1;
	*rcondi = *wssepi + 1;
	*etai = *rcondi + 1;
	*olmavi = *etai + 1;
	*taui = *olmavi + 1;
	*alphai = *taui + 1;
	*actrsi = *alphai + 1;
	*pnormi = *actrsi + 1;
	*rnorsi = *pnormi + 1;
	*prersi = *rnorsi + 1;
	*partli = *prersi + 1;
	*sstoli = *partli + 1;
	*taufci = *sstoli + 1;
	*epsmai = *taufci + 1;
	*beta0i = *epsmai + 1;
	*betaci = *beta0i + *np;
	*betasi = *betaci + *np;
	*betani = *betasi + *np;
	*si = *betani + *np;
	*ssi = *si + *np;
	*ssfi = *ssi + *np;
	*qrauxi = *ssfi + *np;
	*ui = *qrauxi + *np;
	*fsi = *ui + *np;
	*fjacbi = *fsi + *n * *nq;
	*we1i = *fjacbi + *n * *np * *nq;
	*diffi = *we1i + *ldwe * *ld2we * *nq;
	next = *diffi + *nq * (*np + *m);
	if (*isodr) {
	    *deltsi = next;
	    *deltni = *deltsi + *n * *m;
	    *ti = *deltni + *n * *m;
	    *tti = *ti + *n * *m;
	    *omegai = *tti + *n * *m;
	    *fjacdi = *omegai + *nq * *nq;
	    *wrk1i = *fjacdi + *n * *m * *nq;
	    next = *wrk1i + *n * *m * *nq;
	} else {
	    *deltsi = *deltai;
	    *deltni = *deltai;
	    *ti = *deltai;
	    *tti = *deltai;
	    *omegai = *deltai;
	    *fjacdi = *deltai;
	    *wrk1i = *deltai;
	}
	*wrk2i = next;
	*wrk3i = *wrk2i + *n * *nq;
	*wrk4i = *wrk3i + *np;
	*wrk5i = *wrk4i + *m * *m;
	*wrk6i = *wrk5i + *m;
	*wrk7i = *wrk6i + *n * *nq * *np;
	next = *wrk7i + *nq * 5;
	*lwkmn = next;
    } else {
	*deltai = 1;
	*epsi = 1;
	*xplusi = 1;
	*fni = 1;
	*sdi = 1;
	*vcvi = 1;
	*rvari = 1;
	*wssi = 1;
	*wssdei = 1;
	*wssepi = 1;
	*rcondi = 1;
	*etai = 1;
	*olmavi = 1;
	*taui = 1;
	*alphai = 1;
	*actrsi = 1;
	*pnormi = 1;
	*rnorsi = 1;
	*prersi = 1;
	*partli = 1;
	*sstoli = 1;
	*taufci = 1;
	*epsmai = 1;
	*beta0i = 1;
	*betaci = 1;
	*betasi = 1;
	*betani = 1;
	*si = 1;
	*ssi = 1;
	*ssfi = 1;
	*qrauxi = 1;
	*fsi = 1;
	*ui = 1;
	*fjacbi = 1;
	*we1i = 1;
	*diffi = 1;
	*deltsi = 1;
	*deltni = 1;
	*ti = 1;
	*tti = 1;
	*fjacdi = 1;
	*omegai = 1;
	*wrk1i = 1;
	*wrk2i = 1;
	*wrk3i = 1;
	*wrk4i = 1;
	*wrk5i = 1;
	*wrk6i = 1;
	*wrk7i = 1;
	*lwkmn = 1;
    }
    return 0;
} /* dwinf_ */

/* DXMY */
/* Subroutine */ int dxmy_(integer *n, integer *m, doublereal *x, integer *
	ldx, doublereal *y, integer *ldy, doublereal *xmy, integer *ldxmy)
{
    /* System generated locals */
    integer x_dim1, x_offset, xmy_dim1, xmy_offset, y_dim1, y_offset, i__1, 
	    i__2;

    /* Local variables */
    static integer i__, j;

/* ***BEGIN PROLOGUE  DXMY */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  COMPUTE XMY = X - Y */
/* ***END PROLOGUE  DXMY */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   I:       AN INDEXING VARIABLE. */
/*   J:       AN INDEXING VARIABLE. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   LDXMY:   THE LEADING DIMENSION OF ARRAY XMY. */
/*   LDY:     THE LEADING DIMENSION OF ARRAY Y. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN ARRAYS X AND Y. */
/*   N:       THE NUMBER OF ROWS OF DATA IN ARRAYS X AND Y. */
/*   X:       THE FIRST OF THE TWO ARRAYS. */
/*   XMY:     THE VALUES OF X-Y. */
/*   Y:       THE SECOND OF THE TWO ARRAYS. */
/* ***FIRST EXECUTABLE STATEMENT  DXMY */
    /* Parameter adjustments */
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    y_dim1 = *ldy;
    y_offset = 1 + y_dim1;
    y -= y_offset;
    xmy_dim1 = *ldxmy;
    xmy_offset = 1 + xmy_dim1;
    xmy -= xmy_offset;

    /* Function Body */
    i__1 = *m;
    for (j = 1; j <= i__1; ++j) {
	i__2 = *n;
	for (i__ = 1; i__ <= i__2; ++i__) {
	    xmy[i__ + j * xmy_dim1] = x[i__ + j * x_dim1] - y[i__ + j * 
		    y_dim1];
/* L10: */
	}
/* L20: */
    }
    return 0;
} /* dxmy_ */

/* DXPY */
/* Subroutine */ int dxpy_(integer *n, integer *m, doublereal *x, integer *
	ldx, doublereal *y, integer *ldy, doublereal *xpy, integer *ldxpy)
{
    /* System generated locals */
    integer x_dim1, x_offset, xpy_dim1, xpy_offset, y_dim1, y_offset, i__1, 
	    i__2;

    /* Local variables */
    static integer i__, j;

/* ***BEGIN PROLOGUE  DXPY */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  COMPUTE XPY = X + Y */
/* ***END PROLOGUE  DXPY */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   I:       AN INDEXING VARIABLE. */
/*   J:       AN INDEXING VARIABLE. */
/*   LDX:     THE LEADING DIMENSION OF ARRAY X. */
/*   LDXPY:   THE LEADING DIMENSION OF ARRAY XPY. */
/*   LDY:     THE LEADING DIMENSION OF ARRAY Y. */
/*   M:       THE NUMBER OF COLUMNS OF DATA IN ARRAYS X AND Y. */
/*   N:       THE NUMBER OF ROWS OF DATA IN ARRAYS X AND Y. */
/*   X:       THE FIRST OF THE TWO ARRAYS TO BE ADDED TOGETHER. */
/*   XPY:     THE VALUES OF X+Y. */
/*   Y:       THE SECOND OF THE TWO ARRAYS TO BE ADDED TOGETHER. */
/* ***FIRST EXECUTABLE STATEMENT  DXPY */
    /* Parameter adjustments */
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    y_dim1 = *ldy;
    y_offset = 1 + y_dim1;
    y -= y_offset;
    xpy_dim1 = *ldxpy;
    xpy_offset = 1 + xpy_dim1;
    xpy -= xpy_offset;

    /* Function Body */
    i__1 = *m;
    for (j = 1; j <= i__1; ++j) {
	i__2 = *n;
	for (i__ = 1; i__ <= i__2; ++i__) {
	    xpy[i__ + j * xpy_dim1] = x[i__ + j * x_dim1] + y[i__ + j * 
		    y_dim1];
/* L10: */
	}
/* L20: */
    }
    return 0;
} /* dxpy_ */

/* DZERO */
/* Subroutine */ int dzero_(integer *n, integer *m, doublereal *a, integer *
	lda)
{
    /* Initialized data */

    static doublereal zero = 0.;

    /* System generated locals */
    integer a_dim1, a_offset, i__1, i__2;

    /* Local variables */
    static integer i__, j;

/* ***BEGIN PROLOGUE  DZERO */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  SET A = ZERO */
/* ***END PROLOGUE  DZERO */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    a_dim1 = *lda;
    a_offset = 1 + a_dim1;
    a -= a_offset;

    /* Function Body */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*   A:       THE ARRAY TO BE SET TO ZERO. */
/*   I:       AN INDEXING VARIABLE. */
/*   J:       AN INDEXING VARIABLE. */
/*   LDA:     THE LEADING DIMENSION OF ARRAY A. */
/*   M:       THE NUMBER OF COLUMNS TO BE SET TO ZERO. */
/*   N:       THE NUMBER OF ROWS TO BE SET TO ZERO. */
/*   ZERO:    THE VALUE 0.0D0. */
/* ***FIRST EXECUTABLE STATEMENT  DZERO */
    i__1 = *m;
    for (j = 1; j <= i__1; ++j) {
	i__2 = *n;
	for (i__ = 1; i__ <= i__2; ++i__) {
	    a[i__ + j * a_dim1] = zero;
/* L10: */
	}
/* L20: */
    }
    return 0;
} /* dzero_ */

