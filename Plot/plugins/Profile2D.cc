// Profile2D: draw a TH2 (COLZ, optionally normalised per x-bin so that each
// energy column is a probability distribution) with its profile overlaid,
// in TDR style with a wider right margin for the palette.
//
//   job.add("Profile2D", name="pid/nLayers/vsE_pip", hist="pidCLUE3D/pip/h2_nLayers_vs_E", sources=[Source(file, label="#pi^{+}")],
//           normalizeColumns=True, logx=True, logz=False, profile=True, profileColor="kRed+1", zTitle="", text="",
//           yRegions=layer_regions(firstBinEdge=0))   # labelled detector regions along y (see Plotter::drawRegions)
#include "Plot/interface/PlotStyle.h"
#include "Plot/interface/PlotterFactory.h"
#include "Plot/interface/TdrStyle.h"

#include <TH2.h>
#include <TLatex.h>
#include <TPaletteAxis.h>
#include <TProfile.h>

namespace harp {

class Profile2D : public Plotter {
 public:
  using Plotter::Plotter;
  void draw() override {
    const std::string hist = ps_.get<std::string>("hist", "");
    std::vector<PlotSource> srcs = sources();
    const PlotSource& s = srcs.front();
    auto h2 = ctx_.loader().getSummed<TH2>(s.files, s.hist.empty() ? hist : s.hist);
    const bool normalizeColumns = ps_.get<bool>("normalizeColumns", true);
    if (normalizeColumns) {
      for (int ix = 1; ix <= h2->GetNbinsX(); ++ix) {
        const double integral = h2->Integral(ix, ix, 1, h2->GetNbinsY());
        if (integral <= 0) continue;
        for (int iy = 1; iy <= h2->GetNbinsY(); ++iy) { h2->SetBinContent(ix, iy, h2->GetBinContent(ix, iy) / integral); h2->SetBinError(ix, iy, h2->GetBinError(ix, iy) / integral); }
      }
    }
    const bool logx = ps_.get<bool>("logx", true);
    const std::string xTitle = ps_.get<std::string>("xTitle", h2->GetXaxis()->GetTitle());
    const std::string yTitle = ps_.get<std::string>("yTitle", h2->GetYaxis()->GetTitle());
    auto axis = tdr::tdrHist(PlotContext::safeName(name_) + "_axis", yTitle, h2->GetYaxis()->GetXmin(), h2->GetYaxis()->GetXmax(), xTitle,
                             logx ? std::max(h2->GetXaxis()->GetXmin(), 0.1) : h2->GetXaxis()->GetXmin(), h2->GetXaxis()->GetXmax());
    // wider right margin: palette (4% of the pad) + its labels + its title must fit outside the frame
    const double rightMargin = ps_.get<double>("rightMargin", 0.22);
    auto c = tdr::tdrCanvas(PlotContext::safeName(name_), axis.get(), ctx_.lumi, 11, tdr::kSquare, rightMargin);
    gPad->SetLogx(logx); gPad->SetLogz(ps_.get<bool>("logz", false));
    h2->GetZaxis()->SetTitle(ps_.get<std::string>("zTitle", normalizeColumns ? "Fraction per E_{gen} bin" : "").c_str());
    h2->GetZaxis()->SetTitleOffset(1.45); h2->GetZaxis()->SetTitleSize(0.05); h2->GetZaxis()->SetLabelSize(0.045); h2->GetZaxis()->SetLabelOffset(0.005);
    h2->Draw("COLZ SAME");
    std::unique_ptr<TProfile> prof;
    if (ps_.get<bool>("profile", true)) {
      prof.reset(h2->ProfileX((PlotContext::safeName(name_) + "_pfx").c_str()));
      const int pc = PlotStyle::colorFromName(ps_.get<std::string>("profileColor", "kRed+1"));
      tdr::tdrDraw(prof.get(), "PE1", 20, pc, 1, pc, 0, pc, 1.0, 2.0);
    }
    const std::string text = ps_.get<std::string>("text", s.label);
    if (!text.empty()) {
      TLatex l; l.SetNDC(); l.SetTextFont(42); l.SetTextSize(0.045);
      l.DrawLatex(gPad->GetLeftMargin() + 0.04, 1 - gPad->GetTopMargin() - 0.18, text.c_str());
    }
    drawRegions(axis->GetXaxis()->GetXmin(), axis->GetXaxis()->GetXmax(), axis->GetMinimum(), axis->GetMaximum(), logx, false);
    gPad->RedrawAxis();
    gPad->Update();
    if (auto* pal = dynamic_cast<TPaletteAxis*>(h2->GetListOfFunctions()->FindObject("palette"))) {
      pal->SetX1NDC(1 - gPad->GetRightMargin() + 0.012); pal->SetX2NDC(1 - gPad->GetRightMargin() + 0.052);
      pal->SetY1NDC(gPad->GetBottomMargin()); pal->SetY2NDC(1 - gPad->GetTopMargin());
    }
    ctx_.save(*c, name_);
  }
};

}  // namespace harp
HARP_REGISTER_PLOTTER(harp::Profile2D);
