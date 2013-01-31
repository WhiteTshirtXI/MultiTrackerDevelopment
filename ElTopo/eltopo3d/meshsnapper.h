// ---------------------------------------------------------
//
//  meshsnapper.h
//  Christopher Batty 2013
//  
//  Functions to handle snapping of meshes together when they get close.
//
// ---------------------------------------------------------

#ifndef EL_TOPO_MESHSNAPPER_H
#define EL_TOPO_MESHSNAPPER_H

// ---------------------------------------------------------
//  Nested includes
// ---------------------------------------------------------

#include <cstddef>
#include <vector>

#include "facesplitter.h"
#include "edgesplitter.h"

// ---------------------------------------------------------
//  Forwards and typedefs
// ---------------------------------------------------------

namespace ElTopo {

class SurfTrack;
class FaceSplitter;
class EdgeSplitter;
template<unsigned int N, class T> struct Vec;
typedef Vec<3,double> Vec3d;
typedef Vec<3,size_t> Vec3st;

// ---------------------------------------------------------
//  Class definitions
// ---------------------------------------------------------

// ---------------------------------------------------------
///
/// Mesh snapper object.  Snaps together vertices that are close together.
///
// ---------------------------------------------------------

class MeshSnapper
{
    
public:
    
    /// Mesh snapper constructor.  Takes a SurfTrack object.
    ///
    MeshSnapper( SurfTrack& surf );

    /// Collapse all proximal vertices
    ///
    bool snap_pass();
    
  
private:
    
    friend class SurfTrack;
    friend class FaceSplitter;
    friend class EdgeSplitter;

    /// The mesh this object operates on
    /// 
    SurfTrack& m_surf;

    /// Snap threshold (how close to a vertex/edge to revert to lower dimensional element snapping
    ///
    double m_snap_threshold;

    /// The mesh this object operates on
    /// 
    FaceSplitter m_facesplitter;
    EdgeSplitter m_edgesplitter;


    /// Get all triangles which are incident on either involved vertex.
    ///
    void get_moving_triangles(size_t source_vertex, 
                              size_t destination_vertex, 
                              std::vector<size_t>& moving_triangles );
    
    
    /// Get all edges which are incident on either involved vertex.
    ///
    void get_moving_edges(size_t source_vertex, 
                          size_t destination_vertex, 
                          std::vector<size_t>& moving_edges );
    
    /// Check the "pseudo motion" introduced by snapping for collision
    ///
    bool snap_pseudo_motion_introduces_collision( size_t source_vertex, 
                                                          size_t destination_vertex, 
                                                          const Vec3d& vertex_new_position );
    
   
    
    /// Determine if the vertex pair should be allowed to snap
    ///
    bool vert_pair_is_snappable( size_t vert0, size_t vert1 );
   
    /// Determine if the edge pair should be allowed to snap 
    ///
    bool edge_pair_is_snappable( size_t vert0, size_t vert1, double& cur_length );

    /// Determine if the face-vertex pair should be allowed to snap 
    ///
    bool face_vertex_pair_is_snappable( size_t vert0, size_t vert1, double& cur_length );



    /// Perform a split-n-merge operation on a face-vert pair
    ///
    bool snap_face_vertex_pair(size_t face, size_t vert);

    /// Perform a split-n-merge operation on an edge-edge pair
    ///
    bool snap_edge_pair(size_t edge0, size_t edge1);

    /// Snap a vertex pair by moving both to their average point
    ///
    bool snap_vertex_pair( size_t vert0, size_t vert1);

};

}

#endif //EL_TOPO_MESHSNAPPER_H

