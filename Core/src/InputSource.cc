#include "Core/interface/InputSource.h"

#include "Core/interface/Logger.h"

#include <TFile.h>
#include <TTree.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace harp {

std::vector<std::string> expandInputFiles(const std::vector<std::string>& inputs) {
  std::vector<std::string> out;
  for (const auto& in : inputs) {
    if (in.empty()) continue;
    if (in[0] == '@') {
      std::ifstream f(in.substr(1));
      if (!f) throw std::runtime_error("InputSource: cannot open file list " + in.substr(1));
      std::string line;
      while (std::getline(f, line)) {
        auto a = line.find_first_not_of(" \t");
        if (a == std::string::npos || line[a] == '#') continue;
        out.push_back(line.substr(a, line.find_last_not_of(" \t\r") - a + 1));
      }
    } else if (in.rfind("root://", 0) != 0 && fs::is_directory(in)) {
      std::vector<std::string> files;
      for (const auto& e : fs::directory_iterator(in))
        if (e.path().extension() == ".root") files.push_back(e.path().string());
      std::sort(files.begin(), files.end());
      out.insert(out.end(), files.begin(), files.end());
    } else {
      out.push_back(in);
    }
  }
  return out;
}

InputSource::InputSource(const std::vector<std::string>& files, const std::string& treeName) : files_(files) {
  if (files_.empty()) throw std::runtime_error("InputSource: no input files");
  chain_ = std::make_unique<TChain>(treeName.c_str());
  std::vector<std::string> good;
  for (const auto& f : files_) {
    // open each file once: unreadable / truncated files (EOS hiccups) are skipped with a warning
    std::unique_ptr<TFile> probe(TFile::Open(f.c_str()));
    if (!probe || probe->IsZombie() || probe->TestBit(TFile::kRecovered) || !probe->Get<TTree>(treeName.c_str())) {
      HARP_WARNING("InputSource") << "skipping unreadable file " << f;
      continue;
    }
    const long long n = probe->Get<TTree>(treeName.c_str())->GetEntries();
    probe->Close();
    chain_->Add(f.c_str(), n);
    good.push_back(f);
  }
  if (good.size() != files_.size()) HARP_WARNING("InputSource") << (files_.size() - good.size()) << " of " << files_.size() << " files skipped";
  files_ = good;
  if (files_.empty()) throw std::runtime_error("InputSource: no readable input files");
  nEntries_ = chain_->GetEntries();
  reader_ = std::make_unique<TTreeReader>(chain_.get());
  HARP_INFO("InputSource") << files_.size() << " file(s), " << nEntries_ << " entries in tree '" << treeName << "'";
}

std::string InputSource::currentFile() const {
  TFile* f = chain_->GetCurrentFile();
  return f ? f->GetName() : "";
}

}  // namespace harp
