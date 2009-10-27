/**
 * \file Beaker.cc
 *
 * \author acoull@wetafx.co.nz
 * \date 10/26/2009
 */

#include <BASim/BASim>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <sys/time.h>
#include <tclap/CmdLine.h>

#include "Beaker.hh"

using namespace BASim;

Beaker::Beaker()
{
    m_rods.clear();
    
    m_world = new World();
    m_world->add_property( m_time, "time", 0.0 );
    m_world->add_property( m_dt, "time-step", 0.1 );
    m_world->add_property( m_gravity, "gravity", Vec3d(0, 0.0, 0) );
    
    int argc = 0; char **argv = NULL;
    PetscUtils::initializePetsc( &argc, &argv );
}

Beaker::~Beaker()
{
    PetscUtils::finalizePetsc();

    size_t numRods = m_rods.size();
    for ( size_t r=0; r<numRods; r++ )
    {
        // We're safe to clear this vector as the individual destructors will safely delete the 
        // rod data in each vector element.
        delete m_rods[ r ];
    }
    m_rods.clear();
    
    delete m_world;
}

void Beaker::addRod( vector<Vec3d>& i_initialVertexPositions, 
                     vector<Vec3d>& i_undeformedVertexPositions,
                     RodOptions& i_options )
{
    // setupRod() is defined in ../BASim/src/Physics/ElasticRods/RodUtils.hh
    ElasticRod* rod = setupRod( i_options, i_initialVertexPositions, i_undeformedVertexPositions );

    rod->fixVert(0);
    rod->fixVert(1);
    rod->fixVert(rod->nv() - 2);
    rod->fixVert(rod->nv() - 1);
    rod->fixEdge(0);
    rod->fixEdge(rod->ne() - 1);
    
    RodTimeStepper* stepper = setupRodTimeStepper( *rod );
    
    // FIXME: 
    // Why do we have to add rods and steppers seperately? It feels like the stepper should
    // be part of the rod?
    m_world->addObject( rod );
    m_world->addController( stepper );
    
    RodRenderer* rodRenderer = new RodRenderer( *rod );
    
    RodData* rodData = new RodData( rod, stepper, rodRenderer );
    m_rods.push_back( rodData );
}

void Beaker::takeTimeStep()
{
    m_world->execute();

    setTime( getTime() + getDt() );
}

RodTimeStepper* Beaker::setupRodTimeStepper( ElasticRod& rod )
{
    RodTimeStepper* stepper = new RodTimeStepper( rod );
    
    std::string integrator = "implicit";
    
    if (integrator == "symplectic") 
    {
        stepper->setDiffEqSolver( RodTimeStepper::SYMPL_EULER );
    } 
    else if (integrator == "implicit") 
    {
        stepper->setDiffEqSolver( RodTimeStepper::IMPL_EULER );
    } 
    else 
    {
        std::cerr << "Unknown integrator. " << integrator
                  << "Using default instead." << std::endl;
    }
    
    stepper->setTimeStep(0.1);
    
    Scalar massDamping = 0.0;
    if (massDamping != 0) 
    {
        stepper->addExternalForce( new RodMassDamping( massDamping ) );
    }
    
    if (getGravity().norm() > 0) 
    {
        stepper->addExternalForce( new RodGravity( getGravity() ) );
    }
    
    int iterations = 1;
    stepper->setMaxIterations( iterations );
    
    return stepper;
}

void Beaker::draw()
{
    size_t numRods = m_rods.size();
    for ( size_t r=0; r<numRods; r++ )
    {
        m_rods[ r ]->rodRenderer->render();
    }
}

