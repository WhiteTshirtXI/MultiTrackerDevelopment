/**
 * \file Beaker.hh
 *
 * \author acoull@wetafx.co.nz
 * \date 10/26/2009
 */

#ifndef BEAKER_HH_
#define BEAKER_HH_

#ifdef WETA
#include <weta/Wfigaro/Core/EigenIncludes.hh>
#include <weta/Wfigaro/Collisions/CollisionMeshData.hh>
#include <weta/Wfigaro/Core/ObjectControllerBase.hh>
#include <weta/Wfigaro/Physics/World.hh>
#include <weta/Wfigaro/Physics/ElasticRods/RodCollisionTimeStepper.hh>
#include <weta/Wfigaro/Render/RodRenderer.hh>
#else
#include <BASim/src/Core/EigenIncludes.hh>
#include <BASim/src/Collisions/CollisionMeshData.hh>
#include <BASim/src/Core/ObjectControllerBase.hh>
#include <BASim/src/Physics/World.hh>
#include <BASim/src/Physics/ElasticRods/RodCollisionTimeStepper.hh>
#include <BASim/src/Render/RodRenderer.hh>
#endif

#include "RodData.hh"
#include <tr1/unordered_map>
#include <iostream>
#include <ext/hash_map>
#include <iostream>
#include <fstream>
#include "WmFigRodGroup.hh"
#include <weta/Wfigaro/SceneXML/SceneXML.hh>

#undef USE_MPI

/*#include <Geometry/SEGMENTED_CURVE.h>
#include <Grids/GRID_3D.h>
#include <Incompressible_Flows/INCOMPRESSIBLE_UNIFORM.h>
#include <Matrices_And_Vectors/VECTOR.h>*/
#include "VolumetricCollisions/VolumetricCollisionsCPU.hh"

using namespace BASim;
using namespace std;

typedef std::tr1::unordered_map< int, Vec3d > FixedVertexMap ;
typedef std::tr1::unordered_map< int, FixedVertexMap > FixedRodVertexMap ;

class Beaker
{
public:
    Beaker();
    ~Beaker();

    void BaseAtEachTimestep();
    void BaseAfterLoad();
    
    // Accessor functions
    
    const World& getWorld() const { return *m_world; }
    World& getWorld() { return *m_world; }
    
    const Scalar& getTime() const
    {
        return m_world->property( m_timeHandle );
    }

    void setTime( const Scalar& time )
    {
        m_world->property( m_timeHandle ) = time;
    }

    const Scalar& getDt() const
    {
        return m_world->property( m_dtHandle );
    }

    void setDt( const Scalar& dt )
    {
        m_world->property( m_dtHandle ) = dt;
    }

    const Vec3d& getGravity() const
    {
      return m_world->property( m_gravityHandle );
    }
    
    void setGravity( const Vec3d& gravity )
    {
        m_gravity = gravity;
        m_world->property( m_gravityHandle ) = m_gravity;
    }

    const int& getMaxIter() const
    {
        return m_world->property( m_maxIterHandle );
    }

    void setMaxIter(const int& maxIter)
    {
        m_world->property( m_maxIterHandle ) = std::max(maxIter,1);
    }

    /*vector<RodData*>* rodData( size_t i_rodGroup )
    {
        return &( m_rodDataMap[ i_rodGroup ] );
    }*/

    void setPlasticDeformations( bool i_plasticDeformations )
    {
        m_plasticDeformations = i_plasticDeformations;
    }
    
    void shouldDrawSubsteppedVertices( bool i_shouldDrawSubsteppedVertices )
    {
        m_shouldDrawSubsteppedVertices = i_shouldDrawSubsteppedVertices;
    }

    void clumpingEnabled( bool i_isClumpingEnabled )
    {
        m_isClumpingEnabled = i_isClumpingEnabled;
    }

    void clumpingCoefficient( double i_clumpingCoefficient )
    {
        m_clumpingCoefficient = i_clumpingCoefficient;
    }

    ////////////////////////////////////////////////////////////////////////////////////////
    //
    // Volumetric collision stuff
    //
    ////////////////////////////////////////////////////////////////////////////////////////

    void setFlip( double i_flip )
    {
        m_flip = i_flip;
    }
    
    void setSlip( double i_slip )
    {
        m_slip = i_slip;
    }
    
    void setDoVolumetricCollisions( bool i_doVolumetricCollisions )
    {
        m_doVolumetricCollisions = i_doVolumetricCollisions;
    }

    void setTargetEdgeDensity( double i_targetEdgeDensity )
    {
        m_targetEdgeDensity = i_targetEdgeDensity;
    }

    void setVolumetricRadius( double i_volumetricRadius )
    {
        m_volumetricRadius = i_volumetricRadius;
    }

    void setGridDX( double i_gridDX )
    {
        m_gridDX = i_gridDX;
    }

    void setSeparationCondition( double i_separationConditionX, double i_separationConditionY, double i_separationConditionZ )
    {
        m_separationCondition[ 0 ] = i_separationConditionX;
        m_separationCondition[ 1 ] = i_separationConditionY;
        m_separationCondition[ 2 ] = i_separationConditionZ;
    }

    void setDisplayGrid( bool i_displayGrid )
    {
        m_displayGrid = i_displayGrid;
    }    

    void setDisplayGridVelocitiesMultiplier( double i_displayGridVelocitiesMultiplier )
    {
        m_displayGridVelocitiesMultiplier = i_displayGridVelocitiesMultiplier;
    }
    
    void setMaxDisplayDensity( double i_maxDisplayDensity )
    {
        m_maxDisplayDensity = i_maxDisplayDensity;
    }
    
    void setDisplayCollisionBoundary( bool i_displayCollisionBoundary )
    {
        m_displayCollisionBoundary = i_displayCollisionBoundary;
    }

    void setDisplayAirBoundary( bool i_displayAirBoundary )
    {
        m_displayAirBoundary = i_displayAirBoundary;
    }
    
   // RodCollisionTimeStepper* setupRodTimeStepper( RodData* i_rodData );
    
    void draw(void);

    void takeTimeStep( int i_numberOfThreadsToUse, Scalar i_stepSize, 
                       int i_subSteps, bool i_collisionsEnabled,
                       bool i_selfCollisionPenaltyForcesEnabled,
                       bool i_fullSelfCollisionsEnabled,
                       int i_fullSelfCollisionIters,
                       double i_selfCollisionCOR,
                       FixedRodVertexMap* i_fixedVertices = NULL,
                       bool i_zeroAllTwist = false );
    
    /*(void addRod( size_t i_rodGroup,
                 vector<Vec3d>& i_initialVertexPositions, 
                 vector<Vec3d>& i_undeformedVertexPositions,
                 RodOptions& i_options );*/
    
    void initialiseWorld();
    void resetEverything();
    //void createSpaceForRods( size_t i_rodGroup, size_t i_numRods );
    void addRodsToWorld( size_t i_rodGroupIndex, WmFigRodGroup* i_rodGroup );
    bool collisionMeshInitialised( const size_t id );
    void initialiseCollisionMesh( BASim::CollisionMeshData *collisionMeshData, size_t id );
    void removeCollisionMesh( const size_t id );
  // void checkAllRodForces(); 
    void startTimer( timeval& i_startTimer );
    double stopTimer( timeval& i_startTimer );
    void resetTimers();
    void printTimingInfo();
    void setTimingsFile( std::string i_fileName );
    void setTimingEnabled( bool i_timingsEnabled );
    std::string makeString( double i_val );

    void startXMLLogging( std::string& i_xmlFilePath, std::string& i_mayaSceneFilename );
    void writeXMLFileToDisk();


private:
    //void storeMaterialFrames();
    
    World* m_world;
    RodDataMap m_rodDataMap;
    CollisionMeshDataHashMap m_collisionMeshMap;
    
    ObjPropHandle<Scalar> m_timeHandle;
    ObjPropHandle<Scalar> m_dtHandle;
    ObjPropHandle<Vec3d> m_gravityHandle;
    ObjPropHandle<int> m_maxIterHandle;
    bool m_plasticDeformations;    
    Vec3d m_gravity;

    // FIXME:
    // Pointless vector with pointers to the rods. Get rid of it. It 
    // is here simply because the self collision code needed it and I have
    // to get some numbers out of this before I leave Columbia.
    vector<ElasticRod*> m_rods;
    
    bool m_timingEnabled;
    std::string m_timingsFile;
    timeval m_timerStart;
    
    double m_meshInterpolationTime;
    double m_vertexInterpolationTime;
    double m_objectCollisionForces;
    double m_objectCollisionResponse;
    double m_collisionStructuresTidyup;
    double m_selfCollisionPenaltyForceTime;
    double m_selfCollisionsResponseTime;
    double m_fastestSelfCollisionPenaltyForceTime;
    double m_slowestSelfCollisionPenaltyForceTime;
    double m_fastestSelfCollisionsResponseTime;
    double m_slowestSelfCollisionsResponseTime;
    double m_integrationStepTime;
    double m_slowestIntegrationTime;
    double m_fastestIntegrationTime;
    double m_slowestCollisionForcesTime;
    double m_fastestCollisionForcesTime;
    double m_slowestCollisionResponseTime;
    double m_fastestCollisionResponseTime;
    double m_fastestFrameTime;
    double m_slowestFrameTime;
    double m_totalSimTime;
    size_t m_numberOfFramesSimulated;
    
    ofstream m_timingsFP;
    
    int m_numberofThreadsUsed;
    size_t m_numRods;
    
    vector<MaterialFrame> m_rodRootMaterialFrame;
    vector<MaterialFrame> m_strandRootMaterialFrame;
    vector<MaterialFrame> m_rodRefMaterialFrame;

    vector< vector < vector < Vec3d > > > m_subSteppedVertexPositions;
    bool m_shouldDrawSubsteppedVertices;

    bool m_isClumpingEnabled;
    double m_clumpingCoefficient;

    bool m_isXMLLoggingEnabled;
    SceneXML* m_sceneXML;

    std::vector< InitialRodConfiguration > m_initialRodConfigurations;

    VolumetricCollisions* m_volumetricCollisions;
    
    double m_flip;
    double m_slip;
    bool m_doVolumetricCollisions;
    double m_targetEdgeDensity;
    double m_volumetricRadius;
    double m_gridDX;    
    bool m_displayGrid;
    double m_displayGridVelocitiesMultiplier;
    double m_maxDisplayDensity;
    bool m_displayCollisionBoundary;
    bool m_displayAirBoundary;
    Vec3d m_separationCondition;
};

#endif // BEAKER_HH_
