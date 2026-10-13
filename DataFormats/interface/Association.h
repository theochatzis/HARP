#ifndef HARP_DATAFORMATS_ASSOCIATION_H
#define HARP_DATAFORMATS_ASSOCIATION_H
// One-to-many association with a score and shared energy per link, e.g.
// SimCP -> reco tracksters ("ByHits"). Lower score = better match (TICL convention).
#include <algorithm>
#include <vector>

namespace harp {

struct AssociationLink {
  unsigned int index = 0;  // index in the "to" collection
  float score = 1.f;
  float sharedEnergy = 0.f;
};

class AssociationMap {
 public:
  std::vector<std::vector<AssociationLink>> links;  // links[fromIndex]

  std::size_t size() const { return links.size(); }
  const std::vector<AssociationLink>& operator[](std::size_t from) const { return links[from]; }
  // Best link (lowest score), nullptr if none.
  const AssociationLink* best(std::size_t from) const {
    if (from >= links.size() || links[from].empty()) return nullptr;
    return &*std::min_element(links[from].begin(), links[from].end(), [](const AssociationLink& a, const AssociationLink& b) { return a.score < b.score; });
  }
};

}  // namespace harp
#endif
