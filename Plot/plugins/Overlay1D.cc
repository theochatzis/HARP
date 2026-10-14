// Overlay1D: overlay the same (or per-source) 1D histogram from several
// sources in TDR style; optional normalisation, log axes and a ratio panel
// (tdrDiCanvas) with respect to the first source.
//
//   job.add("Overlay1D", name="pid/nLayers/overlay", hist="pidCLUE3D/{hadron}/h_nLayers",
//           sources=[...], normalize=True, logy=False, ratio=False, xTitle="", yTitle="", rebin=1,
//           xRange=[0, 48], yRange=[0, 0.2], ratioRange=[0.5, 1.5], drawOption="PE", legend=[0.6, 0.55, 0.9, 0.9], text="",
//           regions=layer_regions())   # labelled detector regions along x (see Plotter::drawRegions)
#include "Plot/interface/PlotStyle.h"
#include "Plot/interface/PlotterFactory.h"
#include "Plot/interface/TdrStyle.h"

#include <TH1.h>
#include <TLatex.h>
#include <TLine.h>

namespace harp {

class Overlay1D : public Plotter {
 public:
  using Plotter::Plotter;
  void draw() override {
    const std::string hist = ps_.get<std::string>("hist", "");
    const bool normalize = ps_.get<bool>("normalize", false);
    const bool logy = ps_.get<bool>("logy", false), logx = ps_.get<bool>("logx", false);
    const bool ratio = ps_.get<bool>("ratio", false);
    const int rebin = ps_.get<int>("rebin", 1);
    const std::vector<double> xr = ps_.get<std::vector<double>>("xRange", {});
    const std::vector<double> yr = ps_.get<std::vector<double>>("yRange", {});
    const std::vector<double> rr = ps_.get<std::vector<double>>("ratioRange", {0.5, 1.5});
    const std::vector<double> lg = ps_.get<std::vector<double>>("legend", {0.40, 0.70, 0.93, 0.92});
    const int nCol = ps_.get<int>("legendColumns", 2);
    const std::string opt = ps_.get<std::string>("drawOption", "PE");

    std::vector<std::unique_ptr<TH1>> hs;
    std::vector<PlotSource> srcs = sources();
    for (const auto& s : srcs) {
      const std::string path = s.hist.empty() ? hist : s.hist;
      if (path.empty()) throw std::runtime_error("Overlay1D '" + name_ + "': no histogram path");
      auto h = ctx_.loader().getSummed<TH1>(s.files, path);
      if (rebin > 1) h->Rebin(rebin);
      if (normalize && h->Integral() > 0) h->Scale(1.0 / h->Integral());
      hs.push_back(std::move(h));
    }
    TH1* first = hs.front().get();
    double ymax = 0, ymin = 1e30;
    for (auto& h : hs) {
      ymax = std::max(ymax, h->GetMaximum());
      for (int i = 1; i <= h->GetNbinsX(); ++i) if (h->GetBinContent(i) > 0) ymin = std::min(ymin, h->GetBinContent(i));
    }
    const double x1 = xr.empty() ? first->GetXaxis()->GetXmin() : xr.at(0);
    const double x2 = xr.empty() ? first->GetXaxis()->GetXmax() : xr.at(1);
    const double y1 = !yr.empty() ? yr.at(0) : (logy ? std::max(ymin * 0.5, 1e-6) : 0.);
    const double y2 = !yr.empty() ? yr.at(1) : (logy ? ymax * 100 : ymax * 1.7);
    const std::string xTitle = ps_.get<std::string>("xTitle", first->GetXaxis()->GetTitle());
    const std::string yTitle = ps_.get<std::string>("yTitle", normalize ? "Fraction of tracksters" : first->GetYaxis()->GetTitle());

    auto axis = tdr::tdrHist(PlotContext::safeName(name_) + "_axis", yTitle, y1, y2, xTitle, x1, x2);
    std::unique_ptr<TCanvas> c;
    std::unique_ptr<TH1D> axisDown;
    if (ratio) {
      axisDown = tdr::tdrHist(PlotContext::safeName(name_) + "_axisdw", "Ratio", rr.at(0), rr.at(1), xTitle, x1, x2);
      c = tdr::tdrDiCanvas(PlotContext::safeName(name_), axis.get(), axisDown.get(), ctx_.lumi);
      c->cd(1);
    } else {
      c = tdr::tdrCanvas(PlotContext::safeName(name_), axis.get(), ctx_.lumi);
    }
    gPad->SetLogy(logy); gPad->SetLogx(logx);
    auto leg = tdr::tdrLeg(lg.at(0), lg.at(1), lg.at(2), lg.at(3), 0.04, nCol);
    for (std::size_t i = 0; i < hs.size(); ++i) {
      tdr::tdrDraw(hs[i].get(), opt, srcs[i].marker, srcs[i].color, srcs[i].lineStyle, -1, 0, srcs[i].color, 1.0, 2.0);
      if (!srcs[i].label.empty()) leg->AddEntry(hs[i].get(), srcs[i].label.c_str(), opt.find('P') != std::string::npos ? "PLE" : "L");
    }
    drawText(ps_.get<std::string>("text", ""));
    drawRegions(x1, x2, y1, y2, logx, logy);
    gPad->RedrawAxis();
    std::vector<std::unique_ptr<TH1>> ratios;
    if (ratio) {
      c->cd(2);
      gPad->SetLogx(logx);
      TLine* l = new TLine(x1, 1, x2, 1); l->SetLineStyle(2); l->SetLineColor(kGray + 1); l->Draw();
      for (std::size_t i = 1; i < hs.size(); ++i) {
        std::unique_ptr<TH1> r(static_cast<TH1*>(hs[i]->Clone()));
        r->Divide(hs.front().get());
        tdr::tdrDraw(r.get(), "PE", srcs[i].marker, srcs[i].color, srcs[i].lineStyle);
        ratios.push_back(std::move(r));
      }
      gPad->RedrawAxis();
      c->cd(0);
    }
    ctx_.save(*c, name_);
  }

 private:
  static void drawText(const std::string& text) {
    if (text.empty()) return;
    TLatex l; l.SetNDC(); l.SetTextFont(42); l.SetTextSize(0.035);
    l.DrawLatex(gPad->GetLeftMargin() + 0.04, 1 - gPad->GetTopMargin() - 0.18, text.c_str());
  }
};

}  // namespace harp
HARP_REGISTER_PLOTTER(harp::Overlay1D);
