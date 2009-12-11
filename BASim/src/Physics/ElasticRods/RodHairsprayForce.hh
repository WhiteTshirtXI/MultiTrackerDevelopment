#ifndef RODHAIRSPRAYFORCE_HH
#define RODHAIRSPRAYFORCE_HH

#include <BASim/BASim>
#include "ElasticRod.hh"
#include "RodExternalForce.hh"

namespace BASim {

class RodHairsprayForce : public RodExternalForce
{
public:
    RodHairsprayForce(ElasticRod &rod, vector<Scalar>& ks, vector<Vec3d>& curvePositions);
    ~RodHairsprayForce();

    virtual void computeForce(const ElasticRod& rod, VecXd& F);

protected:    
    ElasticRod& m_rod;
    std::vector<Vec3d>& m_curvePositions;
    std::vector<Scalar> m_ks;
    std::vector<Scalar> m_ds;
};

}

#endif
