/**
 * \file SolverUtils.hh
 *
 * \author miklos@cs.columbia.edu
 * \date 11/17/2009
 */

#ifndef SOLVERUTILS_HH
#define SOLVERUTILS_HH

#include "BandMatrix.hh"
#include "ConjugateGradient.hh"

#include "BASim/src/Math/SimpleSparseMatrix.hh"

#ifdef HAVE_PETSC
#include "BASim/src/Math/Petsc/PetscMatrix.hh"
#include "BASim/src/Math/Petsc/PetscLinearSolver.hh"
#endif // HAVE_PETSC

#ifdef HAVE_PARDISO
#include "BASim/src/Math/PardisoMatrix.hh"
#include "BASim/src/Math/Pardiso/PardisoLinearSolver.hh"
#endif // HAVE_PARDISO

#ifdef HAVE_LAPACK
#ifdef WETA
#include "MKL/MKLLinearSolver.hh"
#else
#include "BASim/src/Math/MKL/MKLLinearSolver.hh"
#endif
#endif // HAVE_LAPACK

namespace BASim {

class SolverUtils
{
public:

  static SolverUtils* instance();

  // define solver types
  enum SolverType {
#ifdef HAVE_MKL
    PARDISO_SOLVER,
#endif // HAVE_MKL
#ifdef HAVE_PETSC
    PETSC_SOLVER,
#endif // HAVE_PETSC
#ifdef HAVE_LAPACK
    MKL_LINEAR_SOLVER,
#endif // HAVE_LAPACK
    CONJUGATE_GRADIENT,
    EIGEN_LDLT,
    AUTO_SOLVER
  };

  // define matrix types
  enum MatrixType {
#ifdef HAVE_PARDISO
    PARDISO_MATRIX,
#endif // HAVE_PETSC
#ifdef HAVE_PETSC
    PETSC_MATRIX,
#endif // HAVE_PETSC
    SIMPLE_SPARSE_MATRIX,
    EIGEN_SPARSE_MATRIX,
    BAND_MATRIX,
    AUTO_MATRIX
  };

  SolverType getSolverType() const;
  void setSolverType(SolverType t);

  MatrixType getMatrixType() const;
  void setMatrixType(MatrixType t);

  MatrixBase* createSparseMatrix(int rows, int cols, int nnzPerRow = 1) const;
  MatrixBase* createBandMatrix(int rows, int cols, int kl, int ku) const;

  LinearSolverBase* createLinearSolver(MatrixBase* A) const;

  std::string getSolverName() const;
private:

  SolverUtils()
    : solverType(AUTO_SOLVER)
    , matrixType(AUTO_MATRIX)
  {}

  SolverUtils(const SolverUtils&) {}
  SolverUtils& operator=(const SolverUtils&) { return *this; }

  static SolverUtils* m_instance;

  SolverType solverType;
  MatrixType matrixType;
};

} // namespace BASim

#endif // SOLVERUTILS_HH
