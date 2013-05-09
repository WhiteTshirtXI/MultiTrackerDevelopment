// ---------------------------------------------------------
//
//  edgeflipper.h
//  Tyson Brochu 2011
//  
//  Functions supporting the "edge flip" operation: replacing non-delaunay edges with their dual edges.
//
// ---------------------------------------------------------

#ifndef EL_TOPO_EDGEFLIPPER_H
#define EL_TOPO_EDGEFLIPPER_H

// ---------------------------------------------------------
//  Nested includes
// ---------------------------------------------------------

#include <cstddef>
#include <vector>
#include <mat.h>
// ---------------------------------------------------------
//  Forwards and typedefs
// ---------------------------------------------------------

namespace ElTopo {

class SurfTrack;
template<unsigned int N, class T> struct Vec;
typedef Vec<3,double> Vec3d;
typedef Vec<2,size_t> Vec2st;
typedef Vec<3,size_t> Vec3st;

// ---------------------------------------------------------
//  Class definitions
// ---------------------------------------------------------

// ---------------------------------------------------------
///
/// Edge flipper object.  Tries to produce Delaunay mesh by replacing edges with the "opposite" edge in their neighbourhood.
///
// ---------------------------------------------------------

class EdgeFlipper
{
    
public:
    
    /// Constructor
    ///
    EdgeFlipper( SurfTrack& surf, double edge_flip_min_length_change ) :
    m_surf( surf ),
    m_use_Delaunay_criterion(false)
    {}
    
    /// Flip all non-delaunay edges
    ///
    bool flip_pass();

    
private:

    /// The mesh this object operates on
    /// 
    SurfTrack& m_surf;
    
    /// Delaunay criterion vs. valence regularity
    ///
    bool m_use_Delaunay_criterion; 

    /// Check whether the new triangles created by flipping an edge introduce any intersection
    ///
    bool flip_introduces_collision(size_t edge_index, 
                                   const Vec2st& new_edge, 
                                   const Vec3st& new_triangle_a, 
                                   const Vec3st& new_triangle_b );
    
    /// Flip an edge: remove the edge and its incident triangles, then add a new edge and two new triangles
    ///
    bool flip_edge( size_t edge, size_t tri0, size_t tri1, size_t third_vertex_0, size_t third_vertex_1 );
    
    /// Gather the quadric data for a given vertex
    ///
    void getQuadric(size_t vertex, Mat33d& A);

    /// Check whether the edge meets the Delaunay criterion in the warped space (i.e. accounting for anisotropy)
    ///
    bool is_Delaunay_anisotropic( size_t edge, size_t tri0, size_t tri1, size_t third_vertex_0, size_t third_vertex_1 );

    int edge_count_bordering_region_pair(size_t vertex, Vec2i region_pair);

};

}

#endif
