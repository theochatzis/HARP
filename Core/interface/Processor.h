#ifndef HARP_CORE_PROCESSOR_H
#define HARP_CORE_PROCESSOR_H
// The scheduler: builds the modules from the JSON configuration, runs the
// event loop and executes the path (producers / filters / analyzers in the
// configured order; a failing filter stops the path for that event).
//
// Configuration (produced by python harp.config.Process):
// {
//   "process": "HADRONSKIM",
//   "input":   {"files": [...], "treeName": "Events", "maxEvents": -1, "skipEvents": 0},
//   "output":  {"histFile": "hists.root"},
//   "options": {"logLevel": "INFO", "reportEvery": 1000, "pluginDirs": []},
//   "modules": {"label": {"@type": "ModuleType", ...parameters}},
//   "path":    ["label1", "label2", ...]
// }
#include "Core/interface/Event.h"
#include "Core/interface/HistService.h"
#include "Core/interface/InputSource.h"
#include "Core/interface/Module.h"
#include "Core/interface/ParameterSet.h"
#include "Core/interface/ProductRegistry.h"

#include <memory>
#include <string>
#include <vector>

class TH1D;

namespace harp {

class Processor {
 public:
  explicit Processor(ParameterSet config);
  // Build everything (input, modules). Separate from run() so that --dump can inspect.
  void initialize();
  int run();
  std::string summary() const;  // module table, products, branches

 private:
  struct Slot {
    std::string label;
    std::unique_ptr<ModuleBase> module;
    std::unique_ptr<ModuleContext> context;
    long long passed = 0;
  };
  void buildModules();
  void validateDependencies() const;
  void bookCutflow();

  ParameterSet config_;
  std::unique_ptr<InputSource> input_;
  std::unique_ptr<BranchRegistry> branches_;
  HistService hists_;
  ProductRegistry products_;
  std::vector<Slot> path_;
  long long maxEvents_ = -1;
  long long skipEvents_ = 0;
  long long reportEvery_ = 1000;
  TH1D* cutflow_ = nullptr;
  bool initialized_ = false;
};

}  // namespace harp
#endif
