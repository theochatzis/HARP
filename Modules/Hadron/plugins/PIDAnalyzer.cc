// PIDAnalyzer: particle-identification inputs. For the best matched reco
// trackster of each hadron, fills shower variables versus the true energy
// (TH2, one per variable) per hadron type, plus the TICL candidate pdgId
// assignment, so that distributions can be compared across hadron species in
// energy slices before building a classifier.
// Histograms in <label>/<hadron>/: h2_<var>_vs_E, h_<var>, h2_candPdg_vs_E
//
//   process.pidCLUE3D = Module("PIDAnalyzer",
//       matches = "matchCLUE3D", showerVariables = "showerCLUE3D",
//       energyBins = [...], perHadronType = True)
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/HadronMatch.h"
#include "DataFormats/interface/ShowerVariables.h"
#include "Modules/Hadron/interface/Binning.h"

#include <TH1D.h>
#include <TH2D.h>

#include <functional>
#include <map>

namespace harp {

class PIDAnalyzer : public Analyzer {
 public:
  PIDAnalyzer(const ParameterSet& ps, ModuleContext& ctx)
      : Analyzer(ctx), matches_(ctx.consumesProduct(ps.get<std::string>("matches"))),
        shower_(ctx.consumesProduct(ps.get<std::string>("showerVariables"))), hists_(ctx.hists()) {
    eBins_ = energyBinsFromConfig(ps);
    perType_ = ps.get<bool>("perHadronType", true);
  }

  void analyze(const Event& ev) override {
    const auto& matches = ev.get<HadronMatchCollection>(matches_);
    const auto& shower = ev.get<ShowerVariablesCollection>(shower_);
    for (const HadronMatch& m : matches) {
      if (!m.hasReco() || static_cast<std::size_t>(m.bestIndex) >= shower.size()) continue;
      const ShowerVariables& v = shower[m.bestIndex];
      Hists& h = histsFor(m.pdgId);
      for (std::size_t i = 0; i < variables().size(); ++i) {
        const double x = variables()[i].get(v);
        h.vsE[i]->Fill(m.genEnergy, x);
        h.h1[i]->Fill(x);
      }
      h.nLinkedVsE->Fill(m.genEnergy, m.nLinked);
      h.nTrkVsE->Fill(m.genEnergy, m.nTracksters);
      h.candPdgVsE->Fill(m.genEnergy, candCategory(m.candidatePdgId));
    }
  }

 private:
  struct Var {
    std::string name, title;
    int nbins; double lo, hi;
    std::function<double(const ShowerVariables&)> get;
  };
  static const std::vector<Var>& variables() {
    static const std::vector<Var> v = {
        {"nLayerClusters", "number of layer clusters", 100, 0, 200, [](const ShowerVariables& s) { return s.nLayerClusters; }},
        {"nLayers", "number of layers", 48, 0, 48, [](const ShowerVariables& s) { return s.nLayers; }},
        {"firstLayer", "first layer", 48, 0, 48, [](const ShowerVariables& s) { return s.firstLayer; }},
        {"lastLayer", "last layer", 48, 0, 48, [](const ShowerVariables& s) { return s.lastLayer; }},
        {"layerOfMaxEnergy", "layer of maximum energy", 48, 0, 48, [](const ShowerVariables& s) { return s.layerOfMaxEnergy; }},
        {"length", "longitudinal length (cm)", 100, 0, 200, [](const ShowerVariables& s) { return s.length; }},
        {"fractionEE", "CE-E energy fraction", 50, 0, 1.0001, [](const ShowerVariables& s) { return s.fractionEE; }},
        {"emFraction", "EM energy fraction", 50, 0, 1.0001, [](const ShowerVariables& s) { return s.emFraction; }},
        {"sigma2", "#sqrt{EV2/EV1}", 50, 0, 1, [](const ShowerVariables& s) { return s.sigma2; }},
        {"sigma3", "#sqrt{EV3/EV1}", 50, 0, 1, [](const ShowerVariables& s) { return s.sigma3; }},
        {"maxLayerEnergyFraction", "max layer energy fraction", 50, 0, 1.0001, [](const ShowerVariables& s) { return s.maxLayerEnergyFraction; }},
        {"rmsLayer", "longitudinal RMS (layers)", 50, 0, 15, [](const ShowerVariables& s) { return s.rmsLayer; }},
    };
    return v;
  }
  // 0: none, 1: photon(22), 2: electron(11), 3: muon(13), 4: charged hadron(211), 5: neutral hadron(130), 6: other
  static double candCategory(int pdg) {
    const int a = pdg < 0 ? -pdg : pdg;
    if (pdg == 0) return 0.5;
    if (a == 22) return 1.5;
    if (a == 11) return 2.5;
    if (a == 13) return 3.5;
    if (a == 211) return 4.5;
    if (a == 130) return 5.5;
    return 6.5;
  }
  struct Hists {
    std::vector<TH2D*> vsE;
    std::vector<TH1D*> h1;
    TH2D* nLinkedVsE = nullptr; TH2D* nTrkVsE = nullptr; TH2D* candPdgVsE = nullptr;
  };
  Hists& histsFor(int pdgId) {
    const std::string key = hadronKey(pdgId, perType_);
    auto it = hists_by_type_.find(key);
    if (it != hists_by_type_.end()) return it->second;
    HistDir d = hists_.subdir(key);
    Hists h;
    for (const auto& v : variables()) {
      h.vsE.push_back(d.book2D("h2_" + v.name + "_vs_E", v.title + " (" + key + ");E_{gen} (GeV);" + v.title, eBins_, v.nbins, v.lo, v.hi));
      h.h1.push_back(d.book1D("h_" + v.name, v.title + " (" + key + ");" + v.title + ";tracksters", v.nbins, v.lo, v.hi));
    }
    h.nLinkedVsE = d.book2D("h2_nLinked_vs_E", "linked tracksters;E_{gen} (GeV);N_{linked}", eBins_, 20, 0, 20);
    h.nTrkVsE = d.book2D("h2_nTracksters_vs_E", "tracksters in event;E_{gen} (GeV);N_{tracksters}", eBins_, 40, 0, 40);
    h.candPdgVsE = d.book2D("h2_candPdg_vs_E", "TICL candidate id;E_{gen} (GeV);category", eBins_, 7, 0, 7);
    const char* labels[7] = {"none", "#gamma", "e", "#mu", "h^{#pm}", "h^{0}", "other"};
    for (int i = 0; i < 7; ++i) h.candPdgVsE->GetYaxis()->SetBinLabel(i + 1, labels[i]);
    return hists_by_type_.emplace(key, h).first->second;
  }

  std::string matches_, shower_;
  HistDir hists_;
  std::vector<double> eBins_;
  bool perType_;
  std::map<std::string, Hists> hists_by_type_;
};

}  // namespace harp
HARP_REGISTER_MODULE(harp::PIDAnalyzer);
