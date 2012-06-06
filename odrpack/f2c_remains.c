/**
   These functions come from the f2c library, copied here for the sake
   of simplicity.

   This source code has been modified by Vincent Fourmond, and is
   licensed under the same terms as the ones below (original license).
 */

/****************************************************************
Copyright 1990 - 1995 by AT&T Bell Laboratories and Bellcore.

Permission to use, copy, modify, and distribute this software
and its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the names of AT&T Bell Laboratories or
Bellcore or any of their entities not be used in advertising or
publicity pertaining to distribution of the software without
specific, written prior permission.

AT&T and Bellcore disclaim all warranties with regard to this
software, including all implied warranties of merchantability
and fitness.  In no event shall AT&T or Bellcore be liable for
any special, indirect or consequential damages or any damages
whatsoever resulting from loss of use, data or profits, whether
in an action of contract, negligence or other tortious action,
arising out of or in connection with the use or performance of
this software.
****************************************************************/

#include <math.h>

double d_sign(double *a, double *b)
{
  double x;
  x = (*a >= 0 ? *a : - *a);
  return( *b >= 0 ? x : -x);
}

#define log10e 0.43429448190325182765

double d_lg10(double *x)
{
  return( log10e * log(*x) );
}

double pow_dd(double *ap, double *bp)
{
  return(pow(*ap, *bp) );
}

double pow_di(double *ap, int *bp)
{
  double pow, x;
  int n;
  unsigned long u;

  pow = 1;
  x = *ap;
  n = *bp;

  if(n != 0)
    {
      if(n < 0)
        {
          n = -n;
          x = 1/x;
        }
      for(u = n; ; )
        {
          if(u & 01)
            pow *= x;
          if(u >>= 1)
            x *= x;
          else
            break;
        }
    }
  return(pow);
}
