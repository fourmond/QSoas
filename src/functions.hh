/**
   \file functions.hh
   Mathematical functions
   Copyright 2015 by CNRS/AMU

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
#ifndef __FUNCTIONS_HH
#define __FUNCTIONS_HH



/// This namespace contains mathematical functions
namespace Functions {

  /// "Cardinal" arctangent (accurate also for small x
  double atanc(double x);

  /// "Cardinal" hyperbolic arctangent (accurate also for small x)
  double atanhc(double x);

  /// Pseudo-voigt function
  double pseudoVoigt(double x, double w, double mu);

  /// Approximation to the Marcus-Hush-Chidsey kinetics described in Zeng et al, 2014 (doi: 10.1016/j.jelechem.2014.09.038)
  double marcusHushChidseyZeng(double lambda, double eta);

  /// Numerical integration method for the Marcus-Hush-Chidsey integral
  double marcusHushChidsey(double lambda, double eta);

  /// The position of the peak for a single adsorbed n=1 redox species
  /// in a trumpet plot:
  /// 
  /// @param rate the rate at which one changes the potential by RT/F, divided by k_0
  /// @param alpha the value of, hmmm, alpha
  /// @param prec the precision of the positioning (in units of RT/F
  ///
  /// @returns The position of the peak, in units of RT/F
  double trumpetBV(double rate, double alpha, double prec = 0.01);

};

#endif
