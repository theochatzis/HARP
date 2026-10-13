#include "Fit/interface/SliceFitter.h"

#include "Core/interface/Logger.h"

#include <TF1.h>
#include <TFitResult.h>

#include <algorithm>
#include <cmath>

namespace harp {

std::vector<SliceResult> SliceFitter::fit(const TH2& h2, const std::string& nameTag) const {
  std::vector<SliceResult> out;
  const TAxis* ax = h2.GetXaxis();
  for (int ix = 1; ix <= ax->GetNbins(); ++ix) {
    SliceResult r;
    r.xLow = ax->GetBinLowEdge(ix);
    r.xHigh = ax->GetBinUpEdge(ix);
    r.xCenter = ax->GetBinCenter(ix);
    std::string name = std::string(h2.GetName()) + "_slice" + std::to_string(ix) + nameTag;
    r.slice.reset(h2.ProjectionY(name.c_str(), ix, ix, "e"));
    r.slice->SetDirectory(nullptr);
    r.entries = static_cast<long long>(r.slice->GetEntries());
    if (r.entries >= opt_.minEntries) fitSlice(r);
    if (!opt_.keepSlices) r.slice.reset();
    out.push_back(std::move(r));
  }
  return out;
}

void SliceFitter::fitSlice(SliceResult& r) const {
  TH1D& h = *r.slice;
  const double mean = h.GetMean(), rms = h.GetRMS();
  if (opt_.method == "mean" || rms <= 0) {
    r.scale = mean; r.scaleErr = h.GetMeanError();
    r.width = rms; r.widthErr = h.GetRMSError();
  } else if (opt_.method == "median") {
    double q[3], p[3] = {0.16, 0.5, 0.84};
    h.GetQuantiles(3, q, p);
    r.scale = q[1]; r.scaleErr = 1.2533 * rms / std::sqrt(std::max(1.0, h.GetEffectiveEntries()));
    r.width = 0.5 * (q[2] - q[0]); r.widthErr = h.GetRMSError();
  } else {  // gauss
    double mu = mean, sigma = rms;
    TF1 f("fgaus", "gaus", h.GetXaxis()->GetXmin(), h.GetXaxis()->GetXmax());
    for (int it = 0; it < opt_.iterations; ++it) {
      f.SetParameters(h.GetMaximum(), mu, sigma);
      f.SetRange(mu - opt_.nSigma * sigma, mu + opt_.nSigma * sigma);
      TFitResultPtr res = h.Fit(&f, "QNRS");
      if (!res.Get() || !res->IsValid() || f.GetParameter(2) <= 0) break;
      mu = f.GetParameter(1); sigma = std::fabs(f.GetParameter(2));
      r.scale = mu; r.scaleErr = f.GetParError(1);
      r.width = sigma; r.widthErr = f.GetParError(2);
      r.chi2 = f.GetChisquare(); r.ndf = f.GetNDF();
    }
    if (r.width <= 0) { r.scale = mean; r.scaleErr = h.GetMeanError(); r.width = rms; r.widthErr = h.GetRMSError(); }
  }
  if (r.scale > 0) {
    r.resolution = r.width / r.scale;
    r.resolutionErr = r.resolution * std::sqrt(std::pow(r.widthErr / r.width, 2) + std::pow(r.scaleErr / r.scale, 2));
  }
}

namespace {
std::unique_ptr<TGraphErrors> makeGraph(const std::vector<SliceResult>& res, const std::string& name, double SliceResult::*val, double SliceResult::*err) {
  auto g = std::make_unique<TGraphErrors>();
  g->SetName(name.c_str());
  for (const auto& r : res) {
    if (r.entries <= 0 || r.*val == 0) continue;
    const int i = g->GetN();
    g->SetPoint(i, r.xCenter, r.*val);
    g->SetPointError(i, 0.5 * (r.xHigh - r.xLow), r.*err);
  }
  return g;
}
}  // namespace

std::unique_ptr<TGraphErrors> SliceFitter::scaleGraph(const std::vector<SliceResult>& r, const std::string& name) { return makeGraph(r, name, &SliceResult::scale, &SliceResult::scaleErr); }
std::unique_ptr<TGraphErrors> SliceFitter::resolutionGraph(const std::vector<SliceResult>& r, const std::string& name) { return makeGraph(r, name, &SliceResult::resolution, &SliceResult::resolutionErr); }
std::unique_ptr<TGraphErrors> SliceFitter::widthGraph(const std::vector<SliceResult>& r, const std::string& name) { return makeGraph(r, name, &SliceResult::width, &SliceResult::widthErr); }

}  // namespace harp
