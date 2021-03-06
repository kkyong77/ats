/* -*-  mode: c++; indent-tabs-mode: nil -*- */
/*
 Authors: Daniil Svyatsky (dasvyat@lanl.gov)
*/


#include "biomass_evaluator.hh"

namespace Amanzi {

// registry of method
Utils::RegisteredFactory<FieldEvaluator,BiomassEvaluator> BiomassEvaluator::factory_("biomass");


} // namespace
