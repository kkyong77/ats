/* -*-  mode: c++; indent-tabs-mode: nil -*- */

/*
  ATS

  EOS for salt water (does not implement viscosity at this point!)
  For this model dependcy on concentration is only assumed.

  Authors: Daniil Svyatsky (dasvyat@lanl.gov)
*/

#include "eos_factory.hh"
#include "eos_sw.hh"



namespace Amanzi {
namespace Relations {

EOS_SW::EOS_SW(Teuchos::ParameterList& eos_plist) : eos_plist_(eos_plist) {
  InitializeFromPlist_();
}

double EOS_SW::MassDensity(std::vector<double>& params) {

  double C = params[0];  
  return rho_f_ + E_*C;

};

double EOS_SW::DMassDensityDC(std::vector<double>& params) {
  return E_;
};

double EOS_SW::DMassDensityDT(std::vector<double>& params){
  return 0.;
}

double EOS_SW::DMassDensityDp(std::vector<double>& params){
  return 0.;
}    

double EOS_SW::MolarDensity(std::vector<double>& params) {
  double C = params[0];
  return MassDensity(params)/(M_water_*(1-C) + M_salt_*C);
};

double EOS_SW::DMolarDensityDC(std::vector<double>& params) {
  double C = params[0];

  double b = (M_water_*(1-C) + M_salt_*C);

  return (DMassDensityDC(params)*b - MassDensity(params)*(M_salt_ - M_water_)) / (b*b);
};    

double EOS_SW:: DMolarDensityDT(std::vector<double>& params){
  return 0.;
}

double EOS_SW:: DMolarDensityDp(std::vector<double>& params){
  return 0.;
}         

  
void EOS_SW::InitializeFromPlist_() {

  E_ = eos_plist_.get<double>("sea water density coefficient", 750);
  rho_f_ = eos_plist_.get<double>("fresh water mass density [kg/m^3]", 1000.0);
  if (eos_plist_.isParameter("salt molar mass [kg/mol]")){
    M_salt_ = eos_plist_.get<double>("salt molar mass [kg/mol]");
  }else{
    M_salt_ = eos_plist_.get<double>("salt molar mass [g/mol]", 58.5) * 1e-3;
  }

  if (eos_plist_.isParameter("water molar mass [kg/mol]")){
    M_water_ = eos_plist_.get<double>("water molar mass [kg/mol]");
  }else{
    M_water_ = eos_plist_.get<double>("water molar mass [g/mol]", 18.0153) * 1e-3;
  }

};

} // namespace
} // namespace
