/*
  experimental-commands.cc: experimental stuff. And internal !
  Copyright 2013,2014 by CNRS/AMU

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
#include <linearkineticsystem.hh>

#include <terminal.hh>

#include <utils.hh>

#include <soas.hh>
#include <command.hh>
#include <group.hh>
#include <commandeffector-templates.hh>
#include <general-arguments.hh>
#include <file-arguments.hh>
#include <vector.hh>
#include <dataset.hh>
#include <datastack.hh>
#include <curveview.hh>

#include <rubyodesolver.hh>
#include <kineticsystem.hh>
#include <kineticsystemsteadystate.hh>


static void ksSteadyStateCommand(const QString &, QString file,
                                 QString parameters,
                                 const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();

  KineticSystem sys; 
  sys.parseFile(file);
  bool dispersion = false;
  updateFromOptions(opts, "dispersion", dispersion);

  QStringList extra;
  if(dispersion)
    extra << "bd0";

  sys.prepareForSteadyState(extra);

  KineticSystemSteadyState ss(&sys);

  ss.setParameters(QString("temperature = %1").arg(soas().temperature()));

  ss.setParameters(parameters, false);

  QStringList params = sys.allParameters();
  Terminal::out << "Computing with parameters: \n";
  for(int i = 0; i < params.size(); i++)
    Terminal::out << " * " << params[i] << " = " 
                  << ss.getParameters()[i] << endl;

  Vector cur;
  QList<Vector> concs;
  ss.computeVoltammogram(ds->x(), &cur, &concs);
  concs.insert(0, ds->x());
  concs.insert(1, cur);

  DataSet * nds = ds->derivedDataSet(concs, "_ss_sim.dat");
  soas().stack().pushDataSet(nds); 
}

static ArgumentList 
ksSSArgs(QList<Argument *>() 
         << new FileArgument("file", 
                             "File",
                             "File containing the kinetic system")
         << new StringArgument("parameters", 
                               "Parameter values",
                               "Values of the extra parameters")
         );

static ArgumentList 
ksSSOpts(QList<Argument *>() 
         << new BoolArgument("dispersion", 
                             "Dispersion",
                             "If on, handles the dispersion of k_0 values"));

static Command 
kss("catalytic-wave", // command name
    effector(ksSteadyStateCommand), // action
    "simulations",  // group name
    &ksSSArgs, // arguments
    &ksSSOpts, // options
    "Kinetic system steady-state",
    "Kinetic system steady-state");

//////////////////////////////////////////////////////////////////////


/// Decomposition into singular values.
/// 
/// This function decomposes a given dataset containing more than 1 Y
/// column into singular values, applying a threshold when applicable.
///
/// When fully-functional (ie not only displaying the singular
/// values), this command generates two datasets:
/// \li the "components" dataset, that has the same X values as the original
/// one
/// \li the "amplitudes" dataset, that has as many rows as there are Y
/// columns initially (would be the time amplitude if the original dataset
/// are spectra recorded at different times)
static void svCommand(const QString &,
                      const CommandOptions & opts)
{
  const DataSet * ds = soas().currentDataSet();

  int nbrows = ds->nbRows();
  int nbcols = ds->nbColumns() - 1;
  if(nbcols < 2)
    throw RuntimeError("Need more than 1 Y columns !");
  gsl_matrix * data = gsl_matrix_alloc(nbrows, nbcols);

  // Now, populate the matrix
  for(int i = 0; i < nbcols; i++) {
    gsl_vector_view col = gsl_matrix_column(data, i);
    gsl_vector_memcpy(&col.vector, ds->column(i+1).toGSLVector());
  }

  Terminal::out << "Preparing matrices of " << nbcols << "x" << nbrows << endl;

  gsl_matrix * amplitudes = gsl_matrix_alloc(nbcols, nbcols);
  gsl_vector * values = gsl_vector_alloc(nbcols);
  gsl_vector * work = gsl_vector_alloc(nbcols);

  gsl_linalg_SV_decomp(data, amplitudes, values, work);

  // Now, we show the values:
  Terminal::out << "Singular values: ";
  for(int i = 0; i < nbcols; i++) 
    Terminal::out << gsl_vector_get(values, i) << " ";

  Terminal::out << "\nNormalized singular values: ";
  for(int i = 0; i < nbcols; i++) 
    Terminal::out << gsl_vector_get(values, i)/gsl_vector_get(values, 0) << " ";

  Terminal::out << "\nCondition number: " 
                << gsl_vector_get(values, 0)/
    gsl_vector_get(values, nbcols - 1) << endl;


  // And we must do something with that now !

  int components = -1;
  bool filterOnly = false;
  updateFromOptions(opts, "components", components);
  updateFromOptions(opts, "filter", filterOnly);

  // If components are computed, either directly or though a
  // threshold, and in the case we don't filter, we create two buffers
  // with the same number of columns (number of selected components
  if(components > 0) {
    if(! filterOnly) {
      QList<Vector> ds1;
      QList<Vector> ds2;
      ds1 << ds->x();

      // the amplitude is held by the the main matrix
      for(int i = 0; i < components; i++) {
        double val = gsl_vector_get(values, i);
        gsl_vector_view v = gsl_matrix_column(data, i);
        gsl_vector_scale(&v.vector, val);
        ds1 << Vector::fromGSLVector(&v.vector);
      }

      Vector nx;
      for(int j = 0; j < nbcols; j++)
        nx << j;
      ds2 << nx;
      for(int i = 0; i < components; i++) {
        gsl_vector_view v = gsl_matrix_column(amplitudes, i);
        ds2 << Vector::fromGSLVector(&v.vector);
      }
      soas().pushDataSet(ds->derivedDataSet(ds2, "_amplitudes.dat"));
      soas().pushDataSet(ds->derivedDataSet(ds1, "_components.dat"));
    }
    else {
      // We just remove extra components

      for(int i = 0; i < nbcols; i++) {
        double val = (i < components ? gsl_vector_get(values, i) : 0);
        gsl_vector_view v = gsl_matrix_column(data, i);
        gsl_vector_scale(&v.vector, val);
      }

      gsl_matrix * final = gsl_matrix_alloc(nbrows, nbcols);
      gsl_blas_dgemm(CblasNoTrans, CblasTrans, 1.0, 
                     data, amplitudes, 0, final);
      
      QList<Vector> newCols;
      newCols << ds->x();

      for(int i = 0; i < nbcols; i++) {
        gsl_vector_view v = gsl_matrix_column(final, i);
        newCols << Vector::fromGSLVector(&v.vector);
      }
      gsl_matrix_free(final);

      soas().pushDataSet(ds->derivedDataSet(newCols, "_sv_filtered.dat"));
    }
  }
  else {
    Terminal::out << "Not doing anything. If you want decomposition or "
      "filtering, you must specify the final number of components "
      "using /components" << endl;
  }

  

  // Cleanup !
  gsl_matrix_free(amplitudes);
  gsl_matrix_free(data);
  gsl_vector_free(values);
  gsl_vector_free(work);
}


static ArgumentList 
svOpts(QList<Argument *>() 
       << new IntegerArgument("components", 
                              "Number of components to keep",
                              "If specified, keeps only that many components")
       << new BoolArgument("filter", 
                           "Filter only",
                           "If on, does not decompose, only filter")
       );

static Command 
svc("sv-decomp", // command name
    effector(svCommand), // action
    "buffer",  // group name
    NULL, // arguments
    &svOpts, // options
    "SV decomposition",
    "Singular value decomposition");
