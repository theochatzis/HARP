#ifndef HARP_DATAFORMATS_TRACKSTER_H
#define HARP_DATAFORMATS_TRACKSTER_H
// Reco and Sim tracksters share this type; sim-only fields are filled when the
// branches exist (hasSimInfo).
#include <vector>

namespace harp {

struct Trackster {
  float rawEnergy = 0, rawEmEnergy = 0, rawPt = 0, regressedEnergy = 0;
  float barycenterX = 0, barycenterY = 0, barycenterZ = 0, barycenterEta = 0, barycenterPhi = 0;
  float ev1 = 0, ev2 = 0, ev3 = 0;             // PCA eigenvalues
  float eVector0X = 0, eVector0Y = 0, eVector0Z = 0;  // PCA main axis
  float time = -99, timeError = -1;
  std::vector<unsigned int> vertices;  // indices into the LayerCluster collection
  std::vector<float> vertexMultiplicity;

  // sim-only
  bool hasSimInfo = false;
  int caloParticleIdx = -1;
  int isPU = 0;
  float boundaryX = 0, boundaryY = 0, boundaryZ = 0, boundaryEta = 0, boundaryPhi = 0;
  float boundaryPx = 0, boundaryPy = 0, boundaryPz = 0;
  float simTime = -99, genPt = 0, mass = 0;

  std::size_t nLayerClusters() const { return vertices.size(); }
  float emFraction() const { return rawEnergy > 0 ? rawEmEnergy / rawEnergy : 0.f; }
};
using TracksterCollection = std::vector<Trackster>;

}  // namespace harp
#endif
