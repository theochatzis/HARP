#ifndef HARP_DATAFORMATS_GENPARTICLE_H
#define HARP_DATAFORMATS_GENPARTICLE_H
#include <cstdlib>
#include <string>
#include <vector>

namespace harp {

struct GenParticle {
  int pdgId = 0;
  int status = 0;
  float energy = 0, pt = 0, eta = 0, phi = 0, mass = 0;
  float vx = 0, vy = 0, vz = 0;
  bool isCharged() const { int a = std::abs(pdgId); return a == 211 || a == 321 || a == 2212 || a == 11 || a == 13; }
  bool isHadron() const { int a = std::abs(pdgId); return a == 111 || a == 211 || a == 321 || a == 130 || a == 310 || a == 2212 || a == 2112; }
};
using GenParticleCollection = std::vector<GenParticle>;

// Human readable hadron names used for histogram categories.
inline std::string hadronName(int pdgId) {
  switch (pdgId) {
    case 111: return "pi0";
    case 211: return "pip";
    case -211: return "pim";
    case 321: return "Kp";
    case -321: return "Km";
    case 130: return "K0L";
    case 310: return "K0S";
    case 2212: return "p";
    case -2212: return "pbar";
    case 2112: return "n";
    case -2112: return "nbar";
    case 22: return "gamma";
    case 11: return "em";
    case -11: return "ep";
    default: return "pdg" + std::to_string(pdgId);
  }
}

}  // namespace harp
#endif
