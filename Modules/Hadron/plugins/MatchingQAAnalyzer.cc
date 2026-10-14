// MatchingQAAnalyzer: quality of the reco <-> sim matching. Sim-to-reco
// quantities from the HadronMatch (how many tracksters contain the hadron,
// best score, energy captured) and reco-to-sim quantities for every reco
// trackster. The efficiency / purity / duplicate / fake definitions follow
// CMSSW Validation/HGCalValidation (HGVHistoProducerAlgo, see docs/matching.md):
//   efficient : >= 1 reco trackster with sharedEnergy / E_sim >= effSharedFraction (0.5)
//   pure      : >= 1 reco trackster with sim->reco score < purityScoreCut (0.2)
//   duplicate : >= 2 reco tracksters with sim->reco score < purityScoreCut
//   non-fake  : reco trackster with a reco->sim score < fakeScoreCut (0.6)
// Histograms in <label>/<hadron>/:
//   h2_nLinked_vs_E, h2_bestScore_vs_E, h2_bestSharedFrac_vs_E, h2_linkedSharedFrac_vs_E,
//   h2_bestRawOverSim_vs_E, h2_nTracksters_vs_E, h_recoScore, h2_recoScore_vs_rawE, h_recoMatched,
//   p_hasReco_vs_E, p_hasSim_vs_E (efficiency to have a matched reco / sim trackster vs E_gen)
//   p_efficient_vs_E, p_pure_vs_E, p_duplicate_vs_E (CMSSW definitions vs E_gen), p_nonFake_vs_rawE, p_nonFake_vs_E
//
//   process.qaCLUE3D = Module("MatchingQAAnalyzer",
//       matches = "matchCLUE3D", tracksters = "hgcalReader:clue3d",
//       recoToSim = "hgcalReader:clue3d2simCP", effSharedFraction = 0.5, purityScoreCut = 0.2, fakeScoreCut = 0.6)
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/Association.h"
#include "DataFormats/interface/HadronMatch.h"
#include "DataFormats/interface/Trackster.h"
#include "Modules/Hadron/interface/Binning.h"

#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>

#include <map>

namespace harp {

class MatchingQAAnalyzer : public Analyzer {
 public:
  MatchingQAAnalyzer(const ParameterSet& ps, ModuleContext& ctx)
      : Analyzer(ctx), matches_(ctx.consumesProduct(ps.get<std::string>("matches"))),
        tracksters_(ps.get<std::string>("tracksters", "")), recoToSim_(ps.get<std::string>("recoToSim", "")),
        simToReco_(ps.get<std::string>("simToReco", "")),
        effFrac_(ps.get<double>("effSharedFraction", 0.5)), purityCut_(ps.get<double>("purityScoreCut", 0.2)),
        fakeCut_(ps.get<double>("fakeScoreCut", ps.get<double>("scoreCut", 0.6))), hists_(ctx.hists()) {
    if (!tracksters_.empty()) ctx.consumesProduct(tracksters_);
    if (!recoToSim_.empty()) ctx.consumesProduct(recoToSim_);
    if (!simToReco_.empty()) ctx.consumesProduct(simToReco_);
    eBins_ = energyBinsFromConfig(ps);
    perType_ = ps.get<bool>("perHadronType", true);
  }

  void analyze(const Event& ev) override {
    const auto& matches = ev.get<HadronMatchCollection>(matches_);
    const TracksterCollection* recos = tracksters_.empty() ? nullptr : &ev.get<TracksterCollection>(tracksters_);
    const AssociationMap* r2s = recoToSim_.empty() ? nullptr : &ev.get<AssociationMap>(recoToSim_);
    const AssociationMap* s2r = simToReco_.empty() ? nullptr : &ev.get<AssociationMap>(simToReco_);
    for (const HadronMatch& m : matches) {
      Hists& h = histsFor(m.pdgId);
      // CMSSW-style sim->reco criteria over all links of the hadron's sim trackster
      if (s2r && m.hasSim() && static_cast<std::size_t>(m.simIndex) < s2r->size()) {
        int nEff = 0, nPure = 0;
        for (const auto& l : (*s2r)[m.simIndex]) {
          if (m.simRawEnergy > 0 && l.sharedEnergy / m.simRawEnergy >= effFrac_) ++nEff;
          if (l.score < purityCut_) ++nPure;
        }
        h.efficientVsE->Fill(m.genEnergy, nEff >= 1 ? 1 : 0);
        h.pureVsE->Fill(m.genEnergy, nPure >= 1 ? 1 : 0);
        h.duplicateVsE->Fill(m.genEnergy, nPure >= 2 ? 1 : 0);
      } else if (m.hasSim()) {
        h.efficientVsE->Fill(m.genEnergy, 0); h.pureVsE->Fill(m.genEnergy, 0); h.duplicateVsE->Fill(m.genEnergy, 0);
      }
      h.nLinkedVsE->Fill(m.genEnergy, m.nLinked);
      h.nTrkVsE->Fill(m.genEnergy, m.nTracksters);
      h.hasSim->Fill(m.hasSim() ? 1 : 0);
      h.hasReco->Fill(m.hasReco() ? 1 : 0);
      h.hasSimVsE->Fill(m.genEnergy, m.hasSim() ? 1 : 0);
      h.hasRecoVsE->Fill(m.genEnergy, m.hasReco() ? 1 : 0);
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
          h.recoMatched->Fill(score < fakeCut_ ? 1 : 0);
          h.recoMatchedVsE->Fill(m.genEnergy, score < fakeCut_ ? 1 : 0);
          h.nonFakeVsRawE->Fill((*recos)[it].rawEnergy, score < fakeCut_ ? 1 : 0);
          h.nonFakeVsE->Fill(m.genEnergy, score < fakeCut_ ? 1 : 0);
        }
      }
    }
  }

 private:
  struct Hists {
    TH2D *nLinkedVsE, *nTrkVsE, *bestScoreVsE, *bestSharedFracVsE, *linkedSharedFracVsE, *bestRawOverSimVsE, *recoScoreVsRawE, *recoMatchedVsE;
    TH1D *hasSim, *hasReco, *recoScore, *recoMatched;
    TProfile *hasSimVsE, *hasRecoVsE, *efficientVsE, *pureVsE, *duplicateVsE, *nonFakeVsRawE, *nonFakeVsE;
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
    h.hasSimVsE = d.bookProfile("p_hasSim_vs_E", "hadron has a sim trackster;E_{gen} (GeV);Efficiency", eBins_);
    h.hasRecoVsE = d.bookProfile("p_hasReco_vs_E", "hadron has a matched reco trackster;E_{gen} (GeV);Efficiency", eBins_);
    h.efficientVsE = d.bookProfile("p_efficient_vs_E", "CMSSW efficiency: a trackster shares >= effSharedFraction of E_{sim};E_{gen} (GeV);Efficiency", eBins_);
    h.pureVsE = d.bookProfile("p_pure_vs_E", "CMSSW purity: a trackster with sim->reco score < purityScoreCut;E_{gen} (GeV);Purity", eBins_);
    h.duplicateVsE = d.bookProfile("p_duplicate_vs_E", "CMSSW duplicate: >= 2 tracksters with sim->reco score < purityScoreCut;E_{gen} (GeV);Duplicate rate", eBins_);
    h.nonFakeVsRawE = d.bookProfile("p_nonFake_vs_rawE", "CMSSW non-fake: reco->sim score < fakeScoreCut;E_{raw} (GeV);Non-fake fraction", std::vector<double>{0.5, 1, 2, 3, 5, 7, 10, 15, 20, 30, 50, 70, 100, 150, 200});
    h.nonFakeVsE = d.bookProfile("p_nonFake_vs_E", "CMSSW non-fake: reco->sim score < fakeScoreCut;E_{gen} (GeV);Non-fake fraction of tracksters", eBins_);
    h.hasSim = d.book1D("h_hasSim", "hadron has a sim trackster;has sim;hadrons", 2, 0, 2);
    h.hasReco = d.book1D("h_hasReco", "hadron has a reco trackster;has reco;hadrons", 2, 0, 2);
    h.recoScore = d.book1D("h_recoScore", "reco->sim best score;score;tracksters", 50, 0, 1);
    h.recoMatched = d.book1D("h_recoMatched", "reco trackster correctly matched;matched;tracksters", 2, 0, 2);
    return hists_by_type_.emplace(key, h).first->second;
  }

  std::string matches_, tracksters_, recoToSim_, simToReco_;
  double effFrac_, purityCut_, fakeCut_;
  HistDir hists_;
  std::vector<double> eBins_;
  bool perType_;
  std::map<std::string, Hists> hists_by_type_;
};

}  // namespace harp
HARP_REGISTER_MODULE(harp::MatchingQAAnalyzer);
