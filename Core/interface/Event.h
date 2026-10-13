#ifndef HARP_CORE_EVENT_H
#define HARP_CORE_EVENT_H
// The Event: a per-entry container of *products* (C++ objects created by
// producer modules) plus the identifiers of the current entry.
//
//   ev.put("hgcalReader:tracksters", std::move(tracksters));   // producer
//   const auto& trk = ev.get<harp::TracksterCollection>("hgcalReader:tracksters");  // consumer
//
// Raw nano branches are read through handles obtained from ModuleContext
// (see BranchRegistry.h); they are not stored in the Event.
#include "Core/interface/ProductRegistry.h"

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <vector>

namespace harp {

class ProductHolderBase {
 public:
  virtual ~ProductHolderBase() = default;
  virtual std::type_index type() const = 0;
  virtual const void* ptr() const = 0;
};

template <class T>
class ProductHolder : public ProductHolderBase {
 public:
  explicit ProductHolder(T&& obj) : obj_(std::move(obj)) {}
  std::type_index type() const override { return typeid(T); }
  const void* ptr() const override { return &obj_; }
  const T& get() const { return obj_; }

 private:
  T obj_;
};

struct EventID {
  unsigned int run = 0;
  unsigned int lumi = 0;
  unsigned long long event = 0;
};

class Event {
 public:
  explicit Event(const ProductRegistry& registry) : registry_(registry) {}

  // ---- products ----
  template <class T>
  void put(const std::string& label, T&& obj) {
    if (products_.count(label)) throw std::runtime_error("Event::put: product '" + label + "' already present");
    products_[label] = std::make_unique<ProductHolder<T>>(std::move(obj));
  }
  template <class T>
  const T& get(const std::string& label) const {
    const T* p = getIfExists<T>(label);
    if (!p) throw std::runtime_error("Event::get: product '" + label + "' of type " + ProductRegistry::demangle(typeid(T).name()) + " not found" + hint(label));
    return *p;
  }
  template <class T>
  const T* getIfExists(const std::string& label) const {
    auto it = products_.find(label);
    if (it == products_.end()) {
      const std::string resolved = registry_.resolve(label);
      if (resolved == label) return nullptr;
      it = products_.find(resolved);
      if (it == products_.end()) return nullptr;
    }
    if (it->second->type() != typeid(T))
      throw std::runtime_error("Event::get: product '" + label + "' has type " + ProductRegistry::demangle(it->second->type().name()) + ", requested " + ProductRegistry::demangle(typeid(T).name()));
    return static_cast<const T*>(it->second->ptr());
  }
  bool has(const std::string& label) const { return products_.count(label) > 0; }
  const ProductHolderBase* holder(const std::string& label) const;
  std::vector<std::string> productLabels() const;
  const ProductRegistry& registry() const { return registry_; }

  // ---- bookkeeping, set by the Processor ----
  void clear() { products_.clear(); }
  void setEntry(long long entry, const EventID& id) { entry_ = entry; id_ = id; }
  long long entry() const { return entry_; }
  const EventID& id() const { return id_; }

 private:
  std::string hint(const std::string& label) const;
  const ProductRegistry& registry_;
  std::map<std::string, std::unique_ptr<ProductHolderBase>> products_;
  long long entry_ = -1;
  EventID id_;
};

}  // namespace harp
#endif
