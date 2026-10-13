#ifndef HARP_DATAFORMATS_TRACK_H
#define HARP_DATAFORMATS_TRACK_H
#include <vector>

namespace harp {
struct Track {
  float pt = 0, eta = 0, phi = 0, p = 0, ptErr = 0;
  int charge = 0, nHits = 0, missingOuterHits = 0;
  float vx = 0, vy = 0, vz = 0;
  // propagated to the HGCal front face
  float hgcalX = 0, hgcalY = 0, hgcalZ = 0, hgcalEta = 0, hgcalPhi = 0;
};
using TrackCollection = std::vector<Track>;
}  // namespace harp
#endif
