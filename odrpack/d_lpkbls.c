/* d_lpkbls.f -- translated by f2c (version 20100827).
   You must link the resulting object file with libf2c:
	on Microsoft Windows system, link with libf2c.lib;
	on Linux or Unix systems, link with .../path/to/libf2c.a -lm
	or, if you install libf2c.a in a standard place, with -lf2c -lm
	-- in that order, at the end of the command line, as in
		cc *.o -lf2c -lm
	Source for libf2c is in /netlib/f2c/libf2c.zip, e.g.,

		http://www.netlib.org/f2c/libf2c.zip
*/

/* This file was heavily modified to allow complete removal of 
  dependencies on f2c library and others */
typedef int /* Unknown procedure type */ (*U_fp)();
typedef /* Subroutine */ int (*S_fp)();

#define TRUE_ 1
#define FALSE_ 0
#define abs(x) ((x) >= 0 ? (x) : -(x))
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))

/* Table of constant values */

static int c__1 = 1;

/* DASUM */
double dasum_(int *n, double *dx, int *incx)
{
    /* System generated locals */
    int i__1, i__2;
    double ret_val, d__1, d__2, d__3, d__4, d__5, d__6;

    /* Local variables */
    static int i__, m, ns, mp1;

/* ***BEGIN PROLOGUE  DASUM */
/* ***DATE WRITTEN   791001   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D1A3A */
/* ***KEYWORDS  ADD,BLAS,DOUBLE PRECISION,LINEAR ALGEBRA,MAGNITUDE,SUM, */
/*             VECTOR */
/* ***AUTHOR  LAWSON, C. L., (JPL) */
/*           HANSON, R. J., (SNLA) */
/*           KINCAID, D. R., (U. OF TEXAS) */
/*           KROGH, F. T., (JPL) */
/* ***PURPOSE  SUM OF MAGNITUDES OF D.P. VECTOR COMPONENTS */
/* ***DESCRIPTION */
/*                B L A S  SUBPROGRAM */
/*    DESCRIPTION OF PARAMETERS */
/*     --INPUT-- */
/*        N  NUMBER OF ELEMENTS IN INPUT VECTOR(S) */
/*       DX  DOUBLE PRECISION VECTOR WITH N ELEMENTS */
/*     INCX  STORAGE SPACING BETWEEN ELEMENTS OF DX */
/*     --OUTPUT-- */
/*    DASUM  DOUBLE PRECISION RESULT (ZERO IF N .LE. 0) */
/*     RETURNS SUM OF MAGNITUDES OF DOUBLE PRECISION DX. */
/*     DASUM = SUM FROM 0 TO N-1 OF DABS(DX(1+I*INCX)) */
/* ***REFERENCES  LAWSON C.L., HANSON R.J., KINCAID D.R., KROGH F.T., */
/*                 *BASIC LINEAR ALGEBRA SUBPROGRAMS FOR FORTRAN USAGE*, */
/*                 ALGORITHM NO. 539, TRANSACTIONS ON MATHEMATICAL */
/*                 SOFTWARE, VOLUME 5, NUMBER 3, SEPTEMBER 1979, 308-323 */
/* ***ROUTINES CALLED  (NONE) */
/* ***END PROLOGUE  DASUM */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ***FIRST EXECUTABLE STATEMENT  DASUM */
    /* Parameter adjustments */
    --dx;

    /* Function Body */
    ret_val = 0.;
    if (*n <= 0) {
	return ret_val;
    }
    if (*incx == 1) {
	goto L20;
    }
/*        CODE FOR INCREMENTS NOT EQUAL TO 1. */
    ns = *n * *incx;
    i__1 = ns;
    i__2 = *incx;
    for (i__ = 1; i__2 < 0 ? i__ >= i__1 : i__ <= i__1; i__ += i__2) {
	ret_val += (d__1 = dx[i__], abs(d__1));
/* L10: */
    }
    return ret_val;
/*        CODE FOR INCREMENTS EQUAL TO 1. */
/*        CLEAN-UP LOOP SO REMAINING VECTOR LENGTH IS A MULTIPLE OF 6. */
L20:
    m = *n % 6;
    if (m == 0) {
	goto L40;
    }
    i__2 = m;
    for (i__ = 1; i__ <= i__2; ++i__) {
	ret_val += (d__1 = dx[i__], abs(d__1));
/* L30: */
    }
    if (*n < 6) {
	return ret_val;
    }
L40:
    mp1 = m + 1;
    i__2 = *n;
    for (i__ = mp1; i__ <= i__2; i__ += 6) {
	ret_val = ret_val + (d__1 = dx[i__], abs(d__1)) + (d__2 = dx[i__ + 1],
		 abs(d__2)) + (d__3 = dx[i__ + 2], abs(d__3)) + (d__4 = dx[
		i__ + 3], abs(d__4)) + (d__5 = dx[i__ + 4], abs(d__5)) + (
		d__6 = dx[i__ + 5], abs(d__6));
/* L50: */
    }
    return ret_val;
} /* dasum_ */

/* DAXPY */
/* Subroutine */ int daxpy_(int *n, double *da, double *dx, 
	int *incx, double *dy, int *incy)
{
    /* System generated locals */
    int i__1, i__2;

    /* Local variables */
    static int i__, m, ix, iy, ns, mp1;

/* ***BEGIN PROLOGUE  DAXPY */
/* ***DATE WRITTEN   791001   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D1A7 */
/* ***KEYWORDS  BLAS,DOUBLE PRECISION,LINEAR ALGEBRA,TRIAD,VECTOR */
/* ***AUTHOR  LAWSON, C. L., (JPL) */
/*           HANSON, R. J., (SNLA) */
/*           KINCAID, D. R., (U. OF TEXAS) */
/*           KROGH, F. T., (JPL) */
/* ***PURPOSE  D.P COMPUTATION Y = A*X + Y */
/* ***DESCRIPTION */
/*                B L A S  SUBPROGRAM */
/*    DESCRIPTION OF PARAMETERS */
/*     --INPUT-- */
/*        N  NUMBER OF ELEMENTS IN INPUT VECTOR(S) */
/*       DA  DOUBLE PRECISION SCALAR MULTIPLIER */
/*       DX  DOUBLE PRECISION VECTOR WITH N ELEMENTS */
/*     INCX  STORAGE SPACING BETWEEN ELEMENTS OF DX */
/*       DY  DOUBLE PRECISION VECTOR WITH N ELEMENTS */
/*     INCY  STORAGE SPACING BETWEEN ELEMENTS OF DY */
/*     --OUTPUT-- */
/*       DY  DOUBLE PRECISION RESULT (UNCHANGED IF N .LE. 0) */
/*     OVERWRITE DOUBLE PRECISION DY WITH DOUBLE PRECISION DA*DX + DY. */
/*     FOR I = 0 TO N-1, REPLACE  DY(LY+I*INCY) WITH DA*DX(LX+I*INCX) + */
/*       DY(LY+I*INCY), WHERE LX = 1 IF INCX .GE. 0, ELSE LX = (-INCX)*N */
/*       AND LY IS DEFINED IN A SIMILAR WAY USING INCY. */
/* ***REFERENCES  LAWSON C.L., HANSON R.J., KINCAID D.R., KROGH F.T., */
/*                 *BASIC LINEAR ALGEBRA SUBPROGRAMS FOR FORTRAN USAGE*, */
/*                 ALGORITHM NO. 539, TRANSACTIONS ON MATHEMATICAL */
/*                 SOFTWARE, VOLUME 5, NUMBER 3, SEPTEMBER 1979, 308-323 */
/* ***ROUTINES CALLED  (NONE) */
/* ***END PROLOGUE  DAXPY */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ***FIRST EXECUTABLE STATEMENT  DAXPY */
    /* Parameter adjustments */
    --dy;
    --dx;

    /* Function Body */
    if (*n <= 0 || *da == 0.) {
	return 0;
    }
    if (*incx == *incy) {
	if ((i__1 = *incx - 1) < 0) {
	    goto L5;
	} else if (i__1 == 0) {
	    goto L20;
	} else {
	    goto L60;
	}
    }
L5:
/*        CODE FOR NONEQUAL OR NONPOSITIVE INCREMENTS. */
    ix = 1;
    iy = 1;
    if (*incx < 0) {
	ix = (-(*n) + 1) * *incx + 1;
    }
    if (*incy < 0) {
	iy = (-(*n) + 1) * *incy + 1;
    }
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	dy[iy] += *da * dx[ix];
	ix += *incx;
	iy += *incy;
/* L10: */
    }
    return 0;
/*        CODE FOR BOTH INCREMENTS EQUAL TO 1 */
/*        CLEAN-UP LOOP SO REMAINING VECTOR LENGTH IS A MULTIPLE OF 4. */
L20:
    m = *n % 4;
    if (m == 0) {
	goto L40;
    }
    i__1 = m;
    for (i__ = 1; i__ <= i__1; ++i__) {
	dy[i__] += *da * dx[i__];
/* L30: */
    }
    if (*n < 4) {
	return 0;
    }
L40:
    mp1 = m + 1;
    i__1 = *n;
    for (i__ = mp1; i__ <= i__1; i__ += 4) {
	dy[i__] += *da * dx[i__];
	dy[i__ + 1] += *da * dx[i__ + 1];
	dy[i__ + 2] += *da * dx[i__ + 2];
	dy[i__ + 3] += *da * dx[i__ + 3];
/* L50: */
    }
    return 0;
/*        CODE FOR EQUAL, POSITIVE, NONUNIT INCREMENTS. */
L60:
    ns = *n * *incx;
    i__1 = ns;
    i__2 = *incx;
    for (i__ = 1; i__2 < 0 ? i__ >= i__1 : i__ <= i__1; i__ += i__2) {
	dy[i__] = *da * dx[i__] + dy[i__];
/* L70: */
    }
    return 0;
} /* daxpy_ */

/* DCHEX */
/* Subroutine */ int dchex_(double *r__, int *ldr, int *p, 
	int *k, int *l, double *z__, int *ldz, int *nz, 
	double *c__, double *s, int *job)
{
    /* System generated locals */
    int r_dim1, r_offset, z_dim1, z_offset, i__1, i__2, i__3;

    /* Local variables */
    static int i__, j;
    static double t, t1;
    static int ii, jj, il, iu, km1, lm1, kp1, lmk;
    extern /* Subroutine */ int drotg_(double *, double *, double 
	    *, double *);

/* ***BEGIN PROLOGUE  DCHEX */
/* ***DATE WRITTEN   780814   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D7B */
/* ***KEYWORDS  CHOLESKY DECOMPOSITION,DOUBLE PRECISION,EXCHANGE, */
/*             LINEAR ALGEBRA,LINPACK,MATRIX,POSITIVE DEFINITE */
/* ***AUTHOR  STEWART, G. W., (U. OF MARYLAND) */
/* ***PURPOSE  UPDATES THE CHOLESKY FACTORIZATION  A=TRANS(R)*R  OF A */
/*            POSITIVE DEFINITE MATRIX A OF ORDER P UNDER DIAGONAL */
/*            PERMUTATIONS OF THE FORM  TRANS(E)*A*E  WHERE E IS A */
/*            PERMUTATION MATRIX. */
/* ***DESCRIPTION */
/*     DCHEX UPDATES THE CHOLESKY FACTORIZATION */
/*                   A = TRANS(R)*R */
/*     OF A POSITIVE DEFINITE MATRIX A OF ORDER P UNDER DIAGONAL */
/*     PERMUTATIONS OF THE FORM */
/*                   TRANS(E)*A*E */
/*     WHERE E IS A PERMUTATION MATRIX.  SPECIFICALLY, GIVEN */
/*     AN UPPER TRIANGULAR MATRIX R AND A PERMUTATION MATRIX */
/*     E (WHICH IS SPECIFIED BY K, L, AND JOB), DCHEX DETERMINES */
/*     AN ORTHOGONAL MATRIX U SUCH THAT */
/*                           U*R*E = RR, */
/*     WHERE RR IS UPPER TRIANGULAR.  AT THE USERS OPTION, THE */
/*     TRANSFORMATION U WILL BE MULTIPLIED INTO THE ARRAY Z. */
/*     IF A = TRANS(X)*X, SO THAT R IS THE TRIANGULAR PART OF THE */
/*     QR FACTORIZATION OF X, THEN RR IS THE TRIANGULAR PART OF THE */
/*     QR FACTORIZATION OF X*E, I.E. X WITH ITS COLUMNS PERMUTED. */
/*     FOR A LESS TERSE DESCRIPTION OF WHAT DCHEX DOES AND HOW */
/*     IT MAY BE APPLIED, SEE THE LINPACK GUIDE. */
/*     THE MATRIX Q IS DETERMINED AS THE PRODUCT U(L-K)*...*U(1) */
/*     OF PLANE ROTATIONS OF THE FORM */
/*                           (    C(I)       S(I) ) */
/*                           (                    ) , */
/*                           (    -S(I)      C(I) ) */
/*     WHERE C(I) IS DOUBLE PRECISION.  THE ROWS THESE ROTATIONS OPERATE */
/*     ON ARE DESCRIBED BELOW. */
/*     THERE ARE TWO TYPES OF PERMUTATIONS, WHICH ARE DETERMINED */
/*     BY THE VALUE OF JOB. */
/*     1. RIGHT CIRCULAR SHIFT (JOB = 1). */
/*         THE COLUMNS ARE REARRANGED IN THE FOLLOWING ORDER. */
/*                1,...,K-1,L,K,K+1,...,L-1,L+1,...,P. */
/*         U IS THE PRODUCT OF L-K ROTATIONS U(I), WHERE U(I) */
/*         ACTS IN THE (L-I,L-I+1)-PLANE. */
/*     2. LEFT CIRCULAR SHIFT (JOB = 2). */
/*         THE COLUMNS ARE REARRANGED IN THE FOLLOWING ORDER */
/*                1,...,K-1,K+1,K+2,...,L,K,L+1,...,P. */
/*         U IS THE PRODUCT OF L-K ROTATIONS U(I), WHERE U(I) */
/*         ACTS IN THE (K+I-1,K+I)-PLANE. */
/*     ON ENTRY */
/*         R      DOUBLE PRECISION(LDR,P), WHERE LDR .GE. P. */
/*                R CONTAINS THE UPPER TRIANGULAR FACTOR */
/*                THAT IS TO BE UPDATED.  ELEMENTS OF R */
/*                BELOW THE DIAGONAL ARE NOT REFERENCED. */
/*         LDR    INTEGER. */
/*                LDR IS THE LEADING DIMENSION OF THE ARRAY R. */
/*         P      INTEGER. */
/*                P IS THE ORDER OF THE MATRIX R. */
/*         K      INTEGER. */
/*                K IS THE FIRST COLUMN TO BE PERMUTED. */
/*         L      INTEGER. */
/*                L IS THE LAST COLUMN TO BE PERMUTED. */
/*                L MUST BE STRICTLY GREATER THAN K. */
/*         Z      DOUBLE PRECISION(LDZ,N)Z), WHERE LDZ .GE. P. */
/*                Z IS AN ARRAY OF NZ P-VECTORS INTO WHICH THE */
/*                TRANSFORMATION U IS MULTIPLIED.  Z IS */
/*                NOT REFERENCED IF NZ = 0. */
/*         LDZ    INTEGER. */
/*                LDZ IS THE LEADING DIMENSION OF THE ARRAY Z. */
/*         NZ     INTEGER. */
/*                NZ IS THE NUMBER OF COLUMNS OF THE MATRIX Z. */
/*         JOB    INTEGER. */
/*                JOB DETERMINES THE TYPE OF PERMUTATION. */
/*                       JOB = 1  RIGHT CIRCULAR SHIFT. */
/*                       JOB = 2  LEFT CIRCULAR SHIFT. */
/*     ON RETURN */
/*         R      CONTAINS THE UPDATED FACTOR. */
/*         Z      CONTAINS THE UPDATED MATRIX Z. */
/*         C      DOUBLE PRECISION(P). */
/*                C CONTAINS THE COSINES OF THE TRANSFORMING ROTATIONS. */
/*         S      DOUBLE PRECISION(P). */
/*                S CONTAINS THE SINES OF THE TRANSFORMING ROTATIONS. */
/*     LINPACK.  THIS VERSION DATED 08/14/78 . */
/*     G. W. STEWART, UNIVERSITY OF MARYLAND, ARGONNE NATIONAL LAB. */
/* ***REFERENCES  DONGARRA J.J., BUNCH J.R., MOLER C.B., STEWART G.W., */
/*                 *LINPACK USERS  GUIDE*, SIAM, 1979. */
/* ***ROUTINES CALLED  DROTG */
/* ***END PROLOGUE  DCHEX */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ***FIRST EXECUTABLE STATEMENT  DCHEX */
    /* Parameter adjustments */
    r_dim1 = *ldr;
    r_offset = 1 + r_dim1;
    r__ -= r_offset;
    z_dim1 = *ldz;
    z_offset = 1 + z_dim1;
    z__ -= z_offset;
    --c__;
    --s;

    /* Function Body */
    km1 = *k - 1;
    kp1 = *k + 1;
    lmk = *l - *k;
    lm1 = *l - 1;
/*     PERFORM THE APPROPRIATE TASK. */
    switch (*job) {
	case 1:  goto L10;
	case 2:  goto L130;
    }
/*     RIGHT CIRCULAR SHIFT. */
L10:
/*        REORDER THE COLUMNS. */
    i__1 = *l;
    for (i__ = 1; i__ <= i__1; ++i__) {
	ii = *l - i__ + 1;
	s[i__] = r__[ii + *l * r_dim1];
/* L20: */
    }
    i__1 = lm1;
    for (jj = *k; jj <= i__1; ++jj) {
	j = lm1 - jj + *k;
	i__2 = j;
	for (i__ = 1; i__ <= i__2; ++i__) {
	    r__[i__ + (j + 1) * r_dim1] = r__[i__ + j * r_dim1];
/* L30: */
	}
	r__[j + 1 + (j + 1) * r_dim1] = 0.;
/* L40: */
    }
    if (*k == 1) {
	goto L60;
    }
    i__1 = km1;
    for (i__ = 1; i__ <= i__1; ++i__) {
	ii = *l - i__ + 1;
	r__[i__ + *k * r_dim1] = s[ii];
/* L50: */
    }
L60:
/*        CALCULATE THE ROTATIONS. */
    t = s[1];
    i__1 = lmk;
    for (i__ = 1; i__ <= i__1; ++i__) {
	t1 = s[i__];
	drotg_(&s[i__ + 1], &t, &c__[i__], &t1);
	s[i__] = t1;
	t = s[i__ + 1];
/* L70: */
    }
    r__[*k + *k * r_dim1] = t;
    i__1 = *p;
    for (j = kp1; j <= i__1; ++j) {
/* Computing MAX */
	i__2 = 1, i__3 = *l - j + 1;
	il = max(i__2,i__3);
	i__2 = lmk;
	for (ii = il; ii <= i__2; ++ii) {
	    i__ = *l - ii;
	    t = c__[ii] * r__[i__ + j * r_dim1] + s[ii] * r__[i__ + 1 + j * 
		    r_dim1];
	    r__[i__ + 1 + j * r_dim1] = c__[ii] * r__[i__ + 1 + j * r_dim1] - 
		    s[ii] * r__[i__ + j * r_dim1];
	    r__[i__ + j * r_dim1] = t;
/* L80: */
	}
/* L90: */
    }
/*        IF REQUIRED, APPLY THE TRANSFORMATIONS TO Z. */
    if (*nz < 1) {
	goto L120;
    }
    i__1 = *nz;
    for (j = 1; j <= i__1; ++j) {
	i__2 = lmk;
	for (ii = 1; ii <= i__2; ++ii) {
	    i__ = *l - ii;
	    t = c__[ii] * z__[i__ + j * z_dim1] + s[ii] * z__[i__ + 1 + j * 
		    z_dim1];
	    z__[i__ + 1 + j * z_dim1] = c__[ii] * z__[i__ + 1 + j * z_dim1] - 
		    s[ii] * z__[i__ + j * z_dim1];
	    z__[i__ + j * z_dim1] = t;
/* L100: */
	}
/* L110: */
    }
L120:
    goto L260;
/*     LEFT CIRCULAR SHIFT */
L130:
/*        REORDER THE COLUMNS */
    i__1 = *k;
    for (i__ = 1; i__ <= i__1; ++i__) {
	ii = lmk + i__;
	s[ii] = r__[i__ + *k * r_dim1];
/* L140: */
    }
    i__1 = lm1;
    for (j = *k; j <= i__1; ++j) {
	i__2 = j;
	for (i__ = 1; i__ <= i__2; ++i__) {
	    r__[i__ + j * r_dim1] = r__[i__ + (j + 1) * r_dim1];
/* L150: */
	}
	jj = j - km1;
	s[jj] = r__[j + 1 + (j + 1) * r_dim1];
/* L160: */
    }
    i__1 = *k;
    for (i__ = 1; i__ <= i__1; ++i__) {
	ii = lmk + i__;
	r__[i__ + *l * r_dim1] = s[ii];
/* L170: */
    }
    i__1 = *l;
    for (i__ = kp1; i__ <= i__1; ++i__) {
	r__[i__ + *l * r_dim1] = 0.;
/* L180: */
    }
/*        REDUCTION LOOP. */
    i__1 = *p;
    for (j = *k; j <= i__1; ++j) {
	if (j == *k) {
	    goto L200;
	}
/*              APPLY THE ROTATIONS. */
/* Computing MIN */
	i__2 = j - 1, i__3 = *l - 1;
	iu = min(i__2,i__3);
	i__2 = iu;
	for (i__ = *k; i__ <= i__2; ++i__) {
	    ii = i__ - *k + 1;
	    t = c__[ii] * r__[i__ + j * r_dim1] + s[ii] * r__[i__ + 1 + j * 
		    r_dim1];
	    r__[i__ + 1 + j * r_dim1] = c__[ii] * r__[i__ + 1 + j * r_dim1] - 
		    s[ii] * r__[i__ + j * r_dim1];
	    r__[i__ + j * r_dim1] = t;
/* L190: */
	}
L200:
	if (j >= *l) {
	    goto L210;
	}
	jj = j - *k + 1;
	t = s[jj];
	drotg_(&r__[j + j * r_dim1], &t, &c__[jj], &s[jj]);
L210:
/* L220: */
	;
    }
/*        APPLY THE ROTATIONS TO Z. */
    if (*nz < 1) {
	goto L250;
    }
    i__1 = *nz;
    for (j = 1; j <= i__1; ++j) {
	i__2 = lm1;
	for (i__ = *k; i__ <= i__2; ++i__) {
	    ii = i__ - km1;
	    t = c__[ii] * z__[i__ + j * z_dim1] + s[ii] * z__[i__ + 1 + j * 
		    z_dim1];
	    z__[i__ + 1 + j * z_dim1] = c__[ii] * z__[i__ + 1 + j * z_dim1] - 
		    s[ii] * z__[i__ + j * z_dim1];
	    z__[i__ + j * z_dim1] = t;
/* L230: */
	}
/* L240: */
    }
L250:
L260:
    return 0;
} /* dchex_ */

/* DCOPY */
/* Subroutine */ int dcopy_(int *n, double *dx, int *incx, 
	double *dy, int *incy)
{
    /* System generated locals */
    int i__1, i__2;

    /* Local variables */
    static int i__, m, ix, iy, ns, mp1;

/* ***BEGIN PROLOGUE  DCOPY */
/* ***DATE WRITTEN   791001   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D1A5 */
/* ***KEYWORDS  BLAS,COPY,DOUBLE PRECISION,LINEAR ALGEBRA,VECTOR */
/* ***AUTHOR  LAWSON, C. L., (JPL) */
/*           HANSON, R. J., (SNLA) */
/*           KINCAID, D. R., (U. OF TEXAS) */
/*           KROGH, F. T., (JPL) */
/* ***PURPOSE  D.P. VECTOR COPY Y = X */
/* ***DESCRIPTION */
/*                B L A S  SUBPROGRAM */
/*    DESCRIPTION OF PARAMETERS */
/*     --INPUT-- */
/*        N  NUMBER OF ELEMENTS IN INPUT VECTOR(S) */
/*       DX  DOUBLE PRECISION VECTOR WITH N ELEMENTS */
/*     INCX  STORAGE SPACING BETWEEN ELEMENTS OF DX */
/*       DY  DOUBLE PRECISION VECTOR WITH N ELEMENTS */
/*     INCY  STORAGE SPACING BETWEEN ELEMENTS OF DY */
/*     --OUTPUT-- */
/*       DY  COPY OF VECTOR DX (UNCHANGED IF N .LE. 0) */
/*     COPY DOUBLE PRECISION DX TO DOUBLE PRECISION DY. */
/*     FOR I = 0 TO N-1, COPY DX(LX+I*INCX) TO DY(LY+I*INCY), */
/*     WHERE LX = 1 IF INCX .GE. 0, ELSE LX = (-INCX)*N, AND LY IS */
/*     DEFINED IN A SIMILAR WAY USING INCY. */
/* ***REFERENCES  LAWSON C.L., HANSON R.J., KINCAID D.R., KROGH F.T., */
/*                 *BASIC LINEAR ALGEBRA SUBPROGRAMS FOR FORTRAN USAGE*, */
/*                 ALGORITHM NO. 539, TRANSACTIONS ON MATHEMATICAL */
/*                 SOFTWARE, VOLUME 5, NUMBER 3, SEPTEMBER 1979, 308-323 */
/* ***ROUTINES CALLED  (NONE) */
/* ***END PROLOGUE  DCOPY */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ***FIRST EXECUTABLE STATEMENT  DCOPY */
    /* Parameter adjustments */
    --dy;
    --dx;

    /* Function Body */
    if (*n <= 0) {
	return 0;
    }
    if (*incx == *incy) {
	if ((i__1 = *incx - 1) < 0) {
	    goto L5;
	} else if (i__1 == 0) {
	    goto L20;
	} else {
	    goto L60;
	}
    }
L5:
/*        CODE FOR UNEQUAL OR NONPOSITIVE INCREMENTS. */
    ix = 1;
    iy = 1;
    if (*incx < 0) {
	ix = (-(*n) + 1) * *incx + 1;
    }
    if (*incy < 0) {
	iy = (-(*n) + 1) * *incy + 1;
    }
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	dy[iy] = dx[ix];
	ix += *incx;
	iy += *incy;
/* L10: */
    }
    return 0;
/*        CODE FOR BOTH INCREMENTS EQUAL TO 1 */
/*        CLEAN-UP LOOP SO REMAINING VECTOR LENGTH IS A MULTIPLE OF 7. */
L20:
    m = *n % 7;
    if (m == 0) {
	goto L40;
    }
    i__1 = m;
    for (i__ = 1; i__ <= i__1; ++i__) {
	dy[i__] = dx[i__];
/* L30: */
    }
    if (*n < 7) {
	return 0;
    }
L40:
    mp1 = m + 1;
    i__1 = *n;
    for (i__ = mp1; i__ <= i__1; i__ += 7) {
	dy[i__] = dx[i__];
	dy[i__ + 1] = dx[i__ + 1];
	dy[i__ + 2] = dx[i__ + 2];
	dy[i__ + 3] = dx[i__ + 3];
	dy[i__ + 4] = dx[i__ + 4];
	dy[i__ + 5] = dx[i__ + 5];
	dy[i__ + 6] = dx[i__ + 6];
/* L50: */
    }
    return 0;
/*        CODE FOR EQUAL, POSITIVE, NONUNIT INCREMENTS. */
L60:
    ns = *n * *incx;
    i__1 = ns;
    i__2 = *incx;
    for (i__ = 1; i__2 < 0 ? i__ >= i__1 : i__ <= i__1; i__ += i__2) {
	dy[i__] = dx[i__];
/* L70: */
    }
    return 0;
} /* dcopy_ */

/* DDOT */
double ddot_(int *n, double *dx, int *incx, double *dy, 
	int *incy)
{
    /* System generated locals */
    int i__1, i__2;
    double ret_val;

    /* Local variables */
    static int i__, m, ix, iy, ns, mp1;

/* ***BEGIN PROLOGUE  DDOT */
/* ***DATE WRITTEN   791001   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D1A4 */
/* ***KEYWORDS  BLAS,DOUBLE PRECISION,INNER PRODUCT,LINEAR ALGEBRA,VECTOR */
/* ***AUTHOR  LAWSON, C. L., (JPL) */
/*           HANSON, R. J., (SNLA) */
/*           KINCAID, D. R., (U. OF TEXAS) */
/*           KROGH, F. T., (JPL) */
/* ***PURPOSE  D.P. INNER PRODUCT OF D.P. VECTORS */
/* ***DESCRIPTION */
/*                B L A S  SUBPROGRAM */
/*    DESCRIPTION OF PARAMETERS */
/*     --INPUT-- */
/*        N  NUMBER OF ELEMENTS IN INPUT VECTOR(S) */
/*       DX  DOUBLE PRECISION VECTOR WITH N ELEMENTS */
/*     INCX  STORAGE SPACING BETWEEN ELEMENTS OF DX */
/*       DY  DOUBLE PRECISION VECTOR WITH N ELEMENTS */
/*     INCY  STORAGE SPACING BETWEEN ELEMENTS OF DY */
/*     --OUTPUT-- */
/*     DDOT  DOUBLE PRECISION DOT PRODUCT (ZERO IF N .LE. 0) */
/*     RETURNS THE DOT PRODUCT OF DOUBLE PRECISION DX AND DY. */
/*     DDOT = SUM FOR I = 0 TO N-1 OF  DX(LX+I*INCX) * DY(LY+I*INCY) */
/*     WHERE LX = 1 IF INCX .GE. 0, ELSE LX = (-INCX)*N, AND LY IS */
/*     DEFINED IN A SIMILAR WAY USING INCY. */
/* ***REFERENCES  LAWSON C.L., HANSON R.J., KINCAID D.R., KROGH F.T., */
/*                 *BASIC LINEAR ALGEBRA SUBPROGRAMS FOR FORTRAN USAGE*, */
/*                 ALGORITHM NO. 539, TRANSACTIONS ON MATHEMATICAL */
/*                 SOFTWARE, VOLUME 5, NUMBER 3, SEPTEMBER 1979, 308-323 */
/* ***ROUTINES CALLED  (NONE) */
/* ***END PROLOGUE  DDOT */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ***FIRST EXECUTABLE STATEMENT  DDOT */
    /* Parameter adjustments */
    --dy;
    --dx;

    /* Function Body */
    ret_val = 0.;
    if (*n <= 0) {
	return ret_val;
    }
    if (*incx == *incy) {
	if ((i__1 = *incx - 1) < 0) {
	    goto L5;
	} else if (i__1 == 0) {
	    goto L20;
	} else {
	    goto L60;
	}
    }
L5:
/*         CODE FOR UNEQUAL OR NONPOSITIVE INCREMENTS. */
    ix = 1;
    iy = 1;
    if (*incx < 0) {
	ix = (-(*n) + 1) * *incx + 1;
    }
    if (*incy < 0) {
	iy = (-(*n) + 1) * *incy + 1;
    }
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	ret_val += dx[ix] * dy[iy];
	ix += *incx;
	iy += *incy;
/* L10: */
    }
    return ret_val;
/*        CODE FOR BOTH INCREMENTS EQUAL TO 1. */
/*        CLEAN-UP LOOP SO REMAINING VECTOR LENGTH IS A MULTIPLE OF 5. */
L20:
    m = *n % 5;
    if (m == 0) {
	goto L40;
    }
    i__1 = m;
    for (i__ = 1; i__ <= i__1; ++i__) {
	ret_val += dx[i__] * dy[i__];
/* L30: */
    }
    if (*n < 5) {
	return ret_val;
    }
L40:
    mp1 = m + 1;
    i__1 = *n;
    for (i__ = mp1; i__ <= i__1; i__ += 5) {
	ret_val = ret_val + dx[i__] * dy[i__] + dx[i__ + 1] * dy[i__ + 1] + 
		dx[i__ + 2] * dy[i__ + 2] + dx[i__ + 3] * dy[i__ + 3] + dx[
		i__ + 4] * dy[i__ + 4];
/* L50: */
    }
    return ret_val;
/*         CODE FOR POSITIVE EQUAL INCREMENTS .NE.1. */
L60:
    ns = *n * *incx;
    i__1 = ns;
    i__2 = *incx;
    for (i__ = 1; i__2 < 0 ? i__ >= i__1 : i__ <= i__1; i__ += i__2) {
	ret_val += dx[i__] * dy[i__];
/* L70: */
    }
    return ret_val;
} /* ddot_ */

/* DNRM2 */
double dnrm2_(int *n, double *dx, int *incx)
{
    /* Initialized data */

    static double zero = 0.;
    static double one = 1.;
    static double cutlo = 8.232e-11;
    static double cuthi = 1.304e19;

    /* Format strings */
    static char fmt_30[] = "";
    static char fmt_50[] = "";
    static char fmt_70[] = "";
    static char fmt_110[] = "";

    /* System generated locals */
    int i__1, i__2;
    double ret_val, d__1;

    /* Builtin functions */
    double sqrt(double);

    /* Local variables */
    static int i__, j, nn;
    static double sum, xmax;
    static int next;
    static double hitest;

    /* Assigned format variables */
    static char *next_fmt;

/* ***BEGIN PROLOGUE  DNRM2 */
/* ***DATE WRITTEN   791001   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D1A3B */
/* ***KEYWORDS  BLAS,DOUBLE PRECISION,EUCLIDEAN,L2,LENGTH,LINEAR ALGEBRA, */
/*             NORM,VECTOR */
/* ***AUTHOR  LAWSON, C. L., (JPL) */
/*           HANSON, R. J., (SNLA) */
/*           KINCAID, D. R., (U. OF TEXAS) */
/*           KROGH, F. T., (JPL) */
/* ***PURPOSE  EUCLIDEAN LENGTH (L2 NORM) OF D.P. VECTOR */
/* ***DESCRIPTION */
/*                B L A S  SUBPROGRAM */
/*    DESCRIPTION OF PARAMETERS */
/*     --INPUT-- */
/*        N  NUMBER OF ELEMENTS IN INPUT VECTOR(S) */
/*       DX  DOUBLE PRECISION VECTOR WITH N ELEMENTS */
/*     INCX  STORAGE SPACING BETWEEN ELEMENTS OF DX */
/*     --OUTPUT-- */
/*    DNRM2  DOUBLE PRECISION RESULT (ZERO IF N .LE. 0) */
/*     EUCLIDEAN NORM OF THE N-VECTOR STORED IN DX() WITH STORAGE */
/*     INCREMENT INCX . */
/*     IF    N .LE. 0 RETURN WITH RESULT = 0. */
/*     IF N .GE. 1 THEN INCX MUST BE .GE. 1 */
/*           C.L. LAWSON, 1978 JAN 08 */
/*     FOUR PHASE METHOD     USING TWO BUILT-IN CONSTANTS THAT ARE */
/*     HOPEFULLY APPLICABLE TO ALL MACHINES. */
/*         CUTLO = MAXIMUM OF  DSQRT(U/EPS)  OVER ALL KNOWN MACHINES. */
/*         CUTHI = MINIMUM OF  DSQRT(V)      OVER ALL KNOWN MACHINES. */
/*     WHERE */
/*         EPS = SMALLEST NO. SUCH THAT EPS + 1. .GT. 1. */
/*         U   = SMALLEST POSITIVE NO.   (UNDERFLOW LIMIT) */
/*         V   = LARGEST  NO.            (OVERFLOW  LIMIT) */
/*     BRIEF OUTLINE OF ALGORITHM.. */
/*     PHASE 1    SCANS ZERO COMPONENTS. */
/*     MOVE TO PHASE 2 WHEN A COMPONENT IS NONZERO AND .LE. CUTLO */
/*     MOVE TO PHASE 3 WHEN A COMPONENT IS .GT. CUTLO */
/*     MOVE TO PHASE 4 WHEN A COMPONENT IS .GE. CUTHI/M */
/*     WHERE M = N FOR X() REAL AND M = 2*N FOR COMPLEX. */
/*     VALUES FOR CUTLO AND CUTHI.. */
/*     FROM THE ENVIRONMENTAL PARAMETERS LISTED IN THE IMSL CONVERTER */
/*     DOCUMENT THE LIMITING VALUES ARE AS FOLLOWS.. */
/*     CUTLO, S.P.   U/EPS = 2**(-102) FOR  HONEYWELL.  CLOSE SECONDS ARE */
/*                   UNIVAC AND DEC AT 2**(-103) */
/*                   THUS CUTLO = 2**(-51) = 4.44089E-16 */
/*     CUTHI, S.P.   V = 2**127 FOR UNIVAC, HONEYWELL, AND DEC. */
/*                   THUS CUTHI = 2**(63.5) = 1.30438E19 */
/*     CUTLO, D.P.   U/EPS = 2**(-67) FOR HONEYWELL AND DEC. */
/*                   THUS CUTLO = 2**(-33.5) = 8.23181D-11 */
/*     CUTHI, D.P.   SAME AS S.P.  CUTHI = 1.30438D19 */
/*     DATA CUTLO, CUTHI / 8.232D-11,  1.304D19 / */
/*     DATA CUTLO, CUTHI / 4.441E-16,  1.304E19 / */
/* ***REFERENCES  LAWSON C.L., HANSON R.J., KINCAID D.R., KROGH F.T., */
/*                 *BASIC LINEAR ALGEBRA SUBPROGRAMS FOR FORTRAN USAGE*, */
/*                 ALGORITHM NO. 539, TRANSACTIONS ON MATHEMATICAL */
/*                 SOFTWARE, VOLUME 5, NUMBER 3, SEPTEMBER 1979, 308-323 */
/* ***ROUTINES CALLED  (NONE) */
/* ***END PROLOGUE  DNRM2 */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --dx;

    /* Function Body */
/* ***FIRST EXECUTABLE STATEMENT  DNRM2 */
    xmax = zero;
    if (*n > 0) {
	goto L10;
    }
    ret_val = zero;
    goto L300;
L10:
    next = 0;
    next_fmt = fmt_30;
    sum = zero;
    nn = *n * *incx;
/*                                                 BEGIN MAIN LOOP */
    i__ = 1;
/*  20 GO TO NEXT,(30, 50, 70, 110) */
L20:
    switch (next) {
	case 0: goto L30;
	case 1: goto L50;
	case 2: goto L70;
	case 3: goto L110;
    }
L30:
    if ((d__1 = dx[i__], abs(d__1)) > cutlo) {
	goto L85;
    }
    next = 1;
    next_fmt = fmt_50;
    xmax = zero;
/*                        PHASE 1.  SUM IS ZERO */
L50:
    if (dx[i__] == zero) {
	goto L200;
    }
    if ((d__1 = dx[i__], abs(d__1)) > cutlo) {
	goto L85;
    }
/*                                PREPARE FOR PHASE 2. */
    next = 2;
    next_fmt = fmt_70;
    goto L105;
/*                                PREPARE FOR PHASE 4. */
L100:
    i__ = j;
    next = 3;
    next_fmt = fmt_110;
    sum = sum / dx[i__] / dx[i__];
L105:
    xmax = (d__1 = dx[i__], abs(d__1));
    goto L115;
/*                   PHASE 2.  SUM IS SMALL. */
/*                             SCALE TO AVOID DESTRUCTIVE UNDERFLOW. */
L70:
    if ((d__1 = dx[i__], abs(d__1)) > cutlo) {
	goto L75;
    }
/*                     COMMON CODE FOR PHASES 2 AND 4. */
/*                     IN PHASE 4 SUM IS LARGE.  SCALE TO AVOID OVERFLOW. */
L110:
    if ((d__1 = dx[i__], abs(d__1)) <= xmax) {
	goto L115;
    }
/* Computing 2nd power */
    d__1 = xmax / dx[i__];
    sum = one + sum * (d__1 * d__1);
    xmax = (d__1 = dx[i__], abs(d__1));
    goto L200;
L115:
/* Computing 2nd power */
    d__1 = dx[i__] / xmax;
    sum += d__1 * d__1;
    goto L200;
/*                  PREPARE FOR PHASE 3. */
L75:
    sum = sum * xmax * xmax;
/*     FOR REAL OR D.P. SET HITEST = CUTHI/N */
/*     FOR COMPLEX      SET HITEST = CUTHI/(2*N) */
L85:
    hitest = cuthi / (double) (*n);
/*                   PHASE 3.  SUM IS MID-RANGE.  NO SCALING. */
    i__1 = nn;
    i__2 = *incx;
    for (j = i__; i__2 < 0 ? j >= i__1 : j <= i__1; j += i__2) {
	if ((d__1 = dx[j], abs(d__1)) >= hitest) {
	    goto L100;
	}
/* L95: */
/* Computing 2nd power */
	d__1 = dx[j];
	sum += d__1 * d__1;
    }
    ret_val = sqrt(sum);
    goto L300;
L200:
    i__ += *incx;
    if (i__ <= nn) {
	goto L20;
    }
/*              END OF MAIN LOOP. */
/*              COMPUTE SQUARE ROOT AND ADJUST FOR SCALING. */
    ret_val = xmax * sqrt(sum);
L300:
    return ret_val;
} /* dnrm2_ */

/* DPODI */
/* Subroutine */ int dpodi_(double *a, int *lda, int *n, 
	double *det, int *job)
{
    /* System generated locals */
    int a_dim1, a_offset, i__1, i__2;
    double d__1;

    /* Local variables */
    static int i__, j, k;
    static double s, t;
    static int jm1, kp1;
    extern /* Subroutine */ int dscal_(int *, double *, double *, 
	    int *), daxpy_(int *, double *, double *, int 
	    *, double *, int *);

/* ***BEGIN PROLOGUE  DPODI */
/* ***DATE WRITTEN   780814   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D2B1B,D3B1B */
/* ***KEYWORDS  DETERMINANT,DOUBLE PRECISION,FACTOR,INVERSE, */
/*             LINEAR ALGEBRA,LINPACK,MATRIX,POSITIVE DEFINITE */
/* ***AUTHOR  MOLER, C. B., (U. OF NEW MEXICO) */
/* ***PURPOSE  COMPUTES THE DETERMINANT AND INVERSE OF A CERTAIN DOUBLE */
/*            PRECISION SYMMETRIC POSITIVE DEFINITE MATRIX (SEE ABSTRACT) */
/*            USING THE FACTORS COMPUTED BY DPOCO, DPOFA OR DQRDC. */
/* ***DESCRIPTION */
/*     DPODI COMPUTES THE DETERMINANT AND INVERSE OF A CERTAIN */
/*     DOUBLE PRECISION SYMMETRIC POSITIVE DEFINITE MATRIX (SEE BELOW) */
/*     USING THE FACTORS COMPUTED BY DPOCO, DPOFA OR DQRDC. */
/*     ON ENTRY */
/*        A       DOUBLE PRECISION(LDA, N) */
/*                THE OUTPUT  A  FROM DPOCO OR DPOFA */
/*                OR THE OUTPUT  X  FROM DQRDC. */
/*        LDA     INTEGER */
/*                THE LEADING DIMENSION OF THE ARRAY  A . */
/*        N       INTEGER */
/*                THE ORDER OF THE MATRIX  A . */
/*        JOB     INTEGER */
/*                = 11   BOTH DETERMINANT AND INVERSE. */
/*                = 01   INVERSE ONLY. */
/*                = 10   DETERMINANT ONLY. */
/*     ON RETURN */
/*        A       IF DPOCO OR DPOFA WAS USED TO FACTOR  A , THEN */
/*                DPODI PRODUCES THE UPPER HALF OF INVERSE(A) . */
/*                IF DQRDC WAS USED TO DECOMPOSE  X , THEN */
/*                DPODI PRODUCES THE UPPER HALF OF INVERSE(TRANS(X)*X) */
/*                WHERE TRANS(X) IS THE TRANSPOSE. */
/*                ELEMENTS OF  A  BELOW THE DIAGONAL ARE UNCHANGED. */
/*                IF THE UNITS DIGIT OF JOB IS ZERO,  A  IS UNCHANGED. */
/*        DET     DOUBLE PRECISION(2) */
/*                DETERMINANT OF  A  OR OF  TRANS(X)*X  IF REQUESTED. */
/*                OTHERWISE NOT REFERENCED. */
/*                DETERMINANT = DET(1) * 10.0**DET(2) */
/*                WITH  1.0 .LE. DET(1) .LT. 10.0 */
/*                OR  DET(1) .EQ. 0.0 . */
/*     ERROR CONDITION */
/*        A DIVISION BY ZERO WILL OCCUR IF THE INPUT FACTOR CONTAINS */
/*        A ZERO ON THE DIAGONAL AND THE INVERSE IS REQUESTED. */
/*        IT WILL NOT OCCUR IF THE SUBROUTINES ARE CALLED CORRECTLY */
/*        AND IF DPOCO OR DPOFA HAS SET INFO .EQ. 0 . */
/*     LINPACK.  THIS VERSION DATED 08/14/78 . */
/*     CLEVE MOLER, UNIVERSITY OF NEW MEXICO, ARGONNE NATIONAL LAB. */
/* ***REFERENCES  DONGARRA J.J., BUNCH J.R., MOLER C.B., STEWART G.W., */
/*                 *LINPACK USERS  GUIDE*, SIAM, 1979. */
/* ***ROUTINES CALLED  DAXPY,DSCAL */
/* ***END PROLOGUE  DPODI */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ***FIRST EXECUTABLE STATEMENT  DPODI */
    /* Parameter adjustments */
    a_dim1 = *lda;
    a_offset = 1 + a_dim1;
    a -= a_offset;
    --det;

    /* Function Body */
    if (*job / 10 == 0) {
	goto L70;
    }
    det[1] = 1.;
    det[2] = 0.;
    s = 10.;
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
/* Computing 2nd power */
	d__1 = a[i__ + i__ * a_dim1];
	det[1] = d__1 * d__1 * det[1];
/*        ...EXIT */
	if (det[1] == 0.) {
	    goto L60;
	}
L10:
	if (det[1] >= 1.) {
	    goto L20;
	}
	det[1] = s * det[1];
	det[2] += -1.;
	goto L10;
L20:
L30:
	if (det[1] < s) {
	    goto L40;
	}
	det[1] /= s;
	det[2] += 1.;
	goto L30;
L40:
/* L50: */
	;
    }
L60:
L70:
/*     COMPUTE INVERSE(R) */
    if (*job % 10 == 0) {
	goto L140;
    }
    i__1 = *n;
    for (k = 1; k <= i__1; ++k) {
	a[k + k * a_dim1] = 1. / a[k + k * a_dim1];
	t = -a[k + k * a_dim1];
	i__2 = k - 1;
	dscal_(&i__2, &t, &a[k * a_dim1 + 1], &c__1);
	kp1 = k + 1;
	if (*n < kp1) {
	    goto L90;
	}
	i__2 = *n;
	for (j = kp1; j <= i__2; ++j) {
	    t = a[k + j * a_dim1];
	    a[k + j * a_dim1] = 0.;
	    daxpy_(&k, &t, &a[k * a_dim1 + 1], &c__1, &a[j * a_dim1 + 1], &
		    c__1);
/* L80: */
	}
L90:
/* L100: */
	;
    }
/*        FORM  INVERSE(R) * TRANS(INVERSE(R)) */
    i__1 = *n;
    for (j = 1; j <= i__1; ++j) {
	jm1 = j - 1;
	if (jm1 < 1) {
	    goto L120;
	}
	i__2 = jm1;
	for (k = 1; k <= i__2; ++k) {
	    t = a[k + j * a_dim1];
	    daxpy_(&k, &t, &a[j * a_dim1 + 1], &c__1, &a[k * a_dim1 + 1], &
		    c__1);
/* L110: */
	}
L120:
	t = a[j + j * a_dim1];
	dscal_(&j, &t, &a[j * a_dim1 + 1], &c__1);
/* L130: */
    }
L140:
    return 0;
} /* dpodi_ */

/* DQRDC */
/* Subroutine */ int dqrdc_(double *x, int *ldx, int *n, int *
	p, double *qraux, int *jpvt, double *work, int *job)
{
    /* System generated locals */
    int x_dim1, x_offset, i__1, i__2, i__3;
    double d__1, d__2;

    /* Builtin functions */
    double d_sign(double *, double *), sqrt(double);

    /* Local variables */
    static int j, l;
    static double t;
    static int jj, jp, pl, pu;
    static double tt;
    static int lp1, lup;
    static int negj;
    extern double ddot_(int *, double *, int *, double *, 
	    int *);
    static int maxj;
    extern double dnrm2_(int *, double *, int *);
    extern /* Subroutine */ int dscal_(int *, double *, double *, 
	    int *), dswap_(int *, double *, int *, double 
	    *, int *);
    static int swapj;
    extern /* Subroutine */ int daxpy_(int *, double *, double *, 
	    int *, double *, int *);
    static double nrmxl, maxnrm;

/* ***BEGIN PROLOGUE  DQRDC */
/* ***DATE WRITTEN   780814   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D5 */
/* ***KEYWORDS  DECOMPOSITION,DOUBLE PRECISION,LINEAR ALGEBRA,LINPACK, */
/*             MATRIX,ORTHOGONAL TRIANGULAR */
/* ***AUTHOR  STEWART, G. W., (U. OF MARYLAND) */
/* ***PURPOSE  USES HOUSEHOLDER TRANSFORMATIONS TO COMPUTE THE QR FACTORI- */
/*            ZATION OF N BY P MATRIX X.  COLUMN PIVOTING IS OPTIONAL. */
/* ***DESCRIPTION */
/*     DQRDC USES HOUSEHOLDER TRANSFORMATIONS TO COMPUTE THE QR */
/*     FACTORIZATION OF AN N BY P MATRIX X.  COLUMN PIVOTING */
/*     BASED ON THE 2-NORMS OF THE REDUCED COLUMNS MAY BE */
/*     PERFORMED AT THE USER'S OPTION. */
/*     ON ENTRY */
/*        X       DOUBLE PRECISION(LDX,P), WHERE LDX .GE. N. */
/*                X CONTAINS THE MATRIX WHOSE DECOMPOSITION IS TO BE */
/*                COMPUTED. */
/*        LDX     INTEGER. */
/*                LDX IS THE LEADING DIMENSION OF THE ARRAY X. */
/*        N       INTEGER. */
/*                N IS THE NUMBER OF ROWS OF THE MATRIX X. */
/*        P       INTEGER. */
/*                P IS THE NUMBER OF COLUMNS OF THE MATRIX X. */
/*        JPVT    INTEGER(P). */
/*                JPVT CONTAINS INTEGERS THAT CONTROL THE SELECTION */
/*                OF THE PIVOT COLUMNS.  THE K-TH COLUMN X(K) OF X */
/*                IS PLACED IN ONE OF THREE CLASSES ACCORDING TO THE */
/*                VALUE OF JPVT(K). */
/*                   IF JPVT(K) .GT. 0, THEN X(K) IS AN INITIAL */
/*                                      COLUMN. */
/*                   IF JPVT(K) .EQ. 0, THEN X(K) IS A FREE COLUMN. */
/*                   IF JPVT(K) .LT. 0, THEN X(K) IS A FINAL COLUMN. */
/*                BEFORE THE DECOMPOSITION IS COMPUTED, INITIAL COLUMNS */
/*                ARE MOVED TO THE BEGINNING OF THE ARRAY X AND FINAL */
/*                COLUMNS TO THE END.  BOTH INITIAL AND FINAL COLUMNS */
/*                ARE FROZEN IN PLACE DURING THE COMPUTATION AND ONLY */
/*                FREE COLUMNS ARE MOVED.  AT THE K-TH STAGE OF THE */
/*                REDUCTION, IF X(K) IS OCCUPIED BY A FREE COLUMN */
/*                IT IS INTERCHANGED WITH THE FREE COLUMN OF LARGEST */
/*                REDUCED NORM.  JPVT IS NOT REFERENCED IF */
/*                JOB .EQ. 0. */
/*        WORK    DOUBLE PRECISION(P). */
/*                WORK IS A WORK ARRAY.  WORK IS NOT REFERENCED IF */
/*                JOB .EQ. 0. */
/*        JOB     INTEGER. */
/*                JOB IS AN INTEGER THAT INITIATES COLUMN PIVOTING. */
/*                IF JOB .EQ. 0, NO PIVOTING IS DONE. */
/*                IF JOB .NE. 0, PIVOTING IS DONE. */
/*     ON RETURN */
/*        X       X CONTAINS IN ITS UPPER TRIANGLE THE UPPER */
/*                TRIANGULAR MATRIX R OF THE QR FACTORIZATION. */
/*                BELOW ITS DIAGONAL X CONTAINS INFORMATION FROM */
/*                WHICH THE ORTHOGONAL PART OF THE DECOMPOSITION */
/*                CAN BE RECOVERED.  NOTE THAT IF PIVOTING HAS */
/*                BEEN REQUESTED, THE DECOMPOSITION IS NOT THAT */
/*                OF THE ORIGINAL MATRIX X BUT THAT OF X */
/*                WITH ITS COLUMNS PERMUTED AS DESCRIBED BY JPVT. */
/*        QRAUX   DOUBLE PRECISION(P). */
/*                QRAUX CONTAINS FURTHER INFORMATION REQUIRED TO RECOVER */
/*                THE ORTHOGONAL PART OF THE DECOMPOSITION. */
/*        JPVT    JPVT(K) CONTAINS THE INDEX OF THE COLUMN OF THE */
/*                ORIGINAL MATRIX THAT HAS BEEN INTERCHANGED INTO */
/*                THE K-TH COLUMN, IF PIVOTING WAS REQUESTED. */
/*     LINPACK.  THIS VERSION DATED 08/14/78 . */
/*     G. W. STEWART, UNIVERSITY OF MARYLAND, ARGONNE NATIONAL LAB. */
/* ***REFERENCES  DONGARRA J.J., BUNCH J.R., MOLER C.B., STEWART G.W., */
/*                 *LINPACK USERS  GUIDE*, SIAM, 1979. */
/* ***ROUTINES CALLED  DAXPY,DDOT,DNRM2,DSCAL,DSWAP */
/* ***END PROLOGUE  DQRDC */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL FUNCTIONS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ***FIRST EXECUTABLE STATEMENT  DQRDC */
    /* Parameter adjustments */
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    --qraux;
    --jpvt;
    --work;

    /* Function Body */
    pl = 1;
    pu = 0;
    if (*job == 0) {
	goto L60;
    }
/*        PIVOTING HAS BEEN REQUESTED.  REARRANGE THE COLUMNS */
/*        ACCORDING TO JPVT. */
    i__1 = *p;
    for (j = 1; j <= i__1; ++j) {
	swapj = jpvt[j] > 0;
	negj = jpvt[j] < 0;
	jpvt[j] = j;
	if (negj) {
	    jpvt[j] = -j;
	}
	if (! swapj) {
	    goto L10;
	}
	if (j != pl) {
	    dswap_(n, &x[pl * x_dim1 + 1], &c__1, &x[j * x_dim1 + 1], &c__1);
	}
	jpvt[j] = jpvt[pl];
	jpvt[pl] = j;
	++pl;
L10:
/* L20: */
	;
    }
    pu = *p;
    i__1 = *p;
    for (jj = 1; jj <= i__1; ++jj) {
	j = *p - jj + 1;
	if (jpvt[j] >= 0) {
	    goto L40;
	}
	jpvt[j] = -jpvt[j];
	if (j == pu) {
	    goto L30;
	}
	dswap_(n, &x[pu * x_dim1 + 1], &c__1, &x[j * x_dim1 + 1], &c__1);
	jp = jpvt[pu];
	jpvt[pu] = jpvt[j];
	jpvt[j] = jp;
L30:
	--pu;
L40:
/* L50: */
	;
    }
L60:
/*     COMPUTE THE NORMS OF THE FREE COLUMNS. */
    if (pu < pl) {
	goto L80;
    }
    i__1 = pu;
    for (j = pl; j <= i__1; ++j) {
	qraux[j] = dnrm2_(n, &x[j * x_dim1 + 1], &c__1);
	work[j] = qraux[j];
/* L70: */
    }
L80:
/*     PERFORM THE HOUSEHOLDER REDUCTION OF X. */
    lup = min(*n,*p);
    i__1 = lup;
    for (l = 1; l <= i__1; ++l) {
	if (l < pl || l >= pu) {
	    goto L120;
	}
/*           LOCATE THE COLUMN OF LARGEST NORM AND BRING IT */
/*           INTO THE PIVOT POSITION. */
	maxnrm = 0.;
	maxj = l;
	i__2 = pu;
	for (j = l; j <= i__2; ++j) {
	    if (qraux[j] <= maxnrm) {
		goto L90;
	    }
	    maxnrm = qraux[j];
	    maxj = j;
L90:
/* L100: */
	    ;
	}
	if (maxj == l) {
	    goto L110;
	}
	dswap_(n, &x[l * x_dim1 + 1], &c__1, &x[maxj * x_dim1 + 1], &c__1);
	qraux[maxj] = qraux[l];
	work[maxj] = work[l];
	jp = jpvt[maxj];
	jpvt[maxj] = jpvt[l];
	jpvt[l] = jp;
L110:
L120:
	qraux[l] = 0.;
	if (l == *n) {
	    goto L190;
	}
/*           COMPUTE THE HOUSEHOLDER TRANSFORMATION FOR COLUMN L. */
	i__2 = *n - l + 1;
	nrmxl = dnrm2_(&i__2, &x[l + l * x_dim1], &c__1);
	if (nrmxl == 0.) {
	    goto L180;
	}
	if (x[l + l * x_dim1] != 0.) {
	    nrmxl = d_sign(&nrmxl, &x[l + l * x_dim1]);
	}
	i__2 = *n - l + 1;
	d__1 = 1. / nrmxl;
	dscal_(&i__2, &d__1, &x[l + l * x_dim1], &c__1);
	x[l + l * x_dim1] += 1.;
/*              APPLY THE TRANSFORMATION TO THE REMAINING COLUMNS, */
/*              UPDATING THE NORMS. */
	lp1 = l + 1;
	if (*p < lp1) {
	    goto L170;
	}
	i__2 = *p;
	for (j = lp1; j <= i__2; ++j) {
	    i__3 = *n - l + 1;
	    t = -ddot_(&i__3, &x[l + l * x_dim1], &c__1, &x[l + j * x_dim1], &
		    c__1) / x[l + l * x_dim1];
	    i__3 = *n - l + 1;
	    daxpy_(&i__3, &t, &x[l + l * x_dim1], &c__1, &x[l + j * x_dim1], &
		    c__1);
	    if (j < pl || j > pu) {
		goto L150;
	    }
	    if (qraux[j] == 0.) {
		goto L150;
	    }
/* Computing 2nd power */
	    d__2 = (d__1 = x[l + j * x_dim1], abs(d__1)) / qraux[j];
	    tt = 1. - d__2 * d__2;
	    tt = max(tt,0.);
	    t = tt;
/* Computing 2nd power */
	    d__1 = qraux[j] / work[j];
	    tt = tt * .05 * (d__1 * d__1) + 1.;
	    if (tt == 1.) {
		goto L130;
	    }
	    qraux[j] *= sqrt(t);
	    goto L140;
L130:
	    i__3 = *n - l;
	    qraux[j] = dnrm2_(&i__3, &x[l + 1 + j * x_dim1], &c__1);
	    work[j] = qraux[j];
L140:
L150:
/* L160: */
	    ;
	}
L170:
/*              SAVE THE TRANSFORMATION. */
	qraux[l] = x[l + l * x_dim1];
	x[l + l * x_dim1] = -nrmxl;
L180:
L190:
/* L200: */
	;
    }
    return 0;
} /* dqrdc_ */

/* DQRSL */
/* Subroutine */ int dqrsl_(double *x, int *ldx, int *n, int *
	k, double *qraux, double *y, double *qy, double *qty, 
	double *b, double *rsd, double *xb, int *job, int 
	*info)
{
    /* System generated locals */
    int x_dim1, x_offset, i__1, i__2;

    /* Local variables */
    static int i__, j;
    static double t;
    static int cb;
    static int jj;
    static int cr;
    static int ju, kp1;
    static int cxb, cqy;
    extern double ddot_(int *, double *, int *, double *, 
	    int *);
    static double temp;
    static int cqty;
    extern /* Subroutine */ int dcopy_(int *, double *, int *, 
	    double *, int *), daxpy_(int *, double *, 
	    double *, int *, double *, int *);

/* ***BEGIN PROLOGUE  DQRSL */
/* ***DATE WRITTEN   780814   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D9,D2A1 */
/* ***KEYWORDS  DOUBLE PRECISION,LINEAR ALGEBRA,LINPACK,MATRIX, */
/*             ORTHOGONAL TRIANGULAR,SOLVE */
/* ***AUTHOR  STEWART, G. W., (U. OF MARYLAND) */
/* ***PURPOSE  APPLIES THE OUTPUT OF DQRDC TO COMPUTE COORDINATE */
/*            TRANSFORMATIONS, PROJECTIONS, AND LEAST SQUARES SOLUTIONS. */
/* ***DESCRIPTION */
/*     DQRSL APPLIES THE OUTPUT OF DQRDC TO COMPUTE COORDINATE */
/*     TRANSFORMATIONS, PROJECTIONS, AND LEAST SQUARES SOLUTIONS. */
/*     FOR K .LE. MIN(N,P), LET XK BE THE MATRIX */
/*            XK = (X(JPVT(1)),X(JPVT(2)), ... ,X(JPVT(K))) */
/*     FORMED FROM COLUMNNS JPVT(1), ... ,JPVT(K) OF THE ORIGINAL */
/*     N X P MATRIX X THAT WAS INPUT TO DQRDC (IF NO PIVOTING WAS */
/*     DONE, XK CONSISTS OF THE FIRST K COLUMNS OF X IN THEIR */
/*     ORIGINAL ORDER).  DQRDC PRODUCES A FACTORED ORTHOGONAL MATRIX Q */
/*     AND AN UPPER TRIANGULAR MATRIX R SUCH THAT */
/*              XK = Q * (R) */
/*                       (0) */
/*     THIS INFORMATION IS CONTAINED IN CODED FORM IN THE ARRAYS */
/*     X AND QRAUX. */
/*     ON ENTRY */
/*        X      DOUBLE PRECISION(LDX,P). */
/*               X CONTAINS THE OUTPUT OF DQRDC. */
/*        LDX    INTEGER. */
/*               LDX IS THE LEADING DIMENSION OF THE ARRAY X. */
/*        N      INTEGER. */
/*               N IS THE NUMBER OF ROWS OF THE MATRIX XK.  IT MUST */
/*               HAVE THE SAME VALUE AS N IN DQRDC. */
/*        K      INTEGER. */
/*               K IS THE NUMBER OF COLUMNS OF THE MATRIX XK.  K */
/*               MUST NOT BE GREATER THAN MIN(N,P), WHERE P IS THE */
/*               SAME AS IN THE CALLING SEQUENCE TO DQRDC. */
/*        QRAUX  DOUBLE PRECISION(P). */
/*               QRAUX CONTAINS THE AUXILIARY OUTPUT FROM DQRDC. */
/*        Y      DOUBLE PRECISION(N) */
/*               Y CONTAINS AN N-VECTOR THAT IS TO BE MANIPULATED */
/*               BY DQRSL. */
/*        JOB    INTEGER. */
/*               JOB SPECIFIES WHAT IS TO BE COMPUTED.  JOB HAS */
/*               THE DECIMAL EXPANSION ABCDE, WITH THE FOLLOWING */
/*               MEANING. */
/*                    IF A .NE. 0, COMPUTE QY. */
/*                    IF B,C,D, OR E .NE. 0, COMPUTE QTY. */
/*                    IF C .NE. 0, COMPUTE B. */
/*                    IF D .NE. 0, COMPUTE RSD. */
/*                    IF E .NE. 0, COMPUTE XB. */
/*               NOTE THAT A REQUEST TO COMPUTE B, RSD, OR XB */
/*               AUTOMATICALLY TRIGGERS THE COMPUTATION OF QTY, FOR */
/*               WHICH AN ARRAY MUST BE PROVIDED IN THE CALLING */
/*               SEQUENCE. */
/*     ON RETURN */
/*        QY     DOUBLE PRECISION(N). */
/*               QY CONTAINS Q*Y, IF ITS COMPUTATION HAS BEEN */
/*               REQUESTED. */
/*        QTY    DOUBLE PRECISION(N). */
/*               QTY CONTAINS TRANS(Q)*Y, IF ITS COMPUTATION HAS */
/*               BEEN REQUESTED.  HERE TRANS(Q) IS THE */
/*               TRANSPOSE OF THE MATRIX Q. */
/*        B      DOUBLE PRECISION(K) */
/*               B CONTAINS THE SOLUTION OF THE LEAST SQUARES PROBLEM */
/*                    MINIMIZE NORM2(Y - XK*B), */
/*               IF ITS COMPUTATION HAS BEEN REQUESTED.  (NOTE THAT */
/*               IF PIVOTING WAS REQUESTED IN DQRDC, THE J-TH */
/*               COMPONENT OF B WILL BE ASSOCIATED WITH COLUMN JPVT(J) */
/*               OF THE ORIGINAL MATRIX X THAT WAS INPUT INTO DQRDC.) */
/*        RSD    DOUBLE PRECISION(N). */
/*               RSD CONTAINS THE LEAST SQUARES RESIDUAL Y - XK*B, */
/*               IF ITS COMPUTATION HAS BEEN REQUESTED.  RSD IS */
/*               ALSO THE ORTHOGONAL PROJECTION OF Y ONTO THE */
/*               ORTHOGONAL COMPLEMENT OF THE COLUMN SPACE OF XK. */
/*        XB     DOUBLE PRECISION(N). */
/*               XB CONTAINS THE LEAST SQUARES APPROXIMATION XK*B, */
/*               IF ITS COMPUTATION HAS BEEN REQUESTED.  XB IS ALSO */
/*               THE ORTHOGONAL PROJECTION OF Y ONTO THE COLUMN SPACE */
/*               OF X. */
/*        INFO   INTEGER. */
/*               INFO IS ZERO UNLESS THE COMPUTATION OF B HAS */
/*               BEEN REQUESTED AND R IS EXACTLY SINGULAR.  IN */
/*               THIS CASE, INFO IS THE INDEX OF THE FIRST ZERO */
/*               DIAGONAL ELEMENT OF R AND B IS LEFT UNALTERED. */
/*     THE PARAMETERS QY, QTY, B, RSD, AND XB ARE NOT REFERENCED */
/*     IF THEIR COMPUTATION IS NOT REQUESTED AND IN THIS CASE */
/*     CAN BE REPLACED BY DUMMY VARIABLES IN THE CALLING PROGRAM. */
/*     TO SAVE STORAGE, THE USER MAY IN SOME CASES USE THE SAME */
/*     ARRAY FOR DIFFERENT PARAMETERS IN THE CALLING SEQUENCE.  A */
/*     FREQUENTLY OCCURING EXAMPLE IS WHEN ONE WISHES TO COMPUTE */
/*     ANY OF B, RSD, OR XB AND DOES NOT NEED Y OR QTY.  IN THIS */
/*     CASE ONE MAY IDENTIFY Y, QTY, AND ONE OF B, RSD, OR XB, WHILE */
/*     PROVIDING SEPARATE ARRAYS FOR ANYTHING ELSE THAT IS TO BE */
/*     COMPUTED.  THUS THE CALLING SEQUENCE */
/*          CALL DQRSL(X,LDX,N,K,QRAUX,Y,DUM,Y,B,Y,DUM,110,INFO) */
/*     WILL RESULT IN THE COMPUTATION OF B AND RSD, WITH RSD */
/*     OVERWRITING Y.  MORE GENERALLY, EACH ITEM IN THE FOLLOWING */
/*     LIST CONTAINS GROUPS OF PERMISSIBLE IDENTIFICATIONS FOR */
/*     A SINGLE CALLING SEQUENCE. */
/*          1. (Y,QTY,B) (RSD) (XB) (QY) */
/*          2. (Y,QTY,RSD) (B) (XB) (QY) */
/*          3. (Y,QTY,XB) (B) (RSD) (QY) */
/*          4. (Y,QY) (QTY,B) (RSD) (XB) */
/*          5. (Y,QY) (QTY,RSD) (B) (XB) */
/*          6. (Y,QY) (QTY,XB) (B) (RSD) */
/*     IN ANY GROUP THE VALUE RETURNED IN THE ARRAY ALLOCATED TO */
/*     THE GROUP CORRESPONDS TO THE LAST MEMBER OF THE GROUP. */
/*     LINPACK.  THIS VERSION DATED 08/14/78 . */
/*     G. W. STEWART, UNIVERSITY OF MARYLAND, ARGONNE NATIONAL LAB. */
/* ***REFERENCES  DONGARRA J.J., BUNCH J.R., MOLER C.B., STEWART G.W., */
/*                 *LINPACK USERS  GUIDE*, SIAM, 1979. */
/* ***ROUTINES CALLED  DAXPY,DCOPY,DDOT */
/* ***END PROLOGUE  DQRSL */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL FUNCTIONS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ***FIRST EXECUTABLE STATEMENT  DQRSL */
    /* Parameter adjustments */
    x_dim1 = *ldx;
    x_offset = 1 + x_dim1;
    x -= x_offset;
    --qraux;
    --y;
    --qy;
    --qty;
    --b;
    --rsd;
    --xb;

    /* Function Body */
    *info = 0;
/*     DETERMINE WHAT IS TO BE COMPUTED. */
    cqy = *job / 10000 != 0;
    cqty = *job % 10000 != 0;
    cb = *job % 1000 / 100 != 0;
    cr = *job % 100 / 10 != 0;
    cxb = *job % 10 != 0;
/* Computing MIN */
    i__1 = *k, i__2 = *n - 1;
    ju = min(i__1,i__2);
/*     SPECIAL ACTION WHEN N=1. */
    if (ju != 0) {
	goto L40;
    }
    if (cqy) {
	qy[1] = y[1];
    }
    if (cqty) {
	qty[1] = y[1];
    }
    if (cxb) {
	xb[1] = y[1];
    }
    if (! cb) {
	goto L30;
    }
    if (x[x_dim1 + 1] != 0.) {
	goto L10;
    }
    *info = 1;
    goto L20;
L10:
    b[1] = y[1] / x[x_dim1 + 1];
L20:
L30:
    if (cr) {
	rsd[1] = 0.;
    }
    goto L250;
L40:
/*        SET UP TO COMPUTE QY OR QTY. */
    if (cqy) {
	dcopy_(n, &y[1], &c__1, &qy[1], &c__1);
    }
    if (cqty) {
	dcopy_(n, &y[1], &c__1, &qty[1], &c__1);
    }
    if (! cqy) {
	goto L70;
    }
/*           COMPUTE QY. */
    i__1 = ju;
    for (jj = 1; jj <= i__1; ++jj) {
	j = ju - jj + 1;
	if (qraux[j] == 0.) {
	    goto L50;
	}
	temp = x[j + j * x_dim1];
	x[j + j * x_dim1] = qraux[j];
	i__2 = *n - j + 1;
	t = -ddot_(&i__2, &x[j + j * x_dim1], &c__1, &qy[j], &c__1) / x[j + j 
		* x_dim1];
	i__2 = *n - j + 1;
	daxpy_(&i__2, &t, &x[j + j * x_dim1], &c__1, &qy[j], &c__1);
	x[j + j * x_dim1] = temp;
L50:
/* L60: */
	;
    }
L70:
    if (! cqty) {
	goto L100;
    }
/*           COMPUTE TRANS(Q)*Y. */
    i__1 = ju;
    for (j = 1; j <= i__1; ++j) {
	if (qraux[j] == 0.) {
	    goto L80;
	}
	temp = x[j + j * x_dim1];
	x[j + j * x_dim1] = qraux[j];
	i__2 = *n - j + 1;
	t = -ddot_(&i__2, &x[j + j * x_dim1], &c__1, &qty[j], &c__1) / x[j + 
		j * x_dim1];
	i__2 = *n - j + 1;
	daxpy_(&i__2, &t, &x[j + j * x_dim1], &c__1, &qty[j], &c__1);
	x[j + j * x_dim1] = temp;
L80:
/* L90: */
	;
    }
L100:
/*        SET UP TO COMPUTE B, RSD, OR XB. */
    if (cb) {
	dcopy_(k, &qty[1], &c__1, &b[1], &c__1);
    }
    kp1 = *k + 1;
    if (cxb) {
	dcopy_(k, &qty[1], &c__1, &xb[1], &c__1);
    }
    if (cr && *k < *n) {
	i__1 = *n - *k;
	dcopy_(&i__1, &qty[kp1], &c__1, &rsd[kp1], &c__1);
    }
    if (! cxb || kp1 > *n) {
	goto L120;
    }
    i__1 = *n;
    for (i__ = kp1; i__ <= i__1; ++i__) {
	xb[i__] = 0.;
/* L110: */
    }
L120:
    if (! cr) {
	goto L140;
    }
    i__1 = *k;
    for (i__ = 1; i__ <= i__1; ++i__) {
	rsd[i__] = 0.;
/* L130: */
    }
L140:
    if (! cb) {
	goto L190;
    }
/*           COMPUTE B. */
    i__1 = *k;
    for (jj = 1; jj <= i__1; ++jj) {
	j = *k - jj + 1;
	if (x[j + j * x_dim1] != 0.) {
	    goto L150;
	}
	*info = j;
/*           ......EXIT */
	goto L180;
L150:
	b[j] /= x[j + j * x_dim1];
	if (j == 1) {
	    goto L160;
	}
	t = -b[j];
	i__2 = j - 1;
	daxpy_(&i__2, &t, &x[j * x_dim1 + 1], &c__1, &b[1], &c__1);
L160:
/* L170: */
	;
    }
L180:
L190:
    if (! cr && ! cxb) {
	goto L240;
    }
/*           COMPUTE RSD OR XB AS REQUIRED. */
    i__1 = ju;
    for (jj = 1; jj <= i__1; ++jj) {
	j = ju - jj + 1;
	if (qraux[j] == 0.) {
	    goto L220;
	}
	temp = x[j + j * x_dim1];
	x[j + j * x_dim1] = qraux[j];
	if (! cr) {
	    goto L200;
	}
	i__2 = *n - j + 1;
	t = -ddot_(&i__2, &x[j + j * x_dim1], &c__1, &rsd[j], &c__1) / x[j + 
		j * x_dim1];
	i__2 = *n - j + 1;
	daxpy_(&i__2, &t, &x[j + j * x_dim1], &c__1, &rsd[j], &c__1);
L200:
	if (! cxb) {
	    goto L210;
	}
	i__2 = *n - j + 1;
	t = -ddot_(&i__2, &x[j + j * x_dim1], &c__1, &xb[j], &c__1) / x[j + j 
		* x_dim1];
	i__2 = *n - j + 1;
	daxpy_(&i__2, &t, &x[j + j * x_dim1], &c__1, &xb[j], &c__1);
L210:
	x[j + j * x_dim1] = temp;
L220:
/* L230: */
	;
    }
L240:
L250:
    return 0;
} /* dqrsl_ */

/* DROT */
/* Subroutine */ int drot_(int *n, double *dx, int *incx, 
	double *dy, int *incy, double *dc, double *ds)
{
    /* Initialized data */

    static double zero = 0.;
    static double one = 1.;

    /* System generated locals */
    int i__1, i__2;

    /* Local variables */
    static int i__;
    static double w, z__;
    static int kx, ky, nsteps;

/* ***BEGIN PROLOGUE  DROT */
/* ***DATE WRITTEN   791001   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D1A8 */
/* ***KEYWORDS  BLAS,GIVENS ROTATION,LINEAR ALGEBRA,VECTOR */
/* ***AUTHOR  LAWSON, C. L., (JPL) */
/*           HANSON, R. J., (SNLA) */
/*           KINCAID, D. R., (U. OF TEXAS) */
/*           KROGH, F. T., (JPL) */
/* ***PURPOSE  APPLY D.P. GIVENS ROTATION */
/* ***DESCRIPTION */
/*                B L A S  SUBPROGRAM */
/*    DESCRIPTION OF PARAMETERS */
/*     --INPUT-- */
/*        N  NUMBER OF ELEMENTS IN INPUT VECTOR(S) */
/*       DX  DOUBLE PRECISION VECTOR WITH N ELEMENTS */
/*     INCX  STORAGE SPACING BETWEEN ELEMENTS OF DX */
/*       DY  DOUBLE PRECISION VECTOR WITH N ELEMENTS */
/*     INCY  STORAGE SPACING BETWEEN ELEMENTS OF DY */
/*       DC  D.P. ELEMENT OF ROTATION MATRIX */
/*       DS  D.P. ELEMENT OF ROTATION MATRIX */
/*     --OUTPUT-- */
/*       DX  ROTATED VECTOR (UNCHANGED IF N .LE. 0) */
/*       DY  ROTATED VECTOR (UNCHANGED IF N .LE. 0) */
/*     MULTIPLY THE 2 X 2 MATRIX  ( DC DS) TIMES THE 2 X N MATRIX (DX**T) */
/*                                (-DS DC)                        (DY**T) */
/*     WHERE **T INDICATES TRANSPOSE.  THE ELEMENTS OF DX ARE IN */
/*     DX(LX+I*INCX), I = 0 TO N-1, WHERE LX = 1 IF INCX .GE. 0, ELSE */
/*     LX = (-INCX)*N, AND SIMILARLY FOR DY USING LY AND INCY. */
/* ***REFERENCES  LAWSON C.L., HANSON R.J., KINCAID D.R., KROGH F.T., */
/*                 *BASIC LINEAR ALGEBRA SUBPROGRAMS FOR FORTRAN USAGE*, */
/*                 ALGORITHM NO. 539, TRANSACTIONS ON MATHEMATICAL */
/*                 SOFTWARE, VOLUME 5, NUMBER 3, SEPTEMBER 1979, 308-323 */
/* ***ROUTINES CALLED  (NONE) */
/* ***END PROLOGUE  DROT */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...DATA STATEMENTS */
    /* Parameter adjustments */
    --dy;
    --dx;

    /* Function Body */
/* ***FIRST EXECUTABLE STATEMENT  DROT */
    if (*n <= 0 || *ds == zero && *dc == one) {
	goto L40;
    }
    if (! (*incx == *incy && *incx > 0)) {
	goto L20;
    }
    nsteps = *incx * *n;
    i__1 = nsteps;
    i__2 = *incx;
    for (i__ = 1; i__2 < 0 ? i__ >= i__1 : i__ <= i__1; i__ += i__2) {
	w = dx[i__];
	z__ = dy[i__];
	dx[i__] = *dc * w + *ds * z__;
	dy[i__] = -(*ds) * w + *dc * z__;
/* L10: */
    }
    goto L40;
L20:
    kx = 1;
    ky = 1;
    if (*incx < 0) {
	kx = 1 - (*n - 1) * *incx;
    }
    if (*incy < 0) {
	ky = 1 - (*n - 1) * *incy;
    }
    i__2 = *n;
    for (i__ = 1; i__ <= i__2; ++i__) {
	w = dx[kx];
	z__ = dy[ky];
	dx[kx] = *dc * w + *ds * z__;
	dy[ky] = -(*ds) * w + *dc * z__;
	kx += *incx;
	ky += *incy;
/* L30: */
    }
L40:
    return 0;
} /* drot_ */

/* DROTG */
/* Subroutine */ int drotg_(double *da, double *db, double *dc, 
	double *ds)
{
    /* System generated locals */
    double d__1;

    /* Builtin functions */
    double sqrt(double);

    /* Local variables */
    static double r__, u, v;

/* ***BEGIN PROLOGUE  DROTG */
/* ***DATE WRITTEN   791001   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D1B10 */
/* ***KEYWORDS  BLAS,GIVENS ROTATION,LINEAR ALGEBRA,VECTOR */
/* ***AUTHOR  LAWSON, C. L., (JPL) */
/*           HANSON, R. J., (SNLA) */
/*           KINCAID, D. R., (U. OF TEXAS) */
/*           KROGH, F. T., (JPL) */
/* ***PURPOSE  CONSTRUCT D.P. PLANE GIVENS ROTATION */
/* ***DESCRIPTION */
/*                B L A S  SUBPROGRAM */
/*    DESCRIPTION OF PARAMETERS */
/*     --INPUT-- */
/*       DA  DOUBLE PRECISION SCALAR */
/*       DB  DOUBLE PRECISION SCALAR */
/*     --OUTPUT-- */
/*       DA  DOUBLE PRECISION RESULT R */
/*       DB  DOUBLE PRECISION RESULT Z */
/*       DC  DOUBLE PRECISION RESULT */
/*       DS  DOUBLE PRECISION RESULT */
/*     DESIGNED BY C. L. LAWSON, JPL, 1977 SEPT 08 */
/*     CONSTRUCT THE GIVENS TRANSFORMATION */
/*         ( DC  DS ) */
/*     G = (        ) ,    DC**2 + DS**2 = 1 , */
/*         (-DS  DC ) */
/*     WHICH ZEROS THE SECOND ENTRY OF THE 2-VECTOR  (DA,DB)**T . */
/*     THE QUANTITY R = (+/-)DSQRT(DA**2 + DB**2) OVERWRITES DA IN */
/*     STORAGE.  THE VALUE OF DB IS OVERWRITTEN BY A VALUE Z WHICH */
/*     ALLOWS DC AND DS TO BE RECOVERED BY THE FOLLOWING ALGORITHM. */
/*           IF Z=1  SET  DC=0.D0  AND  DS=1.D0 */
/*           IF DABS(Z) .LT. 1  SET  DC=DSQRT(1-Z**2)  AND  DS=Z */
/*           IF DABS(Z) .GT. 1  SET  DC=1/Z  AND  DS=DSQRT(1-DC**2) */
/*     NORMALLY, THE SUBPROGRAM DROT(N,DX,INCX,DY,INCY,DC,DS) WILL */
/*     NEXT BE CALLED TO APPLY THE TRANSFORMATION TO A 2 BY N MATRIX. */
/* ***REFERENCES  LAWSON C.L., HANSON R.J., KINCAID D.R., KROGH F.T., */
/*                 *BASIC LINEAR ALGEBRA SUBPROGRAMS FOR FORTRAN USAGE*, */
/*                 ALGORITHM NO. 539, TRANSACTIONS ON MATHEMATICAL */
/*                 SOFTWARE, VOLUME 5, NUMBER 3, SEPTEMBER 1979, 308-323 */
/* ***ROUTINES CALLED  (NONE) */
/* ***END PROLOGUE  DROTG */
/* ...SCALAR ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ***FIRST EXECUTABLE STATEMENT  DROTG */
    if (abs(*da) <= abs(*db)) {
	goto L10;
    }
/*     *** HERE DABS(DA) .GT. DABS(DB) *** */
    u = *da + *da;
    v = *db / u;
/*     NOTE THAT U AND R HAVE THE SIGN OF DA */
/* Computing 2nd power */
    d__1 = v;
    r__ = sqrt(d__1 * d__1 + .25) * u;
/*     NOTE THAT DC IS POSITIVE */
    *dc = *da / r__;
    *ds = v * (*dc + *dc);
    *db = *ds;
    *da = r__;
    return 0;
/* *** HERE DABS(DA) .LE. DABS(DB) *** */
L10:
    if (*db == 0.) {
	goto L20;
    }
    u = *db + *db;
    v = *da / u;
/*     NOTE THAT U AND R HAVE THE SIGN OF DB */
/*     (R IS IMMEDIATELY STORED IN DA) */
/* Computing 2nd power */
    d__1 = v;
    *da = sqrt(d__1 * d__1 + .25) * u;
/*     NOTE THAT DS IS POSITIVE */
    *ds = *db / *da;
    *dc = v * (*ds + *ds);
    if (*dc == 0.) {
	goto L15;
    }
    *db = 1. / *dc;
    return 0;
L15:
    *db = 1.;
    return 0;
/* *** HERE DA = DB = 0.D0 *** */
L20:
    *dc = 1.;
    *ds = 0.;
    return 0;
} /* drotg_ */

/* DSCAL */
/* Subroutine */ int dscal_(int *n, double *da, double *dx, 
	int *incx)
{
    /* System generated locals */
    int i__1, i__2;

    /* Local variables */
    static int i__, m, ns, mp1;

/* ***BEGIN PROLOGUE  DSCAL */
/* ***DATE WRITTEN   791001   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D1A6 */
/* ***KEYWORDS  BLAS,LINEAR ALGEBRA,SCALE,VECTOR */
/* ***AUTHOR  LAWSON, C. L., (JPL) */
/*           HANSON, R. J., (SNLA) */
/*           KINCAID, D. R., (U. OF TEXAS) */
/*           KROGH, F. T., (JPL) */
/* ***PURPOSE  D.P. VECTOR SCALE X = A*X */
/* ***DESCRIPTION */
/*                B L A S  SUBPROGRAM */
/*    DESCRIPTION OF PARAMETERS */
/*     --INPUT-- */
/*        N  NUMBER OF ELEMENTS IN INPUT VECTOR(S) */
/*       DA  DOUBLE PRECISION SCALE FACTOR */
/*       DX  DOUBLE PRECISION VECTOR WITH N ELEMENTS */
/*     INCX  STORAGE SPACING BETWEEN ELEMENTS OF DX */
/*     --OUTPUT-- */
/*       DX  DOUBLE PRECISION RESULT (UNCHANGED IF N.LE.0) */
/*     REPLACE DOUBLE PRECISION DX BY DOUBLE PRECISION DA*DX. */
/*     FOR I = 0 TO N-1, REPLACE DX(1+I*INCX) WITH  DA * DX(1+I*INCX) */
/* ***REFERENCES  LAWSON C.L., HANSON R.J., KINCAID D.R., KROGH F.T., */
/*                 *BASIC LINEAR ALGEBRA SUBPROGRAMS FOR FORTRAN USAGE*, */
/*                 ALGORITHM NO. 539, TRANSACTIONS ON MATHEMATICAL */
/*                 SOFTWARE, VOLUME 5, NUMBER 3, SEPTEMBER 1979, 308-323 */
/* ***ROUTINES CALLED  (NONE) */
/* ***END PROLOGUE  DSCAL */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ***FIRST EXECUTABLE STATEMENT  DSCAL */
    /* Parameter adjustments */
    --dx;

    /* Function Body */
    if (*n <= 0) {
	return 0;
    }
    if (*incx == 1) {
	goto L20;
    }
/*        CODE FOR INCREMENTS NOT EQUAL TO 1. */
    ns = *n * *incx;
    i__1 = ns;
    i__2 = *incx;
    for (i__ = 1; i__2 < 0 ? i__ >= i__1 : i__ <= i__1; i__ += i__2) {
	dx[i__] = *da * dx[i__];
/* L10: */
    }
    return 0;
/*        CODE FOR INCREMENTS EQUAL TO 1. */
/*        CLEAN-UP LOOP SO REMAINING VECTOR LENGTH IS A MULTIPLE OF 5. */
L20:
    m = *n % 5;
    if (m == 0) {
	goto L40;
    }
    i__2 = m;
    for (i__ = 1; i__ <= i__2; ++i__) {
	dx[i__] = *da * dx[i__];
/* L30: */
    }
    if (*n < 5) {
	return 0;
    }
L40:
    mp1 = m + 1;
    i__2 = *n;
    for (i__ = mp1; i__ <= i__2; i__ += 5) {
	dx[i__] = *da * dx[i__];
	dx[i__ + 1] = *da * dx[i__ + 1];
	dx[i__ + 2] = *da * dx[i__ + 2];
	dx[i__ + 3] = *da * dx[i__ + 3];
	dx[i__ + 4] = *da * dx[i__ + 4];
/* L50: */
    }
    return 0;
} /* dscal_ */

/* DSWAP */
/* Subroutine */ int dswap_(int *n, double *dx, int *incx, 
	double *dy, int *incy)
{
    /* System generated locals */
    int i__1, i__2;

    /* Local variables */
    static int i__, m, ix, iy, ns, mp1;
    static double dtemp1, dtemp2, dtemp3;

/* ***BEGIN PROLOGUE  DSWAP */
/* ***DATE WRITTEN   791001   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D1A5 */
/* ***KEYWORDS  BLAS,DOUBLE PRECISION,INTERCHANGE,LINEAR ALGEBRA,VECTOR */
/* ***AUTHOR  LAWSON, C. L., (JPL) */
/*           HANSON, R. J., (SNLA) */
/*           KINCAID, D. R., (U. OF TEXAS) */
/*           KROGH, F. T., (JPL) */
/* ***PURPOSE  INTERCHANGE D.P. VECTORS */
/* ***DESCRIPTION */
/*                B L A S  SUBPROGRAM */
/*    DESCRIPTION OF PARAMETERS */
/*     --INPUT-- */
/*        N  NUMBER OF ELEMENTS IN INPUT VECTOR(S) */
/*       DX  DOUBLE PRECISION VECTOR WITH N ELEMENTS */
/*     INCX  STORAGE SPACING BETWEEN ELEMENTS OF DX */
/*       DY  DOUBLE PRECISION VECTOR WITH N ELEMENTS */
/*     INCY  STORAGE SPACING BETWEEN ELEMENTS OF DY */
/*     --OUTPUT-- */
/*       DX  INPUT VECTOR DY (UNCHANGED IF N .LE. 0) */
/*       DY  INPUT VECTOR DX (UNCHANGED IF N .LE. 0) */
/*     INTERCHANGE DOUBLE PRECISION DX AND DOUBLE PRECISION DY. */
/*     FOR I = 0 TO N-1, INTERCHANGE  DX(LX+I*INCX) AND DY(LY+I*INCY), */
/*     WHERE LX = 1 IF INCX .GE. 0, ELSE LX = (-INCX)*N, AND LY IS */
/*     DEFINED IN A SIMILAR WAY USING INCY. */
/* ***REFERENCES  LAWSON C.L., HANSON R.J., KINCAID D.R., KROGH F.T., */
/*                 *BASIC LINEAR ALGEBRA SUBPROGRAMS FOR FORTRAN USAGE*, */
/*                 ALGORITHM NO. 539, TRANSACTIONS ON MATHEMATICAL */
/*                 SOFTWARE, VOLUME 5, NUMBER 3, SEPTEMBER 1979, 308-323 */
/* ***ROUTINES CALLED  (NONE) */
/* ***END PROLOGUE  DSWAP */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ***FIRST EXECUTABLE STATEMENT  DSWAP */
    /* Parameter adjustments */
    --dy;
    --dx;

    /* Function Body */
    if (*n <= 0) {
	return 0;
    }
    if (*incx == *incy) {
	if ((i__1 = *incx - 1) < 0) {
	    goto L5;
	} else if (i__1 == 0) {
	    goto L20;
	} else {
	    goto L60;
	}
    }
L5:
/*       CODE FOR UNEQUAL OR NONPOSITIVE INCREMENTS. */
    ix = 1;
    iy = 1;
    if (*incx < 0) {
	ix = (-(*n) + 1) * *incx + 1;
    }
    if (*incy < 0) {
	iy = (-(*n) + 1) * *incy + 1;
    }
    i__1 = *n;
    for (i__ = 1; i__ <= i__1; ++i__) {
	dtemp1 = dx[ix];
	dx[ix] = dy[iy];
	dy[iy] = dtemp1;
	ix += *incx;
	iy += *incy;
/* L10: */
    }
    return 0;
/*       CODE FOR BOTH INCREMENTS EQUAL TO 1 */
/*       CLEAN-UP LOOP SO REMAINING VECTOR LENGTH IS A MULTIPLE OF 3. */
L20:
    m = *n % 3;
    if (m == 0) {
	goto L40;
    }
    i__1 = m;
    for (i__ = 1; i__ <= i__1; ++i__) {
	dtemp1 = dx[i__];
	dx[i__] = dy[i__];
	dy[i__] = dtemp1;
/* L30: */
    }
    if (*n < 3) {
	return 0;
    }
L40:
    mp1 = m + 1;
    i__1 = *n;
    for (i__ = mp1; i__ <= i__1; i__ += 3) {
	dtemp1 = dx[i__];
	dtemp2 = dx[i__ + 1];
	dtemp3 = dx[i__ + 2];
	dx[i__] = dy[i__];
	dx[i__ + 1] = dy[i__ + 1];
	dx[i__ + 2] = dy[i__ + 2];
	dy[i__] = dtemp1;
	dy[i__ + 1] = dtemp2;
	dy[i__ + 2] = dtemp3;
/* L50: */
    }
    return 0;
L60:
/*     CODE FOR EQUAL, POSITIVE, NONUNIT INCREMENTS. */
    ns = *n * *incx;
    i__1 = ns;
    i__2 = *incx;
    for (i__ = 1; i__2 < 0 ? i__ >= i__1 : i__ <= i__1; i__ += i__2) {
	dtemp1 = dx[i__];
	dx[i__] = dy[i__];
	dy[i__] = dtemp1;
/* L70: */
    }
    return 0;
} /* dswap_ */

/* DTRCO */
/* Subroutine */ int dtrco_(double *t, int *ldt, int *n, 
	double *rcond, double *z__, int *job)
{
    /* System generated locals */
    int t_dim1, t_offset, i__1, i__2;
    double d__1, d__2;

    /* Builtin functions */
    double d_sign(double *, double *);

    /* Local variables */
    static int j, k, l;
    static double s, w;
    static int i1, j1, j2;
    static double ek;
    static int kk;
    static double sm, wk, wkm;
    extern /* Subroutine */ int dscal_(int *, double *, double *, 
	    int *);
    extern double dasum_(int *, double *, int *);
    extern /* Subroutine */ int daxpy_(int *, double *, double *, 
	    int *, double *, int *);
    static int lower;
    static double tnorm, ynorm;

/* ***BEGIN PROLOGUE  DTRCO */
/* ***DATE WRITTEN   780814   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D2A3 */
/* ***KEYWORDS  CONDITION,DOUBLE PRECISION,FACTOR,LINEAR ALGEBRA,LINPACK, */
/*             MATRIX,TRIANGULAR */
/* ***AUTHOR  MOLER, C. B., (U. OF NEW MEXICO) */
/* ***PURPOSE  ESTIMATES THE CONDITION OF A DOUBLE PRECISION TRIANGULAR */
/*            MATRIX. */
/* ***DESCRIPTION */
/*     DTRCO ESTIMATES THE CONDITION OF A DOUBLE PRECISION TRIANGULAR */
/*     MATRIX. */
/*     ON ENTRY */
/*        T       DOUBLE PRECISION(LDT,N) */
/*                T CONTAINS THE TRIANGULAR MATRIX.  THE ZERO */
/*                ELEMENTS OF THE MATRIX ARE NOT REFERENCED, AND */
/*                THE CORRESPONDING ELEMENTS OF THE ARRAY CAN BE */
/*                USED TO STORE OTHER INFORMATION. */
/*        LDT     INTEGER */
/*                LDT IS THE LEADING DIMENSION OF THE ARRAY T. */
/*        N       INTEGER */
/*                N IS THE ORDER OF THE SYSTEM. */
/*        JOB     INTEGER */
/*                = 0         T  IS LOWER TRIANGULAR. */
/*                = NONZERO   T  IS UPPER TRIANGULAR. */
/*     ON RETURN */
/*        RCOND   DOUBLE PRECISION */
/*                AN ESTIMATE OF THE RECIPROCAL CONDITION OF  T . */
/*                FOR THE SYSTEM  T*X = B , RELATIVE PERTURBATIONS */
/*                IN  T  AND  B  OF SIZE  EPSILON  MAY CAUSE */
/*                RELATIVE PERTURBATIONS IN  X  OF SIZE  EPSILON/RCOND . */
/*                IF  RCOND  IS SO SMALL THAT THE LOGICAL EXPRESSION */
/*                           1.0 + RCOND .EQ. 1.0 */
/*                IS TRUE, THEN  T  MAY BE SINGULAR TO WORKING */
/*                PRECISION.  IN PARTICULAR,  RCOND  IS ZERO  IF */
/*                EXACT SINGULARITY IS DETECTED OR THE ESTIMATE */
/*                UNDERFLOWS. */
/*        Z       DOUBLE PRECISION(N) */
/*                A WORK VECTOR WHOSE CONTENTS ARE USUALLY UNIMPORTANT. */
/*                IF  T  IS CLOSE TO A SINGULAR MATRIX, THEN  Z  IS */
/*                AN APPROXIMATE NULL VECTOR IN THE SENSE THAT */
/*                NORM(A*Z) = RCOND*NORM(A)*NORM(Z) . */
/*     LINPACK.  THIS VERSION DATED 08/14/78 . */
/*     CLEVE MOLER, UNIVERSITY OF NEW MEXICO, ARGONNE NATIONAL LAB. */
/* ***REFERENCES  DONGARRA J.J., BUNCH J.R., MOLER C.B., STEWART G.W., */
/*                 *LINPACK USERS  GUIDE*, SIAM, 1979. */
/* ***ROUTINES CALLED  DASUM,DAXPY,DSCAL */
/* ***END PROLOGUE  DTRCO */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL FUNCTIONS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ***FIRST EXECUTABLE STATEMENT  DTRCO */
    /* Parameter adjustments */
    t_dim1 = *ldt;
    t_offset = 1 + t_dim1;
    t -= t_offset;
    --z__;

    /* Function Body */
    lower = *job == 0;
/*     COMPUTE 1-NORM OF T */
    tnorm = 0.;
    i__1 = *n;
    for (j = 1; j <= i__1; ++j) {
	l = j;
	if (lower) {
	    l = *n + 1 - j;
	}
	i1 = 1;
	if (lower) {
	    i1 = j;
	}
/* Computing MAX */
	d__1 = tnorm, d__2 = dasum_(&l, &t[i1 + j * t_dim1], &c__1);
	tnorm = max(d__1,d__2);
/* L10: */
    }
/*     RCOND = 1/(NORM(T)*(ESTIMATE OF NORM(INVERSE(T)))) . */
/*     ESTIMATE = NORM(Z)/NORM(Y) WHERE  T*Z = Y  AND  TRANS(T)*Y = E . */
/*     TRANS(T)  IS THE TRANSPOSE OF T . */
/*     THE COMPONENTS OF  E  ARE CHOSEN TO CAUSE MAXIMUM LOCAL */
/*     GROWTH IN THE ELEMENTS OF Y . */
/*     THE VECTORS ARE FREQUENTLY RESCALED TO AVOID OVERFLOW. */
/*     SOLVE TRANS(T)*Y = E */
    ek = 1.;
    i__1 = *n;
    for (j = 1; j <= i__1; ++j) {
	z__[j] = 0.;
/* L20: */
    }
    i__1 = *n;
    for (kk = 1; kk <= i__1; ++kk) {
	k = kk;
	if (lower) {
	    k = *n + 1 - kk;
	}
	if (z__[k] != 0.) {
	    d__1 = -z__[k];
	    ek = d_sign(&ek, &d__1);
	}
	if ((d__1 = ek - z__[k], abs(d__1)) <= (d__2 = t[k + k * t_dim1], abs(
		d__2))) {
	    goto L30;
	}
	s = (d__1 = t[k + k * t_dim1], abs(d__1)) / (d__2 = ek - z__[k], abs(
		d__2));
	dscal_(n, &s, &z__[1], &c__1);
	ek = s * ek;
L30:
	wk = ek - z__[k];
	wkm = -ek - z__[k];
	s = abs(wk);
	sm = abs(wkm);
	if (t[k + k * t_dim1] == 0.) {
	    goto L40;
	}
	wk /= t[k + k * t_dim1];
	wkm /= t[k + k * t_dim1];
	goto L50;
L40:
	wk = 1.;
	wkm = 1.;
L50:
	if (kk == *n) {
	    goto L90;
	}
	j1 = k + 1;
	if (lower) {
	    j1 = 1;
	}
	j2 = *n;
	if (lower) {
	    j2 = k - 1;
	}
	i__2 = j2;
	for (j = j1; j <= i__2; ++j) {
	    sm += (d__1 = z__[j] + wkm * t[k + j * t_dim1], abs(d__1));
	    z__[j] += wk * t[k + j * t_dim1];
	    s += (d__1 = z__[j], abs(d__1));
/* L60: */
	}
	if (s >= sm) {
	    goto L80;
	}
	w = wkm - wk;
	wk = wkm;
	i__2 = j2;
	for (j = j1; j <= i__2; ++j) {
	    z__[j] += w * t[k + j * t_dim1];
/* L70: */
	}
L80:
L90:
	z__[k] = wk;
/* L100: */
    }
    s = 1. / dasum_(n, &z__[1], &c__1);
    dscal_(n, &s, &z__[1], &c__1);
    ynorm = 1.;
/*     SOLVE T*Z = Y */
    i__1 = *n;
    for (kk = 1; kk <= i__1; ++kk) {
	k = *n + 1 - kk;
	if (lower) {
	    k = kk;
	}
	if ((d__1 = z__[k], abs(d__1)) <= (d__2 = t[k + k * t_dim1], abs(d__2)
		)) {
	    goto L110;
	}
	s = (d__1 = t[k + k * t_dim1], abs(d__1)) / (d__2 = z__[k], abs(d__2))
		;
	dscal_(n, &s, &z__[1], &c__1);
	ynorm = s * ynorm;
L110:
	if (t[k + k * t_dim1] != 0.) {
	    z__[k] /= t[k + k * t_dim1];
	}
	if (t[k + k * t_dim1] == 0.) {
	    z__[k] = 1.;
	}
	i1 = 1;
	if (lower) {
	    i1 = k + 1;
	}
	if (kk >= *n) {
	    goto L120;
	}
	w = -z__[k];
	i__2 = *n - kk;
	daxpy_(&i__2, &w, &t[i1 + k * t_dim1], &c__1, &z__[i1], &c__1);
L120:
/* L130: */
	;
    }
/*     MAKE ZNORM = 1.0 */
    s = 1. / dasum_(n, &z__[1], &c__1);
    dscal_(n, &s, &z__[1], &c__1);
    ynorm = s * ynorm;
    if (tnorm != 0.) {
	*rcond = ynorm / tnorm;
    }
    if (tnorm == 0.) {
	*rcond = 0.;
    }
    return 0;
} /* dtrco_ */

/* DTRSL */
/* Subroutine */ int dtrsl_(double *t, int *ldt, int *n, 
	double *b, int *job, int *info)
{
    /* System generated locals */
    int t_dim1, t_offset, i__1, i__2;

    /* Local variables */
    static int j, jj, case__;
    extern double ddot_(int *, double *, int *, double *, 
	    int *);
    static double temp;
    extern /* Subroutine */ int daxpy_(int *, double *, double *, 
	    int *, double *, int *);

/* ***BEGIN PROLOGUE  DTRSL */
/* ***DATE WRITTEN   780814   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D2A3 */
/* ***KEYWORDS  DOUBLE PRECISION,LINEAR ALGEBRA,LINPACK,MATRIX,SOLVE, */
/*             TRIANGULAR */
/* ***AUTHOR  STEWART, G. W., (U. OF MARYLAND) */
/* ***PURPOSE  SOLVES SYSTEMS OF THE FORM  T*X=B OR  TRANS(T)*X=B  WHERE T */
/*            IS A TRIANGULAR MATRIX OF ORDER N. */
/* ***DESCRIPTION */
/*     DTRSL SOLVES SYSTEMS OF THE FORM */
/*                   T * X = B */
/*     OR */
/*                   TRANS(T) * X = B */
/*     WHERE T IS A TRIANGULAR MATRIX OF ORDER N.  HERE TRANS(T) */
/*     DENOTES THE TRANSPOSE OF THE MATRIX T. */
/*     ON ENTRY */
/*         T         DOUBLE PRECISION(LDT,N) */
/*                   T CONTAINS THE MATRIX OF THE SYSTEM.  THE ZERO */
/*                   ELEMENTS OF THE MATRIX ARE NOT REFERENCED, AND */
/*                   THE CORRESPONDING ELEMENTS OF THE ARRAY CAN BE */
/*                   USED TO STORE OTHER INFORMATION. */
/*         LDT       INTEGER */
/*                   LDT IS THE LEADING DIMENSION OF THE ARRAY T. */
/*         N         INTEGER */
/*                   N IS THE ORDER OF THE SYSTEM. */
/*         B         DOUBLE PRECISION(N). */
/*                   B CONTAINS THE RIGHT HAND SIDE OF THE SYSTEM. */
/*         JOB       INTEGER */
/*                   JOB SPECIFIES WHAT KIND OF SYSTEM IS TO BE SOLVED. */
/*                   IF JOB IS */
/*                        00   SOLVE T*X=B, T LOWER TRIANGULAR, */
/*                        01   SOLVE T*X=B, T UPPER TRIANGULAR, */
/*                        10   SOLVE TRANS(T)*X=B, T LOWER TRIANGULAR, */
/*                        11   SOLVE TRANS(T)*X=B, T UPPER TRIANGULAR. */
/*     ON RETURN */
/*         B         B CONTAINS THE SOLUTION, IF INFO .EQ. 0. */
/*                   OTHERWISE B IS UNALTERED. */
/*         INFO      INTEGER */
/*                   INFO CONTAINS ZERO IF THE SYSTEM IS NONSINGULAR. */
/*                   OTHERWISE INFO CONTAINS THE INDEX OF */
/*                   THE FIRST ZERO DIAGONAL ELEMENT OF T. */
/*     LINPACK.  THIS VERSION DATED 08/14/78 . */
/*     G. W. STEWART, UNIVERSITY OF MARYLAND, ARGONNE NATIONAL LAB. */
/* ***REFERENCES  DONGARRA J.J., BUNCH J.R., MOLER C.B., STEWART G.W., */
/*                 *LINPACK USERS  GUIDE*, SIAM, 1979. */
/* ***ROUTINES CALLED  DAXPY,DDOT */
/* ***END PROLOGUE  DTRSL */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...EXTERNAL FUNCTIONS */
/* ...EXTERNAL SUBROUTINES */
/* ...INTRINSIC FUNCTIONS */
/* ***FIRST EXECUTABLE STATEMENT  DTRSL */
/*     BEGIN BLOCK PERMITTING ...EXITS TO 150 */
/*        CHECK FOR ZERO DIAGONAL ELEMENTS. */
    /* Parameter adjustments */
    t_dim1 = *ldt;
    t_offset = 1 + t_dim1;
    t -= t_offset;
    --b;

    /* Function Body */
    i__1 = *n;
    for (*info = 1; *info <= i__1; ++(*info)) {
/*     ......EXIT */
	if (t[*info + *info * t_dim1] == 0.) {
	    goto L150;
	}
/* L10: */
    }
    *info = 0;
/*        DETERMINE THE TASK AND GO TO IT. */
    case__ = 1;
    if (*job % 10 != 0) {
	case__ = 2;
    }
    if (*job % 100 / 10 != 0) {
	case__ += 2;
    }
    switch (case__) {
	case 1:  goto L20;
	case 2:  goto L50;
	case 3:  goto L80;
	case 4:  goto L110;
    }
/*        SOLVE T*X=B FOR T LOWER TRIANGULAR */
L20:
    b[1] /= t[t_dim1 + 1];
    if (*n < 2) {
	goto L40;
    }
    i__1 = *n;
    for (j = 2; j <= i__1; ++j) {
	temp = -b[j - 1];
	i__2 = *n - j + 1;
	daxpy_(&i__2, &temp, &t[j + (j - 1) * t_dim1], &c__1, &b[j], &c__1);
	b[j] /= t[j + j * t_dim1];
/* L30: */
    }
L40:
    goto L140;
/*        SOLVE T*X=B FOR T UPPER TRIANGULAR. */
L50:
    b[*n] /= t[*n + *n * t_dim1];
    if (*n < 2) {
	goto L70;
    }
    i__1 = *n;
    for (jj = 2; jj <= i__1; ++jj) {
	j = *n - jj + 1;
	temp = -b[j + 1];
	daxpy_(&j, &temp, &t[(j + 1) * t_dim1 + 1], &c__1, &b[1], &c__1);
	b[j] /= t[j + j * t_dim1];
/* L60: */
    }
L70:
    goto L140;
/*        SOLVE TRANS(T)*X=B FOR T LOWER TRIANGULAR. */
L80:
    b[*n] /= t[*n + *n * t_dim1];
    if (*n < 2) {
	goto L100;
    }
    i__1 = *n;
    for (jj = 2; jj <= i__1; ++jj) {
	j = *n - jj + 1;
	i__2 = jj - 1;
	b[j] -= ddot_(&i__2, &t[j + 1 + j * t_dim1], &c__1, &b[j + 1], &c__1);
	b[j] /= t[j + j * t_dim1];
/* L90: */
    }
L100:
    goto L140;
/*        SOLVE TRANS(T)*X=B FOR T UPPER TRIANGULAR. */
L110:
    b[1] /= t[t_dim1 + 1];
    if (*n < 2) {
	goto L130;
    }
    i__1 = *n;
    for (j = 2; j <= i__1; ++j) {
	i__2 = j - 1;
	b[j] -= ddot_(&i__2, &t[j * t_dim1 + 1], &c__1, &b[1], &c__1);
	b[j] /= t[j + j * t_dim1];
/* L120: */
    }
L130:
L140:
L150:
    return 0;
} /* dtrsl_ */

/* IDAMAX */
int idamax_(int *n, double *dx, int *incx)
{
    /* System generated locals */
    int ret_val, i__1, i__2;
    double d__1;

    /* Local variables */
    static int i__, ii, ns;
    static double dmax__, xmag;

/* ***BEGIN PROLOGUE  IDAMAX */
/* ***DATE WRITTEN   791001   (YYMMDD) */
/* ***REVISION DATE  820801   (YYMMDD) */
/* ***CATEGORY NO.  D1A2 */
/* ***KEYWORDS  BLAS,DOUBLE PRECISION,LINEAR ALGEBRA,MAXIMUM COMPONENT, */
/*             VECTOR */
/* ***AUTHOR  LAWSON, C. L., (JPL) */
/*           HANSON, R. J., (SNLA) */
/*           KINCAID, D. R., (U. OF TEXAS) */
/*           KROGH, F. T., (JPL) */
/* ***PURPOSE  FIND LARGEST COMPONENT OF D.P. VECTOR */
/* ***DESCRIPTION */
/*                B L A S  SUBPROGRAM */
/*    DESCRIPTION OF PARAMETERS */
/*     --INPUT-- */
/*        N  NUMBER OF ELEMENTS IN INPUT VECTOR(S) */
/*       DX  DOUBLE PRECISION VECTOR WITH N ELEMENTS */
/*     INCX  STORAGE SPACING BETWEEN ELEMENTS OF DX */
/*     --OUTPUT-- */
/*   IDAMAX  SMALLEST INDEX (ZERO IF N .LE. 0) */
/*     FIND SMALLEST INDEX OF MAXIMUM MAGNITUDE OF DOUBLE PRECISION DX. */
/*     IDAMAX =  FIRST I, I = 1 TO N, TO MINIMIZE  ABS(DX(1-INCX+I*INCX) */
/* ***REFERENCES  LAWSON C.L., HANSON R.J., KINCAID D.R., KROGH F.T., */
/*                 *BASIC LINEAR ALGEBRA SUBPROGRAMS FOR FORTRAN USAGE*, */
/*                 ALGORITHM NO. 539, TRANSACTIONS ON MATHEMATICAL */
/*                 SOFTWARE, VOLUME 5, NUMBER 3, SEPTEMBER 1979, 308-323 */
/* ***ROUTINES CALLED  (NONE) */
/* ***END PROLOGUE  IDAMAX */
/* ...SCALAR ARGUMENTS */
/* ...ARRAY ARGUMENTS */
/* ...LOCAL SCALARS */
/* ...INTRINSIC FUNCTIONS */
/* ***FIRST EXECUTABLE STATEMENT  IDAMAX */
    /* Parameter adjustments */
    --dx;

    /* Function Body */
    ret_val = 0;
    if (*n <= 0) {
	return ret_val;
    }
    ret_val = 1;
    if (*n <= 1) {
	return ret_val;
    }
    if (*incx == 1) {
	goto L20;
    }
/*        CODE FOR INCREMENTS NOT EQUAL TO 1. */
    dmax__ = abs(dx[1]);
    ns = *n * *incx;
    ii = 1;
    i__1 = ns;
    i__2 = *incx;
    for (i__ = 1; i__2 < 0 ? i__ >= i__1 : i__ <= i__1; i__ += i__2) {
	xmag = (d__1 = dx[i__], abs(d__1));
	if (xmag <= dmax__) {
	    goto L5;
	}
	ret_val = ii;
	dmax__ = xmag;
L5:
	++ii;
/* L10: */
    }
    return ret_val;
/*        CODE FOR INCREMENTS EQUAL TO 1. */
L20:
    dmax__ = abs(dx[1]);
    i__2 = *n;
    for (i__ = 2; i__ <= i__2; ++i__) {
	xmag = (d__1 = dx[i__], abs(d__1));
	if (xmag <= dmax__) {
	    goto L30;
	}
	ret_val = i__;
	dmax__ = xmag;
L30:
	;
    }
    return ret_val;
} /* idamax_ */

