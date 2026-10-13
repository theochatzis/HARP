#include "Plot/interface/PlotterFactory.h"

#include <stdexcept>

namespace harp {
PlotterFactory& PlotterFactory::instance() { static PlotterFactory f; return f; }
bool PlotterFactory::registerPlotter(const std::string& type, Creator c) {
  if (creators_.count(type)) throw std::runtime_error("PlotterFactory: '" + type + "' registered twice");
  creators_[type] = std::move(c);
  return true;
}
std::unique_ptr<Plotter> PlotterFactory::create(const std::string& type, const ParameterSet& ps, PlotContext& ctx) const {
  auto it = creators_.find(type);
  if (it == creators_.end()) {
    std::string msg = "PlotterFactory: unknown plotter '" + type + "'. Known:";
    for (const auto& t : available()) msg += " " + t;
    throw std::runtime_error(msg);
  }
  return it->second(ps, ctx);
}
std::vector<std::string> PlotterFactory::available() const {
  std::vector<std::string> out;
  for (const auto& kv : creators_) out.push_back(kv.first);
  return out;
}
}  // namespace harp
