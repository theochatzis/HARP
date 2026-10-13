#ifndef HARP_DATAFORMATS_LAYERCLUSTER_H
#define HARP_DATAFORMATS_LAYERCLUSTER_H
#include <vector>

namespace harp {
struct LayerCluster {
  float energy = 0, correctedEnergy = 0;
  float x = 0, y = 0, z = 0, eta = 0, phi = 0;
  float time = -99, timeError = -1;
  int nHits = 0;
  int algoId = 0;
  int layer = 0;  // 1..47, derived from |z| (see HGCalLayers.h); 0 if unknown
};
using LayerClusterCollection = std::vector<LayerCluster>;
}  // namespace harp
#endif
