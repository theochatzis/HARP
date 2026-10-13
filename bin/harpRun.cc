// harpRun: run a HARP process described by a JSON configuration.
// The JSON is normally produced from a python configuration by `harp run cfg.py ...`.
#include "Core/interface/Glob.h"
#include "Core/interface/Logger.h"
#include "Core/interface/ModuleFactory.h"
#include "Core/interface/ParameterSet.h"
#include "Core/interface/PluginLoader.h"
#include "Core/interface/Processor.h"

#include <TROOT.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {
void usage() {
  std::cout <<
      "harpRun - run a HARP process (producers / filters / analyzers over nano-like input)\n\n"
      "Usage:\n"
      "  harpRun <config.json> [options]\n"
      "  harp run <config_cfg.py> [key=value ...]     (python front-end, generates the JSON)\n\n"
      "Options:\n"
      "  -n, --max-events N      process at most N events (overrides input.maxEvents)\n"
      "  -s, --skip-events N     skip the first N events\n"
      "  -i, --input FILES       comma separated input files (or @list.txt, or a directory); overrides input.files\n"
      "  -o, --hist-file FILE    histogram output file (overrides output.histFile)\n"
      "  -v, --verbose           debug logging\n"
      "  --dump                  print the resolved configuration, module table and products, then exit\n"
      "  --list-modules          list the available module types and exit\n"
      "  -h, --help              this message\n\n"
      "Examples:\n"
      "  harpRun work/skim_hadron_cfg.json -n 1000\n"
      "  harpRun work/hist_hadron_cfg.json -i /eos/user/t/tchatzis/phase2_gun/pip_p1to200/out_000.root -o hists_pip.root\n";
}
}  // namespace

int main(int argc, char** argv) {
  gROOT->SetBatch(true);
  std::string config;
  long long maxEvents = -2, skipEvents = -1;
  std::string inputs, histFile;
  bool dump = false, listModules = false, verbose = false;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    auto next = [&](const char* opt) -> std::string { if (i + 1 >= argc) { std::cerr << "missing value for " << opt << "\n"; std::exit(1); } return argv[++i]; };
    if (a == "-h" || a == "--help") { usage(); return 0; }
    else if (a == "-n" || a == "--max-events") maxEvents = std::stoll(next("-n"));
    else if (a == "-s" || a == "--skip-events") skipEvents = std::stoll(next("-s"));
    else if (a == "-i" || a == "--input") inputs = next("-i");
    else if (a == "-o" || a == "--hist-file") histFile = next("-o");
    else if (a == "-v" || a == "--verbose") verbose = true;
    else if (a == "--dump") dump = true;
    else if (a == "--list-modules") listModules = true;
    else if (!a.empty() && a[0] == '-') { std::cerr << "unknown option " << a << "\n"; usage(); return 1; }
    else config = a;
  }
  if (verbose) harp::Logger::setLevel(harp::LogLevel::Debug);
  try {
    if (listModules) {
      harp::PluginLoader::loadAll({}, "libHarp*Plugin.so", "libHarpPlotters*");
      std::cout << "Available module types:\n";
      for (const auto& t : harp::ModuleFactory::instance().available()) std::cout << "  " << t << "\n";
      return 0;
    }
    if (config.empty()) { std::cerr << "Error: no configuration given. Use -h for help.\n"; return 1; }
    harp::ParameterSet cfg = harp::ParameterSet::fromFile(config);
    nlohmann::json& j = cfg.json();
    if (maxEvents != -2) j["input"]["maxEvents"] = maxEvents;
    if (skipEvents >= 0) j["input"]["skipEvents"] = skipEvents;
    if (!inputs.empty()) { j["input"]["files"] = nlohmann::json::array(); for (const auto& f : harp::splitString(inputs, ',')) j["input"]["files"].push_back(f); }
    if (!histFile.empty()) j["output"]["histFile"] = histFile;
    if (verbose) j["options"]["logLevel"] = "DEBUG";
    harp::Processor processor(cfg);
    if (dump) {
      std::cout << cfg.dump() << "\n";
      processor.initialize();
      std::cout << processor.summary();
      return 0;
    }
    return processor.run();
  } catch (const std::exception& e) {
    std::cerr << "\n[FATAL] " << e.what() << "\n";
    return 1;
  }
}
