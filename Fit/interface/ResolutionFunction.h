#ifndef HARP_FIT_RESOLUTIONFUNCTION_H
#define HARP_FIT_RESOLUTIONFUNCTION_H
// Calorimeter resolution parametrisation sigma/E = S/sqrt(E) (+) C (+) N/E
// fitted to a graph of resolution vs energy. Extend here with other models
// (e.g. scale non-linearity) as the response fitting work develops.
#include <TF1.h>
#include <TGraphErrors.h>

#include <memory>
#include <string>

namespace harp {

struct ResolutionFitResult {
  double stochastic = 0, stochasticErr = 0;
  double constant = 0, constantErr = 0;
  double noise = 0, noiseErr = 0;
  double chi2 = 0; int ndf = 0; bool valid = false;
  std::unique_ptr<TF1> function;
  std::string text() const;  // e.g. "S = 0.52, C = 0.08"
};

class ResolutionFunction {
 public:
  // includeNoise=false fits only S and C.
  static ResolutionFitResult fit(TGraphErrors& g, double eMin, double eMax, bool includeNoise = false);
};

}  // namespace harp
#endif
