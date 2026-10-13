#ifndef HARP_CORE_OUTPUTTREE_H
#define HARP_CORE_OUTPUTTREE_H
// Writer of a nano-like output tree ("skim"):
//   * copies selected input branches (keep/drop glob patterns) entry by entry,
//   * adds FlatTable products as new nano-style branches.
// Used by the NanoSkimWriter module; can be reused by any other output module.
#include "Core/interface/BranchRegistry.h"
#include "Core/interface/FlatTable.h"

#include <TFile.h>
#include <TTree.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace harp {

class BranchCopierBase {
 public:
  virtual ~BranchCopierBase() = default;
  virtual void copy() = 0;  // input reader -> output buffer
  virtual const std::string& name() const = 0;
};

class TableBranches;  // output buffers of one FlatTable

class OutputTree {
 public:
  OutputTree(const std::string& fileName, const std::string& treeName, const std::string& title = "");
  ~OutputTree();

  // Select input branches to copy. Counter branches of kept arrays are added automatically.
  // Returns the list of copied branches.
  std::vector<std::string> addInputBranches(BranchRegistry& registry, const std::vector<std::string>& keep, const std::vector<std::string>& drop);
  // Copy current entry of all selected input branches into the output buffers.
  void copyInputBranches();
  // Set the buffers of a table (branches are created on first call with that name).
  void fillTable(const FlatTable& table);
  void fill();   // TTree::Fill
  void write();  // write + close file
  long long entries() const { return tree_ ? tree_->GetEntries() : 0; }
  TTree* tree() { return tree_; }

 private:
  std::unique_ptr<BranchCopierBase> makeCopier(BranchRegistry& registry, const std::string& branch);
  std::unique_ptr<TFile> file_;
  TTree* tree_ = nullptr;
  std::vector<std::unique_ptr<BranchCopierBase>> copiers_;
  std::map<std::string, std::unique_ptr<TableBranches>> tables_;
};

}  // namespace harp
#endif
