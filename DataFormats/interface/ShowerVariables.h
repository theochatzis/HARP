#ifndef HARP_DATAFORMATS_SHOWERVARIABLES_H
#define HARP_DATAFORMATS_SHOWERVARIABLES_H
// Per-trackster shower-shape variables for particle identification studies.
// Produced by ShowerVariablesProducer (aligned with the trackster collection).
#include <vector>

namespace harp {

struct ShowerVariables {
  int nLayerClusters = 0;
  int nLayers = 0;          // distinct layers with at least one layer cluster
  int firstLayer = 0, lastLayer = 0;
  int layerOfMaxEnergy = 0;
  float zStart = 0, zEnd = 0, length = 0;   // cm, |z|
  float energyEE = 0, energyHad = 0;        // from layer clusters
  float fractionEE = 0;                     // energyEE / (energyEE + energyHad)
  float emFraction = 0;                     // trackster raw_em_energy / raw_energy
  float ev1 = 0, ev2 = 0, ev3 = 0;
  float sigma2 = 0, sigma3 = 0;             // sqrt(ev2/ev1), sqrt(ev3/ev1): transverse extents relative to the axis
  float maxLayerEnergyFraction = 0;         // energy in the most energetic layer / total
  float rmsLayer = 0;                       // energy weighted RMS of the layer number (longitudinal spread)
  float time = -99;
};
using ShowerVariablesCollection = std::vector<ShowerVariables>;

}  // namespace harp
#endif
