#ifndef HARP_CORE_PERSISTENCE_H
#define HARP_CORE_PERSISTENCE_H
// Registry of "how to turn product type T into a FlatTable" so that output
// modules can persist any product whose type has a converter:
//
//   // in some .cc of the package defining HadronMatch:
//   static const bool reg = harp::PersistenceRegistry::instance().add<std::vector<HadronMatch>>(
//       [](const std::vector<HadronMatch>& v, const std::string& name) { ... return table; });
//
// Fundamental types and std::vector of them are registered by the Core.
#include "Core/interface/Event.h"
#include "Core/interface/FlatTable.h"

#include <functional>
#include <map>
#include <string>
#include <typeindex>

namespace harp {

class PersistenceRegistry {
 public:
  using Converter = std::function<FlatTable(const ProductHolderBase&, const std::string& tableName)>;
  static PersistenceRegistry& instance();

  template <class T>
  bool add(std::function<FlatTable(const T&, const std::string&)> fn) {
    converters_[typeid(T)] = [fn](const ProductHolderBase& h, const std::string& name) {
      return fn(*static_cast<const T*>(h.ptr()), name);
    };
    return true;
  }
  bool canPersist(std::type_index t) const { return converters_.count(t) > 0; }
  FlatTable make(const ProductHolderBase& holder, const std::string& tableName) const;

 private:
  std::map<std::type_index, Converter> converters_;
};

}  // namespace harp
#endif
