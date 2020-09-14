/* -*-  mode++; indent-tabs-mode: nil -*- */

/* -------------------------------------------------------------------------
ATS

License: see $ATS_DIR/COPYRIGHT
Author: Svetlana Tokareva

Solves:

c rho dT/dt + q dot grad h = div Ke grad T + S
------------------------------------------------------------------------- */

#include "advection.hh"
#include "FieldEvaluator.hh"
#include "lake_thermo_pk.hh"
#include "Op.hh"

namespace Amanzi {
namespace LakeThermo {

// -------------------------------------------------------------
// Accumulation of energy term c rho dT/dt
// -------------------------------------------------------------
void Lake_Thermo_PK::AddAccumulation_(const Teuchos::Ptr<CompositeVector>& g) {
  double dt = S_next_->time() - S_inter_->time();

//  // update the temperature at both the old and new times.
//  S_next_->GetFieldEvaluator(temperature_key_)->HasFieldChanged(S_next_.ptr(), name_);
//  S_inter_->GetFieldEvaluator(temperature_key_)->HasFieldChanged(S_inter_.ptr(), name_);

  // get the energy at each time
  Teuchos::RCP<const CompositeVector> T1 = S_next_->GetFieldData(temperature_key_);
  Teuchos::RCP<const CompositeVector> T0 = S_inter_->GetFieldData(temperature_key_);

  std::cout << "check 1" << std::endl;
  std::cout << "density_key_ = " << density_key_ << std::endl;
  S_inter_->GetFieldEvaluator(density_key_)->HasFieldChanged(S_inter_.ptr(), name_);
  std::cout << "check 2" << std::endl;

  // evaluate density
  const Epetra_MultiVector& rho =
  *S_inter_->GetFieldData(density_key_)->ViewComponent("cell",false);

  std::cout << "In AddAccumulation_" << std::endl;

  double rho0 = 1000.;

  // Update the residual with the accumulation of energy over the
  // timestep, on cells.
  g->ViewComponent("cell", false)
    ->Update(cp_*rho0/dt, *T1->ViewComponent("cell", false),
          -cp_*rho0/dt, *T0->ViewComponent("cell", false), 1.0);


//  const Epetra_MultiVector& T1_c = *S_next_->GetFieldData(temperature_key_)->ViewComponent("cell", false);
//  const Epetra_MultiVector& T0_c = *S_inter_->GetFieldData(temperature_key_)->ViewComponent("cell", false);
//
//  const Epetra_MultiVector& g_c = *g->ViewComponent("cell", false);
//
//  int ncells_owned = mesh_->num_entities(AmanziMesh::CELL, AmanziMesh::Parallel_type::OWNED);
//
//  for (int c = 0; c < ncells_owned; c++) {
//      g_c[0][c] = cp_*rho[0][c]/dt*(T1_c[0][c] - T0_c[0][c]);
//      std::cout << "g_c[0][c] = " << g_c[0][c] << std::endl;
//      std::cout << "T1_c[0][c] = " << T1_c[0][c] << std::endl;
//      std::cout << "T0_c[0][c] = " << T0_c[0][c] << std::endl;
//      std::cout << "rho[0][c] = " << rho[0][c] << std::endl;
//      std::cout << "cp_ = " << cp_ << std::endl;
//  }

};


// -------------------------------------------------------------
// Advective term for transport of enthalpy, q dot grad h.
// -------------------------------------------------------------
void Lake_Thermo_PK::AddAdvection_(const Teuchos::Ptr<State>& S,
        const Teuchos::Ptr<CompositeVector>& g, bool negate) {

  // set up the operator

  // NOTE: fluxes are a MOLAR flux by choice of the flow pk, i.e.
  // [flux] =  mol/s

  // NOTE: this will be the eventual way to ensure it is up to date,
  // but there is no FieldEvaluator for darcy flux yet.  When there
  // is, we can take the evaluation out of Flow::commit_state(),
  // but for now we'll leave it there and assume it has been updated. --etc
  //  S->GetFieldEvaluator(flux_key_)->HasFieldChanged(S.ptr(), name_);
  Teuchos::RCP<const CompositeVector> flux = S->GetFieldData(flux_key_);

  S_inter_->GetFieldEvaluator(density_key_)->HasFieldChanged(S_inter_.ptr(), name_);

  // evaluate density
  const Epetra_MultiVector& rho =
  *S_inter_->GetFieldData(density_key_)->ViewComponent("cell",false);

  double dhdt = r_ - E_ - R_s_ - R_b_;
  double B_w  = r_ - E_;

  const Epetra_MultiVector& flux_c = *flux->ViewComponent("cell", false);

  int ncells_owned = mesh_->num_entities(AmanziMesh::CELL, AmanziMesh::Parallel_type::OWNED);

  double dt = S_next_->time() - S_inter_->time();

  // GENERALLY, DON'T UPDATE HERE
  // update h
  h_ += dhdt*dt;

  for (int c = 0; c < ncells_owned; c++) {
    const AmanziGeometry::Point& xc = mesh_->cell_centroid(c);
    flux_c[0][c] = cp_*rho[0][c]*(dhdt*xc[0] - B_w)/h_;
  }

  db_->WriteVector(" adv flux", flux.ptr(), true);
  matrix_adv_->global_operator()->Init();
  matrix_adv_->Setup(*flux);
  matrix_adv_->SetBCs(bc_adv_, bc_adv_);
  matrix_adv_->UpdateMatrices(flux.ptr());

  // apply to temperature
//  S->GetFieldEvaluator(temperature_key_)->HasFieldChanged(S.ptr(), name_);
  Teuchos::RCP<const CompositeVector> temp = S->GetFieldData(temperature_key_);
  ApplyDirichletBCsToEnthalpy_(S.ptr());
  matrix_adv_->ApplyBCs(false, true, false);


  // apply
  matrix_adv_->global_operator()->ComputeNegativeResidual(*temp, *g, false);
}

// -------------------------------------------------------------
// Diffusion term, div K grad T
// -------------------------------------------------------------
void Lake_Thermo_PK::ApplyDiffusion_(const Teuchos::Ptr<State>& S,
          const Teuchos::Ptr<CompositeVector>& g) {
  // update the thermal conductivity
  UpdateConductivityData_(S_next_.ptr());
  Teuchos::RCP<const CompositeVector> conductivity =
      S_next_->GetFieldData(uw_conductivity_key_);

//  const Epetra_MultiVector& uw_cond_c = *S_next_->GetFieldData(uw_conductivity_key_)->ViewComponent("face", false);
//  int nfaces_owned = mesh_->num_entities(AmanziMesh::FACE, AmanziMesh::Parallel_type::OWNED);
//
//  for (int f = 0; f < nfaces_owned; f++) {
//      std::cout << uw_cond_c[0][f] << std::endl;
//  }

  Teuchos::RCP<const CompositeVector> temp = S->GetFieldData(key_);

  std::cout << "key_ = " << key_ << std::endl;
  std::cout << "uw_conductivity_key_ = " << uw_conductivity_key_ << std::endl;

  // update the stiffness matrix
  matrix_diff_->global_operator()->Init();
  matrix_diff_->SetScalarCoefficient(conductivity, Teuchos::null);
  matrix_diff_->UpdateMatrices(Teuchos::null, temp.ptr());
  matrix_diff_->ApplyBCs(true, true, true);

  // update the flux
  Teuchos::RCP<CompositeVector> flux = S->GetFieldData(energy_flux_key_, name_);
  matrix_diff_->UpdateFlux(temp.ptr(), flux.ptr());

  // calculate the residual
  matrix_diff_->global_operator()->ComputeNegativeResidual(*temp, *g);
};


// ---------------------------------------------------------------------
// Add in energy source, which are accumulated by a single evaluator.
// ---------------------------------------------------------------------
void Lake_Thermo_PK::AddSources_(const Teuchos::Ptr<State>& S,
        const Teuchos::Ptr<CompositeVector>& g) {
  Teuchos::OSTab tab = vo_->getOSTab();

  // external sources of energy
  if (is_source_term_) {
    Epetra_MultiVector& g_c = *g->ViewComponent("cell",false);

    // Update the source term
//    S->GetFieldEvaluator(source_key_)->HasFieldChanged(S, name_);
//    const Epetra_MultiVector& source1 =
//        *S->GetFieldData(source_key_)->ViewComponent("cell",false);
    const Epetra_MultiVector& cv =
      *S->GetFieldData(Keys::getKey(domain_,"cell_volume"))->ViewComponent("cell",false);

    double dhdt = r_ - E_ - R_s_ - R_b_;

    S->GetFieldEvaluator(density_key_)->HasFieldChanged(S.ptr(), name_);

    // evaluate density
    const Epetra_MultiVector& rho =
    *S->GetFieldData(density_key_)->ViewComponent("cell",false);

    // get temperature
    const Epetra_MultiVector& temp = *S->GetFieldData(temperature_key_)
          ->ViewComponent("cell",false);

    // Add into residual
    unsigned int ncells = g_c.MyLength();
    for (unsigned int c=0; c!=ncells; ++c) {
      const AmanziGeometry::Point& xc = mesh_->cell_centroid(c);
      g_c[0][c] = S0_*exp(-alpha_e_*h_*xc[0])*(-alpha_e_*h_) + cp_*rho[0][c]*temp[0][c]*dhdt/h_;
    }

    if (vo_->os_OK(Teuchos::VERB_EXTREME))
      *vo_->os() << "Adding external source term" << std::endl;
    db_->WriteVector("  Q_ext", S->GetFieldData(source_key_).ptr(), false);
    db_->WriteVector("res (src)", g, false);
  }
}


void Lake_Thermo_PK::AddSourcesToPrecon_(const Teuchos::Ptr<State>& S, double h) {
  // external sources of energy (temperature dependent source)
  //if (is_source_term_ && is_source_term_differentiable_ && 
  //    S->GetFieldEvaluator(source_key_)->IsDependency(S, key_)) {
  //
  //  Teuchos::RCP<const CompositeVector> dsource_dT;
  //  if (!is_source_term_finite_differentiable_) {
  //    // evaluate the derivative through the dag
  //    S->GetFieldEvaluator(source_key_)->HasFieldDerivativeChanged(S, name_, key_);
  //    dsource_dT = S->GetFieldData(Keys::getDerivKey(source_key_, key_));
  //  } else {
  //    // evaluate the derivative through finite differences
  //    double eps = 1.e-8;
  //    S->GetFieldData(key_, name_)->Shift(eps);
  //    ChangedSolution();
  //    S->GetFieldEvaluator(source_key_)->HasFieldChanged(S, name_);
  //    auto dsource_dT_nc = Teuchos::rcp(new CompositeVector(*S->GetFieldData(source_key_)));
  //
  //    S->GetFieldData(key_, name_)->Shift(-eps);
  //    ChangedSolution();
  //    S->GetFieldEvaluator(source_key_)->HasFieldChanged(S, name_);
  //
  //    dsource_dT_nc->Update(-1/eps, *S->GetFieldData(source_key_), 1/eps);
  //    dsource_dT = dsource_dT_nc;
  //  }
  //  db_->WriteVector("  dQ_ext/dT", dsource_dT.ptr(), false);
  //  preconditioner_acc_->AddAccumulationTerm(*dsource_dT, -1.0, "cell", true);
  //}
}

// -------------------------------------------------------------
// Plug enthalpy into the boundary faces manually.
// This will be removed once boundary faces exist.
// -------------------------------------------------------------
void Lake_Thermo_PK::ApplyDirichletBCsToEnthalpy_(const Teuchos::Ptr<State>& S) {
  // put the boundary fluxes in faces for Dirichlet BCs.
  S->GetFieldEvaluator(enthalpy_key_)->HasFieldChanged(S, name_);
  
  const Epetra_MultiVector& enth_bf =
    *S->GetFieldData(enthalpy_key_)->ViewComponent("boundary_face",false);
  const Epetra_Map& vandalay_map = mesh_->exterior_face_map(false);
  const Epetra_Map& face_map = mesh_->face_map(false);
  
  int nbfaces = enth_bf.MyLength();
  for (int bf=0; bf!=nbfaces; ++bf) {
    AmanziMesh::Entity_ID f = face_map.LID(vandalay_map.GID(bf));

    if (bc_adv_->bc_model()[f] == Operators::OPERATOR_BC_DIRICHLET) {
      bc_adv_->bc_value()[f] = enth_bf[0][bf];
    }
  }
}
  

  
} //namespace LakeThermo
} //namespace Amanzi
