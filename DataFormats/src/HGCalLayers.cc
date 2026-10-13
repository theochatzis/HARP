#include "DataFormats/interface/HGCalLayers.h"

#include <cmath>

namespace harp {
namespace {
const double kLayerZ[HGCalLayers::nLayers] = {
    322.20, 323.10, 325.20, 326.20, 328.30, 329.20, 331.30, 332.30, 334.40, 335.40, 337.40, 338.40, 340.50,
    341.50, 343.60, 344.50, 346.60, 347.60, 350.00, 351.00, 353.40, 354.40, 356.80, 357.70, 360.10, 361.10,
    368.00, 374.30, 380.60, 386.90, 393.20, 399.50, 405.80, 412.09, 418.39, 424.69, 430.98, 439.19, 447.47,
    455.65, 463.84, 472.07, 480.34, 488.53, 496.72, 504.96, 513.23};
}

int HGCalLayers::layerFromZ(double z, double tolerance) {
  const double az = std::fabs(z);
  int best = 0;
  double bestDist = tolerance;
  for (int i = 0; i < nLayers; ++i) {
    const double d = std::fabs(az - kLayerZ[i]);
    if (d < bestDist) { bestDist = d; best = i + 1; }
  }
  return best;
}

double HGCalLayers::zOfLayer(int layer) { return (layer >= 1 && layer <= nLayers) ? kLayerZ[layer - 1] : 0.; }
}  // namespace harp
