/* -*-  mode: c++; indent-tabs-mode: nil -*- */

/*
  Interface for a thermal conductivity model with three phases.

  License: BSD
  Authors: Ethan Coon (ecoon@lanl.gov)
*/

#ifndef AMANZI_ENERGY_RELATIONS_TC_THREEPHASE_EVALUATOR_HH_
#define AMANZI_ENERGY_RELATIONS_TC_THREEPHASE_EVALUATOR_HH_

#include "secondary_variable_field_evaluator.hh"
#include "thermal_conductivity_threephase.hh"

namespace Amanzi {
namespace Energy {

// Equation of State model
class ThermalConductivityThreePhaseEvaluator :
    public SecondaryVariableFieldEvaluator {

 public:

  typedef std::pair<std::string,Teuchos::RCP<ThermalConductivityThreePhase> > RegionModelPair;

  // constructor format for all derived classes
  ThermalConductivityThreePhaseEvaluator(Teuchos::ParameterList& plist);
  ThermalConductivityThreePhaseEvaluator(const ThermalConductivityThreePhaseEvaluator& other);

  Teuchos::RCP<FieldEvaluator> Clone() const;

  // Required methods from SecondaryVariableFieldModel
  virtual void EvaluateField_(const Teuchos::Ptr<State>& S,
          const Teuchos::Ptr<CompositeVector>& result);
  virtual void EvaluateFieldPartialDerivative_(const Teuchos::Ptr<State>& S,
          Key wrt_key, const Teuchos::Ptr<CompositeVector>& result);

 protected:
  
  std::vector<RegionModelPair> tcs_;

  // Keys for fields
  // dependencies
  Key poro_key_;
  Key sat_key_;
  Key sat2_key_;
  Key temp_key_;
};

} // namespace
} // namespace

#endif
