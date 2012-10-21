/**
 * \file ShellShellVertexJointCouplingForce.h
 *
 * \author fang@cs.columbia.edu
 * \date 10/01/2012
 */

#ifndef SHELLSHELLVERTEXJOINTCOUPLINGFORCE_HH
#define SHELLSHELLVERTEXJOINTCOUPLINGFORCE_HH

#include "BASim/src/Physics/DeformableObjects/DefoObjForce.hh"
#include "BASim/src/Physics/DeformableObjects/DeformableObject.hh"
#include "BASim/src/Physics/DeformableObjects/Rods/ElasticRodModel.hh"
#include "BASim/src/Physics/DeformableObjects/Shells/ElasticShell.hh"
#include "BASim/src/Physics/DeformableObjects/Solids/ElasticSolid.hh"

#include "BASim/src/Math/ADT/adreal.h"
#include "BASim/src/Math/ADT/advec.h"
#include "BASim/src/Math/ADT/mat3t.h"

namespace BASim 
{
  class ShellShellVertexJointCouplingForce : public DefoObjForce
  {
  public:
    const static int NumDof = 15;

    typedef Eigen::Matrix<Scalar, NumDof, 1>      ElementForce;
    typedef Eigen::Matrix<Scalar, NumDof, NumDof> ElementJacobian;

    typedef CVec3T<Scalar> Vector3d;
    
    struct Stencil
    {
      Stencil() { }
      
      IntArray dofindices;

      // stencil coverage
      FaceHandle f1;  // shell face ADE
      FaceHandle f2;  // shell face ABC
      
//      // cached stiffness
//      Scalar stiffness;
//      Scalar viscous_stiffness;
      
      // reference strain
      Vec3d undeformed_AB;
      Vec3d undeformed_AC;
      Vec3d damping_undeformed_AB;
      Vec3d damping_undeformed_AC;
      
      // cached properties
      Vec3d AB;  // f1 edge AB in f2 frame
      Vec3d AC;  // f1 edge AC in f2's frame
    };

  public:
    ShellShellVertexJointCouplingForce(ElasticShell & shell, const std::vector<Stencil> & stencils, Scalar stiffness, Scalar stiffness_damp, Scalar timestep);
    virtual ~ShellShellVertexJointCouplingForce();

  public:
    void addStencil(Stencil & s) { m_stencils.push_back(s); }
    std::vector<Stencil> & stencils() { return m_stencils; }
    const std::vector<Stencil> & stencils() const { return m_stencils; }
    
  public:
    void updateStiffness();
    void updateViscousReferenceStrain();
    void updateProperties();
    
  public:
    void startStep(Scalar time, Scalar timestep);
    void endStep(Scalar time, Scalar timestep);
    void startIteration(Scalar time, Scalar timestep);
    void endIteration(Scalar time, Scalar timestep);
    
  public:
    Scalar globalEnergy();
    void globalForce(VecXd & force);
    void globalJacobian(Scalar scale, MatrixBase & Jacobian);

  public:
    ElasticShell & shell() { assert(m_shell); return *m_shell; }
    
    DeformableObject & defoObj() { assert(m_shell); return m_shell->getDefoObj(); }
    
  protected:
    template <int DO_HESS>
    adreal<NumDof, DO_HESS, Scalar> adEnergy(const ShellShellVertexJointCouplingForce & mn, const Vec3d & A, const Vec3d & B, const Vec3d & C, const Vec3d & D, const Vec3d & E, const Vec3d & undeformed_AB, const Vec3d & undeformed_AC, Scalar stiffness);

  protected:
    Scalar localEnergy(Stencil & s, bool viscous);
    void localForce(ElementForce & force, Stencil & s, bool viscous);
    void localJacobian(ElementJacobian & jacobian, Stencil & s, bool viscous);

    void computeReferenceStrain();
    
    Vector3d vec2vector(const Vec3d & v);
    std::vector<VertexHandle> getVertices(const Stencil & s);
    
  protected:
    ElasticShell * m_shell;
    
    std::vector<Stencil> m_stencils;
    
    Scalar m_stiffness;
    Scalar m_stiffness_damp;
  };
  
}


#endif // SHELLSOLIDVERTEXJOINTCOUPLINGFORCE_HH