#ifndef HARP_CORE_MODULE_H
#define HARP_CORE_MODULE_H
// Module base classes (the CMSSW analogy: EDProducer / EDFilter / EDAnalyzer).
//
//   class MyProducer : public harp::Producer {
//    public:
//     MyProducer(const harp::ParameterSet& ps, harp::ModuleContext& ctx) : Producer(ctx) {
//       pt_ = ctx.consumesArray<float>("HGCalGenPart_pt");   // raw nano branch
//       out_ = ctx.produces<std::vector<float>>("scaled");    // product label "<label>:scaled"
//     }
//     void produce(harp::Event& ev) override { ... ev.put(out_, std::move(v)); }
//   };
//   HARP_REGISTER_MODULE(MyProducer);
#include "Core/interface/BranchRegistry.h"
#include "Core/interface/Event.h"
#include "Core/interface/HistService.h"
#include "Core/interface/ParameterSet.h"
#include "Core/interface/ProductRegistry.h"

#include <string>
#include <vector>

namespace harp {

enum class ModuleKind { Producer, Filter, Analyzer };

// Everything a module may ask the framework for at construction time.
class ModuleContext {
 public:
  ModuleContext(std::string label, std::string type, BranchRegistry& branches, HistService& hists, ProductRegistry& products)
      : label_(std::move(label)), type_(std::move(type)), branches_(branches), hists_(hists), products_(products) {}

  const std::string& label() const { return label_; }
  const std::string& type() const { return type_; }

  template <class T>
  ArrayHandle<T> consumesArray(const std::string& branch, bool optional = false) { return branches_.array<T>(branch, optional); }
  template <class T>
  ValueHandle<T> consumesValue(const std::string& branch, bool optional = false) { return branches_.value<T>(branch, optional); }
  // Declare a product; returns the full label to use in Event::put.
  template <class T>
  std::string produces(const std::string& instance = "") {
    const std::string full = ProductRegistry::makeLabel(label_, instance);
    products_.add(ProductInfo{full, label_, instance, typeid(T), ProductRegistry::demangle(typeid(T).name())});
    return full;
  }
  // Declare a dependency on another module's product (validated by the Processor).
  std::string consumesProduct(const std::string& label) { consumed_.push_back(label); return label; }

  HistDir hists() { return HistDir(hists_, label_); }
  HistService& histService() { return hists_; }
  BranchRegistry& branches() { return branches_; }
  const ProductRegistry& productRegistry() const { return products_; }
  const std::vector<std::string>& consumedProducts() const { return consumed_; }

 private:
  std::string label_, type_;
  BranchRegistry& branches_;
  HistService& hists_;
  ProductRegistry& products_;
  std::vector<std::string> consumed_;
};

class ModuleBase {
 public:
  explicit ModuleBase(const ModuleContext& ctx) : label_(ctx.label()), type_(ctx.type()) {}
  virtual ~ModuleBase() = default;
  virtual ModuleKind kind() const = 0;

  // Lifecycle hooks (all optional).
  virtual void beginJob() {}
  virtual void beginFile(const std::string& fileName) {}
  virtual void endJob() {}

  const std::string& label() const { return label_; }
  const std::string& type() const { return type_; }
  std::string kindName() const;

 private:
  std::string label_, type_;
};

class Producer : public ModuleBase {
 public:
  using ModuleBase::ModuleBase;
  ModuleKind kind() const final { return ModuleKind::Producer; }
  virtual void produce(Event& event) = 0;
};

class Filter : public ModuleBase {
 public:
  using ModuleBase::ModuleBase;
  ModuleKind kind() const final { return ModuleKind::Filter; }
  // Return false to stop the path for this event.
  virtual bool filter(Event& event) = 0;
};

class Analyzer : public ModuleBase {
 public:
  using ModuleBase::ModuleBase;
  ModuleKind kind() const final { return ModuleKind::Analyzer; }
  virtual void analyze(const Event& event) = 0;
};

}  // namespace harp
#endif
