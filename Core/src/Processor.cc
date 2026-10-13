#include "Core/interface/Processor.h"

#include "Core/interface/Logger.h"
#include "Core/interface/ModuleFactory.h"
#include "Core/interface/PluginLoader.h"

#include <TH1D.h>
#include <TStopwatch.h>

#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>

namespace harp {

Processor::Processor(ParameterSet config) : config_(std::move(config)) {}

void Processor::initialize() {
  if (initialized_) return;
  ParameterSet options = config_.getPSet("options", ParameterSet());
  Logger::setLevel(Logger::levelFromString(options.get<std::string>("logLevel", "INFO")));
  reportEvery_ = options.get<long long>("reportEvery", 1000);

  Logger::banner("HARP process " + config_.get<std::string>("process", "HARP"));

  // plugins
  std::vector<std::string> pluginDirs = options.get<std::vector<std::string>>("pluginDirs", {});
  PluginLoader::loadAll(pluginDirs, "libHarp*Plugin.so", "libHarpPlotters*");

  // input
  ParameterSet in = config_.getPSet("input");
  std::vector<std::string> files = expandInputFiles(in.get<std::vector<std::string>>("files"));
  input_ = std::make_unique<InputSource>(files, in.get<std::string>("treeName", "Events"));
  maxEvents_ = in.get<long long>("maxEvents", -1);
  skipEvents_ = in.get<long long>("skipEvents", 0);
  branches_ = std::make_unique<BranchRegistry>(input_->reader(), input_->chain());

  // output
  ParameterSet out = config_.getPSet("output", ParameterSet());
  const std::string histFile = out.get<std::string>("histFile", "");
  if (!histFile.empty()) hists_.open(histFile);

  buildModules();
  validateDependencies();
  if (hists_.isOpen()) bookCutflow();
  initialized_ = true;
}

void Processor::buildModules() {
  ParameterSet modules = config_.getPSet("modules");
  std::vector<std::string> path = config_.get<std::vector<std::string>>("path");
  std::set<std::string> seen;
  for (const auto& label : path) {
    if (seen.count(label)) throw std::runtime_error("Processor: module '" + label + "' appears twice in the path");
    seen.insert(label);
    if (!modules.has(label)) throw std::runtime_error("Processor: module '" + label + "' in path is not defined in 'modules'");
    ParameterSet ps = modules.getPSet(label);
    const std::string type = ps.get<std::string>("@type");
    Slot slot;
    slot.label = label;
    slot.context = std::make_unique<ModuleContext>(label, type, *branches_, hists_, products_);
    try {
      slot.module = ModuleFactory::instance().create(type, ps, *slot.context);
    } catch (const std::exception& e) {
      throw std::runtime_error("Processor: failed to construct module '" + label + "' (" + type + "): " + e.what());
    }
    HARP_INFO("Processor") << "module " << std::left << std::setw(24) << label << " type " << std::setw(26) << type << " [" << slot.module->kindName() << "]";
    path_.push_back(std::move(slot));
  }
  if (path_.empty()) throw std::runtime_error("Processor: empty path");
}

void Processor::validateDependencies() const {
  std::set<std::string> available;
  for (const auto& slot : path_) {
    for (const auto& dep : slot.context->consumedProducts()) {
      const std::string resolved = products_.resolve(dep);
      if (!available.count(resolved))
        HARP_WARNING("Processor") << "module '" << slot.label << "' consumes product '" << dep << "' which is not produced earlier in the path"
                                  << (products_.has(resolved) ? " (produced later by '" + products_.get(resolved).module + "')" : " (unknown product)");
    }
    for (const auto& p : products_.all())
      if (p.module == slot.label) available.insert(p.label);
  }
}

void Processor::bookCutflow() {
  HistDir d(hists_, "harp");
  cutflow_ = d.book1D("cutflow", "events passing each path step;module;events", static_cast<int>(path_.size()) + 1, 0, path_.size() + 1);
  cutflow_->GetXaxis()->SetBinLabel(1, "all");
  for (size_t i = 0; i < path_.size(); ++i) cutflow_->GetXaxis()->SetBinLabel(static_cast<int>(i) + 2, path_[i].label.c_str());
}

int Processor::run() {
  initialize();
  TStopwatch timer;
  timer.Start();

  for (auto& slot : path_) slot.module->beginJob();

  Event event(products_);
  TTreeReader& reader = input_->reader();
  ValueHandle<unsigned int> runH = branches_->value<unsigned int>("run", true);
  ValueHandle<unsigned int> lumiH = branches_->value<unsigned int>("luminosityBlock", true);
  ValueHandle<unsigned long long> evtH = branches_->value<unsigned long long>("event", true);

  const long long total = input_->entries();
  long long first = skipEvents_;
  long long last = (maxEvents_ < 0) ? total : std::min(total, first + maxEvents_);
  HARP_INFO("Processor") << "processing entries [" << first << ", " << last << ") of " << total;

  int currentTree = -1;
  long long processed = 0;
  if (first > 0) reader.SetEntry(first - 1);
  for (long long entry = first; entry < last; ++entry) {
    if (!reader.Next()) {
      HARP_ERROR("Processor") << "TTreeReader stopped at entry " << entry << " status " << reader.GetEntryStatus();
      break;
    }
    if (input_->currentTreeNumber() != currentTree) {
      currentTree = input_->currentTreeNumber();
      HARP_INFO("Processor") << "opened file " << input_->currentFile();
      for (auto& slot : path_) slot.module->beginFile(input_->currentFile());
    }
    event.clear();
    EventID id;
    if (runH.valid()) id.run = runH.get();
    if (lumiH.valid()) id.lumi = lumiH.get();
    if (evtH.valid()) id.event = evtH.get();
    event.setEntry(entry, id);
    if (cutflow_) cutflow_->Fill(0.5);

    for (size_t i = 0; i < path_.size(); ++i) {
      ModuleBase* m = path_[i].module.get();
      bool pass = true;
      try {
        switch (m->kind()) {
          case ModuleKind::Producer: static_cast<Producer*>(m)->produce(event); break;
          case ModuleKind::Filter: pass = static_cast<Filter*>(m)->filter(event); break;
          case ModuleKind::Analyzer: static_cast<Analyzer*>(m)->analyze(event); break;
        }
      } catch (const std::exception& e) {
        throw std::runtime_error("module '" + path_[i].label + "' failed at entry " + std::to_string(entry) + ": " + e.what());
      }
      if (!pass) break;
      ++path_[i].passed;
      if (cutflow_) cutflow_->Fill(i + 1.5);
    }
    ++processed;
    if (reportEvery_ > 0 && processed % reportEvery_ == 0)
      HARP_INFO("Processor") << "processed " << processed << " / " << (last - first) << " events (" << std::fixed << std::setprecision(1) << timer.RealTime() << " s)", timer.Continue();
  }

  for (auto& slot : path_) slot.module->endJob();
  hists_.write();
  hists_.close();
  timer.Stop();

  std::ostringstream os;
  os << "\n" << std::left << std::setw(26) << "module" << std::setw(12) << "kind" << "passed\n";
  os << std::setw(26) << "(all)" << std::setw(12) << "" << processed << "\n";
  for (const auto& slot : path_) os << std::setw(26) << slot.label << std::setw(12) << slot.module->kindName() << slot.passed << "\n";
  HARP_INFO("Processor") << "done: " << processed << " events in " << std::fixed << std::setprecision(1) << timer.RealTime() << " s" << os.str();
  return 0;
}

std::string Processor::summary() const {
  std::ostringstream os;
  os << "Path:\n";
  for (const auto& slot : path_) os << "  " << std::left << std::setw(24) << slot.label << slot.module->type() << " [" << slot.module->kindName() << "]\n";
  os << "Products:\n";
  for (const auto& p : products_.all()) os << "  " << std::left << std::setw(36) << p.label << p.typeName << "\n";
  os << "Input branches registered: " << branches_->nRegistered() << " of " << branches_->branchNames().size() << "\n";
  return os.str();
}

}  // namespace harp
