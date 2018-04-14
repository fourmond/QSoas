/**
   \file particleswarm.cc
   A particle swarm optimizer fit engine for QSoas
   Copyright 2018 by CNRS/AMU

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
#include <fitdata.hh>
#include <fitengine.hh>
#include <exceptions.hh>

#include <gsl-types.hh>

#include <utils.hh>
#include <argumentlist.hh>
#include <general-arguments.hh>

/// A Particle swarm optimizer fit engine, based on the modifications
/// proposed by Shi and Ebenhart, in 10.1109/icec.1998.699146
class ParticleSwarmFitEngine : public FitEngine {
protected:

  class Particle {
  public:
    /// The current position
    Vector position;

    /// ... and the corresponding residuals
    double currentResiduals;

    /// The current best position
    Vector best;

    /// ... and the corresponding residuals
    double bestResiduals;


    /// The current velocity
    Vector velocity;
  };

  /// The particles
  QList<Particle> particles;

  /// The particle that has the best residuals so far.
  int currentBest;

  /// The number of free parameters
  int n;

  /// @name Engine parameteres
  ///
  /// @{
  int particleNumber;

  /// The initial interial scaling
  double initialInertia;

  /// The minimum inertial scaling
  double minInertia;

  /// The delta
  double delta;

  /// The scaling of the velocity seed
  double velocityScaling;

  /// @}

  double inertiaScaling;

  /// Some storage place
  mutable GSLVector storage;

  void dumpParticles() const {
    QTextStream o(stdout);
    for(int i = 0; i < particles.size(); i++) {
      const Particle & p = particles[i];
      o << "Particle #" << i << ", "
        << p.currentResiduals << " ("
        << p.bestResiduals << "): "
        << p.position.asText().join(",") << " -- "
        << p.velocity.asText().join(",") << " -- "
        << p.best.asText().join(",") 
        << endl;
    }
  };
public:

  /// Static options
  static ArgumentList options;

  ParticleSwarmFitEngine(FitData * data) :
    FitEngine(data),
    storage(data->dataPoints()),
    n(fitData->freeParameters()) {
    resetEngineParameters();
  };

  virtual ~ParticleSwarmFitEngine() {
  };

  double computeResiduals(const Vector & pos) const {
    fitData->f(pos.toGSLVector(), storage);
    return gsl_blas_dnrm2(storage);
  };

  /// Try to evaluate the function at the given position, return true
  /// until evaluation succeeds.
  bool tryPos(const Vector & pos, double * residuals) const {
    try {
      *residuals = computeResiduals(pos);
    }
    catch(RuntimeError & re) {
      return true;
    }
    return false;
  };

  virtual void initialize(const double * initialGuess) override {
    Vector params(n, 0);
    fitData->packParameters(initialGuess, params.toGSLVector());

    currentBest = 0;

    for(int i = 0; i < particleNumber; i++) {
      particles << Particle();
      Particle & p = particles.last();
      int tries = 20;
      do {
        p.position = params;
        if(i > 0)
          p.position.randomize(0.5,2);
        if(--tries == 0)
          throw RuntimeError("Unable to find a starting position");
        
      } while(tryPos(p.position, &p.currentResiduals));
      p.best = p.position;
      p.bestResiduals = p.currentResiduals;
      if(p.bestResiduals < particles[currentBest].bestResiduals)
        currentBest = i;

      p.velocity = p.position;
      p.velocity.randomize(-velocityScaling, velocityScaling);
    }

    inertiaScaling = initialInertia;
  };
  
  virtual const gsl_vector * currentParameters() const override {
    return particles[currentBest].best;
  };
  
  virtual int iterate() override {

    // QTextStream o(stdout);
    // o << "\n\nNext iteration" << endl;
    // dumpParticles();

    // First, we move the particles
    for(int i = 0; i < particles.size(); i++) {
      Particle & p = particles[i];
      int nbtries = 20;
      do {
        Vector np = p.position + p.velocity;
        if(tryPos(np, &p.currentResiduals)) {
          p.velocity *= 0.5;
          if(--nbtries == 0)
            throw RuntimeError("Unable to find a suitable position for next step");
        }
        else {
          p.position = np;
          if(p.bestResiduals > p.currentResiduals) {
            p.best = np;
            p.bestResiduals = p.currentResiduals;
            if(p.currentResiduals < particles[currentBest].bestResiduals)
              currentBest = i;
          }
          break;
        }
      } while(true);
    }

    // Now, we update the velocity
    const Vector & bp = particles[currentBest].best;
    for(int i = 0; i < particles.size(); i++) {
      Particle & p = particles[i];
      Vector dg = bp - p.position;
      dg.randomize(0, 2);
      Vector dp = p.best - p.position;
      dp.randomize(0, 2);
      p.velocity = p.velocity * inertiaScaling + dg + dp;
    }

    inertiaScaling -= delta;
    inertiaScaling = std::max(inertiaScaling, minInertia);
      

    // how do we determine if we converged ?
    return GSL_CONTINUE;
  };
  
  virtual double residuals() const override {
    return particles[currentBest].bestResiduals;
  };

  void computeCovarianceMatrix(gsl_matrix * target) const override {
    gsl_matrix_set_zero(target);
  };


  virtual void resetEngineParameters();
  virtual CommandOptions getEngineParameters() const;
  virtual ArgumentList * engineOptions() const {
    return &options;
  };
  virtual void setEngineParameters(const CommandOptions & params);
  
};

ArgumentList ParticleSwarmFitEngine::
options(QList<Argument*>()
        << new IntegerArgument("particles", "Number of particles")
        << new NumberArgument("starting-inertia", "Starting inertial coefficient")
        << new NumberArgument("min-inertia", "Minimal inertial coefficient")
        << new NumberArgument("delta", "Variation of the inertial coefficient")
        );

void ParticleSwarmFitEngine::resetEngineParameters()
{
  particleNumber = 50;
  initialInertia = 1.4;
  minInertia = 0.6;
  delta = 8e-2;
  velocityScaling = 0.2;
}

CommandOptions ParticleSwarmFitEngine::getEngineParameters() const
{
  CommandOptions val;
  updateOptions(val, "particles", particleNumber);
  updateOptions(val, "starting-inertia", initialInertia);
  updateOptions(val, "min-inertia", minInertia);
  updateOptions(val, "delta", delta);
  return val;
}

void ParticleSwarmFitEngine::setEngineParameters(const CommandOptions & val)
{
  updateFromOptions(val, "particles", particleNumber);
  updateFromOptions(val, "starting-inertia", initialInertia);
  updateFromOptions(val, "min-inertia", minInertia);
  updateFromOptions(val, "delta", delta);
}


static FitEngine * psoFE(FitData * d)
{
  return new ParticleSwarmFitEngine(d);
}


static FitEngineFactoryItem pso("pso", "Particle Swarm Optimizer",
                                &psoFE, &ParticleSwarmFitEngine::options);
