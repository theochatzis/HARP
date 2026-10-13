#ifndef HARP_CORE_INPUTSOURCE_H
#define HARP_CORE_INPUTSOURCE_H
// Chain of input nano-like files, iterated with a TTreeReader.
#include <TChain.h>
#include <TTreeReader.h>

#include <memory>
#include <string>
#include <vector>

namespace harp {

class InputSource {
 public:
  InputSource(const std::vector<std::string>& files, const std::string& treeName);
  TChain* chain() { return chain_.get(); }
  TTreeReader& reader() { return *reader_; }
  long long entries() const { return nEntries_; }
  const std::vector<std::string>& files() const { return files_; }
  std::string currentFile() const;
  int currentTreeNumber() const { return chain_->GetTreeNumber(); }

 private:
  std::vector<std::string> files_;
  std::unique_ptr<TChain> chain_;
  std::unique_ptr<TTreeReader> reader_;
  long long nEntries_ = 0;
};

// Expand a list of inputs: plain files, "@list.txt" (one file per line), or
// directories (all *.root inside). EOS paths are used via the fuse mount as given.
std::vector<std::string> expandInputFiles(const std::vector<std::string>& inputs);

}  // namespace harp
#endif
