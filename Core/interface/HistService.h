#ifndef HARP_CORE_HISTSERVICE_H
#define HARP_CORE_HISTSERVICE_H
// TFileService-like histogram booking. Every module gets its own TDirectory
// (named after the module label) in the process-level histogram file.
//
//   HistDir d = ctx.hists();
//   TH1D* h = d.book1D("h_energy", "E;E [GeV];events", 100, 0, 200);
//   TH2D* h2 = d.book<TH2D>("h_resp", "title", 50, 0, 200, 100, 0, 2);
#include <TDirectory.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>

#include <memory>
#include <string>
#include <vector>

namespace harp {

class HistService {
 public:
  HistService() = default;
  ~HistService();
  void open(const std::string& fileName);
  bool isOpen() const { return file_ != nullptr; }
  TDirectory* directory(const std::string& path);  // creates nested dirs on demand
  void write();
  void close();
  TFile* file() { return file_.get(); }

 private:
  std::unique_ptr<TFile> file_;
};

class HistDir {
 public:
  HistDir(HistService& service, std::string path) : service_(service), path_(std::move(path)) {}
  HistDir subdir(const std::string& name) const { return HistDir(service_, path_ + "/" + name); }
  const std::string& path() const { return path_; }

  template <class H, class... Args>
  H* book(const std::string& name, const std::string& title, Args&&... args) {
    TDirectory* dir = service_.directory(path_);
    TDirectory::TContext ctx(dir);
    H* h = new H(name.c_str(), title.c_str(), std::forward<Args>(args)...);
    h->SetDirectory(dir);
    h->Sumw2();
    return h;
  }
  TH1D* book1D(const std::string& name, const std::string& title, int nbins, double lo, double hi) {
    return book<TH1D>(name, title, nbins, lo, hi);
  }
  TH1D* book1D(const std::string& name, const std::string& title, const std::vector<double>& edges) {
    return book<TH1D>(name, title, static_cast<int>(edges.size()) - 1, edges.data());
  }
  TH2D* book2D(const std::string& name, const std::string& title, int nx, double xlo, double xhi, int ny, double ylo, double yhi) {
    return book<TH2D>(name, title, nx, xlo, xhi, ny, ylo, yhi);
  }
  TH2D* book2D(const std::string& name, const std::string& title, const std::vector<double>& xedges, int ny, double ylo, double yhi) {
    return book<TH2D>(name, title, static_cast<int>(xedges.size()) - 1, xedges.data(), ny, ylo, yhi);
  }
  TH2D* book2D(const std::string& name, const std::string& title, const std::vector<double>& xedges, const std::vector<double>& yedges) {
    return book<TH2D>(name, title, static_cast<int>(xedges.size()) - 1, xedges.data(), static_cast<int>(yedges.size()) - 1, yedges.data());
  }
  TProfile* bookProfile(const std::string& name, const std::string& title, const std::vector<double>& xedges) {
    return book<TProfile>(name, title, static_cast<int>(xedges.size()) - 1, xedges.data());
  }

 private:
  HistService& service_;
  std::string path_;
};

}  // namespace harp
#endif
