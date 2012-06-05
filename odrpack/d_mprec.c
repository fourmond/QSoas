/* d_mprec.f -- translated by f2c (version 20100827).
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

/* DMPREC */
doublereal dmprec_(void)
{
    /* Initialized data */

    static doublereal b = 2.;
    static integer td = 53;

    /* System generated locals */
    integer i__1;
    doublereal ret_val;

    /* Builtin functions */
    double pow_di(doublereal *, integer *);

/* ***BEGIN PROLOGUE  DPREC */
/* ***REFER TO  DODR,DODRC */
/* ***ROUTINES CALLED  (NONE) */
/* ***DATE WRITTEN   860529   (YYMMDD) */
/* ***REVISION DATE  920304   (YYMMDD) */
/* ***PURPOSE  DETERMINE MACHINE PRECISION FOR TARGET MACHINE AND COMPILER */
/*            ASSUMING FLOATING-POINT NUMBERS ARE REPRESENTED IN THE */
/*            T-DIGIT, BASE-B FORM */
/*                  SIGN (B**E)*( (X(1)/B) + ... + (X(T)/B**T) ) */
/*            WHERE 0 .LE. X(I) .LT. B FOR I=1,...,T, AND */
/*                  0 .LT. X(1). */
/*            TO ALTER THIS FUNCTION FOR A PARTICULAR TARGET MACHINE, */
/*            EITHER */
/*                  ACTIVATE THE DESIRED SET OF DATA STATEMENTS BY */
/*                  REMOVING THE C FROM COLUMN 1 */
/*            OR */
/*                  SET B, TD AND TS USING I1MACH BY ACTIVATING */
/*                  THE DECLARATION STATEMENTS FOR I1MACH */
/*                  AND THE STATEMENTS PRECEEDING THE FIRST */
/*                  EXECUTABLE STATEMENT BELOW. */
/* ***END PROLOGUE  DPREC */
/* ...LOCAL SCALARS */
/* ...VARIABLE DEFINITIONS (ALPHABETICALLY) */
/*     DOUBLE PRECISION B */
/*        THE BASE OF THE TARGET MACHINE. */
/*        (MAY BE DEFINED USING I1MACH(10).) */
/*     INTEGER TD */
/*        THE NUMBER OF BASE-B DIGITS IN DOUBLE PRECISION. */
/*        (MAY BE DEFINED USING I1MACH(14).) */
/*     INTEGER TS */
/*        THE NUMBER OF BASE-B DIGITS IN SINGLE PRECISION. */
/*        (MAY BE DEFINED USING I1MACH(11).) */
/*   MACHINE CONSTANTS FOR COMPUTERS FOLLOWING IEEE ARITHMETIC STANDARD */
/*   (E.G., MOTOROLA 68000 BASED MACHINES SUCH AS SUN AND SPARC */
/*   WORKSTATIONS, AND AT&T PC 7300; AND 8087 BASED MICROS SUCH AS THE */
/*   IBM PC AND THE AT&T 6300). */
    i__1 = 1 - td;
    ret_val = pow_di(&b, &i__1);
    return ret_val;
} /* dmprec_ */

