#ifndef HARP_DATAFORMATS_HADRONMATCH_H
#define HARP_DATAFORMATS_HADRONMATCH_H
// Result of matching one generated hadron to the simulation (CaloParticle sim
// trackster) and to a reconstructed trackster collection. Produced by
// HadronTruthMatcher; consumed by the response / PID / matching-QA analyzers
// and persistable in skims as a flat table.
#include <vector>

namespace harp {

struct HadronMatch {
  // generator
  int genIndex = -1;
  int pdgId = 0;
  float genEnergy = 0, genPt = 0, genEta = 0, genPhi = 0;
  // simulation (CaloParticle / sim trackster)
  int simIndex = -1;
  float simRawEnergy = 0;        // energy of the sim trackster (sum of its layer clusters)
  float simRegressedEnergy = 0;
  float simBoundaryEta = 0, simBoundaryPhi = 0;
  // best reco trackster (lowest association score)
  int bestIndex = -1;
  float bestScore = 1, bestSharedEnergy = 0;
  float bestRawEnergy = 0, bestRegressedEnergy = 0;
  // all reco tracksters linked with score < scoreCut
  int nLinked = 0;
  float linkedRawEnergy = 0, linkedRegressedEnergy = 0, linkedSharedEnergy = 0;
  // whole collection (gun samples: everything belongs to the hadron)
  int nTracksters = 0;
  float allRawEnergy = 0, allRegressedEnergy = 0;
  float totalLayerClusterEnergy = 0;
  // TICL candidate containing the best trackster (if a candidate collection was given)
  int candidateIndex = -1;
  int candidatePdgId = 0;
  float candidateEnergy = 0, candidateRawEnergy = 0;
  int candidateHasTrack = 0;

  bool hasSim() const { return simIndex >= 0; }
  bool hasReco() const { return bestIndex >= 0; }
  float response(float reco) const { return genEnergy > 0 ? reco / genEnergy : 0.f; }
};
using HadronMatchCollection = std::vector<HadronMatch>;

}  // namespace harp
#endif
