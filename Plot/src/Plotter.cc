#include "Plot/interface/Plotter.h"

#include "Core/interface/Logger.h"
#include "Plot/interface/PlotStyle.h"

#include <TLatex.h>
#include <TLine.h>
#include <TPad.h>

#include <cmath>
#include <filesystem>

namespace harp {

PlotSource PlotSource::fromConfig(const ParameterSet& ps, std::size_t index) {
  PlotSource s;
  if (ps.has("files")) s.files = ps.get<std::vector<std::string>>("files");
  else s.files = {ps.get<std::string>("file")};
  s.label = ps.get<std::string>("label", "");
  s.hist = ps.get<std::string>("hist", "");
  const std::string c = ps.get<std::string>("color", "");
  s.color = c.empty() ? PlotStyle::color(index) : PlotStyle::colorFromName(c);
  s.marker = ps.get<int>("marker", PlotStyle::marker(index));
  s.lineStyle = ps.get<int>("lineStyle", 1);
  return s;
}

PlotContext::PlotContext(const ParameterSet& job) {
  outputDir_ = job.get<std::string>("outputDir", "plots");
  formats_ = job.get<std::vector<std::string>>("formats", {"pdf", "png"});
  rootFileName_ = job.get<std::string>("rootFile", "");
  ParameterSet label = job.getPSet("label", ParameterSet());
  lumi.cms = label.get<std::string>("cms", "CMS");
  lumi.extra = label.get<std::string>("extra", "Simulation");
  lumi.extra2 = label.get<std::string>("extra2", "");
  lumi.right = label.get<std::string>("right", "Phase-2 (14 TeV)");
  lumi.writeExtra = !lumi.extra.empty();
  std::filesystem::create_directories(outputDir_);
}

std::string PlotContext::safeName(const std::string& name) {
  std::string out = name;
  for (auto& ch : out) if (ch == '/') ch = '_';
  return out;
}

void PlotContext::save(TCanvas& c, const std::string& name) const {
  std::filesystem::create_directories(std::filesystem::path(outputDir_ + "/" + name).parent_path());
  for (const auto& fmt : formats_) {
    const std::string path = outputDir_ + "/" + name + "." + fmt;
    c.SaveAs(path.c_str());
  }
  HARP_INFO("Plot") << "saved " << outputDir_ << "/" << name << " (" << formats_.size() << " format(s))";
}

TFile* PlotContext::rootFile() {
  if (rootFileName_.empty()) return nullptr;
  if (!rootFile_) {
    // remove a previous copy first: overwriting on EOS leaves a hidden .sys.v#.<file>/ version directory
    std::error_code ec;
    std::filesystem::remove(rootFileName_, ec);
    rootFile_.reset(TFile::Open(rootFileName_.c_str(), "RECREATE"));
  }
  return rootFile_.get();
}

namespace {
// axis value -> NDC inside the current pad
double toNDC(double v, double lo, double hi, double mLo, double mHi, bool log) {
  const double f = log ? (std::log(v) - std::log(lo)) / (std::log(hi) - std::log(lo)) : (v - lo) / (hi - lo);
  return mLo + f * (1 - mLo - mHi);
}
}  // namespace

void Plotter::drawRegions(double x1, double x2, double y1, double y2, bool logx, bool logy) const {
  for (const bool alongX : {true, false}) {
    const std::string key = alongX ? "regions" : "yRegions";
    if (!ps_.has(key)) continue;
    const std::vector<ParameterSet> regions = ps_.getVPSet(key);
    if (regions.empty()) continue;
    const double lo = alongX ? x1 : y1, hi = alongX ? x2 : y2;
    const bool log = alongX ? logx : logy;
    const double mLo = alongX ? gPad->GetLeftMargin() : gPad->GetBottomMargin();
    const double mHi = alongX ? gPad->GetRightMargin() : gPad->GetTopMargin();
    const int lineColor = PlotStyle::colorFromName(ps_.get<std::string>("regionLineColor", "kGray+2"));
    // label position across the other axis, NDC: just inside the frame top (x regions) / right edge (y regions)
    const double labelPos = ps_.get<double>("regionLabelPos", alongX ? 1 - gPad->GetTopMargin() - 0.045 : 1 - gPad->GetRightMargin() - 0.03);
    const double textSize = ps_.get<double>("regionTextSize", 0.038);
    for (std::size_t i = 0; i < regions.size(); ++i) {
      const double r1 = std::max(regions[i].get<double>("x1"), lo), r2 = std::min(regions[i].get<double>("x2"), hi);
      if (r2 <= r1) continue;
      if (i + 1 < regions.size() && r2 > lo && r2 < hi) {  // inner boundary
        TLine* l = alongX ? new TLine(r2, y1, r2, y2) : new TLine(x1, r2, x2, r2);
        l->SetLineStyle(ps_.get<int>("regionLineStyle", 7)); l->SetLineColor(lineColor); l->SetLineWidth(1); l->Draw();
      }
      const std::string label = regions[i].get<std::string>("label", "");
      if (label.empty()) continue;
      const double mid = log ? std::sqrt(r1 * r2) : 0.5 * (r1 + r2);
      const double along = toNDC(mid, lo, hi, mLo, mHi, log);
      TLatex t; t.SetNDC(); t.SetTextFont(42); t.SetTextSize(textSize); t.SetTextColor(lineColor);
      if (alongX) { t.SetTextAlign(23); t.DrawLatex(along, labelPos, label.c_str()); }
      else { t.SetTextAlign(21); t.SetTextAngle(90); t.DrawLatex(labelPos, along, label.c_str()); }
    }
  }
}

std::vector<PlotSource> Plotter::sources() const {
  std::vector<PlotSource> out;
  std::size_t i = 0;
  for (const auto& ps : ps_.getVPSet("sources")) out.push_back(PlotSource::fromConfig(ps, i++));
  if (out.empty()) throw std::runtime_error("plot '" + name_ + "': no sources");
  return out;
}

}  // namespace harp
