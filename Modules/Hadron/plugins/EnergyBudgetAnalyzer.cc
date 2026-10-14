// EnergyBudgetAnalyzer: where does the layer-cluster energy of a hadron end up?
// Every layer cluster of the event is classified by the trackster collection:
//   best  : (fraction of) its energy belongs to the best-matched trackster
//   other : belongs to any other trackster
//   none  : belongs to no trackster at all
// (a layer cluster shared by several tracksters contributes 1/multiplicity to each).
// Histograms in <label>/<hadron>/:
//   p_frac_<cat>_vs_E            TProfile  E_cat / E_LC(total)   mean fraction per E_gen bin (stacks to 1)
//   p_resp_<cat>_vs_E            TProfile  E_cat / E_gen         (stacks to the layer-cluster response)
//   p_frac_<EE|HAD>_<cat>_vs_E   TProfile  compartment split of the fraction (6 profiles stack to 1)
//   h2_layer_<cat>_vs_E          TH2 (x=E_gen, y=layer, weight=energy)  energy per layer and category
//   h_lcEnergy_<cat>             TH1 (log bins, weight=energy)   which layer-cluster energies are lost
//   h_lcN_<cat>                  TH1 (log bins)                   same, counting layer clusters
//   h_nHadrons_vs_E              TH1  hadrons per E_gen bin (normalisation of the layer plots)
//
//   process.budgetCLUE3D = Module("EnergyBudgetAnalyzer", matches="matchCLUE3D",
//       tracksters="hgcalReader:clue3d", layerClusters="hgcalReader:layerClusters", energyBins=[...])
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/HGCalLayers.h"
#include "DataFormats/interface/HadronMatch.h"
#include "DataFormats/interface/LayerCluster.h"
#include "DataFormats/interface/Trackster.h"
#include "Modules/Hadron/interface/Binning.h"

#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>

#include <array>
#include <cmath>
#include <map>

namespace harp {

class EnergyBudgetAnalyzer : public Analyzer {
 public:
  EnergyBudgetAnalyzer(const ParameterSet& ps, ModuleContext& ctx)
      : Analyzer(ctx), matches_(ctx.consumesProduct(ps.get<std::string>("matches"))),
        tracksters_(ctx.consumesProduct(ps.get<std::string>("tracksters"))),
        lc_(ctx.consumesProduct(ps.get<std::string>("layerClusters"))), hists_(ctx.hists()) {
    eBins_ = energyBinsFromConfig(ps);
    perType_ = ps.get<bool>("perHadronType", true);
    // log-spaced layer-cluster energy bins 0.01 .. 200 GeV
    const int n = 43;
    for (int i = 0; i <= n; ++i) lcBins_.push_back(0.01 * std::pow(200. / 0.01, double(i) / n));
  }

  void analyze(const Event& ev) override {
    const auto& matches = ev.get<HadronMatchCollection>(matches_);
    const auto& tracksters = ev.get<TracksterCollection>(tracksters_);
    const auto& lcs = ev.get<LayerClusterCollection>(lc_);
    for (const HadronMatch& m : matches) {
      if (m.genEnergy <= 0) continue;
      // fraction of each layer cluster's energy in the best / other tracksters
      std::vector<float> fBest(lcs.size(), 0.f), fOther(lcs.size(), 0.f);
      for (std::size_t it = 0; it < tracksters.size(); ++it) {
        const Trackster& t = tracksters[it];
        std::vector<float>& target = (static_cast<int>(it) == m.bestIndex) ? fBest : fOther;
        for (std::size_t k = 0; k < t.vertices.size(); ++k) {
          const unsigned v = t.vertices[k];
          if (v >= lcs.size()) continue;
          const float mult = (k < t.vertexMultiplicity.size() && t.vertexMultiplicity[k] > 0) ? t.vertexMultiplicity[k] : 1.f;
          target[v] += 1.f / mult;
        }
      }
      Hists& h = histsFor(m.pdgId);
      std::array<double, 3> e{0, 0, 0};             // best, other, none
      std::array<std::array<double, 3>, 2> eComp{};  // [EE, HAD][cat]
      double total = 0;
      for (std::size_t i = 0; i < lcs.size(); ++i) {
        const LayerCluster& c = lcs[i];
        const float fb = std::min(1.f, fBest[i]);
        const float fo = std::min(1.f - fb, fOther[i]);
        const float fn = std::max(0.f, 1.f - fb - fo);
        const std::array<float, 3> f{fb, fo, fn};
        const int comp = HGCalLayers::isEE(c.layer) ? 0 : 1;
        total += c.energy;
        for (int cat = 0; cat < 3; ++cat) {
          if (f[cat] <= 0) continue;
          const double ec = f[cat] * c.energy;
          e[cat] += ec;
          eComp[comp][cat] += ec;
          h.layer[cat]->Fill(m.genEnergy, c.layer, ec);
          h.lcEnergy[cat]->Fill(c.energy, ec);
          h.lcN[cat]->Fill(c.energy, f[cat]);
        }
      }
      if (total <= 0) continue;
      h.nHadrons->Fill(m.genEnergy);
      for (int cat = 0; cat < 3; ++cat) {
        h.frac[cat]->Fill(m.genEnergy, e[cat] / total);
        h.resp[cat]->Fill(m.genEnergy, e[cat] / m.genEnergy);
        h.fracEE[cat]->Fill(m.genEnergy, eComp[0][cat] / total);
        h.fracHad[cat]->Fill(m.genEnergy, eComp[1][cat] / total);
      }
    }
  }

 private:
  static constexpr const char* kCat[3] = {"best", "other", "none"};
  static constexpr const char* kCatTitle[3] = {"best trackster", "other tracksters", "no trackster"};
  struct Hists {
    std::array<TProfile*, 3> frac, resp, fracEE, fracHad;
    std::array<TH2D*, 3> layer;
    std::array<TH1D*, 3> lcEnergy, lcN;
    TH1D* nHadrons = nullptr;
  };
  Hists& histsFor(int pdgId) {
    const std::string key = hadronKey(pdgId, perType_);
    auto it = hists_by_type_.find(key);
    if (it != hists_by_type_.end()) return it->second;
    HistDir d = hists_.subdir(key);
    Hists h;
    h.nHadrons = d.book1D("h_nHadrons_vs_E", "hadrons with layer-cluster energy;E_{gen} (GeV);Hadrons", eBins_);
    for (int c = 0; c < 3; ++c) {
      const std::string n = kCat[c], t = kCatTitle[c];
      h.frac[c] = d.bookProfile("p_frac_" + n + "_vs_E", "fraction of layer-cluster energy in " + t + ";E_{gen} (GeV);Fraction of E_{LC}", eBins_);
      h.resp[c] = d.bookProfile("p_resp_" + n + "_vs_E", "layer-cluster energy in " + t + " / E_{gen};E_{gen} (GeV);E / E_{gen}", eBins_);
      h.fracEE[c] = d.bookProfile("p_frac_EE_" + n + "_vs_E", "CE-E energy fraction in " + t + ";E_{gen} (GeV);Fraction of E_{LC}", eBins_);
      h.fracHad[c] = d.bookProfile("p_frac_HAD_" + n + "_vs_E", "CE-H energy fraction in " + t + ";E_{gen} (GeV);Fraction of E_{LC}", eBins_);
      h.layer[c] = d.book2D("h2_layer_" + n + "_vs_E", "energy per layer in " + t + ";E_{gen} (GeV);Layer", eBins_, 47, 0.5, 47.5);
      h.lcEnergy[c] = d.book1D("h_lcEnergy_" + n, "energy-weighted layer-cluster energy in " + t + ";E_{LC} (GeV);Energy (GeV)", lcBins_);
      h.lcN[c] = d.book1D("h_lcN_" + n, "layer clusters in " + t + ";E_{LC} (GeV);Layer clusters", lcBins_);
    }
    return hists_by_type_.emplace(key, h).first->second;
  }

  std::string matches_, tracksters_, lc_;
  HistDir hists_;
  std::vector<double> eBins_, lcBins_;
  bool perType_;
  std::map<std::string, Hists> hists_by_type_;
};

}  // namespace harp
HARP_REGISTER_MODULE(harp::EnergyBudgetAnalyzer);
