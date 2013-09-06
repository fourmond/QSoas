/*
  kineticsystemsteadystate.cc: implementation of KineticSystemSteadyState
  Copyright 2013 by Vincent Fourmond

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
#include <kineticsystemsteadystate.hh>

#include <vector.hh>
#include <kineticsystem.hh>
#include <expression.hh>
#include <exceptions.hh>

#include <integrator.hh>

KineticSystemSteadyState::KineticSystemSteadyState(KineticSystem * sys) : 
  system(sys)
{

  parameterNames = sys->allParameters();
  parameters = new double[parameterNames.size()];
  tempIndex = -1;
  tcIndex = -1;
  potIndex = -1;
  bd0Index = -1;
  for(int i = 0; i < parameterNames.size(); i++) {
    parameters[i] = 0;          // initialize to 0
    const QString & s = parameterNames[i];
    if(s == "temperature")
      tempIndex = i;
    else if(s == "e")
      potIndex = i;
    else if(s == "c_tot")
      tcIndex = i;
    else if(s == "bd0")
      bd0Index = i;
  }

  /// @question or throw from another function ?
  if(tcIndex < 0 || potIndex < 0 || tempIndex < 0)
    throw InternalError("Using a KineticSystem that was not "
                        "prepared for steady-state computations");
}

KineticSystemSteadyState::~KineticSystemSteadyState()
{
  delete[] parameters;
}

void KineticSystemSteadyState::setParameters(const QString & str)
{
  Expression::setParametersFromExpression(parameterNames,
                                          str, parameters);
}


int KineticSystemSteadyState::dimension() const
{
  return system->speciesNumber();
}


int KineticSystemSteadyState::f(const gsl_vector * x,
                                gsl_vector * tg)
{
  system->computeDerivatives(tg, x, parameters);

  // Now, we need to replace one of these value by the "total
  // concentration" constraint.
  
  double c = parameters[tcIndex];

  /// @todo Move that code into KineticSystem
  int sz = dimension();
  for(int i = 0; i < sz; i++)
    c -= gsl_vector_get(x, i);
  gsl_vector_set(tg, 0, c);

  return GSL_SUCCESS;
}

void KineticSystemSteadyState::computeVoltammogram(const Vector & potentials,
                                                   Vector * current,
                                                   QList<Vector> * concs)
{
  int nb = dimension();
  if(current)
    *current = potentials;
  if(concs) {
    concs->clear();
    if(bd0Index < 0)            // 
      for(int i = 0; i < nb; i++)
        *concs << potentials;
  }

  double concentrations[nb];
  // Initialize by spreading the concentration everywhere.
  for(int i = 0; i < nb; i++)
    concentrations[i] = parameters[tcIndex]/nb;
  
  gsl_vector_view a = gsl_vector_view_array(concentrations, nb);

  Integrator in;
  prepareSolver();              // Does one-time initialization.
  
  std::function<double (double)> fnc = [this, &a] (double bd) -> double {
    system->redoxReactionScaling = exp(-bd);
    reset(&a.vector);
    const gsl_vector * conc = solve();
    return system->computeDerivatives(NULL, conc,
                                      parameters);
  };

  for(int j = 0; j < potentials.size(); j++) {
    parameters[potIndex] = potentials[j];
    
    if(bd0Index >= 0) {
      double c = in.integrateSegment(fnc, 0, parameters[bd0Index]);
      if(current)
        (*current)[j] = c;
    }
    else {
      reset(&a.vector);
      const gsl_vector * conc = solve();
      gsl_vector_memcpy(&a.vector, conc);
      if(current)
        (*current)[j] = system->computeDerivatives(NULL, &a.vector,
                                                   parameters);
      if(concs)
        for(int i = 0; i < nb; i++)
          (*concs)[i][j] = concentrations[i];
    }
  }
}
