#ifndef HARP_FIT_SLICEFITTER_H
#define HARP_FIT_SLICEFITTER_H
// Extract scale and resolution versus x from a TH2 (x = E_gen, y = response).
// Methods: "mean" (arithmetic mean / RMS), "median" (median / 68% interval),
// "gauss" (iterative Gaussian fit within +-nSigma of the core).
#include <TGraphErrors.h>
#include <TH1D.h>
#include <TH2.h>

#include <memory>
#include <string>
#include <vector>

namespace harp {

struct SliceResult {
  double xLow = 0, xHigh = 0, xCenter = 0;
  long long entries = 0;
  double scale = 0, scaleErr = 0;        // central value of the response
  double width = 0, widthErr = 0;        // spread of the response
  double resolution = 0, resolutionErr = 0;  // width / scale
  double chi2 = 0; int ndf = 0;
  std::unique_ptr<TH1D> slice;           // the projected 1D distribution (owned)
};

class SliceFitter {
 public:
  struct Options {
    std::string method = "gauss";
    double nSigma = 1.5;      // fit range in units of sigma around the core (gauss)
    int iterations = 3;       // gauss refinements
    long long minEntries = 20;
    bool keepSlices = true;
  };
  SliceFitter() = default;
  explicit SliceFitter(Options opt) : opt_(std::move(opt)) {}

  std::vector<SliceResult> fit(const TH2& h2, const std::string& nameTag = "") const;
  // Graphs from results: scale vs x and resolution vs x (x error = half bin width).
  static std::unique_ptr<TGraphErrors> scaleGraph(const std::vector<SliceResult>& r, const std::string& name);
  static std::unique_ptr<TGraphErrors> resolutionGraph(const std::vector<SliceResult>& r, const std::string& name);
  static std::unique_ptr<TGraphErrors> widthGraph(const std::vector<SliceResult>& r, const std::string& name);

 private:
  void fitSlice(SliceResult& r) const;
  Options opt_;
};

}  // namespace harp
#endif
