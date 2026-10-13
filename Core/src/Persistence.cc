#include "Core/interface/Persistence.h"

#include <stdexcept>
#include <vector>

namespace harp {

PersistenceRegistry& PersistenceRegistry::instance() {
  static PersistenceRegistry r;
  static bool initialised = false;
  if (!initialised) {
    initialised = true;
    // scalars -> singleton tables with a single column "value"
    r.add<float>([](const float& v, const std::string& n) { FlatTable t(n, 1, true); t.addColumn("value", std::vector<float>{v}); return t; });
    r.add<double>([](const double& v, const std::string& n) { FlatTable t(n, 1, true); t.addColumn("value", std::vector<float>{static_cast<float>(v)}); return t; });
    r.add<int>([](const int& v, const std::string& n) { FlatTable t(n, 1, true); t.addColumn("value", std::vector<int>{v}); return t; });
    r.add<bool>([](const bool& v, const std::string& n) { FlatTable t(n, 1, true); t.addColumnBool("value", std::vector<unsigned char>{static_cast<unsigned char>(v)}); return t; });
    // vectors -> one column "value" with n rows
    r.add<std::vector<float>>([](const std::vector<float>& v, const std::string& n) { FlatTable t(n, v.size()); t.addColumn("value", v); return t; });
    r.add<std::vector<int>>([](const std::vector<int>& v, const std::string& n) { FlatTable t(n, v.size()); t.addColumn("value", v); return t; });
    r.add<std::vector<double>>([](const std::vector<double>& v, const std::string& n) { FlatTable t(n, v.size()); t.addColumn("value", std::vector<float>(v.begin(), v.end())); return t; });
    r.add<FlatTable>([](const FlatTable& v, const std::string& n) { FlatTable t = v; if (t.name().empty()) t.setName(n); return t; });
  }
  return r;
}

FlatTable PersistenceRegistry::make(const ProductHolderBase& holder, const std::string& tableName) const {
  auto it = converters_.find(holder.type());
  if (it == converters_.end())
    throw std::runtime_error("PersistenceRegistry: no FlatTable converter registered for type " + ProductRegistry::demangle(holder.type().name()) + " (product '" + tableName + "')");
  return it->second(holder, tableName);
}

}  // namespace harp
