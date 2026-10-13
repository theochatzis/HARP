// GenParticleFilter: keep events with (at least / exactly) N generated
// particles passing pdgId / energy / |eta| requirements.
//
//   process.genFilter = Module("GenParticleFilter",
//       src = "hgcalReader:genParticles", pdgIds = [211, -211], status = 1,
//       minEnergy = 1.0, maxEnergy = 1e9, minAbsEta = 1.6, maxAbsEta = 2.9, minCount = 1, exactCount = -1)
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/GenParticle.h"

#include <cmath>

namespace harp {

class GenParticleFilter : public Filter {
 public:
  GenParticleFilter(const ParameterSet& ps, ModuleContext& ctx)
      : Filter(ctx),
        src_(ctx.consumesProduct(ps.get<std::string>("src"))),
        pdgIds_(ps.get<std::vector<int>>("pdgIds", {})),
        status_(ps.get<int>("status", -1)),
        minE_(ps.get<double>("minEnergy", 0.)),
        maxE_(ps.get<double>("maxEnergy", 1e12)),
        minAbsEta_(ps.get<double>("minAbsEta", 0.)),
        maxAbsEta_(ps.get<double>("maxAbsEta", 1e9)),
        minCount_(ps.get<int>("minCount", 1)),
        exactCount_(ps.get<int>("exactCount", -1)) {}

  bool filter(Event& ev) override {
    const auto& gens = ev.get<GenParticleCollection>(src_);
    int n = 0;
    for (const auto& g : gens) {
      if (status_ >= 0 && g.status != status_) continue;
      if (!pdgIds_.empty() && std::find(pdgIds_.begin(), pdgIds_.end(), g.pdgId) == pdgIds_.end()) continue;
      if (g.energy < minE_ || g.energy > maxE_) continue;
      const double ae = std::fabs(g.eta);
      if (ae < minAbsEta_ || ae > maxAbsEta_) continue;
      ++n;
    }
    if (exactCount_ >= 0) return n == exactCount_;
    return n >= minCount_;
  }

 private:
  std::string src_;
  std::vector<int> pdgIds_;
  int status_;
  double minE_, maxE_, minAbsEta_, maxAbsEta_;
  int minCount_, exactCount_;
};

}  // namespace harp
HARP_REGISTER_MODULE(harp::GenParticleFilter);
