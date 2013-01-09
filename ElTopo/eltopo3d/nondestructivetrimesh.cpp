// ---------------------------------------------------------
//
//  nondestructivetrimesh.cpp
//  Tyson Brochu 2008
//  
//  Implementation of NonDestructiveTriMesh: the graph of a 
//  triangle surface mesh.  See header for more details.
//
// ---------------------------------------------------------

// ---------------------------------------------------------
// Includes
// ---------------------------------------------------------

#include <nondestructivetrimesh.h>

#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <wallclocktime.h>

// ---------------------------------------------------------
// Local constants, typedefs, macros
// ---------------------------------------------------------

namespace ElTopo {

namespace {
    
/// Avoid modulo operator in (i+1)%3
const unsigned int i_plus_one_mod_three[3] = {1,2,0};
    
}   // namespace


// ---------------------------------------------------------
// Member function definitions
// ---------------------------------------------------------

// --------------------------------------------------------
///
/// Clear all mesh information
///
// --------------------------------------------------------

void NonDestructiveTriMesh::clear()
{
    m_tris.clear();

    ////////////////////////////////////////////////////////////
    // FD 20121126
    //
    // clear the face label data
    //
    m_triangle_labels.clear();
    
    ////////////////////////////////////////////////////////////

    clear_connectivity();
}


// --------------------------------------------------------
///
/// Mark a triangle as deleted without actually changing the data structures
///
// --------------------------------------------------------

void NonDestructiveTriMesh::nondestructive_remove_triangle(size_t tri)
{
    // Update the vertex->triangle map, m_vertex_to_triangle_map
    
    Vec3st& t = m_tris[tri];
    for(unsigned int i = 0; i < 3; i++)
    {
        // Get the set of triangles incident on vertex t[i]
        std::vector<size_t>& vt = m_vertex_to_triangle_map[t[i]];
        
        for( int j = 0; j < (int)vt.size(); j++ )
        {
            // If a triangle incident on vertex t[i] is tri, delete it
            if(vt[j] == tri)
            {  
                vt.erase( vt.begin() + j );
                --j;
            }
        }
    }
    
    // Update the triangle->edge map, m_triangle_to_edge_map
    
    Vec3st& te = m_triangle_to_edge_map[tri];
    
    for(unsigned int i = 0; i < 3; i++)
    {
        size_t inc_edge = te[i];
        
        std::vector<size_t>& et = m_edge_to_triangle_map[inc_edge];
        
        for( int j = 0; j < (int) et.size(); j++)
        {
            if(et[j] == tri)
            {
                et.erase( et.begin() + j );
                --j;
            }
        }
        
        if ( et.size() == 1 )
        {
            m_is_boundary_edge[inc_edge] = true;
        }
        else
        {
            m_is_boundary_edge[inc_edge] = false;
        }
        
        if ( et.empty() )
        {
            // No triangles are incident on this edge.  Delete it.
            nondestructive_remove_edge( inc_edge );
        }         
    }
    
    // triangle is deleted, clear its auxiliary structures
    te[0] = te[1] = te[2] = 0;
    
    update_is_boundary_vertex( t[0] );
    update_is_boundary_vertex( t[1] );   
    update_is_boundary_vertex( t[2] );
    
    // Clear t, marking it as deleted
    t[0] = t[1] = t[2] = 0;
    
    
}


// --------------------------------------------------------
///
/// Add a triangle to the tris structure, update connectivity
///
// --------------------------------------------------------

size_t NonDestructiveTriMesh::nondestructive_add_triangle( const Vec3st& tri )
{
    assert( tri[0] < m_vertex_to_edge_map.size() );
    assert( tri[1] < m_vertex_to_edge_map.size() );
    assert( tri[2] < m_vertex_to_edge_map.size() );
    
    size_t idx = m_tris.size();
    m_tris.push_back(tri);
    m_triangle_to_edge_map.resize(idx+1);
    
    ////////////////////////////////////////////////////////////
    // FD 20121126
    //
    // create a placeholder face label (this is not made a 
    //  function argument because we want to minimize changes
    //  to El Topo)
    //
    m_triangle_labels.push_back(Vec2i(-1, -1));
    
    ////////////////////////////////////////////////////////////
    
    for(unsigned int i = 0; i < 3; i++)
    {
        size_t vtx0 = tri[ i ];
        size_t vtx1 = tri[ i_plus_one_mod_three[i] ];
        
        // Find the edge composed of these two vertices
        size_t e = get_edge_index(vtx0, vtx1);
        if(e == m_edges.size())
        {
            // if the edge doesn't exist, add it
            e = nondestructive_add_edge(vtx0, vtx1);
        }
        
        // Update connectivity
        m_edge_to_triangle_map[e].push_back(idx);       // edge->triangle
        
        if ( m_edge_to_triangle_map[e].size() == 1 )
        {
            m_is_boundary_edge[e] = true; 
        }
        else
        {
            m_is_boundary_edge[e] = false;
        }
        
        m_triangle_to_edge_map[idx][i] = e;                // triangle->edge
        m_vertex_to_triangle_map[tri[i]].push_back(idx);   // vertex->triangle      
    }
    
    update_is_boundary_vertex( tri[0] );
    update_is_boundary_vertex( tri[1] );
    update_is_boundary_vertex( tri[2] );
    
    return idx;
    
}

// --------------------------------------------------------
///
/// Add a vertex, update connectivity.  Return index of new vertex.
///
// --------------------------------------------------------

size_t NonDestructiveTriMesh::nondestructive_add_vertex( )
{  
    assert( m_vertex_to_edge_map.size() == m_vertex_to_triangle_map.size() );
    assert( m_vertex_to_edge_map.size() == m_is_boundary_vertex.size() );
    
    ////////////////////////////////////////////////////////////
    // FD 20130102
  
    assert(m_vertex_constraint_labels.size() == m_vertex_to_edge_map.size());
  
    m_vertex_constraint_labels.push_back(0);
  
    ////////////////////////////////////////////////////////////

    m_vertex_to_edge_map.resize( m_vertex_to_edge_map.size() + 1 );
    m_vertex_to_triangle_map.resize( m_vertex_to_triangle_map.size() + 1 );
    m_is_boundary_vertex.resize( m_is_boundary_vertex.size() + 1 );
    
    return m_vertex_to_triangle_map.size() - 1;
}


// --------------------------------------------------------
///
/// Remove a vertex, update connectivity
///
// --------------------------------------------------------

void NonDestructiveTriMesh::nondestructive_remove_vertex(size_t vtx)
{
    
    m_vertex_to_triangle_map[vtx].clear();    //triangles incident on vertices
    
    // check any m_edges incident on this vertex are marked as deleted
    for ( size_t i = 0; i < m_vertex_to_edge_map[vtx].size(); ++i )
    {
        assert( m_edges[ m_vertex_to_edge_map[vtx][i] ][0] == m_edges[ m_vertex_to_edge_map[vtx][i] ][1] );
    }
    
    m_vertex_to_edge_map[vtx].clear();   //edges incident on vertices
}


// ---------------------------------------------------------
///
/// Update the number of vertices in the mesh.
///
// ---------------------------------------------------------

void NonDestructiveTriMesh::set_num_vertices( size_t num_vertices )
{
    if ( num_vertices >= m_vertex_to_triangle_map.size() )
    {
        // expand the vertex data structures with empties
    }
    else
    {
        // reduce the number of vertices
        
        assert( m_vertex_to_triangle_map.size() == m_vertex_to_edge_map.size() );
        assert( m_vertex_to_triangle_map.size() == m_is_boundary_vertex.size() );
        
        ////////////////////////////////////////////////////////////
        // FD 20130102
        
        assert(m_vertex_constraint_labels.size() == m_vertex_to_edge_map.size());
        
        ////////////////////////////////////////////////////////////
        
        for ( size_t i = num_vertices; i < m_vertex_to_triangle_map.size(); ++i )
        {
            assert( vertex_is_deleted(i) );
            assert( m_vertex_to_edge_map[i].size() == 0 );
            assert( m_vertex_to_triangle_map[i].size() == 0 );
        }
    }
    
    m_vertex_to_edge_map.resize( num_vertices );
    m_vertex_to_triangle_map.resize( num_vertices );
    m_is_boundary_vertex.resize( num_vertices );

    ////////////////////////////////////////////////////////////
    // FD 20130102
    
    m_vertex_constraint_labels.resize( num_vertices );
    
    ////////////////////////////////////////////////////////////
    
    test_connectivity();
    
}


// --------------------------------------------------------
///
/// Add an edge to the list.  Return the index of the new edge.
///
// --------------------------------------------------------

size_t NonDestructiveTriMesh::nondestructive_add_edge(size_t vtx0, size_t vtx1)
{
    
    size_t edge_index = m_edges.size();
    m_edges.push_back(Vec2st(vtx0, vtx1));
    
    m_edge_to_triangle_map.push_back( std::vector<size_t>( 0 ) );
    
    m_is_boundary_edge.push_back( true );
    
    m_vertex_to_edge_map[vtx0].push_back(edge_index);
    m_vertex_to_edge_map[vtx1].push_back(edge_index);
    
    return edge_index;
}


// --------------------------------------------------------
///
/// Mark an edge as deleted, update connectivity
///
// --------------------------------------------------------

void NonDestructiveTriMesh::nondestructive_remove_edge( size_t edge_index )
{
    // vertex 0
    {
        std::vector<size_t>& vertex_to_edge_map = m_vertex_to_edge_map[ m_edges[edge_index][0] ];
        for ( int i=0; i < (int)vertex_to_edge_map.size(); ++i)
        {
            if ( vertex_to_edge_map[i] == edge_index )
            {
                vertex_to_edge_map.erase( vertex_to_edge_map.begin() + i );
                --i;
            }
        }
    }
    
    // vertex 1
    {
        std::vector<size_t>& vertex_to_edge_map = m_vertex_to_edge_map[ m_edges[edge_index][1] ];
        for ( int i=0; i < (int)vertex_to_edge_map.size(); ++i)
        {
            if ( vertex_to_edge_map[i] == edge_index )
            {
                vertex_to_edge_map.erase( vertex_to_edge_map.begin() + i );
                --i;
            }
        }
    }
    
    m_edges[edge_index][0] = 0;
    m_edges[edge_index][1] = 0; 
    
}


// ---------------------------------------------------------
///
/// Determine if the given vertex is on a boundary edge and store in data structure.
///
// ---------------------------------------------------------

void NonDestructiveTriMesh::update_is_boundary_vertex( size_t v )
{
    m_is_boundary_vertex[v] = false;
    
    for ( size_t i = 0; i < m_vertex_to_edge_map[v].size(); ++i )
    {
        size_t edge_index = m_vertex_to_edge_map[v][i];
        
        if ( m_is_boundary_edge[edge_index] )
        {
            m_is_boundary_vertex[v] = true;
            return;
        }
    }
    
}


// ---------------------------------------------------------
///
/// Ensure that all adjacent triangles have consistent orientation.
///
// ---------------------------------------------------------

void NonDestructiveTriMesh::verify_orientation( )
{
    for ( size_t i = 0; i < m_edges.size(); ++i )
    {
        if ( m_edge_to_triangle_map[i].size() != 2 )
        {
            continue;
        }
        
        if ( edge_is_deleted(i) ) { continue; }
        
        size_t a = m_edges[i][0];
        size_t b = m_edges[i][1];
        const Vec3st& tri0 = m_tris[ m_edge_to_triangle_map[i][0] ];
        const Vec3st& tri1 = m_tris[ m_edge_to_triangle_map[i][1] ]; 
        
        bool orient0 = oriented(a, b, tri0 );
        bool orient1 = oriented(a, b, tri1 );
        
        assert( orient0 != orient1 );
    }
}


// --------------------------------------------------------
///
/// Find edge specified by two vertices.  Return edges.size if the edge is not found.
///
// --------------------------------------------------------

size_t NonDestructiveTriMesh::get_edge_index(size_t vtx0, size_t vtx1) const
{
    assert( vtx0 < m_vertex_to_edge_map.size() );
    assert( vtx1 < m_vertex_to_edge_map.size() );
    
    const std::vector<size_t>& edges0 = m_vertex_to_edge_map[vtx0];
    const std::vector<size_t>& edges1 = m_vertex_to_edge_map[vtx1];
    
    for(size_t e0 = 0; e0 < edges0.size(); e0++)
    {
        size_t edge0 = edges0[e0];
        
        for(size_t e1 = 0; e1 < edges1.size(); e1++)
        {
            if( edge0 == edges1[e1] && m_edges[edge0][0] != m_edges[edge0][1] )
            {
                assert( ( m_edges[edge0][0] == vtx0 && m_edges[edge0][1] == vtx1 ) ||
                       ( m_edges[edge0][1] == vtx0 && m_edges[edge0][0] == vtx1 ) );
                
                return edge0;
            }
        }
    }
    
    return m_edges.size();
}


// --------------------------------------------------------
///
/// Find triangle specified by three vertices.  Return triangles.size if the triangle is not found.
///
// --------------------------------------------------------

size_t NonDestructiveTriMesh::get_triangle_index( size_t vtx0, size_t vtx1, size_t vtx2 ) const
{
    Vec3st verts( vtx0, vtx1, vtx2 );
    
    const std::vector<size_t>& triangles0 = m_vertex_to_triangle_map[vtx0];
    for ( size_t i = 0; i < triangles0.size(); ++i )
    {
        if ( triangle_has_these_verts( m_tris[triangles0[i]], verts ) )
        {
            return triangles0[i];
        }
    }
    
    return m_tris.size();
    
}



// --------------------------------------------------------
///
/// Remove triangles which have been deleted by nondestructive_remove_triangle
///
// --------------------------------------------------------

void NonDestructiveTriMesh::clear_deleted_triangles( std::vector<Vec2st>* defragged_triangle_map )
{  
    
    std::vector<Vec3st> new_tris;
    new_tris.reserve( m_tris.size() );
    
    if ( defragged_triangle_map != NULL )
    {
        for ( size_t i = 0; i < m_tris.size(); ++i )
        {
            if ( !triangle_is_deleted(i) ) 
            {
                new_tris.push_back( m_tris[i] );
                Vec2st map_entry(i, new_tris.size()-1);
                defragged_triangle_map->push_back( map_entry );
            }
        }
    }
    else
    {
        for ( size_t i = 0; i < m_tris.size(); ++i )
        {
            if ( !triangle_is_deleted(i) ) 
            {
                new_tris.push_back( m_tris[i] );
            }
        }      
    }
    
    replace_all_triangles( new_tris );
    
    ////////////////////////////////////////////////////////////
    // FD 20121126
    //
    // replace the face label data
    //
    std::vector<Vec2i> new_labels;
    new_labels.reserve(new_tris.size());
    
    for (size_t i = 0; i < defragged_triangle_map->size(); i++)
    {
        new_labels[defragged_triangle_map->at(i)[1]] = m_triangle_labels[defragged_triangle_map->at(i)[0]];
    }
    
    m_triangle_labels = new_labels;
    
    ////////////////////////////////////////////////////////////

}


// --------------------------------------------------------
///
/// Remove auxiliary connectivity information
///
// --------------------------------------------------------

void NonDestructiveTriMesh::clear_connectivity()
{
    m_edges.clear();
    m_vertex_to_edge_map.clear();
    m_vertex_to_triangle_map.clear();
    m_edge_to_triangle_map.clear();
    m_triangle_to_edge_map.clear();
    m_is_boundary_edge.clear();
    m_is_boundary_vertex.clear();
    
}


// --------------------------------------------------------
///
/// Clear and rebuild connectivity information
///
// --------------------------------------------------------

void NonDestructiveTriMesh::update_connectivity( )
{
    
    clear_connectivity();
    
    size_t nv = 0;
    for ( size_t i = 0; i < m_tris.size(); ++i )
    {
        nv = max( nv, m_tris[i][0] );
        nv = max( nv, m_tris[i][1] );
        nv = max( nv, m_tris[i][2] );      
    }
    ++nv;
    
    m_vertex_to_triangle_map.resize(nv);
    m_vertex_to_edge_map.resize(nv);
    m_triangle_to_edge_map.resize(m_tris.size());
    
    for(size_t i = 0; i < m_tris.size(); i++)
    {
        Vec3st& t = m_tris[i];
        
        if(t[0] != t[1])
        {
            
            assert( t[0] < nv );
            assert( t[1] < nv );
            assert( t[2] < nv );
            
            for(unsigned int j = 0; j < 3; j++)
                m_vertex_to_triangle_map[t[j]].push_back(i);
            
            Vec3st& te = m_triangle_to_edge_map[i];
            
            for(int j = 0; j < 3; j++)
            {
                size_t vtx0 = t[j];
                size_t vtx1 = t[ i_plus_one_mod_three[j] ];
                
                size_t e = get_edge_index(vtx0, vtx1);
                
                if(e == m_edges.size())
                {
                    e = nondestructive_add_edge(vtx0, vtx1);
                }
                
                te[j] = e;
                m_edge_to_triangle_map[e].push_back(i);
            }
        }
    }
    
    // find boundary edges and vertices
    m_is_boundary_edge.resize( m_edges.size() );
    m_is_boundary_vertex.resize( nv, false );
    
    for ( size_t e = 0; e < m_edge_to_triangle_map.size(); ++e )
    {
        //if ( m_edge_to_triangle_map[e].size() % 2 == 0 )
        if ( m_edge_to_triangle_map[e].size() != 1 )
        {
            m_is_boundary_edge[e] = false;
        }
        else
        {
            m_is_boundary_edge[e] = true;
            m_is_boundary_vertex[ m_edges[e][0] ] = true;
            m_is_boundary_vertex[ m_edges[e][1] ] = true;
        }
    }
    
}


// --------------------------------------------------------
///
/// Check the consistency of auxiliary data structures
///
// --------------------------------------------------------

void NonDestructiveTriMesh::test_connectivity() const
{
    
    // check sizes
    
    assert( m_is_boundary_edge.size() == m_edges.size() );
    assert( m_edge_to_triangle_map.size() == m_edges.size() );
    
    assert( m_is_boundary_vertex.size() == m_vertex_to_edge_map.size() );
    assert( m_is_boundary_vertex.size() == m_vertex_to_triangle_map.size() );   
    
    assert( m_triangle_to_edge_map.size() == m_tris.size() );
    
    // m_is_boundary_edge
    
    for ( size_t i = 0; i < m_is_boundary_edge.size(); ++i )
    {
        if ( edge_is_deleted(i) ) { continue; }
        if ( m_is_boundary_edge[i] )
        {
            assert( m_edge_to_triangle_map[i].size() == 1 );
        }
        else
        {
            assert( m_edge_to_triangle_map[i].size() > 1 );
        }
    }
    
    // m_is_boundary_vertex
    
    for ( size_t i = 0; i < m_is_boundary_vertex.size(); ++i )
    {
        if ( vertex_is_deleted(i) ) { continue; }
        
        bool found_incident_boundary_edge = false;
        for ( size_t j = 0; j < m_vertex_to_edge_map[i].size(); ++j )
        {
            size_t inc_edge = m_vertex_to_edge_map[i][j];
            if ( m_is_boundary_edge[inc_edge] )
            {
                found_incident_boundary_edge = true;
            }
        }
        assert( m_is_boundary_vertex[i] == found_incident_boundary_edge );
    }
    
    // m_vertex_to_edge_map
    
    for ( size_t i = 0; i < m_vertex_to_edge_map.size(); ++i )
    {
        if ( vertex_is_deleted(i) ) { continue; }
        for ( size_t j = 0; j < m_vertex_to_edge_map[i].size(); ++j )
        {
            size_t inc_edge = m_vertex_to_edge_map[i][j];         
            assert( !edge_is_deleted( inc_edge ) );
            assert( m_edges[inc_edge][0] == i || m_edges[inc_edge][1] == i );
        }         
    }
    
    
    // m_vertex_to_triangle_map
    
    for ( size_t i = 0; i < m_vertex_to_triangle_map.size(); ++i )
    {
        if ( vertex_is_deleted(i) ) { continue; }
        for ( size_t j = 0; j < m_vertex_to_triangle_map[i].size(); ++j )
        {
            size_t inc_triangle = m_vertex_to_triangle_map[i][j];
            assert( m_tris[inc_triangle][0] == i || m_tris[inc_triangle][1] == i || m_tris[inc_triangle][2] == i );
        }         
    }
    
    // m_edge_to_triangle_map
    
    for ( size_t i = 0; i < m_edge_to_triangle_map.size(); ++i )
    {
        if ( edge_is_deleted(i) ) { continue; }
        for ( size_t j = 0; j < m_edge_to_triangle_map[i].size(); ++j )
        {
            size_t triangle_index = m_edge_to_triangle_map[i][j];
            size_t num_common_verts = 0;
            if ( m_tris[triangle_index][0] == m_edges[i][0] || m_tris[triangle_index][0] == m_edges[i][1] )
            {
                ++num_common_verts;
            }
            if ( m_tris[triangle_index][1] == m_edges[i][0] || m_tris[triangle_index][1] == m_edges[i][1] )
            {
                ++num_common_verts;
            }
            if ( m_tris[triangle_index][2] == m_edges[i][0] || m_tris[triangle_index][2] == m_edges[i][1] )
            {
                ++num_common_verts;
            }
            assert( num_common_verts == 2 );
        }
    }
    
    // m_triangle_to_edge_map
    
    for ( size_t i = 0; i < m_triangle_to_edge_map.size(); ++i )
    {
        if ( triangle_is_deleted(i) ) { continue; }
        
        const Vec3st& inc_edges = m_triangle_to_edge_map[i];
        
        const Vec2st& edge0 = m_edges[inc_edges[0]];
        const Vec2st& edge1 = m_edges[inc_edges[1]];
        const Vec2st& edge2 = m_edges[inc_edges[2]];
        
        assert( !edge_is_deleted( inc_edges[0] ) );
        assert( !edge_is_deleted( inc_edges[1] ) );
        assert( !edge_is_deleted( inc_edges[2] ) );
        
        assert( edge0[0] != edge0[1] );
        assert( edge1[0] != edge1[1] );
        assert( edge2[0] != edge2[1] );
        
        assert( edge0[0] == m_tris[i][0] || edge0[0] == m_tris[i][1] || edge0[0] == m_tris[i][2] );
        assert( edge0[1] == m_tris[i][0] || edge0[1] == m_tris[i][1] || edge0[1] == m_tris[i][2] );
        assert( edge1[0] == m_tris[i][0] || edge1[0] == m_tris[i][1] || edge1[0] == m_tris[i][2] );
        assert( edge1[1] == m_tris[i][0] || edge1[1] == m_tris[i][1] || edge1[1] == m_tris[i][2] );
        assert( edge2[0] == m_tris[i][0] || edge2[0] == m_tris[i][1] || edge2[0] == m_tris[i][2] );
        assert( edge2[1] == m_tris[i][0] || edge2[1] == m_tris[i][1] || edge2[1] == m_tris[i][2] );
    }
    
}
    
////////////////////////////////////////////////////////////
// FD 20121126
//
// Check the consistency of the triangle labels
//
void NonDestructiveTriMesh::test_labels() const
{
    // todo
    
}

////////////////////////////////////////////////////////////


}



