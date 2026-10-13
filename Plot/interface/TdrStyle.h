#ifndef HARP_PLOT_TDRSTYLE_H
#define HARP_PLOT_TDRSTYLE_H
// CMS TDR style, ported from tdrstyle_mod22.C (setTDRStyle + CMS_lumi + the
// tdrCanvas / tdrDiCanvas / tdrHist / tdrLeg / tdrDraw helpers by Mikko
// Voutilainen). The only change is the "lumi" text, which is a free string
// (e.g. "Phase-2 (14 TeV)") instead of a run period code.
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>
#include <TH1D.h>
#include <TLegend.h>
#include <TPad.h>

#include <memory>
#include <string>

namespace harp {
namespace tdr {

constexpr bool kSquare = true;
constexpr bool kRectangular = false;

// Text written by cmsLumi(): "CMS" (bold), the extra line(s) (italic) and the right-hand text.
struct LumiText {
  std::string cms = "CMS";
  std::string extra = "Simulation";   // e.g. "Simulation", "Simulation Preliminary"
  std::string extra2 = "";             // optional second line
  std::string right = "Phase-2 (14 TeV)";
  bool writeExtra = true;
};

void setTDRStyle();
// iPosX: 11 top-left in frame, 33 top-right, 22 centre, 0 out of frame (CMS left, extra text next to it)
void cmsLumi(TPad* pad, const LumiText& text, int iPosX = 11);

// Empty axis histogram (not attached to any directory); x1..x2 with MoreLogLabels / NoExponent set.
std::unique_ptr<TH1D> tdrHist(const std::string& name, const std::string& ylabel, double y1, double y2,
                              const std::string& xlabel, double x1, double x2);

// Canvas with the axis histogram drawn ("AXIS") and the CMS text. rightMargin > 0 overrides the
// default right margin (useful for COLZ palettes).
std::unique_ptr<TCanvas> tdrCanvas(const std::string& name, TH1D* axis, const LumiText& text, int iPos = 11,
                                   bool square = kSquare, double rightMargin = -1);
// Two pads sharing the x axis (main + ratio). Pad 1 is the upper, pad 2 the lower.
std::unique_ptr<TCanvas> tdrDiCanvas(const std::string& name, TH1D* axisUp, TH1D* axisDown, const LumiText& text, int iPos = 11);

std::unique_ptr<TLegend> tdrLeg(double x1, double y1, double x2, double y2, double textSize = 0.045, int nColumns = 1);

void tdrDraw(TH1* h, const std::string& opt, int marker = 20, int mcolor = 1, int lstyle = 1, int lcolor = -1,
             int fstyle = 1001, int fcolor = 401, double msize = 1, double lwidth = 1, double falpha = 1);
void tdrDraw(TGraph* g, const std::string& opt, int marker = 20, int mcolor = 1, int lstyle = 1, int lcolor = -1,
             int fstyle = 1001, int fcolor = 401, double msize = 1, double lwidth = 1, double falpha = 1);

}  // namespace tdr
}  // namespace harp
#endif
