// CollectionSizeFilter: require min <= size(collection) <= max for a trackster,
// layer-cluster, candidate or gen collection.
//
//   process.hasTrackster = Module("CollectionSizeFilter", src = "hgcalReader:clue3d", kind = "tracksters", min = 1)
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/GenParticle.h"
#include "DataFormats/interface/LayerCluster.h"
#include "DataFormats/interface/TICLCandidate.h"
#include "DataFormats/interface/Trackster.h"

namespace harp {

class CollectionSizeFilter : public Filter {
 public:
  CollectionSizeFilter(const ParameterSet& ps, ModuleContext& ctx)
      : Filter(ctx), src_(ctx.consumesProduct(ps.get<std::string>("src"))), kind_(ps.get<std::string>("kind", "tracksters")),
        min_(ps.get<long long>("min", 0)), max_(ps.get<long long>("max", 1LL << 40)) {}

  bool filter(Event& ev) override {
    long long n = 0;
    if (kind_ == "tracksters") n = ev.get<TracksterCollection>(src_).size();
    else if (kind_ == "layerClusters") n = ev.get<LayerClusterCollection>(src_).size();
    else if (kind_ == "candidates") n = ev.get<TICLCandidateCollection>(src_).size();
    else if (kind_ == "genParticles") n = ev.get<GenParticleCollection>(src_).size();
    else throw std::runtime_error("CollectionSizeFilter: unknown kind '" + kind_ + "'");
    return n >= min_ && n <= max_;
  }

 private:
  std::string src_, kind_;
  long long min_, max_;
};

}  // namespace harp
HARP_REGISTER_MODULE(harp::CollectionSizeFilter);
