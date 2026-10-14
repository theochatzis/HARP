// StackedHist: stack several components of one source on top of each other
// (energy-budget style). Components may be TProfile (their per-bin means are
// stacked), TH2 (projected on y over an x range) or TH1.
//
//   job.add("StackedHist", name="budget/pip/fraction_vsE", sources=[Source(file, label="#pi^{+}")],
//           components=[{"hist": "budgetCLUE3D/pip/p_frac_best_vs_E", "label": "best trackster", "color": "#0072B2"}, ...],
//           normalize=False, logx=True, xTitle="", yTitle="", yRange=[0, 1.2], unityLine=False,
//           projectX=[40, 70],      # TH2 only: x range to project (e.g. an E_gen window)
//           normalizeToEntries="budgetCLUE3D/pip/h_nHadrons_vs_E",   # divide by the integral of this TH1 over projectX (per-hadron quantities)
//           text="", legend=[0.40, 0.70, 0.93, 0.92], legendColumns=1, drawTotal=False,
//           headroom=1.6,           # automatic y maximum = headroom * stack maximum (linear y)
//           regions=layer_regions())  # labelled detector regions along x (see Plotter::drawRegions)
#include "Plot/interface/PlotStyle.h"
#include "Plot/interface/PlotterFactory.h"
#include "Plot/interface/TdrStyle.h"

#include <TH1D.h>
#include <TH2.h>
#include <THStack.h>
#include <TLatex.h>
#include <TLine.h>
#include <TProfile.h>

namespace harp {

class StackedHist : public Plotter {
 public:
  using Plotter::Plotter;
  void draw() override {
    std::vector<PlotSource> srcs = sources();
    const PlotSource& s = srcs.front();
    const bool normalize = ps_.get<bool>("normalize", false);
    const bool logx = ps_.get<bool>("logx", false), logy = ps_.get<bool>("logy", false);
    const std::vector<double> proj = ps_.get<std::vector<double>>("projectX", {});
    const std::vector<double> yr = ps_.get<std::vector<double>>("yRange", {});
    const std::vector<double> xr = ps_.get<std::vector<double>>("xRange", {});
    const int nCol = ps_.get<int>("legendColumns", 1);
    const std::vector<double> lg = ps_.get<std::vector<double>>("legend", nCol > 1 ? std::vector<double>{0.40, 0.70, 0.93, 0.92} : std::vector<double>{0.55, 0.70, 0.93, 0.92});

    std::vector<std::unique_ptr<TH1D>> comps;
    std::vector<std::string> labels;
    std::vector<int> colors;
    std::size_t idx = 0;
    for (const auto& c : ps_.getVPSet("components")) {
      auto raw = ctx_.loader().getSummed<TH1>(s.files, c.get<std::string>("hist"));
      std::unique_ptr<TH1D> h;
      const std::string cname = PlotContext::safeName(name_) + "_c" + std::to_string(idx);
      if (auto* p = dynamic_cast<TProfile*>(raw.get())) {
        h.reset(p->ProjectionX(cname.c_str(), "E"));
      } else if (auto* h2 = dynamic_cast<TH2*>(raw.get())) {
        int b1 = 1, b2 = h2->GetNbinsX();
        if (!proj.empty()) { b1 = h2->GetXaxis()->FindBin(proj.at(0) * 1.0001); b2 = h2->GetXaxis()->FindBin(proj.at(1) * 0.9999); }
        h.reset(h2->ProjectionY(cname.c_str(), b1, b2, "e"));
      } else {
        h.reset(static_cast<TH1D*>(raw->Clone(cname.c_str())));
      }
      h->SetDirectory(nullptr);
      labels.push_back(c.get<std::string>("label", ""));
      const std::string col = c.get<std::string>("color", "");
      colors.push_back(col.empty() ? PlotStyle::color(idx) : PlotStyle::colorFromName(col));
      comps.push_back(std::move(h));
      ++idx;
    }
    if (comps.empty()) throw std::runtime_error("StackedHist '" + name_ + "': no components");
    const std::string normHist = ps_.get<std::string>("normalizeToEntries", "");
    if (!normHist.empty()) {
      auto hn = ctx_.loader().getSummed<TH1>(s.files, normHist);
      int b1 = 1, b2 = hn->GetNbinsX();
      if (!proj.empty()) { b1 = hn->GetXaxis()->FindBin(proj.at(0) * 1.0001); b2 = hn->GetXaxis()->FindBin(proj.at(1) * 0.9999); }
      const double n = hn->Integral(b1, b2);
      if (n > 0) for (auto& h : comps) h->Scale(1.0 / n);
    }
    std::unique_ptr<TH1D> total(static_cast<TH1D*>(comps.front()->Clone((PlotContext::safeName(name_) + "_total").c_str())));
    total->SetDirectory(nullptr);
    for (std::size_t i = 1; i < comps.size(); ++i) total->Add(comps[i].get());
    if (normalize) {
      for (auto& h : comps)
        for (int b = 1; b <= h->GetNbinsX(); ++b) {
          const double t = total->GetBinContent(b);
          h->SetBinContent(b, t > 0 ? h->GetBinContent(b) / t : 0);
          h->SetBinError(b, t > 0 ? h->GetBinError(b) / t : 0);
        }
      for (int b = 1; b <= total->GetNbinsX(); ++b) total->SetBinContent(b, total->GetBinContent(b) > 0 ? 1 : 0);
    }
    const double x1 = xr.empty() ? comps.front()->GetXaxis()->GetXmin() : xr.at(0);
    const double x2 = xr.empty() ? comps.front()->GetXaxis()->GetXmax() : xr.at(1);
    const double y1 = yr.empty() ? (logy ? 1e-3 : 0.) : yr.at(0);
    const double y2 = yr.empty() ? total->GetMaximum() * (logy ? 50 : ps_.get<double>("headroom", 1.6)) : yr.at(1);
    const std::string xTitle = ps_.get<std::string>("xTitle", comps.front()->GetXaxis()->GetTitle());
    const std::string yTitle = ps_.get<std::string>("yTitle", normalize ? "Fraction" : comps.front()->GetYaxis()->GetTitle());

    auto axis = tdr::tdrHist(PlotContext::safeName(name_) + "_axis", yTitle, y1, y2, xTitle, logx ? std::max(x1, 0.5) : x1, x2);
    auto c = tdr::tdrCanvas(PlotContext::safeName(name_), axis.get(), ctx_.lumi);
    gPad->SetLogx(logx); gPad->SetLogy(logy);
    THStack stack((PlotContext::safeName(name_) + "_stack").c_str(), "");
    for (std::size_t i = 0; i < comps.size(); ++i) {
      comps[i]->SetFillColor(colors[i]); comps[i]->SetLineColor(kBlack); comps[i]->SetLineWidth(1); comps[i]->SetFillStyle(1001);
      stack.Add(comps[i].get(), "HIST");
    }
    stack.Draw("HIST SAME");
    if (ps_.get<bool>("drawTotal", false)) tdr::tdrDraw(total.get(), "PE", 20, kBlack, 1, kBlack, 0, kBlack, 0.9, 2.0);
    if (ps_.get<bool>("unityLine", false)) { TLine* l = new TLine(x1, 1, x2, 1); l->SetLineStyle(2); l->SetLineColor(kGray + 2); l->Draw(); }
    auto leg = tdr::tdrLeg(lg.at(0), lg.at(1), lg.at(2), lg.at(3), ps_.get<double>("legendTextSize", 0.038), nCol);
    for (int i = static_cast<int>(comps.size()) - 1; i >= 0; --i) if (!labels[i].empty()) leg->AddEntry(comps[i].get(), labels[i].c_str(), "F");
    const std::string text = ps_.get<std::string>("text", s.label);
    if (!text.empty()) { TLatex l; l.SetNDC(); l.SetTextFont(42); l.SetTextSize(0.04); l.DrawLatex(gPad->GetLeftMargin() + 0.04, 1 - gPad->GetTopMargin() - 0.18, text.c_str()); }
    drawRegions(logx ? std::max(x1, 0.5) : x1, x2, y1, y2, logx, logy);
    gPad->RedrawAxis();
    ctx_.save(*c, name_);
    for (auto& h : comps) stack.GetHists()->Remove(h.get());
  }
};

}  // namespace harp
HARP_REGISTER_PLOTTER(harp::StackedHist);
