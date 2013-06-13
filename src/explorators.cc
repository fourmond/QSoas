/*
  parameterspaceexplorator.cc: implementation of the style generators
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
#include <parameterspaceexplorator.hh>
#include <fittrajectorydisplay.hh>
#include <exceptions.hh>

#include <widgets.hh>

class MonteCarloExplorator : public ParameterSpaceExplorator {

  int iterations;

  int currentIteration;
public:

  MonteCarloExplorator(FitData * data) : ParameterSpaceExplorator(data) {
    ;
  };

  virtual int prepareIterations(QWidget * parameters,
                                QList<ParameterRangeEditor*> * eds) {
    LabeledEdit * ed = dynamic_cast<LabeledEdit *>(parameters);
    if(! ed)
      throw InternalError("Invalid widget passed onto the "
                          "monte carlo explorator");
    iterations = ed->text().toInt();
    if(iterations <= 0)
      return 0;
    currentIteration = 0;
    editors = eds;
    return iterations;
  };


  QWidget * createParametersWidget(QWidget * parent) {
    return new LabeledEdit("Iterations", "20", parent);
  };

  virtual int sendNextParameters() {
    if(currentIteration >= iterations)
      return -1;
    for(int i = 0; i < editors->size(); i++) {
      ParameterRangeEditor * ed = (*editors)[i];
      if(! ed->isVariable())
        continue;
      double x = rand() * 1.0/RAND_MAX;

      x = ed->value(x);
      ed->chosenValue = x;
    }
    ++currentIteration;
    return currentIteration;
  };

};

ParameterSpaceExploratorFactoryItem 
montecarlo("monte-carlo", "Monte Carlo", 
           [](FitData *d) -> ParameterSpaceExplorator * {
             return new MonteCarloExplorator(d);
           });
                                               
