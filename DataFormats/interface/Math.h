#ifndef HARP_DATAFORMATS_MATH_H
#define HARP_DATAFORMATS_MATH_H
#include <cmath>

namespace harp {
inline double deltaPhi(double a, double b) {
  double d = a - b;
  while (d > M_PI) d -= 2 * M_PI;
  while (d <= -M_PI) d += 2 * M_PI;
  return d;
}
inline double deltaR(double eta1, double phi1, double eta2, double phi2) {
  const double dphi = deltaPhi(phi1, phi2);
  const double deta = eta1 - eta2;
  return std::sqrt(deta * deta + dphi * dphi);
}
inline double etaFromXYZ(double x, double y, double z) {
  const double r = std::sqrt(x * x + y * y);
  if (r == 0) return z > 0 ? 1e9 : -1e9;
  return std::asinh(z / r);
}
}  // namespace harp
#endif
