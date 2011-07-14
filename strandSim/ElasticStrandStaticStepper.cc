/*
 * ElasticStrandStaticStepper.cc
 *
 *  Created on: 8/07/2011
 *      Author: jaubry
 */

#include "ElasticStrandStaticStepper.hh"

namespace strandsim
{

ElasticStrandStaticStepper::ElasticStrandStaticStepper()
{
    // TODO Auto-generated constructor stub

}

ElasticStrandStaticStepper::~ElasticStrandStaticStepper()
{
    // TODO Auto-generated destructor stub
}

void ElasticStrandStaticStepper::execute(ElasticStrand& strand) const
{
    assert(strand.readyForSolving());

    const Scalar E = strand.getTotalEnergy();
    const ElasticStrand::ForceVectorType& F = strand.getTotalForces();
    const ElasticStrand::JacobianMatrixType& J = strand.getTotalJacobian();
    VecXd& newDOFs = strand.getNewDegreesOfFreedom();

    // If we want to add a constant diagonal to J to enforce a trust region here, we need a non-const getTotalJacobian()

    solve(newDOFs, J, F); // X = J^{-1} F
    newDOFs -= strand.getDegreesOfFreedom(); // X = J^{-1} F -X_0
    newDOFs *= -1;// X = X_0 - J^{-1} F
    // AND THE MASSES????

    // Update the new position's frames and stuff
    strand.prepareForExamining();

    // TODO: Examine here the new position.

    // Accept the new position. This also updates the strand for the next solve.
    strand.acceptNewPositions();
}

}
