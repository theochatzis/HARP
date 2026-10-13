// MatchingQAAnalyzer: quality of the reco <-> sim matching. Sim-to-reco
// quantities from the HadronMatch (how many tracksters contain the hadron,
// best score, energy captured) and reco-to-sim purity for every reco
// trackster (best sim score, fraction of "correctly matched" tracksters).
// Histograms in <label>/<hadron>/:
//   h2_nLinked_vs_E, h2_bestScore_vs_E, h2_bestSharedFrac_vs_E, h2_linkedSharedFrac_vs_E,
//   h2_bestRawOverSim_vs_E, h2_nTracksters_vs_E, h_recoScore, h2_recoScore_vs_rawE, h_recoMatched
//
//   process.qaCLUE3D = Module("MatchingQAAnalyzer",
//       matches = "matchCLUE3D", tracksters = "hgcalReader:clue3d",
//       recoToSim = "hgcalReader:clue3d2simCP", scoreCut = 0.6)
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/Association.h"
#include "DataFormats/interface/HadronMatch.h"
#include "DataFormats/interface/Trackster.h"
#include "Modules/Hadron/interface/Binning.h"

#include <TH1D.h>
#include <TH2D.h>

#include <map>

namespace harp {

class MatchingQAAnalyzer : public Analyzer {
 public:
  MatchingQAAnalyzer(const ParameterSet& ps, ModuleContext& ctx)
      : Analyzer(ctx), matches_(ctx.consumesProduct(ps.get<std::string>("matches"))),
        tracksters_(ps.get<std::string>("tracksters", "")), recoToSim_(ps.get<std::string>("recoToSim", "")),
        scoreCut_(ps.get<double>("scoreCut", 0.6)), hists_(ctx.hists()) {
    if (!tracksters_.empty()) ctx.consumesProduct(tracksters_);
    if (!recoToSim_.empty()) ctx.consumesProduct(recoToSim_);
    eBins_ = energyBinsFromConfig(ps);
    perType_ = ps.get<bool>("perHadronType", true);
  }

  void analyze(const Event& ev) override {
    const auto& matches = ev.get<HadronMatchCollection>(matches_);
    const TracksterCollection* recos = tracksters_.empty() ? nullptr : &ev.get<TracksterCollection>(tracksters_);
    const AssociationMap* r2s = recoToSim_.empty() ? nullptr : &ev.get<AssociationMap>(recoToSim_);
    for (const HadronMatch& m : matches) {
      Hists& h = histsFor(m.pdgId);
      h.nLinkedVsE->Fill(m.genEnergy, m.nLinked);
      h.nTrkVsE->Fill(m.genEnergy, m.nTracksters);
      h.hasSim->Fill(m.hasSim() ? 1 : 0);
      h.hasReco->Fill(m.hasReco() ? 1 : 0);
      if (m.hasReco()) {
        h.bestScoreVsE->Fill(m.genEnergy, m.bestScore);
        if (m.simRawEnergy > 0) {
          h.bestSharedFracVsE->Fill(m.genEnergy, m.bestSharedEnergy / m.simRawEnergy);
          h.linkedSharedFracVsE->Fill(m.genEnergy, m.linkedSharedEnergy / m.simRawEnergy);
          h.bestRawOverSimVsE->Fill(m.genEnergy, m.bestRawEnergy / m.simRawEnergy);
        }
      }
      if (recos && r2s) {
        for (std::size_t it = 0; it < recos->size() && it < r2s->size(); ++it) {
          const AssociationLink* best = r2s->best(it);
          const double score = best ? best->score : 1.0;
          h.recoScore->Fill(score);
          h.recoScoreVsRawE->Fill((*recos)[it].rawEnergy, score);
          h.recoMatched->Fill(score < scoreCut_ ? 1 : 0);
          h.recoMatchedVsE->Fill(m.genEnergy, score < scoreCut_ ? 1 : 0);
        }
      }
    }
  }

 private:
  struct Hists {
    TH2D *nLinkedVsE, *nTrkVsE, *bestScoreVsE, *bestSharedFracVsE, *linkedSharedFracVsE, *bestRawOverSimVsE, *recoScoreVsRawE, *recoMatchedVsE;
    TH1D *hasSim, *hasReco, *recoScore, *recoMatched;
  };
  Hists& histsFor(int pdgId) {
    const std::string key = hadronKey(pdgId, perType_);
    auto it = hists_by_type_.find(key);
    if (it != hists_by_type_.end()) return it->second;
    HistDir d = hists_.subdir(key);
    Hists h;
    h.nLinkedVsE = d.book2D("h2_nLinked_vs_E", "tracksters linked to the hadron (score < cut);E_{gen} (GeV);N_{linked}", eBins_, 20, 0, 20);
    h.nTrkVsE = d.book2D("h2_nTracksters_vs_E", "tracksters in the event;E_{gen} (GeV);N_{tracksters}", eBins_, 40, 0, 40);
    h.bestScoreVsE = d.book2D("h2_bestScore_vs_E", "best sim->reco score;E_{gen} (GeV);score", eBins_, 50, 0, 1);
    h.bestSharedFracVsE = d.book2D("h2_bestSharedFrac_vs_E", "shared energy of best trackster / E_{sim};E_{gen} (GeV);fraction", eBins_, 55, 0, 1.1);
    h.linkedSharedFracVsE = d.book2D("h2_linkedSharedFrac_vs_E", "shared energy of linked tracksters / E_{sim};E_{gen} (GeV);fraction", eBins_, 55, 0, 1.1);
    h.bestRawOverSimVsE = d.book2D("h2_bestRawOverSim_vs_E", "raw energy of best trackster / E_{sim};E_{gen} (GeV);ratio", eBins_, 60, 0, 1.2);
    h.recoScoreVsRawE = d.book2D("h2_recoScore_vs_rawE", "reco->sim best score vs trackster raw energy;E_{raw} (GeV);score", 50, 0, 200, 50, 0, 1);
    h.recoMatchedVsE = d.book2D("h2_recoMatched_vs_E", "reco trackster correctly matched (score < cut);E_{gen} (GeV);matched", eBins_, 2, 0, 2);
    h.hasSim = d.book1D("h_hasSim", "hadron has a sim trackster;has sim;hadrons", 2, 0, 2);
    h.hasReco = d.book1D("h_hasReco", "hadron has a reco trackster;has reco;hadrons", 2, 0, 2);
    h.recoScore = d.book1D("h_recoScore", "reco->sim best score;score;tracksters", 50, 0, 1);
    h.recoMatched = d.book1D("h_recoMatched", "reco trackster correctly matched;matched;tracksters", 2, 0, 2);
    return hists_by_type_.emplace(key, h).first->second;
  }

  std::string matches_, tracksters_, recoToSim_;
  double scoreCut_;
  HistDir hists_;
  std::vector<double> eBins_;
  bool perType_;
  std::map<std::string, Hists> hists_by_type_;
};

}  // namespace harp
HARP_REGISTER_MODULE(harp::MatchingQAAnalyzer);
