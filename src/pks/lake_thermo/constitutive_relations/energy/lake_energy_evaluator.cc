/* -*-  mode: c++; indent-tabs-mode: nil -*- */

/* -----------------------------------------------------------------------------
ATS

Authors: Svetlana Tokareva (tokareva@lanl.gov)

FieldEvaluator for water density.
----------------------------------------------------------------------------- */


#include "lake_energy_evaluator.hh"

namespace Amanzi {
namespace LakeThermo {

LakeEnergyEvaluator::LakeEnergyEvaluator(Teuchos::ParameterList& plist) :
    SecondaryVariableFieldEvaluator(plist) {
  if (my_key_.empty()) {

    my_key_ = plist_.get<std::string>("energy key", "surface-energy");
  }

  // Set up my dependencies.
  std::string domain_name = Keys::getDomain(my_key_);

  // -- temperature
  temperature_key_ = Keys::readKey(plist_, domain_name, "temperature", "temperature");
  dependencies_.insert(temperature_key_);

};

LakeEnergyEvaluator::LakeEnergyEvaluator(const LakeEnergyEvaluator& other) :
    SecondaryVariableFieldEvaluator(other),
    temperature_key_(other.temperature_key_) {};

Teuchos::RCP<FieldEvaluator>
LakeEnergyEvaluator::Clone() const {
  return Teuchos::rcp(new LakeEnergyEvaluator(*this));
};


void LakeEnergyEvaluator::EvaluateField_(const Teuchos::Ptr<State>& S,
        const Teuchos::Ptr<CompositeVector>& result) {
  Teuchos::RCP<const CompositeVector> temp = S->GetFieldData(temperature_key_);

  double rho0 = 1000.;
  double cp = 4184.;

  for (CompositeVector::name_iterator comp=result->begin();
       comp!=result->end(); ++comp) {
    const Epetra_MultiVector& temp_v = *temp->ViewComponent(*comp,false);
    Epetra_MultiVector& result_v = *result->ViewComponent(*comp,false);

    int ncomp = result->size(*comp, false);
    for (int i=0; i!=ncomp; ++i) {
      double T = temp_v[0][i];
      result_v[0][i] = rho0*cp*T;
    }
  }
};


void LakeEnergyEvaluator::EvaluateFieldPartialDerivative_(const Teuchos::Ptr<State>& S,
        Key wrt_key, const Teuchos::Ptr<CompositeVector>& result) {
  if (wrt_key == temperature_key_) {
    Teuchos::RCP<const CompositeVector> temp = S->GetFieldData(temperature_key_);

    double rho0 = 1000.;
    double cp = 4184.;

    for (CompositeVector::name_iterator comp=result->begin();
         comp!=result->end(); ++comp) {
      const Epetra_MultiVector& temp_v = *temp->ViewComponent(*comp,false);
      Epetra_MultiVector& result_v = *result->ViewComponent(*comp,false);

      int ncomp = result->size(*comp, false);
      for (int i=0; i!=ncomp; ++i) {
        double T = temp_v[0][i];
        result_v[0][i] = rho0*cp;
      }
    }
  }
};


} //namespace
} //namespace