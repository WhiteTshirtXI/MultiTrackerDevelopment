#include "BASim/src/Collisions/ElTopo/broadphasegrid.hh"
#include "BASim/src/Physics/DeformableObjects/Shells/ElasticShell.hh"
#include "BASim/src/Physics/DeformableObjects/Shells/ElasticShellForce.hh"
#include "BASim/src/Physics/DeformableObjects/DeformableObject.hh"
#include "BASim/src/Core/TopologicalObject/TopObjUtil.hh"
#include "BASim/src/Collisions/ElTopo/ccd_wrapper.hh"
#include "BASim/src/Physics/DeformableObjects/Shells/CSTMembraneForce.hh"
#include <algorithm>

namespace BASim {

ElasticShell::ElasticShell(DeformableObject* object, const FaceProperty<char>& shellFaces) : 
  PhysicalModel(*object), m_obj(object), 
    m_active_faces(shellFaces), 
    m_undeformed_positions(object), 
    m_undef_xi(object),
    m_damping_undeformed_positions(object), 
    m_damping_undef_xi(object),
    m_vertex_masses(object),
    m_edge_masses(object),
    m_thicknesses(object),
    m_volumes(object),
    m_positions(object), 
    m_xi(object), 
    m_velocities(object),
    m_xi_vel(object),
    m_density(1),
    m_proximity_epsilon(0.01)
{
 
}

ElasticShell::~ElasticShell() {

}

void ElasticShell::computeForces( VecXd& force )
{
  const std::vector<ElasticShellForce*>& forces = getForces();
  std::vector<ElasticShellForce*>::const_iterator fIt;
  
  VecXd curr_force(force.size());
  for (fIt = forces.begin(); fIt != forces.end(); ++fIt) {
    curr_force.setZero();
    (*fIt)->globalForce(curr_force);
    
    force += curr_force;
  }

}

void ElasticShell::computeJacobian( Scalar scale, MatrixBase& J )
{
  const std::vector<ElasticShellForce*>& forces = getForces();
  std::vector<ElasticShellForce*>::const_iterator fIt;

  for (fIt = forces.begin(); fIt != forces.end(); ++fIt)
    (*fIt)->globalJacobian(scale, J);
}

const std::vector<ElasticShellForce*>& ElasticShell::getForces() const
{
  return m_shell_forces;
}

void ElasticShell::addForce( ElasticShellForce* force )
{
  assert(force != NULL);

  m_shell_forces.push_back(force);
}

void ElasticShell::setThickness( Scalar thickness )
{
  for(FaceIterator fit = m_obj->faces_begin(); fit != m_obj->faces_end(); ++fit) {
    m_thicknesses[*fit] = thickness;
    Scalar area = getArea(*fit, false);
    m_volumes[*fit] = thickness * area;
  }

}

void ElasticShell::setDensity(Scalar density) {
  m_density = density;
}

void ElasticShell::setVertexUndeformed( const VertexProperty<Vec3d>& undef )
{
  m_undeformed_positions = undef;
}

void ElasticShell::setEdgeUndeformed( const EdgeProperty<Scalar>& undef )
{
  m_undef_xi = undef;
}

void ElasticShell::setVertexPositions( const VertexProperty<Vec3d>& positions )
{
  m_positions = positions;
}

void ElasticShell::setEdgeXis( const EdgeProperty<Scalar>& positions )
{
  m_xi = positions;
}

void ElasticShell::setVertexVelocities( const VertexProperty<Vec3d>& velocities) 
{
  m_velocities = velocities;
}

void ElasticShell::setEdgeVelocities(const EdgeProperty<Scalar>& velocities)
{
  m_xi_vel = velocities;
}


Scalar ElasticShell::getArea(const FaceHandle& f, bool current) const  {
  FaceVertexIterator fvit = m_obj->fv_iter(f);
  VertexHandle v0_hnd = *fvit; ++fvit; assert(fvit);
  VertexHandle v1_hnd = *fvit; ++fvit; assert(fvit);
  VertexHandle v2_hnd = *fvit; ++fvit; assert(!fvit);

  //compute triangle areas
  if(current) {
    Vec3d v0 = m_positions[v1_hnd] - m_positions[v0_hnd];
    Vec3d v1 = m_positions[v2_hnd] - m_positions[v0_hnd];
    Vec3d triVec = v0.cross(v1);
    return 0.5*triVec.norm();
  }
  else {
    Vec3d v0 = m_undeformed_positions[v1_hnd] - m_undeformed_positions[v0_hnd];
    Vec3d v1 = m_undeformed_positions[v2_hnd] - m_undeformed_positions[v0_hnd];
    Vec3d triVec = v0.cross(v1);
    return 0.5*triVec.norm();
  }
}

void ElasticShell::computeMasses()
{
  //Compute vertex masses in a lumped mass way.

  m_vertex_masses.assign(0);
  m_edge_masses.assign(0);

  Scalar area = 0;

  //Iterate over all triangles active in this shell and accumulate vertex masses
  for(FaceIterator f_iter = m_obj->faces_begin(); f_iter != m_obj->faces_end(); ++f_iter) {
    FaceHandle& f_hnd = *f_iter;
    if(m_active_faces[f_hnd]) {

      //get the three vertices
      FaceVertexIterator fvit = m_obj->fv_iter(f_hnd);
      VertexHandle v0_hnd = *fvit; ++fvit; assert(fvit);
      VertexHandle v1_hnd = *fvit; ++fvit; assert(fvit);
      VertexHandle v2_hnd = *fvit; ++fvit; assert(!fvit);

      //compute triangle areas
      Vec3d v0 = m_positions[v1_hnd] - m_positions[v0_hnd];
      Vec3d v1 = m_positions[v2_hnd] - m_positions[v0_hnd];
      Vec3d triVec = v0.cross(v1);
      Scalar area = 0.5*sqrt(triVec.dot(triVec)) / 3.0;
      Scalar contribution = m_thicknesses[f_hnd] * m_density * area;
      
      //accumulate mass to the vertices
      m_vertex_masses[v0_hnd] += contribution;
      m_vertex_masses[v1_hnd] += contribution;
      m_vertex_masses[v2_hnd] += contribution;

      //also accumulate mass to the edges (this mass computation is probably not consistent with what we want)
      FaceEdgeIterator feit = m_obj->fe_iter(f_hnd);
      EdgeHandle e0_hnd = *feit; ++feit; assert(feit);
      EdgeHandle e1_hnd = *feit; ++feit; assert(feit);
      EdgeHandle e2_hnd = *feit; ++feit; assert(feit);

      m_edge_masses[e0_hnd] += contribution;
      m_edge_masses[e1_hnd] += contribution;
      m_edge_masses[e2_hnd] += contribution;

      //store the current volumes
      m_volumes[f_hnd] = 3*area*m_thicknesses[f_hnd];
    }
  }
 
}

bool ElasticShell::isVertexActive( const VertexHandle& v ) const
{
  //determine if the vertex is on any active face
  VertexFaceIterator vf = m_obj->vf_iter(v);
  for(;vf; ++vf) {
    if(isFaceActive(*vf)) {
      return true;
    }
  }
  
  return false;
}

bool ElasticShell::isEdgeActive( const EdgeHandle& e) const {
  //if any adjacent face is active, we say this edge is active.
  EdgeFaceIterator ef = m_obj->ef_iter(e);
  for(;ef;++ef) {
    if(isFaceActive(*ef)) {
      return true;
    }
  }
  
  return false;
}

const Scalar& ElasticShell::getDof( const DofHandle& hnd ) const
{
  //they're all vertex Dofs for a shell
  assert(hnd.getType() == DofHandle::VERTEX_DOF || hnd.getType == DofHandle::EDGE_DOF);

  //return reference to the appropriate position in the vector
  if(hnd.getType() == DofHandle::VERTEX_DOF) {
    const VertexHandle& vh = static_cast<const VertexHandle&>(hnd.getHandle());
    return const_cast<Vec3d&>(m_positions[vh])[hnd.getNum()];
  }
  else {
    const EdgeHandle& eh = static_cast<const EdgeHandle&>(hnd.getHandle());
    return const_cast<Scalar&>(m_xi[eh]);
  }
}

void ElasticShell::setDof( const DofHandle& hnd, const Scalar& dof )
{
  //they're all vertex Dofs for a shell
  assert(hnd.getType() == DofHandle::VERTEX_DOF || hnd.getType == DofHandle::EDGE_DOF);

  if(hnd.getType() == DofHandle::VERTEX_DOF) {
    const VertexHandle& vh = static_cast<const VertexHandle&>(hnd.getHandle());
    m_positions[vh][hnd.getNum()] = dof;
  }
  else {
    const EdgeHandle& eh = static_cast<const EdgeHandle&>(hnd.getHandle());
    m_xi[eh] = dof;
  }
}

const Scalar& ElasticShell::getVel( const DofHandle& hnd ) const
{
  assert(hnd.getType() == DofHandle::VERTEX_DOF || hnd.getType == DofHandle::EDGE_DOF);

  //return reference to the appropriate position in the vector
  if(hnd.getType() == DofHandle::VERTEX_DOF) {
    const VertexHandle& vh = static_cast<const VertexHandle&>(hnd.getHandle());
    return const_cast<Vec3d&>(m_velocities[vh])[hnd.getNum()];
  }
  else{
    const EdgeHandle& eh = static_cast<const EdgeHandle&>(hnd.getHandle());
    return const_cast<Scalar&>(m_xi_vel[eh]);
  }
}

void ElasticShell::setVel( const DofHandle& hnd, const Scalar& vel )
{
  assert(hnd.getType() == DofHandle::VERTEX_DOF || hnd.getType == DofHandle::EDGE_DOF);

  if(hnd.getType() == DofHandle::VERTEX_DOF) {
    const VertexHandle& vh = static_cast<const VertexHandle&>(hnd.getHandle());
    m_velocities[vh][hnd.getNum()] = vel;
  }
  else{
    const EdgeHandle& eh = static_cast<const EdgeHandle&>(hnd.getHandle());
    m_xi_vel[eh] = vel;
  }
}

const Scalar& ElasticShell::getMass( const DofHandle& hnd ) const
{
  assert(hnd.getType() == DofHandle::VERTEX_DOF || hnd.getType == DofHandle::EDGE_DOF);

  if(hnd.getType() == DofHandle::VERTEX_DOF) {
    const VertexHandle& vh = static_cast<const VertexHandle&>(hnd.getHandle());
    return m_vertex_masses[vh];
  }
  else {
    const EdgeHandle& eh = static_cast<const EdgeHandle&>(hnd.getHandle());
    return m_edge_masses[eh];
  }
}

void ElasticShell::getScriptedDofs( IntArray& dofIndices, std::vector<Scalar>& dofValues, Scalar time ) const
{
  for(unsigned int i = 0; i < m_constrained_vertices.size(); ++i) {
    int dofBase = getVertexDofBase(m_constrained_vertices[i]);
    Vec3d pos = m_constraint_positions[i]->operator()(time);
    dofIndices.push_back(dofBase); dofValues.push_back(pos[0]);
    dofIndices.push_back(dofBase+1); dofValues.push_back(pos[1]);
    dofIndices.push_back(dofBase+2); dofValues.push_back(pos[2]);
  }
}

void ElasticShell::constrainVertex( const VertexHandle& v, const Vec3d& pos )
{
  m_constrained_vertices.push_back(v);
  PositionConstraint* c = new FixedPositionConstraint(pos);
  m_constraint_positions.push_back(c);
}

void ElasticShell::constrainVertex( const VertexHandle& v, PositionConstraint* c )
{
  m_constrained_vertices.push_back(v);
  m_constraint_positions.push_back(c);
}



void ElasticShell::startStep()
{

  /* //Debugging forces
   const std::vector<ElasticShellForce*>& forces = getForces();
   std::vector<ElasticShellForce*>::const_iterator fIt;

   
   for (fIt = forces.begin(); fIt != forces.end(); ++fIt) {
      if(typeid(*(*fIt)) == typeid(CSTMembraneForce)) {
         Scalar potEnergy = (*fIt)->globalEnergy();
         //std::cout << "Energy " << potEnergy;
         VecXd curr_force(m_obj->nv()*3);
         curr_force.setZero();
         (*fIt)->setDebug(true);
         (*fIt)->globalForce(curr_force);
         (*fIt)->setDebug(false);

         ////Now sum forces on all the vertices along the circumference
         Vec3d forceSum;
         forceSum.setZero();
         for(VertexIterator vit = m_obj->vertices_begin(); vit != m_obj->vertices_end(); ++vit) {
            Vec3d vertPos = getVertexPosition(*vit);
            if(vertPos[2] < -1e-5 || vertPos[2] > 1e-5) continue; //skip non-circumferential vertices

            int startIndex = getVertexDofBase(*vit);
            Vec3d vertForce(curr_force[startIndex], curr_force[startIndex+1], curr_force[startIndex+2]);
            forceSum += vertForce;
         }
         std::cout << "Force sum is: " << forceSum << std::endl;
      }
   }
   */

  //update the damping "reference configuration" for computing viscous forces.
  m_damping_undeformed_positions = m_positions;
  m_damping_undef_xi = m_xi;

  //tell the forces to update anything they need to update
  const std::vector<ElasticShellForce*>& forces = getForces();
  for(int i = 0; i < forces.size(); ++i) {
    forces[i]->update();
  }

}

void ElasticShell::endStep() {
  
  //Adjust thicknesses based on area changes
  updateThickness();

  //Remeshing
  if(m_do_remeshing) {
    for(int i = 0; i < m_remeshing_iters; ++i)
      remesh(m_remesh_edge_length);  

    //Relabel DOFs if necessary
    m_obj->computeDofIndexing();
  }

  //extendMesh();
  
  //Update masses based on new areas/thicknesses
  computeMasses();
  
  


  /*
  Scalar position = 0;
  Scalar velocity = 0;
  Scalar thickness = 0;
  int count = 0;
  for(VertexIterator vit = m_obj->vertices_begin(); vit != m_obj->vertices_end(); ++vit) {
    Vec3d curPos = getVertexPosition(*(vit));
    if(curPos[2] > -1e-5 && curPos[2] < 1e-5) {
      position += getVertexPosition(*(vit)).norm();
      velocity += getVertexVelocity(*(vit)).norm();
      ++count;
    }
  }
  int count2 = 0;
  for(FaceIterator fit = m_obj->faces_begin(); fit != m_obj->faces_end(); ++fit) {
    bool anyFound = false;
    for(FaceVertexIterator fvit = m_obj->fv_iter(*fit); fvit; ++fvit) {
      VertexHandle vh = *fvit;
      Vec3d curPos = getVertexPosition(vh);
      if(curPos[2] > -1e-5 && curPos[2] < 1e-5)
        anyFound = true;
    }
    if(anyFound) {
      thickness += m_thicknesses[*fit];
      ++count2;
    }
  }
  std::cout << "\n\nAverage radius: " << position/(Scalar)count << "\nAverage velocity: " << velocity/(Scalar)count << "\nAverage thickness: " << thickness/(Scalar)count2 << "\n\n" << std::endl;    
  */

  //What if we update rest config first?
  //m_damping_undeformed_positions = m_positions;

 
  //Advance any constraints!
  
 
}

void ElasticShell::remesh( Scalar desiredEdge )
{

  m_broad_phase.update_broad_phase_static(*m_obj, m_positions, m_proximity_epsilon);
  
  //Parameters adapted from Jiao et al. "Anisotropic Mesh Adaptation for Evolving Triangulated Surfaces"
  Scalar ratio_R = 0.45;
  Scalar ratio_r = 0.25;
  
  Scalar sEdge = 0.25*desiredEdge;
  Scalar minEdge = ratio_R*desiredEdge;
  Scalar maxEdge = 1.5*desiredEdge;

  Scalar minAngle = 15.0*M_PI/180.0;
  Scalar maxAngle = 145.0*M_PI/180.0;
  
  int counter = 0;
  for(int i = 0; i < 5; ++i) { //do a few passes over the data for good measure
  
    splitEdges(desiredEdge, maxEdge, maxAngle);
    
    flipEdges();
    
    collapseEdges(minAngle, desiredEdge, ratio_R, ratio_r, minEdge);

  }
  
  
}


bool ElasticShell::edgeFlipCausesCollision( const EdgeHandle& edge_index, 
                                          const VertexHandle& new_end_a,
                                          const VertexHandle& new_end_b)
{  
  //printf("Checking edge flip safety\n");
  
  VertexHandle old_end_a = m_obj->fromVertex(edge_index);
  VertexHandle old_end_b = m_obj->toVertex(edge_index);
  VertexHandle tet_vertex_indices[4] = { old_end_a, old_end_b, new_end_a, new_end_b };

  const ElTopo::Vec3d tet_vertex_positions[4] = { ElTopo::toElTopo(m_positions[ tet_vertex_indices[0] ]), 
    ElTopo::toElTopo(m_positions[ tet_vertex_indices[1] ]), 
    ElTopo::toElTopo(m_positions[ tet_vertex_indices[2] ]), 
    ElTopo::toElTopo(m_positions[ tet_vertex_indices[3] ]) };

  ElTopo::Vec3d low, high;
  ElTopo::minmax( tet_vertex_positions[0], tet_vertex_positions[1], tet_vertex_positions[2], tet_vertex_positions[3], low, high );

  std::vector<unsigned int> overlapping_vertices;
  m_broad_phase.get_potential_vertex_collisions( low, high, overlapping_vertices );

  // do point-in-tet tests (any points already in the tet formed by the flipping edge)
  for ( unsigned int i = 0; i < overlapping_vertices.size(); ++i ) 
  { 
    if ( (overlapping_vertices[i] == old_end_a.idx()) || (overlapping_vertices[i] == old_end_b.idx()) || 
      (overlapping_vertices[i] == new_end_a.idx()) || (overlapping_vertices[i] == new_end_b.idx()) ) 
    {
      continue;
    }

    if ( ElTopo::point_tetrahedron_intersection( ElTopo::toElTopo(m_positions[VertexHandle(overlapping_vertices[i])]), overlapping_vertices[i],
      tet_vertex_positions[0], tet_vertex_indices[0].idx(),
      tet_vertex_positions[1], tet_vertex_indices[1].idx(),
      tet_vertex_positions[2], tet_vertex_indices[2].idx(),
      tet_vertex_positions[3], tet_vertex_indices[3].idx() ) ) 
    {
      return true;
    }
  }

  //
  // Check new triangle A vs existing edges
  //
  const ElTopo::Vec3d old_a_pos = ElTopo::toElTopo(m_positions[old_end_a]), 
    old_b_pos = ElTopo::toElTopo(m_positions[old_end_b]), 
    new_a_pos = ElTopo::toElTopo(m_positions[new_end_a]), 
    new_b_pos = ElTopo::toElTopo(m_positions[new_end_b]);

  ElTopo::minmax( old_a_pos, new_a_pos, new_b_pos, low, high );
  std::vector<unsigned int> overlapping_edges;
  m_broad_phase.get_potential_edge_collisions( low, high, overlapping_edges );

  for ( unsigned int i = 0; i < overlapping_edges.size(); ++i )
  {
    EdgeHandle overlapping_edge_index(overlapping_edges[i]);
    VertexHandle edge_0 = m_obj->fromVertex(overlapping_edge_index), 
                 edge_1 = m_obj->toVertex(overlapping_edge_index);

    if( segment_triangle_intersection( 
          ElTopo::toElTopo(m_positions[edge_0]), edge_0.idx(), 
          ElTopo::toElTopo(m_positions[edge_1]), edge_1.idx(),
          old_a_pos, old_end_a.idx(), 
          new_a_pos, new_end_a.idx(), 
          new_b_pos, new_end_b.idx(),
          true, false ) ) 
    {
        return true;
    }
  }

  //
  // Check new triangle B vs existing edges
  //

  ElTopo::minmax(old_b_pos, new_a_pos, new_b_pos, low, high );

  overlapping_edges.clear();
  m_broad_phase.get_potential_edge_collisions( low, high, overlapping_edges );

  for ( unsigned int i = 0; i < overlapping_edges.size(); ++i )
  {
    EdgeHandle overlapping_edge_index(overlapping_edges[i]);
    VertexHandle edge_0 = m_obj->fromVertex(overlapping_edge_index), 
                 edge_1 = m_obj->toVertex(overlapping_edge_index);

    if( segment_triangle_intersection( 
      ElTopo::toElTopo(m_positions[edge_0]), edge_0.idx(), 
      ElTopo::toElTopo(m_positions[edge_1]), edge_1.idx(),
      old_b_pos, old_end_b.idx(), 
      new_a_pos, new_end_a.idx(), 
      new_b_pos, new_end_b.idx(),
      true, false ) ) 
    {
      return true;
    }
  }

  //
  // Check new edge vs existing triangles
  //   

  minmax( new_a_pos, new_b_pos, low, high );
  std::vector<unsigned int> overlapping_triangles;
  m_broad_phase.get_potential_triangle_collisions( low, high, overlapping_triangles );

  for ( unsigned int i = 0; i <  overlapping_triangles.size(); ++i )
  {
    FaceHandle face(overlapping_triangles[i]);
    FaceVertexIterator fvit = m_obj->fv_iter(face);
    std::vector< std::pair<ElTopo::Vec3d,int> > face_verts;
    for(;fvit; ++fvit) {
      ElTopo::Vec3d vert = ElTopo::toElTopo(m_positions[*fvit]);
      int id = (*fvit).idx();
      face_verts.push_back(std::make_pair(vert, id));
    }
    assert (face_verts.size() == 3);

    if( segment_triangle_intersection( 
      new_a_pos, new_end_a.idx(), 
      new_b_pos, new_end_b.idx(),
      face_verts[0].first, face_verts[0].second,
      face_verts[1].first, face_verts[1].second,
      face_verts[2].first, face_verts[2].second,
      true, false ) ) 
    {
      return true;
    }
  }

  return false;

}

void ElasticShell::flipEdges() {

  EdgeIterator e_it = m_obj->edges_begin();
  for(;e_it != m_obj->edges_end(); ++e_it) {
    EdgeHandle eh = *e_it;

    VertexHandle v0 = m_obj->fromVertex(eh);
    VertexHandle v1 = m_obj->toVertex(eh);
    assert(v0!=v1);
    Vec3d p0 = getVertexPosition(v0);
    Vec3d p1 = getVertexPosition(v1);
    VertexHandle v2, v3;
    Scalar edgeLength = (p1-p0).norm();
    bool success = getEdgeOppositeVertices(*m_obj, eh, v2, v3);
    if(!success) continue;


    Vec3d p2 = getVertexPosition(v2);
    Vec3d p3 = getVertexPosition(v3);
    Scalar oppEdgeLength = (p3-p2).norm();

    Vec3d dir0 = p0-p2, dir1 = p1 - p2;
    Scalar angle0 = acos(dir0.dot(dir1));

    dir0 = p0-p3; dir1 = p1 - p3;
    Scalar angle1 = acos(dir0.dot(dir1));
    if(angle0 + angle1 > 1.00001*M_PI) {

      //determine volume of the region being flipped
      FaceHandle f0, f1;
      getEdgeFacePair(*m_obj, eh, f0, f1);
      Scalar totalVolume = m_volumes[f0] + m_volumes[f1];

      //check for collision safety
      //if(edgeFlipCausesCollision(eh, v2, v3))
      //  continue;

      EdgeHandle newEdge = flipEdge(*m_obj, eh);
      if(!newEdge.isValid()) //couldn't flip the edge, such an edge already existed
        continue;
      
      //update the collision data
      //remove the old edge/tris
      m_broad_phase.remove_edge(eh.idx());
      m_broad_phase.remove_triangle(f0.idx());
      m_broad_phase.remove_triangle(f1.idx());
      
      FaceHandle f0new, f1new;
      getEdgeFacePair(*m_obj, newEdge, f0new, f1new);
      setFaceActive(f0new);
      setFaceActive(f1new);

      //update the collision data
      //add the new edge/tris
      m_broad_phase.add_edge(newEdge.idx(), ElTopo::toElTopo(p2), ElTopo::toElTopo(p3), m_proximity_epsilon);
      //(vertex ordering shouldn't matter here)
      m_broad_phase.add_triangle(f0new.idx(), ElTopo::toElTopo(p2), ElTopo::toElTopo(p3), ElTopo::toElTopo(p0), m_proximity_epsilon);
      m_broad_phase.add_triangle(f1new.idx(), ElTopo::toElTopo(p2), ElTopo::toElTopo(p3), ElTopo::toElTopo(p1), m_proximity_epsilon);

      //assign new thicknesses
      Scalar f0newArea = getArea(f0new, true);
      Scalar f1newArea = getArea(f1new, true);
      Scalar newThickness = totalVolume / (f0newArea + f1newArea);
      m_thicknesses[f0new] = newThickness;
      m_thicknesses[f1new] = newThickness;
      m_volumes[f0new] = f0newArea*newThickness;
      m_volumes[f1new] = f1newArea*newThickness;

    }
  }

}

struct SortableEdge
{
  EdgeHandle edge_hnd;
  double edge_length;

  SortableEdge( EdgeHandle eh, double el ) : edge_hnd(eh), edge_length(el) {}

  bool operator<( const SortableEdge& other ) const
  {
    return (this->edge_length < other.edge_length);
  }
};

bool ElasticShell::splitEdges( double desiredEdge, double maxEdge, double maxAngle) {

  //sort edges in order of length, so we split long ones first
  std::vector<SortableEdge> sortable_edges_to_try;
  
  EdgeIterator e_it = m_obj->edges_begin();
  for(;e_it != m_obj->edges_end(); ++e_it) {   
    
    EdgeHandle eh = *e_it;
    
    VertexHandle vertex_a = m_obj->fromVertex(eh);
    VertexHandle vertex_b = m_obj->toVertex(eh);

    if ( !m_obj->edgeExists(eh) || !isEdgeActive(eh))   { continue; }     // skip inactive/non-existent edges
    
    //skip edges that are flowing into the domain
    bool isConstrained = false;
    for(unsigned int i = 0; i < m_inflow_boundaries.size(); ++i) {
      if(std::find(m_inflow_boundaries[i].begin(), m_inflow_boundaries[i].end(),eh) != m_inflow_boundaries[i].end()) {
        isConstrained = true;
        break;
      }
    }

    //don't split constrained edges. (alternatively, we could split them, and add the new vert to the constrained list.)
    bool aConstrained = false, bConstrained = false;
    for(int i = 0; i < m_constrained_vertices.size(); ++i) {
      if(m_constrained_vertices[i] == vertex_a)
        aConstrained = true;
      if(m_constrained_vertices[i] == vertex_b)
        bConstrained = true;
    }

    if(aConstrained && bConstrained || isConstrained) continue;

    //if ( m_mesh.m_edgetri[i].size() < 2 ) { continue; }                     // skip boundary edges - Why should we?
    //if ( m_masses[ m_mesh.m_edges[i][0] ] > 100.0 && m_masses[ m_mesh.m_edges[i][1] ] > 100.0 )     { continue; }    // skip solids

    
    assert( m_obj->vertexExists(vertex_a) );
    assert( m_obj->vertexExists(vertex_b) );

    double current_length = (m_positions[ vertex_a ] - m_positions[ vertex_b ]).norm();

    if ( current_length > maxEdge ) {
      sortable_edges_to_try.push_back( SortableEdge( eh, current_length ) );
    }
  }

  
  //sort into ascending order
  std::sort( sortable_edges_to_try.begin(), sortable_edges_to_try.end() );
  bool anySplits = false;

  //iterate from biggest to smallest (end to start)
  for(int i = sortable_edges_to_try.size()-1; i >= 0; --i) {

    EdgeHandle eh = sortable_edges_to_try[i].edge_hnd;
    
    if ( !m_obj->edgeExists(eh) || !isEdgeActive(eh))   { continue; }
    if(isSplitDesired(eh, maxEdge, desiredEdge, maxAngle)) {
      bool splitSuccess = performSplit(eh);
      anySplits |= splitSuccess;
    }
  }
  
  return anySplits;
}

bool ElasticShell::isSplitDesired(const EdgeHandle& eh, double maxEdge, double desiredEdge, double maxAngle) {

  Scalar sEdge = 0.25*desiredEdge;

  VertexHandle v0 = m_obj->fromVertex(eh);
  VertexHandle v1 = m_obj->toVertex(eh);
  assert(v0!=v1);
  Vec3d p0 = getVertexPosition(v0);
  Vec3d p1 = getVertexPosition(v1);
  Scalar edgeLength = (p1-p0).norm();
  bool doSplit = false;

  if(edgeLength > maxEdge) {
    //"Absolute longness"

    //only do the split if the adjacent faces have no edges longer than this one.
    bool edgeLongest = true;
    for(EdgeFaceIterator ef_it = m_obj->ef_iter(eh); ef_it && edgeLongest; ++ef_it) {
      for(FaceEdgeIterator fe_it = m_obj->fe_iter(*ef_it); fe_it; ++fe_it) {
        EdgeHandle otherEdge = *fe_it;
        if(otherEdge == eh) continue;

        Scalar otherLen = (getVertexPosition(m_obj->fromVertex(otherEdge)) - getVertexPosition(m_obj->toVertex(otherEdge))).norm();
        if(otherLen > edgeLength) {
          edgeLongest = false;
          break;
        }
      }
    }
    if(!edgeLongest) return false;

    doSplit = true;
  }

  
  if(!doSplit && edgeLength > desiredEdge) {

    //Additional criterion from Jiao et al. - Brochu uses just the above

    //"Relative longness"
    VertexHandle opp0, opp1;
    getEdgeOppositeVertices(*m_obj, eh, opp0, opp1);
    bool bothAnglesSmall = true;
    if(opp0.isValid()) {
      Vec3d p2 = getVertexPosition(opp0);
      Vec3d dir0 = p0-p2, dir1 = p1 - p2;
      Scalar angle0 = acos(dir0.dot(dir1));
      if(angle0 > maxAngle)
        bothAnglesSmall = false;
    }
    if(opp1.isValid()) {
      Vec3d p3 = getVertexPosition(opp1);
      Vec3d dir0 = p0-p3, dir1 = p1 - p3;
      Scalar angle1 = acos(dir0.dot(dir1));
      if(angle1 > maxAngle)
        bothAnglesSmall = false;
    }

    if(!bothAnglesSmall) {
      //check if shortest edge is above a threshold
      Scalar shortestEdge = 100*maxEdge;
      for(EdgeFaceIterator ef_it = m_obj->ef_iter(eh); ef_it; ++ef_it) {
        for(FaceEdgeIterator fe_it = m_obj->fe_iter(*ef_it); fe_it; ++fe_it) {
          EdgeHandle otherEdge = *fe_it;

          if(!otherEdge.isValid())continue;

          Scalar otherLen = (getVertexPosition(m_obj->fromVertex(otherEdge)) - getVertexPosition(m_obj->toVertex(otherEdge))).norm();
          if(otherLen < shortestEdge) {
            shortestEdge = otherLen;
          }
        }
      }
      if(shortestEdge > sEdge) {
        doSplit = true;
      }
    }
  }
  

  return doSplit;
}

bool ElasticShell::performSplit(const EdgeHandle& eh) {
  
  
  //Check for self-intersections being induced...

  VertexHandle v0 = m_obj->fromVertex(eh);
  VertexHandle v1 = m_obj->toVertex(eh);
  assert(v0!=v1);
  Vec3d p0 = getVertexPosition(v0);
  Vec3d p1 = getVertexPosition(v1);
  
  Vec3d midpoint = 0.5f*(p0+p1);

  std::vector<FaceHandle> oldFaces;
  std::vector<Scalar> oldThicknesses;
  for(EdgeFaceIterator efit = m_obj->ef_iter(eh); efit; ++efit) {
    FaceHandle f = *efit;
    oldFaces.push_back(f);
    oldThicknesses.push_back(getThickness(f));
  }
  
  VertexHandle v2, v3;
  getEdgeOppositeVertices(*m_obj, eh, v2, v3);

  ElTopo::Vec3d midpoint_ET = ElTopo::toElTopo(midpoint);
  //if(edgeSplitCausesCollision(midpoint_ET, midpoint_ET, eh))
  //  return false;

  //remove the edge and surrounding faces from the collision structure
  m_broad_phase.remove_edge(eh.idx());
  for(unsigned int i = 0; i < oldFaces.size(); ++i)
    m_broad_phase.remove_triangle(oldFaces[i].idx());
  
  //perform the actual split
  std::vector<FaceHandle> newFaces;
  VertexHandle v_new = splitEdge(*m_obj, eh, newFaces);

  Vec3d velocity = 0.5f*(getVertexVelocity(v0) + getVertexVelocity(v1));
  Vec3d undef = 0.5f*(getVertexUndeformed(v0) + getVertexUndeformed(v1));
  setVertexVelocity(v_new, velocity);
  setVertexPosition(v_new, midpoint);
  setUndeformedVertexPosition(v_new, undef);

  //set consistent volumes and thickness for new faces
  assert(oldFaces.size() == newFaces.size()/2);
  Scalar newVolume = 0;
  for(unsigned int i = 0; i < oldFaces.size(); ++i) {
    m_thicknesses[newFaces[i*2]] = oldThicknesses[i];
    m_thicknesses[newFaces[i*2+1]] = oldThicknesses[i];
    m_volumes[newFaces[i*2]] = oldThicknesses[i] * getArea(newFaces[i*2], true);
    m_volumes[newFaces[i*2+1]] = oldThicknesses[i] * getArea(newFaces[i*2+1], true);
  }

  VertexFaceIterator vf_iter = m_obj->vf_iter(v_new);
  for(;vf_iter; ++vf_iter)
    setFaceActive(*vf_iter);

  //update collision data structures

  //add vertices
  m_broad_phase.add_vertex(v_new.idx(), ElTopo::toElTopo(midpoint), m_proximity_epsilon);
  
  //add edges
  VertexEdgeIterator ve_iter = m_obj->ve_iter(v_new);
  for(; ve_iter; ++ve_iter) {
    std::vector<ElTopo::Vec3d> edge_verts; 
    EdgeVertexIterator ev_iter = m_obj->ev_iter(*ve_iter);
    for(; ev_iter; ++ev_iter)
      edge_verts.push_back(ElTopo::toElTopo(getVertexPosition(*ev_iter)));
    m_broad_phase.add_edge((*ve_iter).idx(), edge_verts[0], edge_verts[1], m_proximity_epsilon);
  }

  //add tris
  vf_iter = m_obj->vf_iter(v_new);
  for(;vf_iter; ++vf_iter) {
    std::vector<ElTopo::Vec3d> tri_verts; 
    FaceVertexIterator fv_iter = m_obj->fv_iter(*vf_iter);
    for(; fv_iter; ++fv_iter)
      tri_verts.push_back(ElTopo::toElTopo(getVertexPosition(*fv_iter)));
    m_broad_phase.add_triangle((*vf_iter).idx(), tri_verts[0], tri_verts[1], tri_verts[2], m_proximity_epsilon);
  }
  
  return true;
}


bool ElasticShell::edgeSplitCausesCollision( const ElTopo::Vec3d& new_vertex_position, 
                                              const ElTopo::Vec3d& new_vertex_smooth_position, 
                                              EdgeHandle edge)
{
  
  std::vector<FaceHandle> tris;
  EdgeFaceIterator efit = m_obj->ef_iter(edge);
  for(;efit; ++efit) tris.push_back(*efit);

  // new point vs all triangles
  {

    ElTopo::Vec3d aabb_low, aabb_high;
    ElTopo::minmax( new_vertex_position, new_vertex_smooth_position, aabb_low, aabb_high );

    aabb_low -= m_proximity_epsilon * ElTopo::Vec3d(1,1,1);
    aabb_high += m_proximity_epsilon * ElTopo::Vec3d(1,1,1);

    std::vector<unsigned int> overlapping_triangles;
    m_broad_phase.get_potential_triangle_collisions( aabb_low, aabb_high, overlapping_triangles );

    for ( unsigned int i = 0; i < overlapping_triangles.size(); ++i )
    {
      
      if(std::find(tris.begin(), tris.end(), FaceHandle(overlapping_triangles[i])) != tris.end())
      {
        continue;
      }
      
      FaceHandle fh(overlapping_triangles[i]);

      //extract vertices
      FaceVertexIterator fvit = m_obj->fv_iter(fh);
      VertexHandle triangle_vertex_0 = *fvit; ++fvit;
      VertexHandle triangle_vertex_1 = *fvit; ++fvit;
      VertexHandle triangle_vertex_2 = *fvit; ++fvit; 
      assert(!fvit);

      double t_zero_distance;
      unsigned int dummy_index = -1;
      ElTopo::point_triangle_distance( new_vertex_position, dummy_index, 
        ElTopo::toElTopo(m_positions[ triangle_vertex_0 ]), triangle_vertex_0.idx(),
        ElTopo::toElTopo(m_positions[ triangle_vertex_1 ]), triangle_vertex_1.idx(),
        ElTopo::toElTopo(m_positions[ triangle_vertex_2 ]), triangle_vertex_2.idx(),
        t_zero_distance );

      if ( t_zero_distance < m_improve_collision_epsilon )
      {
        return true;
      }
     
      //Not sorting the indices here... 
      //Apparently that only matters for the exact CCD stuff Robert put together (since it relies on simulation of simplicity(SOS)).
      //I'm using standard cubic CCD.
      ElTopo::Vec3d s0 = ElTopo::toElTopo(m_positions[ triangle_vertex_0 ]), 
        s1 = ElTopo::toElTopo(m_positions[ triangle_vertex_1 ]), 
        s2 = ElTopo::toElTopo(m_positions[ triangle_vertex_2 ]);

      if ( ElTopo::point_triangle_collision(  new_vertex_position, new_vertex_smooth_position, dummy_index,
        s0, s0, triangle_vertex_0.idx(),
        s1, s2, triangle_vertex_1.idx(),
        s2, s2, triangle_vertex_2.idx() ) )

      {
        return true;
      }
    }

  }

  
  // new edges vs all edges
  
  {

    //make list of all vertices that would connect to the new central vertex
    //this is basically the list of proposed edges
    std::vector<VertexHandle> vertex_list;
    vertex_list.push_back(m_obj->fromVertex(edge));
    vertex_list.push_back(m_obj->toVertex(edge));
    for(unsigned int i = 0; i < tris.size(); ++i) {
      VertexHandle third_vert;
      bool success = getFaceThirdVertex(*m_obj, tris[i], edge, third_vert);
      if(success) vertex_list.push_back(third_vert);
    }

    ElTopo::Vec3d edge_aabb_low, edge_aabb_high;

    // do one big query into the broadphase for all new edges
    ElTopo::minmax(new_vertex_position, new_vertex_smooth_position, edge_aabb_low, edge_aabb_high);
    std::vector<ElTopo::Vec3d> verts_pos;
    for(unsigned int i = 0; i < vertex_list.size(); ++i) {
      ElTopo::Vec3d pos = ElTopo::toElTopo(m_positions[vertex_list[i]]); 
      verts_pos.push_back(pos);
      ElTopo::update_minmax(pos, edge_aabb_low, edge_aabb_high);
    }
    
    edge_aabb_low -= m_proximity_epsilon * ElTopo::Vec3d(1,1,1);
    edge_aabb_high += m_proximity_epsilon * ElTopo::Vec3d(1,1,1);

    std::vector<unsigned int> overlapping_edges;
    m_broad_phase.get_potential_edge_collisions( edge_aabb_low, edge_aabb_high, overlapping_edges );

    for ( unsigned int i = 0; i < overlapping_edges.size(); ++i )
    {

      if ( overlapping_edges[i] == edge.idx() ) { continue; }

      EdgeHandle eh(overlapping_edges[i]);
      VertexHandle edge_vertex_0 = m_obj->fromVertex(eh);
      VertexHandle edge_vertex_1 = m_obj->toVertex(eh);
      unsigned int dummy_index = -1;

      ElTopo::Vec3d edge_vert_0_pos = ElTopo::toElTopo(m_positions[edge_vertex_0]);
      ElTopo::Vec3d edge_vert_1_pos = ElTopo::toElTopo(m_positions[edge_vertex_1]);

      for(unsigned int v_ind = 0; v_ind < vertex_list.size(); ++v_ind) {
        VertexHandle vertex_a = vertex_list[v_ind];
        ElTopo::Vec3d vert_a_pos = verts_pos[v_ind];

        if ( vertex_a != edge_vertex_0 && vertex_a != edge_vertex_1 ) 
        {
          double t_zero_distance;   
          segment_segment_distance( new_vertex_position, dummy_index,
            vert_a_pos, vertex_a.idx(), 
            edge_vert_0_pos, edge_vertex_0.idx(),
            edge_vert_1_pos, edge_vertex_1.idx(),
            t_zero_distance );

          if ( t_zero_distance < m_improve_collision_epsilon )
          {
            return true;
          }

          if ( segment_segment_collision( vert_a_pos, vert_a_pos, vertex_a.idx(),
            new_vertex_position, new_vertex_smooth_position, dummy_index,
            edge_vert_0_pos, edge_vert_0_pos, edge_vertex_0.idx(),
            edge_vert_1_pos, edge_vert_1_pos, edge_vertex_1.idx() ) )
          {      
            return true;
          }
        }
      }
    }
  }

  // new triangle vs all points
  
  {

    VertexHandle vertex_a = m_obj->fromVertex(edge);
    VertexHandle vertex_b = m_obj->toVertex(edge);
    const ElTopo::Vec3d& position_a = ElTopo::toElTopo(m_positions[vertex_a]);
    const ElTopo::Vec3d& position_b = ElTopo::toElTopo(m_positions[vertex_b]);
    const ElTopo::Vec3d& position_e = new_vertex_position;
    const ElTopo::Vec3d& newposition_e = new_vertex_smooth_position;
    unsigned int dummy_e = -1;

    //make list of vertices that are not on the central edge
    //this is basically (half) the list of proposed triangles
    std::vector<VertexHandle> vertex_list;
    for(unsigned int i = 0; i < tris.size(); ++i) {
      VertexHandle third_vert;
      bool success = getFaceThirdVertex(*m_obj, tris[i], edge, third_vert);
      if(success) vertex_list.push_back(third_vert);
    }

    ElTopo::Vec3d triangle_aabb_low, triangle_aabb_high;

    // do one big query into the broadphase for all new triangles
    ElTopo::minmax(new_vertex_position, new_vertex_smooth_position, position_a, position_b, triangle_aabb_low, triangle_aabb_high);
    std::vector<ElTopo::Vec3d> verts_pos;
    for(unsigned int i = 0; i < vertex_list.size(); ++i) {
      ElTopo::Vec3d pos = ElTopo::toElTopo(m_positions[vertex_list[i]]); 
      verts_pos.push_back(pos);
      ElTopo::update_minmax(pos, triangle_aabb_low, triangle_aabb_high);
    }

    triangle_aabb_low -= m_proximity_epsilon * ElTopo::Vec3d(1,1,1);
    triangle_aabb_high += m_proximity_epsilon * ElTopo::Vec3d(1,1,1);

    std::vector<unsigned int> overlapping_vertices;
    m_broad_phase.get_potential_vertex_collisions( triangle_aabb_low, triangle_aabb_high, overlapping_vertices );

    for ( unsigned int i = 0; i < overlapping_vertices.size(); ++i )
    {

      //if ( m_mesh.m_vtxtri[overlapping_vertices[i]].empty() ) { continue; } //an optimization?

      unsigned int overlapping_vert_index = overlapping_vertices[i];
      const ElTopo::Vec3d& vert = ElTopo::toElTopo(m_positions[VertexHandle(overlapping_vert_index)]);

      //visit all vertices that are opposite to the edge to be split in the triangles containing it, and construct two resulting triangles
      for(unsigned int j = 0; j < vertex_list.size(); ++j) { 
        
        VertexHandle vertex_j = vertex_list[j];
        ElTopo::Vec3d position_j = ElTopo::toElTopo(m_positions[vertex_j]);

        // triangle ae_j (top half)
        if ( overlapping_vertices[i] != vertex_a.idx() && overlapping_vertices[i] != vertex_j.idx() )
        {
          double t_zero_distance;
          ElTopo::point_triangle_distance( vert, overlapping_vert_index, position_a, vertex_a.idx(), position_e, dummy_e, position_j, vertex_j.idx(), t_zero_distance );
          if ( t_zero_distance < m_improve_collision_epsilon )
          {
            return true;
          }

          if ( ElTopo::point_triangle_collision( vert, vert, overlapping_vert_index,
            position_a, position_a, vertex_a.idx(),
            position_j, position_j, vertex_j.idx(),
            position_e, newposition_e, dummy_e ) )
          {         
            return true;
          }
        }

        // triangle be_j (bottom half)
        if ( overlapping_vertices[i] != vertex_b.idx() && overlapping_vertices[i] != vertex_j.idx() )
        {
          double t_zero_distance;
          ElTopo::point_triangle_distance( vert, overlapping_vert_index, position_b, vertex_b.idx(), position_e, dummy_e, position_j, vertex_j.idx(), t_zero_distance );
          if ( t_zero_distance < m_improve_collision_epsilon )
          {
            return true;
          }

          if ( ElTopo::point_triangle_collision( vert, vert, overlapping_vert_index,
            position_b, position_b, vertex_b.idx(),
            position_j, position_j, vertex_j.idx(),
            position_e, newposition_e, dummy_e ) )
          {         
            return true;
          }
                 
        }
      }
    }
  }
  

  return false;
}

void ElasticShell::updateBroadPhaseStatic(const VertexHandle& vertex_a) 
{

  //update the broad phase grid to reflect the fact that two of the vertices are moving
  //i.e. change their bounding boxes to include their whole motion, while all other data
  //has just static bound boxes.

  ElTopo::Vec3d offset(m_proximity_epsilon, m_proximity_epsilon, m_proximity_epsilon);
  ElTopo::Vec3d low, high;
  
  //update vertices bounding box to include pseudomotion
  ElTopo::Vec3d pos = ElTopo::toElTopo(m_positions[vertex_a]);
  m_broad_phase.update_vertex( vertex_a.idx(), pos + offset, pos - offset);

  for ( VertexEdgeIterator veit = m_obj->ve_iter(vertex_a); veit; ++veit)
  {
    //get current edge pos
    EdgeHandle eh = *veit;
    VertexHandle vha = m_obj->fromVertex(eh);
    VertexHandle vhb = m_obj->toVertex(eh);
    ElTopo::Vec3d pa = ElTopo::toElTopo(m_positions[vha]);
    ElTopo::Vec3d pb = ElTopo::toElTopo(m_positions[vhb]);
    ElTopo::minmax(pa, pb, low, high);

    m_broad_phase.update_edge( (*veit).idx(), low, high );
  }

  for ( VertexFaceIterator vfit = m_obj->vf_iter(vertex_a); vfit; ++vfit )
  {
    //get current tri pos
    FaceHandle fh = *vfit;
    int c = 0;
    for(FaceVertexIterator fvit = m_obj->fv_iter(fh); fvit; ++fvit, ++c) {
      VertexHandle vha = *fvit;
      ElTopo::Vec3d pa = ElTopo::toElTopo(m_positions[vha]);
      if(c == 0)
        low = high = pa;
      else
        ElTopo::update_minmax(pa, low, high);
    }

    m_broad_phase.update_triangle( (*vfit).idx(), low, high );
  }
  
}

void ElasticShell::updateBroadPhaseForCollapse(const VertexHandle& vertex_a, const ElTopo::Vec3d& new_pos_a, 
                                               const VertexHandle& vertex_b, const ElTopo::Vec3d& new_pos_b) 
{

  //update the broad phase grid to reflect the fact that two of the vertices are moving
  //i.e. change their bounding boxes to include their whole motion, while all other data
  //has just static bound boxes.

  ElTopo::Vec3d offset(m_proximity_epsilon, m_proximity_epsilon, m_proximity_epsilon);
  ElTopo::Vec3d low, high;

  VertexHandle verts[2] = {vertex_a, vertex_b};
  ElTopo::Vec3d verts_pos[2] = {new_pos_a, new_pos_b};

  for(int i = 0; i < 2; ++i) {
    //update vertices bounding box to include pseudomotion
    ElTopo::minmax(ElTopo::toElTopo(m_positions[verts[i]]), verts_pos[i], low, high);
    low -= offset; high += offset;
    m_broad_phase.update_vertex( verts[i].idx(), low, high );

    for ( VertexEdgeIterator veit = m_obj->ve_iter(verts[i]); veit; ++veit)
    {
      //get current edge pos
      EdgeHandle eh = *veit;
      VertexHandle vha = m_obj->fromVertex(eh);
      VertexHandle vhb = m_obj->toVertex(eh);
      ElTopo::Vec3d pa = ElTopo::toElTopo(m_positions[vha]);
      ElTopo::Vec3d pb = ElTopo::toElTopo(m_positions[vhb]);
      ElTopo::minmax(pa, pb, low, high);
      
      //check if either vertex moved, and if so, add the position to the bound box
      for(int j = 0; j < 2; ++j) {
        if(vha == verts[j]) {
          ElTopo::update_minmax(verts_pos[j], low, high);
        }
        if(vhb == verts[j]) {
          ElTopo::update_minmax(verts_pos[j], low, high);
        }
      }
      
      m_broad_phase.update_edge( (*veit).idx(), low, high );
    }

    for ( VertexFaceIterator vfit = m_obj->vf_iter(verts[i]); vfit; ++vfit )
    {
      //get current tri pos
      FaceHandle fh = *vfit;
      int c = 0;
      for(FaceVertexIterator fvit = m_obj->fv_iter(fh); fvit; ++fvit, ++c) {
        VertexHandle vha = *fvit;
        ElTopo::Vec3d pa = ElTopo::toElTopo(m_positions[vha]);
        if(c == 0)
          low = high = pa;
        else
          ElTopo::update_minmax(pa, low, high);
      }

      //check if any vertex moved, and if so, add that position to the bound box
      for(int j = 0; j < 2; ++j) {
        for(FaceVertexIterator fvit = m_obj->fv_iter(fh); fvit; ++fvit) {
          VertexHandle vha = *fvit;
          if(vha == verts[j]) {
            ElTopo::update_minmax(verts_pos[j], low, high);
          }
        }
      }

      m_broad_phase.update_triangle( (*vfit).idx(), low, high );
    }
  }
}

// --------------------------------------------------------
///
/// Continuous collision detection between two triangles.  Duplicates collision detection, so 
/// should be used rarely, and only when inconvenient to run edge-edge and point-tri tests individually.
///
// --------------------------------------------------------

bool ElasticShell::checkTriangleVsTriangleCollisionForCollapse( const FaceHandle& triangle_a, const FaceHandle& triangle_b,  
                                                               const VertexHandle& source_vert, const VertexHandle& dest_vert,
                                                               ElTopo::Vec3d new_position)
{
  // --------------------------------------------------------
  // Point-triangle
  // --------------------------------------------------------

  // one point from triangle_a vs triangle_b

  for ( FaceVertexIterator fvit = m_obj->fv_iter(triangle_a); fvit; ++fvit)
  {
    VertexHandle vertex_0 = *fvit;
    
    VertexHandle vert_ids[3];
    ElTopo::Vec3d vert_pos[3];
    ElTopo::Vec3d new_vert_pos[3];

    int c = 0;
    bool shared_vert = false;
    for(FaceVertexIterator fvit2 = m_obj->fv_iter(triangle_b); fvit2; ++fvit2, ++c) {
      VertexHandle v = *fvit2;
      if(v == vertex_0) {
        shared_vert = true;
        break;
      }
      vert_ids[c] = v;
      vert_pos[c] = ElTopo::toElTopo(m_positions[vert_ids[c]]);
      new_vert_pos[c] = (v == source_vert || v == dest_vert)? new_position : vert_pos[c];
    }

    if(shared_vert) continue;
  
    ElTopo::Vec3d vert_0_pos, vert_0_newpos;
    vert_0_pos = ElTopo::toElTopo(m_positions[ vertex_0 ]);
    vert_0_newpos = (vertex_0 == source_vert || vertex_0 == dest_vert)? new_position : vert_0_pos;

    if ( ElTopo::point_triangle_collision(  
      vert_0_pos, vert_0_newpos, vertex_0.idx(),
      vert_pos[0], new_vert_pos[0], vert_ids[0].idx(), 
      vert_pos[1], new_vert_pos[1], vert_ids[1].idx(),
      vert_pos[2], new_vert_pos[2], vert_ids[2].idx() ) )

    {
      return true;
    }
  }

  // one point from triangle_b vs triangle_a

  for ( FaceVertexIterator fvit = m_obj->fv_iter(triangle_b); fvit; ++fvit)
  {
    VertexHandle vertex_0 = *fvit;

    VertexHandle vert_ids[3];
    ElTopo::Vec3d vert_pos[3];
    ElTopo::Vec3d new_vert_pos[3];

    int c = 0;
    bool shared_vert = false;
    for(FaceVertexIterator fvit2 = m_obj->fv_iter(triangle_a); fvit2; ++fvit2, ++c) {
      VertexHandle v = *fvit2;
      if(v == vertex_0) {
        shared_vert = true;
        break;
      }
      vert_ids[c] = v;
      vert_pos[c] = ElTopo::toElTopo(m_positions[vert_ids[c]]);
      new_vert_pos[c] = (v == source_vert || v == dest_vert)? new_position : vert_pos[c];
    }

    if(shared_vert) continue;

    ElTopo::Vec3d vert_0_pos, vert_0_newpos;
    vert_0_pos = ElTopo::toElTopo(m_positions[ vertex_0 ]);
    vert_0_newpos = (vertex_0 == source_vert || vertex_0 == dest_vert)? new_position : vert_0_pos;

    if ( ElTopo::point_triangle_collision(  
      vert_0_pos, vert_0_newpos, vertex_0.idx(),
      vert_pos[0], new_vert_pos[0], vert_ids[0].idx(), 
      vert_pos[1], new_vert_pos[1], vert_ids[1].idx(),
      vert_pos[2], new_vert_pos[2], vert_ids[2].idx() ) )

    {
      return true;
    }
  }



  // --------------------------------------------------------
  // edge-edge
  // --------------------------------------------------------
  
 
  for ( FaceEdgeIterator feit = m_obj->fe_iter(triangle_a); feit; ++feit)
  {

    EdgeHandle edge_0 = *feit;
    
    // one edge
    VertexHandle vertex_0 = m_obj->fromVertex(edge_0);
    VertexHandle vertex_1 = m_obj->toVertex(edge_0);

    for ( FaceEdgeIterator feit2 = m_obj->fe_iter(triangle_b); feit2; ++feit2)
    {
      
      EdgeHandle edge_1 = *feit2;

      // another edge
      VertexHandle vertex_2 = m_obj->fromVertex(edge_1);
      VertexHandle vertex_3 = m_obj->toVertex(edge_1);

      if ( vertex_0 == vertex_2 || vertex_0 == vertex_3 || vertex_1 == vertex_2 || vertex_1 == vertex_3 )
      {
        continue;
      }

      VertexHandle vert_ids[4] = {vertex_0, vertex_1, vertex_2, vertex_3};
      ElTopo::Vec3d vert_pos[4], new_vert_pos[4];
      for(int c = 0; c < 4; ++c) {
        VertexHandle v = vert_ids[c];
        vert_pos[c] = ElTopo::toElTopo(m_positions[vert_ids[c]]);
        new_vert_pos[c] = (v == source_vert || v == dest_vert)? new_position : vert_pos[c];
      }

      if ( ElTopo::segment_segment_collision(  
        vert_pos[0], new_vert_pos[0], vertex_0.idx(), 
        vert_pos[1], new_vert_pos[1], vertex_1.idx(),
        vert_pos[2], new_vert_pos[2], vertex_2.idx(),
        vert_pos[3], new_vert_pos[3], vertex_3.idx() ) )               
      {
        return true;
      }
    }
  }

  return false;
}

bool ElasticShell::edgeCollapseCausesCollision(const VertexHandle& source_vertex, 
                                             const VertexHandle& destination_vertex, 
                                             const EdgeHandle& edge_index, 
                                             const ElTopo::Vec3d& vertex_new_position ) {
 
  // Change source vertex predicted position to superimpose onto dest vertex
  //printf("Checking edge collapse safety\n");

  updateBroadPhaseForCollapse(source_vertex, ElTopo::toElTopo(m_positions[source_vertex]),
                              destination_vertex, ElTopo::toElTopo(m_positions[destination_vertex]));
  
  // Get the set of triangles which are going to be deleted
  std::vector< FaceHandle > triangles_incident_to_edge;
  for(EdgeFaceIterator efit = m_obj->ef_iter(edge_index); efit; ++efit) {
    triangles_incident_to_edge.push_back(*efit);
  }

  // Get the set of triangles which move because of this motion
  std::vector<FaceHandle> moving_triangles;
  for ( VertexFaceIterator vfit = m_obj->vf_iter(source_vertex); vfit; ++vfit ){
    moving_triangles.push_back( *vfit );
  }
  for ( VertexFaceIterator vfit = m_obj->vf_iter(destination_vertex); vfit; ++vfit ) {
    moving_triangles.push_back( *vfit );
  }
  

  // Check this set of triangles for collisions, holding everything else static
  for ( unsigned int i = 0; i < moving_triangles.size(); ++i )
  { 

    // Disregard triangles which will end up being deleted - those triangles incident to the collapsing edge.
    bool triangle_will_be_deleted = false;
    for ( unsigned int j = 0; j < triangles_incident_to_edge.size(); ++j )
    {
      if ( moving_triangles[i] == triangles_incident_to_edge[j] )
      {
        triangle_will_be_deleted = true;
        break;
      }
    }

    if ( triangle_will_be_deleted ) { continue; }

    std::vector<ElTopo::Vec3d> tri_verts_old;
    std::vector<ElTopo::Vec3d> tri_verts_new;
    for(FaceVertexIterator fvit = m_obj->fv_iter(moving_triangles[i]); fvit; ++fvit) {
      VertexHandle vh = *fvit;
      ElTopo::Vec3d old_pos = ElTopo::toElTopo(m_positions[vh]);
      tri_verts_old.push_back(old_pos);

      //check if the vertex is being moved
      ElTopo::Vec3d vert_pos = (vh == source_vertex || vh == destination_vertex)? old_pos : vertex_new_position; 
      tri_verts_new.push_back(vert_pos);
    }
    assert(tri_verts_old.size() == 3);
    assert(tri_verts_new.size() == 3);

    // Test the triangle vs all other triangles
    ElTopo::Vec3d aabb_low, aabb_high;
    minmax( tri_verts_old[0], tri_verts_old[1], tri_verts_old[2], 
      tri_verts_new[0], tri_verts_new[1], tri_verts_new[2], 
      aabb_low, aabb_high );

    std::vector<unsigned int> overlapping_triangles;
    m_broad_phase.get_potential_triangle_collisions( aabb_low, aabb_high, overlapping_triangles );

    for ( unsigned j=0; j < overlapping_triangles.size(); ++j )
    {
      // Don't check against triangles which are incident to the dest vertex
      bool triangle_incident_to_dest = false;
      for ( unsigned int k = 0; k < moving_triangles.size(); ++k )
      {
        if ( moving_triangles[k].idx() == overlapping_triangles[j] )
        {
          triangle_incident_to_dest = true;
          break;
        }
      }
      if ( triangle_incident_to_dest )    { continue; }

      if ( checkTriangleVsTriangleCollisionForCollapse( moving_triangles[i], FaceHandle(overlapping_triangles[j]), source_vertex, destination_vertex, vertex_new_position) )
      {
        return true;
      }
    }
  
  }

  return false;
}



void ElasticShell::collapseEdges(double minAngle, double desiredEdge, double ratio_R, double ratio_r, double minEdge) {

  EdgeIterator e_it = m_obj->edges_begin();
  for(;e_it != m_obj->edges_end(); ++e_it) {

    EdgeHandle eh = *e_it;

    VertexHandle v0 = m_obj->fromVertex(eh);
    VertexHandle v1 = m_obj->toVertex(eh);
    assert(v0!=v1);
    Vec3d p0 = getVertexPosition(v0);
    Vec3d p1 = getVertexPosition(v1);
    Scalar edgeLength = (p1-p0).norm();

    bool doCollapse = false;

    //"Absolute small angle", "Relative shortness", "Absolute small angle", "Relative small triangle"
    bool smallAngle = false;
    bool relativeShortness = false;
    bool isSmallest = true;
    Scalar longestEdgeAll = 0;
    for(EdgeFaceIterator ef_iter = m_obj->ef_iter(eh); ef_iter; ++ef_iter) {
      Scalar longestEdge = edgeLength;
      for(FaceEdgeIterator fe_iter = m_obj->fe_iter(*ef_iter); fe_iter; ++fe_iter) {
        EdgeHandle edge_other = *fe_iter;
        if(edge_other == eh) continue;

        VertexHandle fromV = m_obj->fromVertex(edge_other);
        VertexHandle toV = m_obj->toVertex(edge_other);
        Scalar thisLength = (getVertexPosition(fromV) - getVertexPosition(toV)).norm();
        longestEdge = std::max(longestEdge, thisLength);
        if(thisLength < edgeLength)
          isSmallest = false;
        if(smallAngle) continue;

        VertexHandle oppV = fromV == v0 || fromV == v1 ? toV : fromV;
        Vec3d p2 = getVertexPosition(oppV);
        Vec3d dir0 = p0-p2, dir1 = p1 - p2;
        Scalar angle0 = acos(dir0.dot(dir1));
        if(angle0 < minAngle) {
          smallAngle = true;
        }
      }

      longestEdgeAll = std::max(longestEdgeAll, longestEdge);

      if(smallAngle && longestEdge < desiredEdge) //absolute shortness
        doCollapse = true;

    }

    if(longestEdgeAll < desiredEdge && edgeLength < ratio_R*longestEdgeAll) //Relative small triangle
      doCollapse = true;

    if(edgeLength < ratio_r*longestEdgeAll) //relative shortness
      doCollapse = true;

    if(isSmallest && longestEdgeAll < minEdge) //absolute small angle
      doCollapse = true;

    if(doCollapse) {

      //don't collapse a constrained vertex
      bool pinnedVert = false;
      for(unsigned int i = 0; i < m_constrained_vertices.size(); ++i) {
        if(v0 == m_constrained_vertices[i] || v1 == m_constrained_vertices[i]) {
          pinnedVert = true;
        }
      }
      if(pinnedVert) continue;

      //check if either point is on the boundary
      bool v0_bdry, v1_bdry;
      v0_bdry = isVertexOnBoundary(*m_obj, v0);
      v1_bdry = isVertexOnBoundary(*m_obj, v1);

      Vec3d newPoint, newVel, newUndef;
      if(v0_bdry && v1_bdry) continue; //both edges on the boundary, don't collapse!
      else if(v0_bdry) {
        newPoint = p0;
        newVel = getVertexVelocity(v0);
        newUndef = getVertexUndeformed(v0);
      }
      else if(v1_bdry) {
        newPoint = p1;
        newVel = getVertexVelocity(v1);
        newUndef = getVertexUndeformed(v1);
      }
      else {
        newPoint = 0.5f*(p0+p1);
        newVel = 0.5f*(getVertexVelocity(v0) + getVertexVelocity(v1));
        newUndef = 0.5f*(getVertexUndeformed(v0) + getVertexUndeformed(v1));
      }

      //determine area of collapsing faces
      EdgeFaceIterator efit = m_obj->ef_iter(eh);
      Scalar totalVolume = 0;
      for(;efit; ++efit)
        totalVolume += getVolume(*efit);

      //if(edgeCollapseCausesCollision(v0, v1, eh, ElTopo::toElTopo(newPoint)))
      //  return;

      //remove to-be-deleted stuff from collision grid
      m_broad_phase.remove_vertex(v0.idx());
      m_broad_phase.remove_edge(eh.idx());
      for(EdgeFaceIterator efit = m_obj->ef_iter(eh); efit; ++efit)
        m_broad_phase.remove_triangle((*efit).idx());
      
      //do the collapse itself
      std::vector<EdgeHandle> deletedEdges;
      m_obj->collapseEdge(eh, v0, deletedEdges);

      //remove the edges that were deleted as a side-effect.
      for(unsigned int q = 0; q < deletedEdges.size(); ++q) {
         m_broad_phase.remove_edge(deletedEdges[q].idx());
      }

      setVertexVelocity(v1, newVel);
      setVertexPosition(v1, newPoint);
      setUndeformedVertexPosition(v1, newUndef);

      //increment the thickness of the nearby faces to account for the lost volume
      VertexFaceIterator vfit = m_obj->vf_iter(v1);
      Scalar totalNewArea = 0;
      for(;vfit; ++vfit) {
        FaceHandle fh = *vfit;
        totalNewArea += std::max(0.0, getArea(fh, true) - m_volumes[fh] / m_thicknesses[fh]); //only consider increases
      }

      vfit = m_obj->vf_iter(v1);
      for(;vfit; ++vfit) {
        FaceHandle fh = *vfit;
        Scalar newArea = getArea(fh) - m_volumes[fh] / m_thicknesses[fh];
        if(newArea <= 0) continue;

        m_volumes[fh] += (newArea / totalNewArea) * totalVolume; //allocate extra volume proportional to area increases
        m_thicknesses[fh] = m_volumes[fh] / getArea(fh);
      }

      //update static collision data for everything incident on the kept vertex
      updateBroadPhaseStatic(v1);


    }
  }
}

void ElasticShell::updateThickness() {
  FaceIterator fit = m_obj->faces_begin();
  for(;fit != m_obj->faces_end(); ++fit) {
    FaceHandle f = *fit;
    if(!isFaceActive(f)) continue;

    //compute face area
    std::vector<Vec3d> verts(3);
    int i = 0;
    for(FaceVertexIterator fvit = m_obj->fv_iter(f); fvit; ++fvit, ++i)
      verts[i] = getVertexPosition(*fvit);
    Vec3d v0 = verts[1] - verts[0]; Vec3d v1 = verts[2] - verts[0];
    Scalar area = 0.5f*(v0.cross(v1)).norm();

    //figure out new thickness from previous volume, and update it
    Scalar newThickness = m_volumes[f] / area;
    m_thicknesses[f] = newThickness;
  }


}

void ElasticShell::extendMesh() {

  //only do this if the mesh has moved some minimum distance from the original
  //inflow position, yeah?

  for(unsigned int boundary = 0; boundary < m_inflow_boundaries.size(); ++boundary) {
    int count = m_inflow_boundaries[boundary].size();
    int last = m_inflow_boundaries[boundary].size()-1;
    
    //check if open or closed loop
    VertexHandle loopVertex = getSharedVertex(*m_obj, m_inflow_boundaries[boundary][0], m_inflow_boundaries[boundary][last]);

    //VertexHandle prevVert = m_obj->addVertex(); //loopVertex.isValid() ? loopVertex : m_obj->addVertex();
    VertexHandle prevVert = m_obj->addVertex(); //loopVertex.isValid() ? loopVertex : m_obj->addVertex();

    std::vector<VertexHandle> vertices;
    vertices.push_back(prevVert);
    std::vector<FaceHandle> faces;
    
    VertexHandle sharedVert = getSharedVertex(*m_obj, m_inflow_boundaries[boundary][0], m_inflow_boundaries[boundary][1]);
    VertexHandle prevLowerVert = getEdgesOtherVertex(*m_obj, m_inflow_boundaries[boundary][0], sharedVert);

    EdgeHandle prevEdge = m_obj->addEdge(prevLowerVert, prevVert);
    VertexHandle loopTopVertex = prevVert;
    EdgeHandle startEdge = prevEdge; //save this for wrapping around.
    
    std::vector<EdgeHandle> newList;
    for(unsigned int edge = 0; edge < m_inflow_boundaries[boundary].size(); ++edge) {
      
      EdgeHandle eh1 = m_inflow_boundaries[boundary][edge];
      EdgeHandle eh2 = m_inflow_boundaries[boundary][(edge+1)%count];

      VertexHandle sharedVert, otherVert; 
      if((int)edge < count - 1 || loopVertex.isValid()) { //a middle edge, or the last edge if things loop around
        sharedVert = getSharedVertex(*m_obj, eh1, eh2);
        otherVert = getEdgesOtherVertex(*m_obj, eh1, sharedVert);
      }
      else {
        otherVert = prevLowerVert;
        sharedVert = getEdgesOtherVertex(*m_obj, eh1, otherVert);
      }

      VertexHandle newVert;
      EdgeHandle newEdge3;
      if(edge == count-1 && loopVertex.isValid()) {
        newVert = loopTopVertex;
        newEdge3 = startEdge;
      }
      else {
        newVert = m_obj->addVertex();
        vertices.push_back(newVert);
        newEdge3 = m_obj->addEdge(sharedVert, newVert);
      }
      
      EdgeHandle newEdge4 = m_obj->addEdge(prevVert, newVert);


      //A option
      EdgeHandle newEdge2 = m_obj->addEdge(prevVert, sharedVert);

      FaceHandle newFace1 = m_obj->addFace(prevEdge, eh1, newEdge2);
      FaceHandle newFace2 = m_obj->addFace(newEdge2, newEdge3, newEdge4);
      
      setFaceActive(newFace1);
      setFaceActive(newFace2);
      faces.push_back(newFace1);
      faces.push_back(newFace2);
      //Todo: mess with ordering to automatically determine the correct front/back faces
      //based on the faces we are connecting to.
      
      newList.push_back(newEdge4);

      //advance to the next edge
      prevVert = newVert;
      prevLowerVert = sharedVert;
      prevEdge = newEdge3;
    }
    

    m_inflow_boundaries[boundary] = newList;

    for(unsigned int i = 0; i < vertices.size(); ++i) {
      setVertexPosition(vertices[i], m_inflow_positions[boundary][i]);
      setUndeformedVertexPosition(vertices[i], m_inflow_positions[boundary][i]);
      setVertexVelocity(vertices[i], m_inflow_velocity[boundary]);
      m_vertex_masses[vertices[i]] = 0;
      m_damping_undeformed_positions[vertices[i]] = m_inflow_positions[boundary][i];
    }


    for(unsigned int i = 0; i < faces.size(); ++i) {
      m_thicknesses[faces[i]] = m_inflow_thickness;
      m_volumes[faces[i]] = 0;
      m_volumes[faces[i]] = getArea(faces[i])*m_inflow_thickness;
      FaceVertexIterator fvit = m_obj->fv_iter(faces[i]);
      for(;fvit;++fvit) {
        VertexHandle vh = *fvit;
        m_vertex_masses[vh] += m_volumes[faces[i]] * m_density / 3.0;
      }
    }

  }

}

void ElasticShell::setInflowSection(std::vector<EdgeHandle> edgeList, const Vec3d& vel) {
  m_inflow_boundaries.push_back(edgeList);
  m_inflow_velocity.push_back(vel);
  VertexHandle prevVert;
  std::vector<Vec3d> posList;
  for(unsigned int edge = 0; edge < edgeList.size(); ++edge) {
    EdgeHandle eh1 = edgeList[edge];
    EdgeHandle eh2 = edgeList[(edge+1)%edgeList.size()];
    
    VertexHandle sharedVert, otherVert;
    if(edge == edgeList.size() - 1) {
      otherVert = prevVert;
      sharedVert = getEdgesOtherVertex(*m_obj, eh1, otherVert); 
    }
    else {
      sharedVert = getSharedVertex(*m_obj, eh1, eh2);
      otherVert = getEdgesOtherVertex(*m_obj, eh1, sharedVert); 
    }

    Vec3d pos = getVertexPosition(otherVert);
    posList.push_back(pos);
    std::cout << "Vertex: " << otherVert.idx() << std::endl;
    
    prevVert = sharedVert;
  }
  
  VertexHandle wrapVert = getSharedVertex(*m_obj, edgeList[0], edgeList[edgeList.size()-1]);
  if(!wrapVert.isValid()) {
    std::cout << "Edges don't wrap.\n";
    Vec3d pos = getVertexPosition(prevVert);
    posList.push_back(pos);
    std::cout << "Vertex: " << prevVert.idx() << std::endl;
  } 
  else {
    std::cout << "Edges do wrap.\n";
  }
  std::cout << "Vertices: " << std::endl;
  for(unsigned int i = 0; i < posList.size(); ++i)
    std::cout << posList[i] << " " << std::endl;
  std::cout << std::endl;

  m_inflow_positions.push_back(posList);

}
/*
void ElasticShell::setMass( const DofHandle& hnd, const Scalar& mass )
{
  assert(hnd.getType() == DofHandle::VERTEX_DOF);

  const VertexHandle vh = static_cast<const VertexHandle&>(hnd.getHandle());
  m_vertex_masses[vh] = mass;
}
*/


} //namespace BASim