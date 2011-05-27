/**
 * \file ImplicitEuler.hh
 *
 * \author miklos@cs.columbia.edu
 * \date 09/04/2009
 */

#ifndef IMPLICITEULER_HH
#define IMPLICITEULER_HH

#include "TimeSteppingBase.hh"
#include "MatrixBase.hh"
#include "LinearSolverBase.hh"
#include "SolverUtils.hh"
#include "../Core/Timer.hh"
#include "../Core/StatTracker.hh"

namespace BASim {

/** This class implements the implicit Euler time-stepping
    method. This assumes a pair of equations of the form
    \f{eqnarray*}\frac{dx}{dt} &=& v \\ M\frac{dv}{dt} &=& f(t,x) \
    . \f} The mass matrix \f$M\f$ is assumed to be diagonal.
*/
template <class ODE>
class ImplicitEuler : public DiffEqSolver
{
public:

  explicit ImplicitEuler(ODE& ode)
    : m_diffEq(ode)
    , m_ndof(-1)
    , m_mass()
    , x0()
    , v0()
    , m_rhs()
    , m_lhs()
    , m_deltaX()
    , m_deltaV()
    , m_increment()
    , m_fixed()
    , m_desired()
    , m_initial_residual(0)
    , m_residual(0)
    , m_curit(0)
    , m_solve_for_dv(false)
    , m_A(NULL)
    , m_solver(NULL)
  {
    m_A = m_diffEq.createMatrix();
    m_solver = SolverUtils::instance()->createLinearSolver(m_A);
  }

  ~ImplicitEuler()
  {
    assert( m_A != NULL );
    assert( m_solver != NULL );
    
    if( m_A != NULL ) 
    {
      delete m_A;
      m_A = NULL;
    }
    
    if( m_solver != NULL )
    {
      delete m_solver;
      m_solver = NULL;
    }
  }

  bool execute()
  {
    if (m_solve_for_dv) return velocity_solve();
    else return position_solve();
  }

  std::string getName() const
  {
    std::string name;
    if (m_solve_for_dv) name = "Implicit Euler, velocity-dof solve";
    else name = "Implicit Euler, position-dof solve";
    return name;
  }

  void resize()
  {
    m_ndof = m_diffEq.ndof();
    m_mass.resize(m_ndof);
    x0.resize(m_ndof);
    v0.resize(m_ndof);
    m_rhs.resize(m_ndof);
    m_lhs.resize(m_ndof);
    m_deltaX.resize(m_ndof);
    m_deltaV.resize(m_ndof);
    m_increment.resize(m_ndof);
    assert( m_A->rows() == m_A->cols() );
    if (m_A->rows() != m_ndof) 
    {
      assert( m_A != NULL );
      delete m_A;
      m_A = m_diffEq.createMatrix();
      assert( m_solver != NULL );
      delete m_solver;
      m_solver = SolverUtils::instance()->createLinearSolver(m_A);
    }
  }

  void setZero()
  {
    x0.setZero();
    v0.setZero();
    m_rhs.setZero();
    m_lhs.setZero();
    m_deltaX.setZero();
    m_deltaV.setZero();
    m_increment.setZero();
    m_A->setZero();
  }

  // Computes the inf norm of the residual
  Scalar computeResidual()
  {
    // Sanity checks for NANs
//    assert( (x0.cwise() == x0).all() );
//    assert( (m_deltaV.cwise() == m_deltaV).all() );
//    assert( (m_deltaX.cwise() == m_deltaX).all() );
    
    // rhs == forces
    m_rhs.setZero();
    m_diffEq.evaluatePDot(m_rhs);

    // lhs == M*deltaV/dt
    m_lhs.array() = m_mass.array()*m_deltaV.array()/m_dt;

    for (size_t i = 0; i < m_fixed.size(); ++i) 
    {
      int idx = m_fixed[i];
      //m_lhs[idx] = m_deltaV[idx] + v0[idx];
      //m_rhs[idx] = (m_desired[i] - x0[idx]) / m_dt;
      m_rhs[idx] = m_desired[i];
      //assert( m_diffEq.getX(idx) == x0(idx) + m_deltaX(idx) );
      //m_lhs[idx] = m_diffEq.getX(idx);
      m_lhs[idx] = x0(idx) + m_deltaX(idx);
    }

    // Save the infinity norm
    m_infnorm = (m_lhs - m_rhs).lpNorm<Eigen::Infinity>();

    // Return the L2 norm
    return (m_lhs - m_rhs).norm();
  }

  bool isConverged()
  {
    m_residual = computeResidual();
    //std::cout << "atol " << m_residual << std::endl
    //          << "infnorm " << m_infnorm << std::endl
    //          << "rtol " << m_residual / m_initial_residual << std::endl
    //          << "stol " << m_increment.norm() << std::endl;
    // L2 norm of the residual is less than tolerance
    if ( m_residual < m_atol ) {
      //std::cout << "converged atol" << std::endl;
      return true;
    }
    // Infinity norm of residual is less than tolerance
    if ( m_infnorm < m_inftol ) {
      //std::cout << "converged inftol" << std::endl;
      return true;
    }
    if ( m_residual / m_initial_residual < m_rtol ) {
      //std::cout << "converged rtol" << std::endl;
      return true;
    }
    // L2 norm of change in solution at last step of solve is less than tolerance
    if ( m_increment.norm() < m_stol ) {
      //std::cout << "converged stol" << std::endl;
      return true;
    }
    return false;
  }

  bool& solve_for_dv() { return m_solve_for_dv; }

protected:

  bool velocity_solve()
  {
    bool successfull_solve = true;

    m_diffEq.startStep();

    resize();
    setZero();
    m_diffEq.getScriptedDofs(m_fixed, m_desired);

    #ifdef TIMING_ON
      m_rhs.setZero();
      m_diffEq.evaluatePDot(m_rhs);
      double timing_force_at_start_over_dt_timing = m_rhs.norm()/m_dt;
    #endif

    // Copy masses
    m_diffEq.getMass(m_mass);

    // copy start of step positions and velocities
    m_diffEq.getX(x0);
    m_diffEq.getV(v0);

    // computeResidual also sets m_rhs = F
    m_initial_residual = computeResidual();
    //std::cout << "initial residual: " << m_initial_residual << std::endl;
    int m_curit = 0;

    for (; m_curit < m_maxit; ++m_curit) 
    {
      START_TIMER("ImplicitEuler::velocity_solve/setup");
      m_rhs *= m_dt;
      m_diffEq.startIteration();
      m_diffEq.evaluatePDotDX(1.0, *m_A);
      m_A->finalize();
      m_A->scale(m_dt);

      if (m_curit == 0) 
      {
        m_A->finalize();
        m_A->multiply(m_rhs, m_dt, v0);
      } 
      else
      {
        m_rhs -= m_mass.array()*m_deltaV.array();
      }

      m_diffEq.evaluatePDotDV(1.0, *m_A);
      m_A->finalize();
      m_A->scale(-m_dt);

      for (int i = 0; i < m_ndof; ++i) {
        m_A->add(i, i, m_mass(i));
      }
      m_A->finalize();

      for (size_t i = 0; i < m_fixed.size(); ++i)
      {
        int idx = m_fixed[i];
        m_rhs(idx)
          = (m_desired[i] - x0[idx]) / m_dt - (v0[idx] + m_deltaV[idx]);
      }
      m_A->zeroRows(m_fixed);
      
      // Finalize the nonzero structure before the linear solve (for sparse matrices only)
      m_A->finalizeNonzeros();
      STOP_TIMER("ImplicitEuler::velocity_solve/setup");

      START_TIMER("ImplicitEuler::velocity_solve/solver");
      int status = m_solver->solve(m_increment, m_rhs);
      if( status < 0 )
      {
        successfull_solve = false;
        std::cerr << "\033[31;1mWARNING IN IMPLICITEULER:\033[m Problem during linear solve detected. " << std::endl;
      }
      STOP_TIMER("ImplicitEuler::velocity_solve/solver");

      START_TIMER("ImplicitEuler::velocity_solve/setup");
      m_deltaV += m_increment;

      m_diffEq.setV(v0+m_deltaV);
      m_diffEq.setX(x0+m_dt*(v0+m_deltaV));

      m_diffEq.endIteration();

      if (m_curit == m_maxit - 1)
      {
        STOP_TIMER("ImplicitEuler::velocity_solve/setup");
	break;
      }

      // check for convergence
      if ( isConverged() )
      {
        STOP_TIMER("ImplicitEuler::velocity_solve/setup"); 
        break;
      }

      m_increment.setZero();
      m_A->setZero();
      // Allow the nonzero structure to be modified again (for sparse matrices only)
      m_A->resetNonzeros();
      STOP_TIMER("ImplicitEuler::velocity_solve/setup");
    }

    //std::cout << "Iterations: " << m_curit << std::endl;
    if (m_curit == m_maxit - 1)
    {
      successfull_solve = false;
      std::cerr << "\033[31;1mWARNING IN IMPLICITEULER:\033[m Newton solver failed to converge in max iterations: " << m_maxit << std::endl;
    }

    #ifdef TIMING_ON
      std::string itrstrng = toString(m_curit+1);
      while( itrstrng.size() < 3 ) itrstrng = "0" + itrstrng;
      itrstrng = "NEWTON_SOLVES_OF_LENGTH_" + itrstrng;
      IntStatTracker::getIntTracker(itrstrng) += 1;

      PairVectorBase::insertPair( "ForcesVsIterations", std::pair<double,int>(timing_force_at_start_over_dt_timing,m_curit+1), PairVectorBase::StringPair("ForceNorm","ImplicitIterations") );
    #endif

    m_diffEq.endStep();
    
    return successfull_solve;
  }

  bool position_solve()
  {
    bool successfull_solve = true;

    m_diffEq.startStep();

    resize();
    setZero();
    m_diffEq.getScriptedDofs(m_fixed, m_desired);

    #ifdef TIMING_ON
      m_rhs.setZero();
      m_diffEq.evaluatePDot(m_rhs);
      double timing_force_at_start = m_rhs.norm();
    #endif

    // Copy masses
    m_diffEq.getMass(m_mass);

    // copy start of step positions and velocities
    m_diffEq.getX(x0);
    m_diffEq.getV(v0);

    // computeResidual also sets m_rhs = F
    m_initial_residual = computeResidual();
    //     m_increment[0] = 1;
    //     if ( isConverged() ) {
    //       m_diffEq.endStep();
    //       return;
    //     }
    //     m_increment.setZero();
    //std::cout << "initial residual: " << m_initial_residual << std::endl;
    int m_curit = 0;

    // m_rhs = m_dt^2 * F
    //m_rhs.setZero();
    //m_diffEq.evaluatePDot(m_rhs);
    //m_rhs *= (m_dt * m_dt);

    for (; m_curit < m_maxit; ++m_curit) 
    {
      START_TIMER("ImplicitEuler::position_solve/setup");
      // m_rhs = m_dt^2 * F
      m_rhs *= m_dt * m_dt;
      m_diffEq.startIteration();

      // m_A = M
      for (int i = 0; i < m_ndof; ++i) m_A->add(i,i,m_mass(i));
      //for (int i = 0; i < m_ndof; ++i) {
      //  m_A->add(i, i, m_diffEq.getMass(i));
      //}
      m_A->finalize();

      // m_A += -m_dt * dF/dv
      // (m_A = M - m_dt * dF/dv)
      m_diffEq.evaluatePDotDV(-m_dt, *m_A);
      m_A->finalize();

      if (m_curit == 0) 
      {
        // m_rhs += m_dt * m_A * v0
        // (m_rhs = m_dt^2 * F + m_dt * (M - m_dt * dF/dv) * v0)
        m_A->multiply(m_rhs, m_dt, v0);
      } 
      else 
      {
        // m_rhs += -m_dt * M * m_deltaV
        // m_rhs = m_dt^2 * F - m_dt * M * m_deltaV
        //for (int i = 0; i < m_diffEq.ndof(); ++i) {
        //  m_rhs[i] += -m_dt * m_diffEq.getMass(i) * m_deltaV(i);
        //}
        m_rhs += -m_dt*(m_mass.array()*m_deltaV.array());
      }

      // m_A += -m_dt^2 * dF/dx
      // (m_A = M - m_dt * dF/dv - m_dt^2 * dF/dx)
      m_diffEq.evaluatePDotDX(-m_dt*m_dt, *m_A);
      m_A->finalize();

      for (size_t i = 0; i < m_fixed.size(); ++i) {
        int idx = m_fixed[i];
        //assert( x0(idx)+m_deltaX(idx) == m_diffEq.getX(idx) );
        //m_rhs[idx] = m_desired[i] - m_diffEq.getX(idx);
        m_rhs[idx] = m_desired[i] - (x0(idx)+m_deltaX(idx));
      }
      m_A->zeroRows(m_fixed);

      // Finalize the nonzero structure before the linear solve (for sparse matrices only)
      m_A->finalizeNonzeros();
      STOP_TIMER("ImplicitEuler::position_solve/setup");

      START_TIMER("ImplicitEuler::position_solve/solver");
      int status = m_solver->solve(m_increment, m_rhs);
      if( status < 0 )
      {
        successfull_solve = false;
        std::cerr << "\033[31;1mWARNING IN IMPLICITEULER:\033[m Problem during linear solve detected. " << std::endl;
      }
      STOP_TIMER("ImplicitEuler::position_solve/solver");

      START_TIMER("ImplicitEuler::position_solve/setup");
      m_diffEq.increment_q(m_increment);
      m_deltaX += m_increment;
      m_diffEq.increment_qdot(m_deltaX / m_dt - v0 - m_deltaV);
      m_deltaV = m_deltaX / m_dt - v0;
      //       for (int i = 0; i < m_deltaV.size(); ++i)
      //         m_diffEq.setV(i, v0[i] + m_deltaV[i]);

      //       for (int i = 0; i < m_deltaV.size(); ++i) {
      //         m_diffEq.setV(i, v0(i) + m_deltaV(i));
      //         m_diffEq.setX(i, x0(i) + m_dt * m_diffEq.getV(i));
      //       }

      m_diffEq.endIteration();

      if (m_curit == m_maxit - 1)
      {
        STOP_TIMER("ImplicitEuler::position_solve/solver");
        break;
      }

      // check for convergence
      if ( isConverged() ) 
      {
        STOP_TIMER("ImplicitEuler::position_solve/solver");
        break;
      }

      m_increment.setZero();
      m_A->setZero();
      
      // Allow the nonzero structure to be modified again (for sparse matrices only)
      m_A->resetNonzeros();
      STOP_TIMER("ImplicitEuler::position_solve/solver");
    }

    //std::cout << "Iterations: " << m_curit << std::endl;
    if (m_curit == m_maxit - 1)
    {
      successfull_solve = false;
      std::cerr << "\033[31;1mWARNING IN IMPLICITEULER:\033[m Newton solver failed to converge in max iterations: " << m_maxit << std::endl;
    }

    #ifdef TIMING_ON
      std::string itrstrng = toString(m_curit+1);
      while( itrstrng.size() < 3 ) itrstrng = "0" + itrstrng;
      itrstrng = "NEWTON_SOLVES_OF_LENGTH_" + itrstrng;
      IntStatTracker::getIntTracker(itrstrng) += 1;

      PairVectorBase::insertPair( "ForcesVsIterations", std::pair<double,int>(timing_force_at_start,(m_curit+1)/m_dt), PairVectorBase::StringPair("ForceNorm","ImplicitIterations") );
    #endif
    
    m_diffEq.endStep();
    
    return successfull_solve;
  }

  ODE& m_diffEq;
  
  int m_ndof;

  VecXd m_mass;
  VecXd x0;
  VecXd v0;
  VecXd m_rhs;
  VecXd m_lhs;
  VecXd m_deltaX;
  VecXd m_deltaV;
  VecXd m_increment;

  IntArray m_fixed;
  std::vector<Scalar> m_desired;

  Scalar m_initial_residual;
  Scalar m_residual;
  int m_curit;

  bool m_solve_for_dv;

  MatrixBase* m_A;
  LinearSolverBase* m_solver;
};

} // namespace BASim

#endif // IMPLICITEULER_HH
