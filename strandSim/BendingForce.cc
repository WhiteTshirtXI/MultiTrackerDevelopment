/*
 * BendingForce.cc
 *
 *  Created on: 12/07/2011
 *      Author: jaubry
 */

#include "BendingForce.hh"

namespace strandsim
{

BendingForce::BendingForce()
{
    // TODO Auto-generated constructor stub

}

BendingForce::~BendingForce()
{
    // TODO Auto-generated destructor stub
}

Scalar BendingForce::localEnergy(const ElasticStrand& strand, const StrandGeometry& geometry, const IndexType vtx)
{
    const Mat2d& B = strand.m_bendingMatrices[vtx];
    const Scalar ilen = 2.0 / (strand.m_restLengths[vtx - 1] + strand.m_restLengths[vtx]);
    const Vec2d& kappa = geometry.m_kappa[vtx];
    const Vec2d& kappaBar = strand.m_kappaBar[vtx];

    return 0.5 * ilen * (kappa - kappaBar).dot(B * (kappa - kappaBar));
}

BendingForce::LocalForceType BendingForce::localForce(const ElasticStrand& strand, const StrandGeometry& geometry, const IndexType vtx)
{
    const Mat2d& B = strand.m_bendingMatrices[vtx];
    const Scalar ilen = 2.0 / (strand.m_restLengths[vtx - 1] + strand.m_restLengths[vtx]);
    const Vec2d& kappa = geometry.m_kappa[vtx];
    const Vec2d& kappaBar = strand.m_kappaBar[vtx];
    const Eigen::Matrix<Scalar, 11, 2>& gradKappa = geometry.m_gradKappa[vtx];

    return -ilen * gradKappa * B * (kappa - kappaBar);
}

BendingForce::LocalJacobianType BendingForce::localJacobian(const ElasticStrand& strand, const StrandGeometry& geometry, const IndexType vtx)
{
    LocalJacobianType localJ;

    const Mat2d& B = strand.m_bendingMatrices[vtx];
    const Scalar milen = -2.0 / (strand.m_restLengths[vtx - 1] + strand.m_restLengths[vtx]);
    const Vec2d& kappa = geometry.m_kappa[vtx];
    const Vec2d& kappaBar = strand.m_kappaBar[vtx];
    const Eigen::Matrix<Scalar, 11, 2>& gradKappa = geometry.m_gradKappa[vtx];

    symBProduct(localJ, B, gradKappa);
    localJ *= milen;

    const std::pair<LocalJacobianType, LocalJacobianType>& hessKappa = geometry.m_HessKappa[vtx]; // Could we compute HessKappa on the spot instead?
    const Vec2d temp = milen * (kappa - kappaBar).transpose() * B;
    localJ += temp(0) * hessKappa.first + temp(1) * hessKappa.second;

    return localJ;
}

void BendingForce::addInPosition(ForceVectorType& globalForce, const IndexType vtx, const LocalForceType& localForce)
{
    globalForce.segment<11> (4 * vtx) += localForce;
}

void BendingForce::addInPosition(JacobianMatrixType& globalJacobian, const IndexType vtx,
        const LocalJacobianType& localJacobian)
{
    globalJacobian.localStencilAdd<11> (4 * (vtx - 1), localJacobian);
}

}