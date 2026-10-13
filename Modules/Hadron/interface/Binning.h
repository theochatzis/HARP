#ifndef HARP_MODULES_HADRON_BINNING_H
#define HARP_MODULES_HADRON_BINNING_H
// Shared helpers for the hadron analyzers: energy binning from the
// configuration and per-hadron-type histogram keys.
#include "Core/interface/ParameterSet.h"
#include "DataFormats/interface/GenParticle.h"

#include <string>
#include <vector>

namespace harp {

inline std::vector<double> energyBinsFromConfig(const ParameterSet& ps, const std::string& key = "energyBins") {
  std::vector<double> def = {1, 2, 3, 5, 7, 10, 15, 20, 30, 40, 50, 70, 100, 130, 160, 200};
  return ps.get<std::vector<double>>(key, def);
}

// Histogram sub-directory for a hadron type: per pdgId ("pip", "Km", ...) or "all".
inline std::string hadronKey(int pdgId, bool perType) { return perType ? hadronName(pdgId) : "all"; }

}  // namespace harp
#endif
