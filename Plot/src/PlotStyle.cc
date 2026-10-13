#include "Plot/interface/PlotStyle.h"
#include "Plot/interface/TdrStyle.h"

#include <TColor.h>
#include <TLatex.h>
#include <TPad.h>
#include <TROOT.h>
#include <TStyle.h>

#include <cstdlib>
#include <map>

namespace harp {

void PlotStyle::apply() { tdr::setTDRStyle(); }

int PlotStyle::color(std::size_t i) {
  // colour-blind friendly palette (Okabe-Ito + extras)
  static const std::vector<int> cols = {TColor::GetColor("#000000"), TColor::GetColor("#E69F00"), TColor::GetColor("#56B4E9"), TColor::GetColor("#009E73"),
                                        TColor::GetColor("#D55E00"), TColor::GetColor("#CC79A7"), TColor::GetColor("#0072B2"), TColor::GetColor("#F0E442"),
                                        TColor::GetColor("#999999"), TColor::GetColor("#882255")};
  return cols[i % cols.size()];
}

int PlotStyle::marker(std::size_t i) {
  static const std::vector<int> m = {20, 21, 22, 23, 33, 34, 29, 24, 25, 26};
  return m[i % m.size()];
}

int PlotStyle::colorFromName(const std::string& name) {
  if (name.empty()) return -1;
  if (name[0] == '#') return TColor::GetColor(name.c_str());
  static const std::map<std::string, int> base = {{"kBlack", kBlack}, {"kRed", kRed}, {"kBlue", kBlue}, {"kGreen", kGreen}, {"kMagenta", kMagenta},
                                                  {"kCyan", kCyan}, {"kOrange", kOrange}, {"kGray", kGray}, {"kViolet", kViolet}, {"kAzure", kAzure},
                                                  {"kTeal", kTeal}, {"kSpring", kSpring}, {"kPink", kPink}, {"kYellow", kYellow}};
  std::string key = name;
  int offset = 0;
  const auto pos = name.find_first_of("+-");
  if (pos != std::string::npos) { key = name.substr(0, pos); offset = std::atoi(name.substr(pos).c_str()); }
  auto it = base.find(key);
  if (it != base.end()) return it->second + offset;
  return std::atoi(name.c_str());
}

void PlotStyle::drawLabel(const std::string& left, const std::string& right, const std::string& extra) {
  const double lm = gPad ? gPad->GetLeftMargin() : gStyle->GetPadLeftMargin();
  const double rm = gPad ? gPad->GetRightMargin() : gStyle->GetPadRightMargin();
  const double tm = gPad ? gPad->GetTopMargin() : gStyle->GetPadTopMargin();
  TLatex l;
  l.SetNDC(); l.SetTextFont(42); l.SetTextSize(0.04);
  l.SetTextAlign(11); l.DrawLatex(lm, 1 - tm + 0.015, ("#bf{" + left + "}").c_str());
  l.SetTextAlign(31); l.DrawLatex(1 - rm, 1 - tm + 0.015, right.c_str());
  if (!extra.empty()) { l.SetTextAlign(11); l.SetTextSize(0.035); l.DrawLatex(lm + 0.03, 1 - tm - 0.06, extra.c_str()); }
}

std::unique_ptr<TLegend> PlotStyle::legend(double x1, double y1, double x2, double y2) {
  auto leg = std::make_unique<TLegend>(x1, y1, x2, y2);
  leg->SetTextSize(0.035);
  leg->SetFillStyle(0);
  return leg;
}

std::unique_ptr<TCanvas> PlotStyle::canvas(const std::string& name, int w, int h) {
  std::string safe = name;
  for (auto& ch : safe) if (ch == '/') ch = '_';
  auto c = std::make_unique<TCanvas>(safe.c_str(), safe.c_str(), w, h);
  return c;
}

}  // namespace harp
