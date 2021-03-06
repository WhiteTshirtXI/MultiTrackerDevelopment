/**
 * \file EigenSparseMatrix.hh
 *
 * \author batty@cs.columbia.edu
 * \date 01/06/2011
 */

#ifndef EIGENSPARSEMATRIX_HH
#define EIGENSPARSEMATRIX_HH

#include "BASim/src/Math/MatrixBase.hh"
#include <Eigen/Sparse>

namespace BASim {

class EigenSparseMatrix : public MatrixBase
{
   
public:

  EigenSparseMatrix (int s);
  EigenSparseMatrix (int r, int c, int nnz = 1);
  EigenSparseMatrix (const EigenSparseMatrix& M);
  ~EigenSparseMatrix ();

  Scalar operator() (int i, int j) const;
  int set(int r, int c, Scalar val);
  int add(int r, int c, Scalar val);
  int add(const IntArray& rowIdx, const IntArray& colIdx, const MatXd& values);
  int add(const IndexArray& rowIdx, const IndexArray& colIdx,
          const MatXd& values);
  int scale(Scalar val);
  int setZero();
  int zeroRows(const IntArray& idx, Scalar diag = 1.0);
  int multiply(VecXd& y, Scalar s, const VecXd& x) const;

  //junk from the rods side of things - stubs for now
  void vertexStencilAdd(int start, const Eigen::Matrix<Scalar, 11, 11>& localJ) {}
  void edgeStencilAdd(int start, const Eigen::Matrix<Scalar, 6, 6>& localJ) {}
  void pointStencilAdd(int start, const Eigen::Matrix<Scalar, 3, 3>& localJ) {}

  int zeroCols(const IntArray& idx, Scalar diag);
  bool isApproxSymmetric( Scalar eps ) const;
  std::string name() const;

  int resetNonzeros();


  int finalize();
  int finalizeNonzeros();

  const Eigen::SparseMatrix<Scalar,Eigen::RowMajor>& getEigenMatrix() const { return m_dynamic; }
  Eigen::SparseMatrix<Scalar,Eigen::RowMajor>& getEigenMatrix() { return m_dynamic; }

protected:
  
  //build matrix as a list of triplets
  std::vector< Eigen::Triplet<Scalar> > m_triplets;

  //then convert to a sparse matrix
  Eigen::SparseMatrix<Scalar, Eigen::RowMajor> m_dynamic;

  bool m_pattern_fixed;
  
};

#include "EigenSparseMatrix.inl"

} // namespace BASim

#endif // EIGENSPARSEMATRIX_HH
