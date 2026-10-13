#ifndef HARP_PLOT_PLOTTERFACTORY_H
#define HARP_PLOT_PLOTTERFACTORY_H
#include "Core/interface/ModuleFactory.h"
#include "Plot/interface/Plotter.h"

#include <functional>
#include <map>
#include <memory>

namespace harp {
class PlotterFactory {
 public:
  using Creator = std::function<std::unique_ptr<Plotter>(const ParameterSet&, PlotContext&)>;
  static PlotterFactory& instance();
  bool registerPlotter(const std::string& type, Creator c);
  std::unique_ptr<Plotter> create(const std::string& type, const ParameterSet& ps, PlotContext& ctx) const;
  std::vector<std::string> available() const;

 private:
  std::map<std::string, Creator> creators_;
};
}  // namespace harp

#define HARP_REGISTER_PLOTTER_IMPL(CLASS, ID)                                                        \
  namespace {                                                                                         \
  const bool harp_registered_plotter_##ID = ::harp::PlotterFactory::instance().registerPlotter(       \
      ::harp::ModuleFactory::stripNamespace(#CLASS), [](const ::harp::ParameterSet& ps, ::harp::PlotContext& ctx) { \
        return std::unique_ptr<::harp::Plotter>(new CLASS(ps, ctx));                                  \
      });                                                                                             \
  }
#define HARP_REGISTER_PLOTTER_EXPAND(CLASS, ID) HARP_REGISTER_PLOTTER_IMPL(CLASS, ID)
#define HARP_REGISTER_PLOTTER(CLASS) HARP_REGISTER_PLOTTER_EXPAND(CLASS, __COUNTER__)
#endif
