#include "Core/interface/Event.h"

namespace harp {

const ProductHolderBase* Event::holder(const std::string& label) const {
  auto it = products_.find(label);
  return it == products_.end() ? nullptr : it->second.get();
}

std::vector<std::string> Event::productLabels() const {
  std::vector<std::string> out;
  for (const auto& kv : products_) out.push_back(kv.first);
  return out;
}

std::string Event::hint(const std::string& label) const {
  if (registry_.has(label)) return " (declared by module '" + registry_.get(label).module + "' but not put in this event; is the module in the path before the consumer?)";
  std::string s = ". Available products:";
  for (const auto& p : registry_.all()) s += " " + p.label;
  return s;
}

}  // namespace harp
