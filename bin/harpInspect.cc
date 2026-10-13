// harpInspect: print the structure of a nano-like file (trees, collections,
// branches with types and documentation), optionally a few entries.
#include "Core/interface/Glob.h"

#include <TBranch.h>
#include <TFile.h>
#include <TKey.h>
#include <TLeaf.h>
#include <TROOT.h>
#include <TTree.h>

#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {
void usage() {
  std::cout <<
      "harpInspect - inspect a nano-like ROOT file\n\n"
      "Usage:\n"
      "  harpInspect <file.root> [options]\n\n"
      "Options:\n"
      "  -t, --tree NAME        tree to inspect (default: Events)\n"
      "  -c, --collections      summary of collections (counter branches) only\n"
      "  -b, --branches GLOB    print branches matching GLOB (default: all)\n"
      "  -e, --entries N        print the collection sizes of the first N entries\n"
      "  -h, --help             this message\n\n"
      "Examples:\n"
      "  harpInspect /eos/user/t/tchatzis/phase2_gun/pip_p1to200/out_000.root -c\n"
      "  harpInspect skim.root -b 'HadronMatch_*'\n";
}
}  // namespace

int main(int argc, char** argv) {
  gROOT->SetBatch(true);
  std::string file, tree = "Events", pattern = "*";
  bool collections = false;
  int entries = 0;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    auto next = [&](const char* opt) -> std::string { if (i + 1 >= argc) { std::cerr << "missing value for " << opt << "\n"; std::exit(1); } return argv[++i]; };
    if (a == "-h" || a == "--help") { usage(); return 0; }
    else if (a == "-t" || a == "--tree") tree = next("-t");
    else if (a == "-c" || a == "--collections") collections = true;
    else if (a == "-b" || a == "--branches") pattern = next("-b");
    else if (a == "-e" || a == "--entries") entries = std::stoi(next("-e"));
    else file = a;
  }
  if (file.empty()) { usage(); return 1; }
  std::unique_ptr<TFile> f(TFile::Open(file.c_str()));
  if (!f || f->IsZombie()) { std::cerr << "cannot open " << file << "\n"; return 1; }
  std::cout << "File: " << file << "\nTrees:";
  for (auto* k : *f->GetListOfKeys()) {
    TKey* key = static_cast<TKey*>(k);
    if (std::string(key->GetClassName()) == "TTree") std::cout << " " << key->GetName() << "(" << static_cast<TTree*>(key->ReadObj())->GetEntries() << ")";
  }
  std::cout << "\n";
  TTree* t = f->Get<TTree>(tree.c_str());
  if (!t) { std::cerr << "tree " << tree << " not found\n"; return 1; }
  std::map<std::string, std::vector<std::string>> byCounter;
  std::vector<std::string> scalars;
  for (auto* b : *t->GetListOfBranches()) {
    TBranch* br = static_cast<TBranch*>(b);
    TLeaf* l = static_cast<TLeaf*>(br->GetListOfLeaves()->At(0));
    const std::string name = br->GetName();
    if (l && l->GetLeafCount()) byCounter[l->GetLeafCount()->GetName()].push_back(name);
    else scalars.push_back(name);
    if (!collections && harp::globMatch(pattern, name))
      std::cout << "  " << std::left << std::setw(70) << name << std::setw(12) << (l ? l->GetTypeName() : "?") << (l && l->GetLeafCount() ? std::string("[") + l->GetLeafCount()->GetName() + "] " : "") << br->GetTitle() << "\n";
  }
  std::cout << "\nCollections (" << byCounter.size() << "):\n";
  for (const auto& kv : byCounter) std::cout << "  " << std::left << std::setw(60) << kv.first << kv.second.size() << " branches\n";
  std::cout << "Scalar branches: " << scalars.size() << "\n";
  if (entries > 0) {
    std::vector<std::pair<std::string, int>> counters;
    for (const auto& kv : byCounter) counters.emplace_back(kv.first, 0);
    std::vector<std::unique_ptr<int>> buf;
    t->SetBranchStatus("*", 0);
    for (auto& c : counters) { t->SetBranchStatus(c.first.c_str(), 1); t->SetBranchAddress(c.first.c_str(), &c.second); }
    for (int e = 0; e < entries && e < t->GetEntries(); ++e) {
      t->GetEntry(e);
      std::cout << "entry " << e << ":";
      for (const auto& c : counters) std::cout << " " << c.first.substr(1) << "=" << c.second;
      std::cout << "\n";
    }
  }
  return 0;
}
