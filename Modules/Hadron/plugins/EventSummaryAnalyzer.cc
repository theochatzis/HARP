// EventSummaryAnalyzer: basic control distributions of the input (gen
// spectrum, multiplicities). Useful as the first module of any hist job.
//
//   process.summary = Module("EventSummaryAnalyzer",
//       genParticles = "hgcalReader:genParticles", layerClusters = "hgcalReader:layerClusters",
//       tracksters = {"clue3d": "hgcalReader:clue3d"})
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/GenParticle.h"
#include "DataFormats/interface/LayerCluster.h"
#include "DataFormats/interface/Trackster.h"

#include <TH1D.h>
#include <TH2D.h>

#include <cmath>
#include <map>

namespace harp {

class EventSummaryAnalyzer : public Analyzer {
 public:
  EventSummaryAnalyzer(const ParameterSet& ps, ModuleContext& ctx) : Analyzer(ctx) {
    HistDir d = ctx.hists();
    gen_ = ps.get<std::string>("genParticles", "");
    if (!gen_.empty()) {
      ctx.consumesProduct(gen_);
      hGenE_ = d.book1D("h_gen_energy", "generated energy;E (GeV);particles", 100, 0, 250);
      hGenEta_ = d.book1D("h_gen_eta", "generated #eta;#eta;particles", 60, -3.5, 3.5);
      hGenPdg_ = d.book1D("h_gen_pdgId", "generated pdgId;pdgId;particles", 6001, -3000.5, 3000.5);
      hNGen_ = d.book1D("h_gen_n", "number of generated particles;N;events", 10, 0, 10);
    }
    lc_ = ps.get<std::string>("layerClusters", "");
    if (!lc_.empty()) {
      ctx.consumesProduct(lc_);
      hNLC_ = d.book1D("h_lc_n", "number of layer clusters;N;events", 100, 0, 1500);
      hLCLayer_ = d.book1D("h_lc_layer", "layer cluster layer (energy weighted);layer;energy (GeV)", 48, 0, 48);
    }
    for (const auto& kv : ps.get<std::map<std::string, std::string>>("tracksters", {})) {
      ctx.consumesProduct(kv.second);
      TrkHists h;
      h.label = kv.second;
      h.n = d.book1D("h_" + kv.first + "_n", kv.first + " multiplicity;N;events", 40, 0, 40);
      h.rawE = d.book1D("h_" + kv.first + "_rawEnergy", kv.first + " raw energy;E_{raw} (GeV);tracksters", 100, 0, 250);
      h.nLC = d.book1D("h_" + kv.first + "_nLC", kv.first + " layer clusters per trackster;N_{LC};tracksters", 100, 0, 200);
      trk_.push_back(h);
    }
  }

  void analyze(const Event& ev) override {
    if (!gen_.empty()) {
      const auto& gens = ev.get<GenParticleCollection>(gen_);
      hNGen_->Fill(gens.size());
      for (const auto& g : gens) { hGenE_->Fill(g.energy); hGenEta_->Fill(g.eta); hGenPdg_->Fill(g.pdgId); }
    }
    if (!lc_.empty()) {
      const auto& lcs = ev.get<LayerClusterCollection>(lc_);
      hNLC_->Fill(lcs.size());
      for (const auto& c : lcs) hLCLayer_->Fill(c.layer, c.energy);
    }
    for (auto& h : trk_) {
      const auto& ts = ev.get<TracksterCollection>(h.label);
      h.n->Fill(ts.size());
      for (const auto& t : ts) { h.rawE->Fill(t.rawEnergy); h.nLC->Fill(t.nLayerClusters()); }
    }
  }

 private:
  struct TrkHists { std::string label; TH1D *n, *rawE, *nLC; };
  std::string gen_, lc_;
  TH1D *hGenE_ = nullptr, *hGenEta_ = nullptr, *hGenPdg_ = nullptr, *hNGen_ = nullptr, *hNLC_ = nullptr, *hLCLayer_ = nullptr;
  std::vector<TrkHists> trk_;
};

}  // namespace harp
HARP_REGISTER_MODULE(harp::EventSummaryAnalyzer);
