#ifndef HARP_CORE_MODULEFACTORY_H
#define HARP_CORE_MODULEFACTORY_H
// Plugin registry. A module becomes available to configurations by adding
//   HARP_REGISTER_MODULE(ClassName);
// at the end of its .cc file; the shared library is dlopen'ed by harpRun.
#include "Core/interface/Module.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace harp {

class ModuleFactory {
 public:
  using Creator = std::function<std::unique_ptr<ModuleBase>(const ParameterSet&, ModuleContext&)>;
  static ModuleFactory& instance();
  bool registerModule(const std::string& type, Creator creator);
  std::unique_ptr<ModuleBase> create(const std::string& type, const ParameterSet& ps, ModuleContext& ctx) const;
  bool has(const std::string& type) const { return creators_.count(type) > 0; }
  std::vector<std::string> available() const;
  static std::string stripNamespace(const std::string& qualified);

 private:
  std::map<std::string, Creator> creators_;
};

}  // namespace harp

// Registers CLASS (may be namespace qualified) under its unqualified name.
#define HARP_REGISTER_MODULE_IMPL(CLASS, ID)                                                            \
  namespace {                                                                                            \
  const bool harp_registered_module_##ID = ::harp::ModuleFactory::instance().registerModule(            \
      ::harp::ModuleFactory::stripNamespace(#CLASS), [](const ::harp::ParameterSet& ps, ::harp::ModuleContext& ctx) { \
        return std::unique_ptr<::harp::ModuleBase>(new CLASS(ps, ctx));                                  \
      });                                                                                                \
  }
#define HARP_REGISTER_MODULE_EXPAND(CLASS, ID) HARP_REGISTER_MODULE_IMPL(CLASS, ID)
#define HARP_REGISTER_MODULE(CLASS) HARP_REGISTER_MODULE_EXPAND(CLASS, __COUNTER__)
#endif
