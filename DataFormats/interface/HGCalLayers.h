#ifndef HARP_DATAFORMATS_HGCALLAYERS_H
#define HARP_DATAFORMATS_HGCALLAYERS_H
// Layer numbering from the |z| of layer clusters. The table was derived from
// the Phase-2 gun nanos (47 planes: 26 CE-E + 21 CE-H); see docs/edm.md.
namespace harp {
struct HGCalLayers {
  static constexpr int nLayers = 47;
  static constexpr int nLayersEE = 26;
  // layer index 1..47 for |z| in cm, 0 if not within tolerance of any plane
  static int layerFromZ(double z, double tolerance = 0.5);
  static double zOfLayer(int layer);
  static bool isEE(int layer) { return layer >= 1 && layer <= nLayersEE; }
  static bool isHad(int layer) { return layer > nLayersEE && layer <= nLayers; }
};
}  // namespace harp
#endif
