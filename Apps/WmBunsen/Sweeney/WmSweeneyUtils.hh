#ifndef WMSWEENEYUTILS_HH_
#define WMSWEENEYUTILS_HH_

#include "WmSweeneyNode.hh"

#include <maya/MFnMesh.h>
#include <maya/MFloatPoint.h>

// This is a generic place for utility functions that are needed across files and do not really
// live in any one class.

namespace sweeney {
namespace utils {

// Generic Functionality
MStatus findASelectedNodeByTypeName( MString& i_typeName, MObject* o_selectedNodeObject,
    MDagPath* o_selectedNodDagpath = NULL );
    
// Sweeney Functionality
MStatus findSelectedSweeneyNodeAndRodManager( WmSweeneyNode* o_wmSweeneyNode,
    WmSweeneyRodManager* o_wmSweenyRodManager, MDagPath* o_selectedNodDagpath = NULL );

// General Functionality
MStatus findPointOnMeshFrom2dScreenCoords(  MFnMesh& i_meshFn, short i_x, short i_y, 
    MFloatPoint& o_position, MFloatVector& o_normal );
    
MStatus findClosestRayMeshIntersection( MFnMesh& i_meshFn, const MFloatPoint& i_rayStart, 
    const MFloatVector& i_rayVec, MFloatPoint& o_hit, MFloatVector& o_normal ) ;

}
}

#endif
