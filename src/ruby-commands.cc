/**
   \file ruby-commands.cc Ruby-related commands
   Copyright 2011 by Vincent Fourmond

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
#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <terminal.hh>
#include <soas.hh>

#include <datastack.hh>
#include <ruby.hh>
#include <ruby-templates.hh>


namespace RubyCommands {

//////////////////////////////////////////////////////////////////////



  static ArgumentList 
  fA(QList<Argument *>() 
     << new StringArgument("formula", 
                           QT_TR_NOOP("Formula"),
                           QT_TR_NOOP("Formula (valid Ruby code)")));

  static VALUE applyFormula(VALUE code, const DataSet * ds,
                            Vector * newX, Vector * newY)
  {
    ID id = rb_intern("call");
    VALUE args[2];
    const Vector & xc = ds->x();
    const Vector & yc = ds->y();
    for(int i = 0; i < xc.size(); i++) {
      args[0] = rb_float_new(xc[i]);
      args[1] = rb_float_new(yc[i]);
      VALUE ret = rb_funcall2(code, id, 2, args);
      *newX << NUM2DBL(rb_ary_entry(ret, 0));
      *newY << NUM2DBL(rb_ary_entry(ret, 1));
    }      
    return Qnil;
  }
  
  static void applyFormulaCommand(const QString &, QString formula)
  {
    const DataSet * ds = soas().currentDataSet();
    Terminal::out << QObject::tr("Applying formula '%1' to buffer %2").
      arg(formula).arg(ds->name) << endl;
    formula = QString("proc do |x,y|\n  %1\n  [x,y]\nend").arg(formula);
    VALUE block = Ruby::run(Ruby::eval, 
                            formula.toLocal8Bit().constData());
    Vector newX, newY;

    Ruby::run(&applyFormula, block, ds, &newX, &newY);

    DataSet * newDs = new DataSet(QList<Vector>() << newX << newY);
    newDs->name = ds->cleanedName() + "_mod.dat";
    soas().pushDataSet(newDs);
  }


  static Command 
  load("apply-formula", // command name
       optionLessEffector(applyFormulaCommand), // action
       "stack",  // group name
       &fA, // arguments
       NULL, // options
       QT_TR_NOOP("Apply formula"),
       QT_TR_NOOP("Applies a formula to the current dataset"),
       QT_TR_NOOP("Applies a formula to the current dataset. "
                  "The formula must be valid Ruby code."),
       "F");

  //////////////////////////////////////////////////////////////////////
  
}
