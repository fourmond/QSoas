/*
  signals.cc: interruption of various functions by UNIX signals
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
#include <soas.hh>
#include <hook.hh>

#include <signal.h>

#include <stdio.h>
#include <mruby.hh>



void handleSignal(int sig)
{
  switch(sig) {
  case SIGUSR1:
    soas().shouldStopFit = true;
    fprintf(stderr, "Caught signal USR1, cancelling current fits if any\n");
    break;
  case SIGUSR2:
    {
      mrb_state * mrb = MRuby::ruby()->mrb;
      fprintf(stderr, "Caught signal USR2, dumping mruby GC info:\n"
              " -> live: %ld, arena: %d\n",
              mrb->gc.live, mrb->gc.arena_idx);
    }
    break;
  default:
    ;
  }
}

void setupSignals()
{
  signal(SIGUSR1, handleSignal); 
  signal(SIGUSR2, handleSignal); 
}

static Hook sg(&setupSignals);
       
