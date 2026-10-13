#include "Core/interface/ModuleFactory.h"

#include <stdexcept>

namespace harp {

ModuleFactory& ModuleFactory::instance() {
  static ModuleFactory f;
  return f;
}

bool ModuleFactory::registerModule(const std::string& type, Creator creator) {
  if (creators_.count(type)) throw std::runtime_error("ModuleFactory: module type '" + type + "' registered twice");
  creators_[type] = std::move(creator);
  return true;
}

std::unique_ptr<ModuleBase> ModuleFactory::create(const std::string& type, const ParameterSet& ps, ModuleContext& ctx) const {
  auto it = creators_.find(type);
  if (it == creators_.end()) {
    std::string msg = "ModuleFactory: unknown module type '" + type + "'. Known types:";
    for (const auto& t : available()) msg += " " + t;
    throw std::runtime_error(msg);
  }
  return it->second(ps, ctx);
}

std::vector<std::string> ModuleFactory::available() const {
  std::vector<std::string> out;
  for (const auto& kv : creators_) out.push_back(kv.first);
  return out;
}

std::string ModuleFactory::stripNamespace(const std::string& qualified) {
  const auto pos = qualified.rfind("::");
  return pos == std::string::npos ? qualified : qualified.substr(pos + 2);
}

}  // namespace harp
