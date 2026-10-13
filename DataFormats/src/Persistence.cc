// FlatTable converters for the DataFormats products (makes them writable by NanoSkimWriter).
#include "Core/interface/Persistence.h"
#include "DataFormats/interface/HadronMatch.h"
#include "DataFormats/interface/ShowerVariables.h"

namespace harp {
namespace {

template <class T, class F>
std::vector<float> columnF(const std::vector<T>& v, F f) {
  std::vector<float> out;
  out.reserve(v.size());
  for (const auto& x : v) out.push_back(static_cast<float>(f(x)));
  return out;
}
template <class T, class F>
std::vector<int> columnI(const std::vector<T>& v, F f) {
  std::vector<int> out;
  out.reserve(v.size());
  for (const auto& x : v) out.push_back(static_cast<int>(f(x)));
  return out;
}

FlatTable hadronMatchTable(const HadronMatchCollection& v, const std::string& name) {
  FlatTable t(name, v.size());
#define HM_F(field, doc) t.addColumn(#field, columnF(v, [](const HadronMatch& m) { return m.field; }), doc)
#define HM_I(field, doc) t.addColumn(#field, columnI(v, [](const HadronMatch& m) { return m.field; }), doc)
  HM_I(genIndex, "index of the generated hadron");
  HM_I(pdgId, "pdgId of the generated hadron");
  HM_F(genEnergy, "generated energy [GeV]");
  HM_F(genPt, "generated pT [GeV]");
  HM_F(genEta, "generated eta");
  HM_F(genPhi, "generated phi");
  HM_I(simIndex, "index of the matched sim trackster");
  HM_F(simRawEnergy, "raw energy of the sim trackster");
  HM_F(simRegressedEnergy, "regressed energy of the sim trackster");
  HM_I(bestIndex, "index of the best matched reco trackster");
  HM_F(bestScore, "association score of the best trackster");
  HM_F(bestSharedEnergy, "shared energy with the best trackster");
  HM_F(bestRawEnergy, "raw energy of the best trackster");
  HM_F(bestRegressedEnergy, "regressed energy of the best trackster");
  HM_I(nLinked, "number of reco tracksters linked below the score cut");
  HM_F(linkedRawEnergy, "sum of raw energies of linked tracksters");
  HM_F(linkedRegressedEnergy, "sum of regressed energies of linked tracksters");
  HM_F(linkedSharedEnergy, "sum of shared energies of linked tracksters");
  HM_I(nTracksters, "number of reco tracksters in the event");
  HM_F(allRawEnergy, "sum of raw energies of all reco tracksters");
  HM_F(allRegressedEnergy, "sum of regressed energies of all reco tracksters");
  HM_F(totalLayerClusterEnergy, "sum of all layer cluster energies");
  HM_I(candidateIndex, "index of the TICL candidate containing the best trackster");
  HM_I(candidatePdgId, "pdgId assigned by TICL to the candidate");
  HM_F(candidateEnergy, "energy of the TICL candidate");
  HM_F(candidateRawEnergy, "raw energy of the TICL candidate");
  HM_I(candidateHasTrack, "1 if the candidate has a track");
#undef HM_F
#undef HM_I
  return t;
}

FlatTable showerVariablesTable(const ShowerVariablesCollection& v, const std::string& name) {
  FlatTable t(name, v.size());
#define SV_F(field, doc) t.addColumn(#field, columnF(v, [](const ShowerVariables& m) { return m.field; }), doc)
#define SV_I(field, doc) t.addColumn(#field, columnI(v, [](const ShowerVariables& m) { return m.field; }), doc)
  SV_I(nLayerClusters, "number of layer clusters");
  SV_I(nLayers, "number of distinct layers");
  SV_I(firstLayer, "first layer");
  SV_I(lastLayer, "last layer");
  SV_I(layerOfMaxEnergy, "layer with the maximum energy");
  SV_F(zStart, "|z| of the first layer cluster [cm]");
  SV_F(zEnd, "|z| of the last layer cluster [cm]");
  SV_F(length, "longitudinal extent [cm]");
  SV_F(energyEE, "layer cluster energy in CE-E");
  SV_F(energyHad, "layer cluster energy in CE-H");
  SV_F(fractionEE, "CE-E energy fraction");
  SV_F(emFraction, "raw_em_energy / raw_energy");
  SV_F(ev1, "PCA eigenvalue 1");
  SV_F(ev2, "PCA eigenvalue 2");
  SV_F(ev3, "PCA eigenvalue 3");
  SV_F(sigma2, "sqrt(ev2/ev1)");
  SV_F(sigma3, "sqrt(ev3/ev1)");
  SV_F(maxLayerEnergyFraction, "max layer energy / total");
  SV_F(rmsLayer, "energy weighted RMS of the layer number");
  SV_F(time, "trackster time");
#undef SV_F
#undef SV_I
  return t;
}

const bool registered = [] {
  PersistenceRegistry::instance().add<HadronMatchCollection>(hadronMatchTable);
  PersistenceRegistry::instance().add<ShowerVariablesCollection>(showerVariablesTable);
  return true;
}();

}  // namespace
}  // namespace harp
