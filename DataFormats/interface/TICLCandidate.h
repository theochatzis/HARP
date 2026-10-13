#ifndef HARP_DATAFORMATS_TICLCANDIDATE_H
#define HARP_DATAFORMATS_TICLCANDIDATE_H
#include <vector>

namespace harp {
struct TICLCandidate {
  int pdgId = 0, charge = 0, trackIdx = -1;
  float energy = 0, rawEnergy = 0, pt = 0, eta = 0, phi = 0, p = 0, mass = 0;
  float px = 0, py = 0, pz = 0;
  float time = -99, timeError = -1, mtdTime = -99, mtdTimeError = -1;
  std::vector<unsigned int> tracksterIndices;  // into the candidate trackster collection
  bool hasTrack() const { return trackIdx >= 0; }
};
using TICLCandidateCollection = std::vector<TICLCandidate>;
}  // namespace harp
#endif
