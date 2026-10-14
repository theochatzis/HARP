// HadronTruthMatcher: for every generated hadron find the sim trackster
// (CaloParticle) and, through a sim->reco association map, the reconstructed
// tracksters that contain its energy. Produces a HadronMatchCollection.
//
//   process.matchCLUE3D = Module("HadronTruthMatcher",
//       genParticles  = "hgcalReader:genParticles",
//       simTracksters = "hgcalReader:simCP",            # sim tracksters from CaloParticles
//       tracksters    = "hgcalReader:clue3d",           # reco collection to evaluate
//       simToReco     = "hgcalReader:simCP2clue3d",     # AssociationMap sim -> reco
//       layerClusters = "hgcalReader:layerClusters",    # optional (total LC energy)
//       candidates    = "hgcalReader:ticlCandidates",   # optional, only meaningful if 'tracksters' is the candidate trackster collection
//       genToSim      = "energyEta",  # how the gen hadron is matched to its sim trackster (CaloParticle):
//                                   #  "energyEta": same energy (sim regressed_energy = CaloParticle energy) and eta,
//                                   #               immune to the magnetic bending of charged hadrons (default)
//                                   #  "deltaR":    gen direction vs sim boundary position (fails for low-pT charged hadrons)
//                                   #  "index":     sim trackster i <-> gen particle i (gun samples)
//       maxEnergyDiff = 0.05, maxDeltaEta = 0.2,   # energyEta tolerances (relative energy, |delta eta|)
//       maxDeltaR     = 0.3,                       # deltaR tolerance
//       scoreCut      = 1.0,        # a reco trackster is "linked" if its sim->reco score is below this
//       minSharedEnergy = 0.0,      # ... and it shares at least this energy (GeV) with the sim trackster
//       pdgIds = [], genStatus = 1)
//
// Score semantics (TICL "ByHits" associators): 0 = perfect overlap, 1 = none;
// the best trackster is the one with the lowest score. Hadron showers are often
// split into several tracksters, so the best one may carry only part of the
// energy; the "linked*" quantities sum all tracksters passing the cuts.
#include "Core/interface/Logger.h"
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/Association.h"
#include "DataFormats/interface/GenParticle.h"
#include "DataFormats/interface/HadronMatch.h"
#include "DataFormats/interface/LayerCluster.h"
#include "DataFormats/interface/Math.h"
#include "DataFormats/interface/TICLCandidate.h"
#include "DataFormats/interface/Trackster.h"

#include <algorithm>
#include <cmath>

namespace harp {

class HadronTruthMatcher : public Producer {
 public:
  HadronTruthMatcher(const ParameterSet& ps, ModuleContext& ctx)
      : Producer(ctx),
        gen_(ctx.consumesProduct(ps.get<std::string>("genParticles"))),
        sim_(ctx.consumesProduct(ps.get<std::string>("simTracksters"))),
        reco_(ctx.consumesProduct(ps.get<std::string>("tracksters"))),
        simToReco_(ctx.consumesProduct(ps.get<std::string>("simToReco"))),
        lc_(ps.get<std::string>("layerClusters", "")),
        cand_(ps.get<std::string>("candidates", "")),
        genToSim_(ps.get<std::string>("genToSim", "energyEta")),
        maxDR_(ps.get<double>("maxDeltaR", 0.3)),
        maxEDiff_(ps.get<double>("maxEnergyDiff", 0.05)),
        maxDEta_(ps.get<double>("maxDeltaEta", 0.2)),
        scoreCut_(ps.get<double>("scoreCut", 1.0)),
        minShared_(ps.get<double>("minSharedEnergy", 0.)),
        pdgIds_(ps.get<std::vector<int>>("pdgIds", {})),
        genStatus_(ps.get<int>("genStatus", 1)) {
    if (!lc_.empty()) ctx.consumesProduct(lc_);
    if (!cand_.empty()) ctx.consumesProduct(cand_);
    out_ = ctx.produces<HadronMatchCollection>();
  }

  void produce(Event& ev) override {
    const auto& gens = ev.get<GenParticleCollection>(gen_);
    const auto& sims = ev.get<TracksterCollection>(sim_);
    const auto& recos = ev.get<TracksterCollection>(reco_);
    const auto& assoc = ev.get<AssociationMap>(simToReco_);
    const LayerClusterCollection* lcs = lc_.empty() ? nullptr : &ev.get<LayerClusterCollection>(lc_);
    const TICLCandidateCollection* cands = cand_.empty() ? nullptr : &ev.get<TICLCandidateCollection>(cand_);

    float allRaw = 0, allReg = 0;
    for (const auto& t : recos) { allRaw += t.rawEnergy; allReg += t.regressedEnergy; }
    float lcTotal = 0;
    if (lcs) for (const auto& c : *lcs) lcTotal += c.energy;

    HadronMatchCollection out;
    for (std::size_t ig = 0; ig < gens.size(); ++ig) {
      const GenParticle& g = gens[ig];
      if (genStatus_ >= 0 && g.status != genStatus_) continue;
      if (!pdgIds_.empty() && std::find(pdgIds_.begin(), pdgIds_.end(), g.pdgId) == pdgIds_.end()) continue;

      HadronMatch m;
      m.genIndex = static_cast<int>(ig);
      m.pdgId = g.pdgId; m.genEnergy = g.energy; m.genPt = g.pt; m.genEta = g.eta; m.genPhi = g.phi;
      m.nTracksters = static_cast<int>(recos.size());
      m.allRawEnergy = allRaw; m.allRegressedEnergy = allReg; m.totalLayerClusterEnergy = lcTotal;

      m.simIndex = findSim(g, ig, sims);
      if (m.simIndex < 0) { out.push_back(m); continue; }
      const Trackster& s = sims[m.simIndex];
      m.simRawEnergy = s.rawEnergy; m.simRegressedEnergy = s.regressedEnergy;
      m.simBoundaryEta = s.boundaryEta; m.simBoundaryPhi = s.boundaryPhi;

      if (static_cast<std::size_t>(m.simIndex) < assoc.size()) {
        const AssociationLink* best = assoc.best(m.simIndex);
        if (best && best->index < recos.size()) {
          m.bestIndex = static_cast<int>(best->index);
          m.bestScore = best->score; m.bestSharedEnergy = best->sharedEnergy;
          m.bestRawEnergy = recos[best->index].rawEnergy;
          m.bestRegressedEnergy = recos[best->index].regressedEnergy;
        }
        for (const auto& l : assoc[m.simIndex]) {
          if (l.score >= scoreCut_ || l.sharedEnergy < minShared_ || l.index >= recos.size()) continue;
          ++m.nLinked;
          m.linkedRawEnergy += recos[l.index].rawEnergy;
          m.linkedRegressedEnergy += recos[l.index].regressedEnergy;
          m.linkedSharedEnergy += l.sharedEnergy;
        }
      }
      if (cands && m.bestIndex >= 0) {
        for (std::size_t ic = 0; ic < cands->size(); ++ic) {
          const auto& idx = (*cands)[ic].tracksterIndices;
          if (std::find(idx.begin(), idx.end(), static_cast<unsigned int>(m.bestIndex)) == idx.end()) continue;
          m.candidateIndex = static_cast<int>(ic);
          m.candidatePdgId = (*cands)[ic].pdgId;
          m.candidateEnergy = (*cands)[ic].energy;
          m.candidateRawEnergy = (*cands)[ic].rawEnergy;
          m.candidateHasTrack = (*cands)[ic].hasTrack() ? 1 : 0;
          break;
        }
      }
      out.push_back(m);
    }
    ev.put(out_, std::move(out));
  }

 private:
  int findSim(const GenParticle& g, std::size_t ig, const TracksterCollection& sims) const {
    if (sims.empty()) return -1;
    if (genToSim_ == "index") return ig < sims.size() ? static_cast<int>(ig) : -1;
    if (genToSim_ == "energyEta") {
      // the sim trackster's regressed energy is the CaloParticle energy, i.e. the gen energy; eta is not bent by the field
      int best = -1;
      double bestDiff = maxEDiff_;
      for (std::size_t is = 0; is < sims.size(); ++is) {
        const Trackster& s = sims[is];
        if (s.isPU || g.energy <= 0) continue;
        const double eSim = s.regressedEnergy > 0 ? s.regressedEnergy : s.rawEnergy;
        const double diff = std::fabs(eSim - g.energy) / g.energy;
        const double eta = (s.hasSimInfo && s.boundaryEta != 0) ? s.boundaryEta : s.barycenterEta;
        if (std::fabs(eta - g.eta) > maxDEta_) continue;
        if (diff < bestDiff) { bestDiff = diff; best = static_cast<int>(is); }
      }
      return best;
    }
    if (genToSim_ != "deltaR") throw std::runtime_error("HadronTruthMatcher: unknown genToSim '" + genToSim_ + "' (energyEta, deltaR, index)");
    int best = -1;
    double bestDR = maxDR_;
    for (std::size_t is = 0; is < sims.size(); ++is) {
      const Trackster& s = sims[is];
      if (s.isPU) continue;
      const bool useBoundary = s.hasSimInfo && (s.boundaryEta != 0 || s.boundaryPhi != 0);
      const double eta = useBoundary ? s.boundaryEta : s.barycenterEta;
      const double phi = useBoundary ? s.boundaryPhi : s.barycenterPhi;
      const double dr = deltaR(g.eta, g.phi, eta, phi);
      if (dr < bestDR) { bestDR = dr; best = static_cast<int>(is); }
    }
    return best;
  }

  std::string gen_, sim_, reco_, simToReco_, lc_, cand_, genToSim_;
  double maxDR_, maxEDiff_, maxDEta_, scoreCut_, minShared_;
  std::vector<int> pdgIds_;
  int genStatus_;
  std::string out_;
};

}  // namespace harp
HARP_REGISTER_MODULE(harp::HadronTruthMatcher);
