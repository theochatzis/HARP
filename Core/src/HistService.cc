#include "Core/interface/HistService.h"

#include "Core/interface/Logger.h"

#include <filesystem>
#include <stdexcept>

namespace harp {

HistService::~HistService() { close(); }

void HistService::open(const std::string& fileName) {
  // remove a previous copy first: overwriting on EOS leaves a hidden .sys.v#.<file>/ version directory
  std::error_code ec;
  std::filesystem::remove(fileName, ec);
  file_.reset(TFile::Open(fileName.c_str(), "RECREATE"));
  if (!file_ || file_->IsZombie()) throw std::runtime_error("HistService: cannot create " + fileName);
  HARP_INFO("HistService") << "histogram file " << fileName;
}

TDirectory* HistService::directory(const std::string& path) {
  if (!file_) throw std::runtime_error("HistService: histogram file not open; set output.histFile in the configuration");
  TDirectory* dir = file_.get();
  std::string token;
  for (size_t i = 0; i <= path.size(); ++i) {
    if (i == path.size() || path[i] == '/') {
      if (!token.empty()) {
        TDirectory* sub = dir->GetDirectory(token.c_str());
        if (!sub) sub = dir->mkdir(token.c_str());
        dir = sub;
        token.clear();
      }
    } else {
      token += path[i];
    }
  }
  return dir;
}

void HistService::write() {
  if (!file_) return;
  file_->Write(nullptr, TObject::kOverwrite);
}

void HistService::close() {
  if (!file_) return;
  file_->Close();
  file_.reset();
}

}  // namespace harp
