#include "Plot/interface/HistLoader.h"

#include <stdexcept>

namespace harp {

TFile* HistLoader::file(const std::string& path) {
  auto it = files_.find(path);
  if (it != files_.end()) return it->second.get();
  std::unique_ptr<TFile> f(TFile::Open(path.c_str(), "READ"));
  if (!f || f->IsZombie()) throw std::runtime_error("HistLoader: cannot open " + path);
  TFile* raw = f.get();
  files_[path] = std::move(f);
  return raw;
}

TObject* HistLoader::getObject(const std::string& filePath, const std::string& histPath) {
  TObject* o = file(filePath)->Get(histPath.c_str());
  if (!o) throw std::runtime_error("HistLoader: object '" + histPath + "' not found in " + filePath);
  return o;
}

bool HistLoader::exists(const std::string& filePath, const std::string& histPath) {
  return file(filePath)->Get(histPath.c_str()) != nullptr;
}

}  // namespace harp
