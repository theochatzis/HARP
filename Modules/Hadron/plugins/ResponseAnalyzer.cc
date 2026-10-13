// ResponseAnalyzer: energy response R = E_reco / E_gen versus the true hadron
// energy, for several reco energy definitions, per hadron type.
// Histograms (per definition <d>, in <label>/<hadron>/):
//   h2_<d>_vs_E     TH2D  E_gen (configurable bins) vs R      <- input of the money plot
//   p_<d>_vs_E      TProfile <R> vs E_gen
//   h2_<d>_vs_eta   TH2D  |eta_gen| vs R
//   h_<d>           TH1D  R integrated
//
//   process.respCLUE3D = Module("ResponseAnalyzer",
//       matches = "matchCLUE3D",
//       definitions = ["bestRaw", "bestRegressed", "linkedRaw", "linkedRegressed", "allRaw", "allRegressed", "simRaw", "candidate", "layerClusters"],
//       energyBins = [1,2,3,5,7,10,15,20,30,40,50,70,100,130,160,200],
//       responseBins = [200, 0, 2], perHadronType = True, requireMatch = True)
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/HadronMatch.h"
#include "Modules/Hadron/interface/Binning.h"

#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>

#include <cmath>
#include <functional>
#include <map>

namespace harp {

class ResponseAnalyzer : public Analyzer {
 public:
  ResponseAnalyzer(const ParameterSet& ps, ModuleContext& ctx)
      : Analyzer(ctx), matches_(ctx.consumesProduct(ps.get<std::string>("matches"))), hists_(ctx.hists()) {
    defs_ = ps.get<std::vector<std::string>>("definitions", {"bestRaw", "bestRegressed", "linkedRaw", "allRaw", "simRaw"});
    eBins_ = energyBinsFromConfig(ps);
    std::vector<double> rb = ps.get<std::vector<double>>("responseBins", {200, 0, 2});
    nR_ = static_cast<int>(rb.at(0)); rLo_ = rb.at(1); rHi_ = rb.at(2);
    perType_ = ps.get<bool>("perHadronType", true);
    requireMatch_ = ps.get<bool>("requireMatch", true);
    for (const auto& d : defs_) getter(d);  // validate names early
  }

  void analyze(const Event& ev) override {
    for (const HadronMatch& m : ev.get<HadronMatchCollection>(matches_)) {
      if (m.genEnergy <= 0) continue;
      if (requireMatch_ && !m.hasReco()) continue;
      Hists& h = histsFor(m.pdgId);
      for (std::size_t i = 0; i < defs_.size(); ++i) {
        const double r = getter(defs_[i])(m) / m.genEnergy;
        h.vsE[i]->Fill(m.genEnergy, r);
        h.prof[i]->Fill(m.genEnergy, r);
        h.vsEta[i]->Fill(std::fabs(m.genEta), r);
        h.h1[i]->Fill(r);
      }
    }
  }

 private:
  using Getter = std::function<double(const HadronMatch&)>;
  static Getter getter(const std::string& name) {
    static const std::map<std::string, Getter> table = {
        {"bestRaw", [](const HadronMatch& m) { return m.bestRawEnergy; }},
        {"bestRegressed", [](const HadronMatch& m) { return m.bestRegressedEnergy; }},
        {"linkedRaw", [](const HadronMatch& m) { return m.linkedRawEnergy; }},
        {"linkedRegressed", [](const HadronMatch& m) { return m.linkedRegressedEnergy; }},
        {"allRaw", [](const HadronMatch& m) { return m.allRawEnergy; }},
        {"allRegressed", [](const HadronMatch& m) { return m.allRegressedEnergy; }},
        {"simRaw", [](const HadronMatch& m) { return m.simRawEnergy; }},
        {"simRegressed", [](const HadronMatch& m) { return m.simRegressedEnergy; }},
        {"candidate", [](const HadronMatch& m) { return m.candidateEnergy; }},
        {"candidateRaw", [](const HadronMatch& m) { return m.candidateRawEnergy; }},
        {"layerClusters", [](const HadronMatch& m) { return m.totalLayerClusterEnergy; }},
    };
    auto it = table.find(name);
    if (it == table.end()) {
      std::string msg = "ResponseAnalyzer: unknown response definition '" + name + "'. Known:";
      for (const auto& kv : table) msg += " " + kv.first;
      throw std::runtime_error(msg);
    }
    return it->second;
  }

  struct Hists {
    std::vector<TH2D*> vsE, vsEta;
    std::vector<TProfile*> prof;
    std::vector<TH1D*> h1;
  };
  Hists& histsFor(int pdgId) {
    const std::string key = hadronKey(pdgId, perType_);
    auto it = hists_by_type_.find(key);
    if (it != hists_by_type_.end()) return it->second;
    HistDir d = hists_.subdir(key);
    Hists h;
    for (const auto& def : defs_) {
      const std::string t = def + " response (" + key + ")";
      h.vsE.push_back(d.book2D("h2_" + def + "_vs_E", t + ";E_{gen} (GeV);E_{reco}/E_{gen}", eBins_, nR_, rLo_, rHi_));
      h.prof.push_back(d.bookProfile("p_" + def + "_vs_E", t + ";E_{gen} (GeV);<E_{reco}/E_{gen}>", eBins_));
      h.vsEta.push_back(d.book2D("h2_" + def + "_vs_eta", t + ";|#eta_{gen}|;E_{reco}/E_{gen}", 16, 1.5, 3.1, nR_, rLo_, rHi_));
      h.h1.push_back(d.book1D("h_" + def, t + ";E_{reco}/E_{gen};hadrons", nR_, rLo_, rHi_));
    }
    return hists_by_type_.emplace(key, h).first->second;
  }

  std::string matches_;
  HistDir hists_;
  std::vector<std::string> defs_;
  std::vector<double> eBins_;
  int nR_; double rLo_, rHi_;
  bool perType_, requireMatch_;
  std::map<std::string, Hists> hists_by_type_;
};

}  // namespace harp
HARP_REGISTER_MODULE(harp::ResponseAnalyzer);
