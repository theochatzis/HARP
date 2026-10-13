#include "Plot/interface/Plotter.h"

#include "Core/interface/Logger.h"
#include "Plot/interface/PlotStyle.h"

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

std::vector<PlotSource> Plotter::sources() const {
  std::vector<PlotSource> out;
  std::size_t i = 0;
  for (const auto& ps : ps_.getVPSet("sources")) out.push_back(PlotSource::fromConfig(ps, i++));
  if (out.empty()) throw std::runtime_error("plot '" + name_ + "': no sources");
  return out;
}

}  // namespace harp
