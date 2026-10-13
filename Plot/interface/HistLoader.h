#ifndef HARP_PLOT_HISTLOADER_H
#define HARP_PLOT_HISTLOADER_H
// Opens histogram files once and fetches objects by path; also sums several
// files (e.g. different hadron samples) into one histogram.
#include <TFile.h>
#include <TH1.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace harp {

class HistLoader {
 public:
  TFile* file(const std::string& path);
  // Returns an owned clone (detached from the file); throws if missing.
  template <class H = TH1>
  std::unique_ptr<H> get(const std::string& file, const std::string& histPath, const std::string& newName = "") {
    TObject* o = getObject(file, histPath);
    H* h = dynamic_cast<H*>(o);
    if (!h) throw std::runtime_error("HistLoader: '" + histPath + "' in " + file + " is not of the requested type");
    H* c = static_cast<H*>(h->Clone(newName.empty() ? (std::string(h->GetName()) + "_clone" + std::to_string(counter_++)).c_str() : newName.c_str()));
    c->SetDirectory(nullptr);
    return std::unique_ptr<H>(c);
  }
  // Sum the same histogram from several files.
  template <class H = TH1>
  std::unique_ptr<H> getSummed(const std::vector<std::string>& files, const std::string& histPath, const std::string& newName = "") {
    std::unique_ptr<H> sum;
    for (const auto& f : files) {
      auto h = get<H>(f, histPath, newName);
      if (!sum) sum = std::move(h); else sum->Add(h.get());
    }
    if (!sum) throw std::runtime_error("HistLoader: no files given for " + histPath);
    return sum;
  }
  bool exists(const std::string& file, const std::string& histPath);

 private:
  TObject* getObject(const std::string& file, const std::string& histPath);
  std::map<std::string, std::unique_ptr<TFile>> files_;
  int counter_ = 0;
};

}  // namespace harp
#endif
