// ResponseVsEnergy: the "money plot" in TDR style. For every source take a TH2
// (x = E_gen, y = E_reco/E_gen), extract scale and resolution in each energy
// bin with Fit/SliceFitter and draw scale vs E and resolution vs E.
// Graphs are also written to the job's ROOT file for later fits.
//
//   job.add("ResponseVsEnergy", name="response/clue3d/allRaw/vsE", hist="respCLUE3D/{hadron}/h2_allRaw_vs_E",
//           sources=[Source(file, label="#pi^{+}", hist=...), ...],
//           method="gauss", nSigma=1.5, minEntries=20, logx=True, xRange=[1, 200],
//           scaleRange=[0, 1.4], resolutionRange=[0, 0.6], fitResolution=False, drawSlices=False,
//           legend=[0.62, 0.55, 0.92, 0.90], text="")
#include "Fit/interface/ResolutionFunction.h"
#include "Fit/interface/SliceFitter.h"
#include "Plot/interface/PlotStyle.h"
#include "Plot/interface/PlotterFactory.h"
#include "Plot/interface/TdrStyle.h"

#include <TH2.h>
#include <TLatex.h>
#include <TLine.h>

namespace harp {

class ResponseVsEnergy : public Plotter {
 public:
  using Plotter::Plotter;
  void draw() override {
    const std::string hist = ps_.get<std::string>("hist", "");
    SliceFitter::Options opt;
    opt.method = ps_.get<std::string>("method", "gauss");
    opt.nSigma = ps_.get<double>("nSigma", 1.5);
    opt.minEntries = ps_.get<long long>("minEntries", 20);
    opt.keepSlices = ps_.get<bool>("drawSlices", false);
    SliceFitter fitter(opt);
    const bool logx = ps_.get<bool>("logx", true);
    const std::vector<double> sr = ps_.get<std::vector<double>>("scaleRange", {0., 1.4});
    const std::vector<double> rr = ps_.get<std::vector<double>>("resolutionRange", {0., 1.2});
    const bool fitRes = ps_.get<bool>("fitResolution", false);
    const std::string xTitle = ps_.get<std::string>("xTitle", "E_{gen} (GeV)");
    const std::string scaleTitle = ps_.get<std::string>("scaleTitle", "#LTE_{reco}/E_{gen}#GT");
    const std::string resTitle = ps_.get<std::string>("resolutionTitle", "#sigma(E_{reco}) / #LTE_{reco}#GT");

    std::vector<PlotSource> srcs = sources();
    std::vector<std::unique_ptr<TGraphErrors>> scales, resolutions;
    std::vector<std::string> fitTexts;
    double xmin = 1e9, xmax = -1e9;
    for (std::size_t i = 0; i < srcs.size(); ++i) {
      const std::string path = srcs[i].hist.empty() ? hist : srcs[i].hist;
      if (path.empty()) throw std::runtime_error("ResponseVsEnergy '" + name_ + "': no histogram path");
      auto h2 = ctx_.loader().getSummed<TH2>(srcs[i].files, path);
      xmin = std::min(xmin, h2->GetXaxis()->GetXmin()); xmax = std::max(xmax, h2->GetXaxis()->GetXmax());
      auto res = fitter.fit(*h2, "_" + std::to_string(i));
      auto gs = SliceFitter::scaleGraph(res, PlotContext::safeName(name_) + "_scale_" + std::to_string(i));
      auto gr = SliceFitter::resolutionGraph(res, PlotContext::safeName(name_) + "_resolution_" + std::to_string(i));
      for (auto* g : {gs.get(), gr.get()}) g->SetTitle(srcs[i].label.c_str());
      if (fitRes && gr->GetN() > 2) {
        ResolutionFitResult fr = ResolutionFunction::fit(*gr, xmin, xmax);
        if (fr.valid) { fr.function->SetLineColor(srcs[i].color); fr.function->SetLineStyle(2); fr.function->SetLineWidth(2); fitTexts.push_back(fr.text()); resolutionFits_.push_back(std::move(fr.function)); }
      }
      if (TFile* f = ctx_.rootFile()) { f->cd(); gs->Write(); gr->Write(); }
      if (opt.keepSlices) drawSlices(res, srcs[i], i);
      scales.push_back(std::move(gs)); resolutions.push_back(std::move(gr));
    }
    xRange_ = ps_.get<std::vector<double>>("xRange", {logx ? std::max(xmin, 0.1) : xmin, xmax});
    drawGraphs(scales, srcs, name_ + "_scale", xTitle, scaleTitle, sr, logx, true, {});
    if (fitTexts.size() != srcs.size()) fitTexts.clear();
    drawGraphs(resolutions, srcs, name_ + "_resolution", xTitle, resTitle, rr, logx, false, fitTexts);
  }

 private:
  void drawGraphs(std::vector<std::unique_ptr<TGraphErrors>>& gs, const std::vector<PlotSource>& srcs, const std::string& cname,
                  const std::string& xTitle, const std::string& yTitle, const std::vector<double>& yr, bool logx, bool unityLine, const std::vector<std::string>& texts) {
    const std::vector<double> lg = ps_.get<std::vector<double>>("legend", {0.40, 0.70, 0.93, 0.92});
    const int nCol = ps_.get<int>("legendColumns", 2);
    auto axis = tdr::tdrHist(PlotContext::safeName(cname) + "_axis", yTitle, yr.at(0), yr.at(1), xTitle, xRange_.at(0), xRange_.at(1));
    auto c = tdr::tdrCanvas(PlotContext::safeName(cname), axis.get(), ctx_.lumi);
    gPad->SetLogx(logx);
    if (unityLine) { TLine* l = new TLine(xRange_.at(0), 1, xRange_.at(1), 1); l->SetLineStyle(2); l->SetLineColor(kGray + 1); l->Draw(); }
    auto leg = tdr::tdrLeg(lg.at(0), lg.at(1), lg.at(2), lg.at(3), texts.empty() ? 0.04 : 0.028, nCol);
    for (std::size_t i = 0; i < gs.size(); ++i) {
      if (gs[i]->GetN() == 0) continue;
      tdr::tdrDraw(gs[i].get(), "Pz", srcs[i].marker, srcs[i].color, srcs[i].lineStyle, -1, 0, srcs[i].color, 1.0, 1.0);
      std::string entry = srcs[i].label;
      if (!texts.empty()) entry += "  " + texts[i];   // resolution fit: S and C next to the hadron name
      if (!entry.empty()) leg->AddEntry(gs[i].get(), entry.c_str(), "PLE");
    }
    if (!unityLine) for (auto& f : resolutionFits_) f->Draw("SAME");
    TLatex l; l.SetNDC(); l.SetTextFont(42);
    const std::string text = ps_.get<std::string>("text", "");
    if (!text.empty()) { l.SetTextSize(0.035); l.DrawLatex(gPad->GetLeftMargin() + 0.04, 1 - gPad->GetTopMargin() - 0.18, text.c_str()); }
    gPad->RedrawAxis();
    ctx_.save(*c, cname);
  }

  void drawSlices(const std::vector<SliceResult>& res, const PlotSource& src, std::size_t idx) {
    int n = 0;
    for (const auto& r : res) if (r.slice && r.entries > 0) ++n;
    if (n == 0) return;
    const int nx = static_cast<int>(std::ceil(std::sqrt(n))), ny = static_cast<int>(std::ceil(double(n) / nx));
    auto c = std::make_unique<TCanvas>((PlotContext::safeName(name_) + "_slices_" + std::to_string(idx)).c_str(), "", 300 * nx, 280 * ny);
    c->Divide(nx, ny);
    int pad = 1;
    for (const auto& r : res) {
      if (!r.slice || r.entries <= 0) continue;
      c->cd(pad++);
      r.slice->SetLineColor(src.color);
      r.slice->Draw("HIST");
      TLatex l; l.SetNDC(); l.SetTextSize(0.06);
      l.DrawLatex(0.2, 0.85, Form("%s %.0f-%.0f GeV", src.label.c_str(), r.xLow, r.xHigh));
      l.DrawLatex(0.2, 0.78, Form("#mu=%.3f #sigma=%.3f", r.scale, r.width));
    }
    ctx_.save(*c, name_ + "_slices_" + std::to_string(idx));
  }

  std::vector<std::unique_ptr<TF1>> resolutionFits_;
  std::vector<double> xRange_ = {1., 200.};
};

}  // namespace harp
HARP_REGISTER_PLOTTER(harp::ResponseVsEnergy);
