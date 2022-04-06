#pragma once

#include <vector>

#include "drake/common/drake_copyable.h"
#include "drake/multibody/contact_solvers/block_sparse_matrix.h"
#include "drake/multibody/contact_solvers/sap/partial_permutation.h"
#include "drake/multibody/contact_solvers/sap/sap_constraint.h"
#include "drake/multibody/contact_solvers/sap/sap_contact_problem.h"

namespace drake {
namespace multibody {
namespace contact_solvers {
namespace internal {

/* Given a contact problem, this class provides a representation for the entire
 "bundle" of constraints in the problem. This class re-arranges constraints
 according to the problem's graph in order to exploit structure of the problem
 and provides high level operations used by the SAP algorithm. With this
 abstraction, SAP is agnostic to the specific type of constraints in the
 problem, but it only operates on the bundle as a whole.

 More specifically, the i-th SAP constraint is defined by:
   1. A Jacobian mapping generalized velocities v to constraint velocities vᵢ,
      i.e. vᵢ = Jᵢ⋅v.
   2. Regularization Rᵢ and bias v̂ᵢ. "Unprojected" impulses yᵢ are computed
      according to yᵢ = −Rᵢ⁻¹⋅(vᵢ−v̂ᵢ).
   3. A convex set 𝒞ᵢ. Impulses γᵢ are constrained to live in 𝒞ᵢ.
   4. A projection operation γᵢ = Pᵢ(yᵢ) on 𝒞ᵢ.

 We concatenate vᵢ to form vector vc, γᵢ to form γ and yᵢ to form y. This
 concatenation is not performed in the original order constraints are declared
 in problem, but in the order dictated by the graph of the contact problem.
 The bundle's constraint set is defined as the Cartesian product 𝒞 =
 𝒞₁×𝒞₂×…×𝒞ₙ, with n the number of constraints. With these definitions, the
 bundle is given by:
   1. A Jacobian J mapping generalized velocities v to constraint velocities vc,
      i.e. vc = J⋅v.
   2. Regularization R and bias v̂, as the concatenation of individual Rᵢ and
      v̂ᵢ. Then y = −R⁻¹⋅(v−v̂) holds.
   3. Convex set 𝒞 = 𝒞₁×𝒞₂×…×𝒞ₙ.
   4. A projection operation γ = P(y) on 𝒞. Given the separable structure of
      the projection, we have that γ is the concatenation of individual γᵢ.

 For further details on the SAP formulation and the operations above, please
 refer to:
 - [Castro et al., 2021] Castro A., Permenter F. and Han X., 2021. An
   Unconstrained Convex Formulation of Compliant Contact. Available online at
   https://arxiv.org/abs/2110.10107.

@tparam_nonsymbolic_scalar */
template <typename T>
class SapConstraintBundle {
 public:
  DRAKE_NO_COPY_NO_MOVE_NO_ASSIGN(SapConstraintBundle);

  /* Constructs a bundle for the given `problem`.
   @param[in] problem This bundle keeps a reference to the constraints owned by
   `problem` and therefore it must outlive this object. An exception is thrown
   if nullptr.
   @param[in] delassus_diagonal It must have size problem.num_constraint() or an
   exception is thrown. The i-th entry stores the scaling parameter used for
   regularization estimation by the i-th constraint in `problem`, see
   SapConstraint::CalcDiagonalRegularization(). */
  SapConstraintBundle(const SapContactProblem<T>* problem,
                      const VectorX<T>& delassus_diagonal);

  /* Returns the number of constraints in this bundle. */
  int num_constraints() const;

  /* Returns the number of constraint equations in this bundle. This number
   equals the number of rows in the bundle's Jacobian. */
  int num_constraint_equations() const;

  /* Returns the Jacobian of the bundle. Rows correspond to constraint equations
   and columns correspond to generalized velocities of the contact problem
   supplied at construction.
   Rows (i.e. constraints) in the bundle's Jacobian are sorted according to the
   problem's graph; each cluster (edge) in the graph corresponds to a block row,
   with rows within this block row sorted in the order enumerated within the
   cluster (see ContactProblemGraph::ConstraintCluster::constraint_index()).
   Columns correspond to the generalized velocities of participating cliques
   only, see ContactProblemGraph::participating_cliques(); each participating
   clique corresponds to a block column, in the order enumerated by
   ContactProblemGraph::participating_cliques(). */
  const BlockSparseMatrix<T>& J() const { return J_; }

  /* Returns the diagonal regularization matrix R. Of size
   num_constraint_equations(). */
  const VectorX<T>& R() const { return R_; }

  /* Returns the diagonal of the inverse of the regularization matrix. Of size
   num_constraint_equations(). */
  const VectorX<T>& Rinv() const { return Rinv_; }

  /* Returns the bias v̂ for the entire bundle. Of size
   num_constraint_equations().*/
  const VectorX<T>& vhat() const { return vhat_; }

  /* Computes unprojected impulses y according to y = −R⁻¹⋅(v−v̂), where R is
   the regularization matrix, R(), and v̂ is the bias term, vhat().
   @pre vc.size() equals num_constraint_equations().
   @pre y != nullptr and y->size() equals num_constraint_equations(). */
  void CalcUnprojectedImpulses(const VectorX<T>& vc, VectorX<T>* y) const;

  /* Computes the projection γ = P(y) for all impulses and the gradient
   dP/dy if dPdy != nullptr. On output dPdy[i] stores the gradient dPᵢ/dyᵢ for
   the i-th constraint.
   @pre y.size() equals num_constraint_equations().
   @pre gamma != nullptr and gamma->size() equals num_constraint_equations().
   @pre if dPdy != nullptr, then dPdy->size() equals num_constraints(). */
  void ProjectImpulses(const VectorX<T>& y, VectorX<T>* gamma,
                       std::vector<MatrixX<T>>* dPdy = nullptr) const;

  /*  SAP's regularizer cost is defined as ℓᵣ = 1/2γᵀ⋅R⋅γ. The Hessian computed
   with respect to vc (defined as vc = J⋅v, see class's documentation) is a
   block diagonal matrix G where the i-th block diagonal entry corresponds to
   Gᵢ = dPᵢ/dyᵢ⋅Rᵢ⁻¹. This method computes γ = P(y) and Hessian matrix G.
   See Appendix E [Castro et al., 2021] for further details.
   @pre y.size() equals num_constraint_equations().
   @pre gamma != nullptr and gamma->size() equals num_constraint_equations().
   @pre G != nullptr and G->size() equals num_constraints(). */
  void ProjectImpulsesAndCalcConstraintsHessian(
      const VectorX<T>& y, VectorX<T>* gamma, std::vector<MatrixX<T>>* G) const;

 private:
  /* This method builds the BlockSparseMatrix representation of the Jacobian
   matrix for the given contact problem. For further details on its structure,
   refer to the documentation for the public accessor J(). */
  void MakeConstraintBundleJacobian(const SapContactProblem<T>& problem);

  BlockSparseMatrix<T> J_;
  VectorX<T> vhat_;
  VectorX<T> R_;
  VectorX<T> Rinv_;
  // Constraint references in the order dictated by the ContactProblemGraph.
  std::vector<const SapConstraint<T>*> constraints_;
};

}  // namespace internal
}  // namespace contact_solvers
}  // namespace multibody
}  // namespace drake
