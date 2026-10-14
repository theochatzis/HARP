#ifndef HARP_PLOT_PLOTTER_H
#define HARP_PLOT_PLOTTER_H
// Plotter plugins: one class per figure type, driven by a JSON block produced
// by python harp.plotcfg. A plot job is
// {
//   "outputDir": "plots", "formats": ["pdf","png"], "rootFile": "plots/graphs.root",
//   "label": {"left": "CMS Phase-2 Simulation", "right": "HGCAL"},
//   "plots": [ {"@type": "Overlay1D", "name": "...", "sources": [{"file": "...", "label": "...", "color": "...", "hist": "..."}], ...}, ... ]
// }
#include "Core/interface/ParameterSet.h"
#include "Plot/interface/HistLoader.h"
#include "Plot/interface/TdrStyle.h"

#include <TCanvas.h>
#include <TFile.h>

#include <string>
#include <vector>

namespace harp {

// One input to a plot: file(s) + legend entry + style.
struct PlotSource {
  std::vector<std::string> files;
  std::string label;
  std::string hist;       // optional override of the plot-level histogram path
  int color = -1, marker = -1, lineStyle = 1;
  static PlotSource fromConfig(const ParameterSet& ps, std::size_t index);
};

class PlotContext {
 public:
  PlotContext(const ParameterSet& job);
  HistLoader& loader() { return loader_; }
  const std::string& outputDir() const { return outputDir_; }
  const std::vector<std::string>& formats() const { return formats_; }
  // outputDir/name.<fmt> for each format; 'name' may contain sub-directories ("response/clue3d/allRaw_scale")
  void save(TCanvas& c, const std::string& name) const;
  static std::string safeName(const std::string& name);  // '/' -> '_' for ROOT object names
  TFile* rootFile();                                       // optional file for derived objects (graphs)
  tdr::LumiText lumi;   // CMS / Simulation / right-hand text, from the job's "label" block

 private:
  HistLoader loader_;
  std::string outputDir_;
  std::vector<std::string> formats_;
  std::string rootFileName_;
  std::unique_ptr<TFile> rootFile_;
};

class Plotter {
 public:
  Plotter(const ParameterSet& ps, PlotContext& ctx) : ps_(ps), ctx_(ctx), name_(ps.get<std::string>("name")) {}
  virtual ~Plotter() = default;
  virtual void draw() = 0;
  const std::string& name() const { return name_; }

 protected:
  std::vector<PlotSource> sources() const;
  // Detector regions along an axis ("regions" for x, "yRegions" for y):
  //   regions=[{"label": "CE-E", "x1": 0.5, "x2": 26.5}, {"label": "CE-H", "x1": 26.5, "x2": 47.5}]
  // Draws a dashed line at every inner boundary and the labels centred in each region,
  // just inside the frame (top for x regions, right for y regions). Call after the
  // frame has been drawn, with the frame limits in axis units.
  void drawRegions(double x1, double x2, double y1, double y2, bool logx = false, bool logy = false) const;
  const ParameterSet ps_;
  PlotContext& ctx_;
  std::string name_;
};

}  // namespace harp
#endif
