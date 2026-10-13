// ShowerVariablesProducer: per-trackster shower shape variables computed from
// the trackster PCA quantities and its layer clusters (depth, number of
// layers, CE-E/CE-H sharing, longitudinal profile).
//
//   process.showerCLUE3D = Module("ShowerVariablesProducer",
//       tracksters = "hgcalReader:clue3d", layerClusters = "hgcalReader:layerClusters")
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/HGCalLayers.h"
#include "DataFormats/interface/LayerCluster.h"
#include "DataFormats/interface/ShowerVariables.h"
#include "DataFormats/interface/Trackster.h"

#include <array>
#include <cmath>

namespace harp {

class ShowerVariablesProducer : public Producer {
 public:
  ShowerVariablesProducer(const ParameterSet& ps, ModuleContext& ctx)
      : Producer(ctx),
        trk_(ctx.consumesProduct(ps.get<std::string>("tracksters"))),
        lc_(ctx.consumesProduct(ps.get<std::string>("layerClusters"))),
        out_(ctx.produces<ShowerVariablesCollection>()) {}

  void produce(Event& ev) override {
    const auto& tracksters = ev.get<TracksterCollection>(trk_);
    const auto& lcs = ev.get<LayerClusterCollection>(lc_);
    ShowerVariablesCollection out;
    out.reserve(tracksters.size());
    for (const auto& t : tracksters) out.push_back(compute(t, lcs));
    ev.put(out_, std::move(out));
  }

 private:
  static ShowerVariables compute(const Trackster& t, const LayerClusterCollection& lcs) {
    ShowerVariables v;
    v.nLayerClusters = static_cast<int>(t.vertices.size());
    v.emFraction = t.emFraction();
    v.ev1 = t.ev1; v.ev2 = t.ev2; v.ev3 = t.ev3;
    v.sigma2 = t.ev1 > 0 ? std::sqrt(std::max(0.f, t.ev2 / t.ev1)) : 0.f;
    v.sigma3 = t.ev1 > 0 ? std::sqrt(std::max(0.f, t.ev3 / t.ev1)) : 0.f;
    v.time = t.time;

    std::array<double, HGCalLayers::nLayers + 1> eLayer{};
    double zmin = 1e9, zmax = -1e9, etot = 0;
    for (unsigned int idx : t.vertices) {
      if (idx >= lcs.size()) continue;
      const LayerCluster& c = lcs[idx];
      const double az = std::fabs(c.z);
      zmin = std::min(zmin, az); zmax = std::max(zmax, az);
      etot += c.energy;
      if (c.layer >= 1 && c.layer <= HGCalLayers::nLayers) {
        eLayer[c.layer] += c.energy;
        if (HGCalLayers::isEE(c.layer)) v.energyEE += c.energy; else v.energyHad += c.energy;
      }
    }
    if (etot > 0) {
      v.zStart = static_cast<float>(zmin); v.zEnd = static_cast<float>(zmax); v.length = static_cast<float>(zmax - zmin);
      v.fractionEE = (v.energyEE + v.energyHad) > 0 ? v.energyEE / (v.energyEE + v.energyHad) : 0.f;
      double emax = 0, sumL = 0, sumL2 = 0;
      for (int l = 1; l <= HGCalLayers::nLayers; ++l) {
        if (eLayer[l] <= 0) continue;
        ++v.nLayers;
        if (v.firstLayer == 0) v.firstLayer = l;
        v.lastLayer = l;
        if (eLayer[l] > emax) { emax = eLayer[l]; v.layerOfMaxEnergy = l; }
        sumL += l * eLayer[l]; sumL2 += l * l * eLayer[l];
      }
      v.maxLayerEnergyFraction = static_cast<float>(emax / etot);
      const double mean = sumL / etot;
      v.rmsLayer = static_cast<float>(std::sqrt(std::max(0., sumL2 / etot - mean * mean)));
    }
    return v;
  }

  std::string trk_, lc_, out_;
};

}  // namespace harp
HARP_REGISTER_MODULE(harp::ShowerVariablesProducer);
