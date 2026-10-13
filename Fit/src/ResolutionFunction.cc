#include "Fit/interface/ResolutionFunction.h"

#include <TFitResult.h>
#include <TList.h>

#include <cmath>
#include <sstream>
#include <iomanip>

namespace harp {

std::string ResolutionFitResult::text() const {
  std::ostringstream os;
  os << std::fixed << std::setprecision(2) << "S=" << stochastic << ", C=" << constant;
  if (noise != 0) os << ", N=" << noise;
  return os.str();
}

ResolutionFitResult ResolutionFunction::fit(TGraphErrors& g, double eMin, double eMax, bool includeNoise) {
  ResolutionFitResult r;
  const char* formula = includeNoise ? "sqrt([0]*[0]/x + [1]*[1] + [2]*[2]/(x*x))" : "sqrt([0]*[0]/x + [1]*[1])";
  r.function = std::make_unique<TF1>((std::string(g.GetName()) + "_resfit").c_str(), formula, eMin, eMax);
  r.function->SetParameters(0.5, 0.05, 0.1);
  r.function->SetParNames("S", "C", "N");
  if (!includeNoise) r.function->FixParameter(2, 0);
  TFitResultPtr res = g.Fit(r.function.get(), "QRS0");
  g.GetListOfFunctions()->Clear();  // the caller owns r.function; do not let the graph draw its own copy
  if (res.Get() && res->IsValid()) {
    r.valid = true;
    r.stochastic = std::fabs(r.function->GetParameter(0)); r.stochasticErr = r.function->GetParError(0);
    r.constant = std::fabs(r.function->GetParameter(1)); r.constantErr = r.function->GetParError(1);
    r.noise = includeNoise ? std::fabs(r.function->GetParameter(2)) : 0; r.noiseErr = includeNoise ? r.function->GetParError(2) : 0;
    r.chi2 = r.function->GetChisquare(); r.ndf = r.function->GetNDF();
  }
  return r;
}

}  // namespace harp
