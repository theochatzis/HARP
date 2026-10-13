#ifndef HARP_PLOT_PLOTSTYLE_H
#define HARP_PLOT_PLOTSTYLE_H
// Global ROOT style (TDR-like) plus the label drawn on every canvas.
#include <TCanvas.h>
#include <TLegend.h>

#include <memory>
#include <string>
#include <vector>

namespace harp {

class PlotStyle {
 public:
  static void apply();                                   // set the global TStyle
  static int color(std::size_t i);                       // palette entry (cycles)
  static int marker(std::size_t i);
  static int colorFromName(const std::string& name);     // "kRed+1", "#1f77b4", "633"
  static void drawLabel(const std::string& left = "CMS Phase-2 Simulation", const std::string& right = "HGCAL", const std::string& extra = "");
  static std::unique_ptr<TLegend> legend(double x1 = 0.55, double y1 = 0.68, double x2 = 0.92, double y2 = 0.90);
  static std::unique_ptr<TCanvas> canvas(const std::string& name, int w = 800, int h = 700);
};

}  // namespace harp
#endif
