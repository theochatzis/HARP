#include "Core/interface/BranchRegistry.h"

#include "Core/interface/Logger.h"

#include <TBranch.h>
#include <TLeaf.h>
#include <TObjArray.h>

#include <stdexcept>

namespace harp {

BranchRegistry::BranchRegistry(TTreeReader& reader, TTree* tree) : reader_(reader), tree_(tree) {}

bool BranchRegistry::hasBranch(const std::string& name) const { return tree_ && tree_->GetBranch(name.c_str()) != nullptr; }

std::vector<std::string> BranchRegistry::branchNames() const {
  std::vector<std::string> out;
  if (!tree_) return out;
  TObjArray* branches = tree_->GetListOfBranches();
  for (int i = 0; i < branches->GetEntries(); ++i) out.push_back(branches->At(i)->GetName());
  return out;
}

std::string BranchRegistry::leafTypeName(const std::string& name) const {
  if (!tree_) return "";
  TBranch* b = tree_->GetBranch(name.c_str());
  if (!b) return "";
  TLeaf* l = b->GetLeaf(name.c_str());
  if (!l && b->GetListOfLeaves()->GetEntries() > 0) l = static_cast<TLeaf*>(b->GetListOfLeaves()->At(0));
  return l ? l->GetTypeName() : "";
}

std::string BranchRegistry::counterBranch(const std::string& name) const {
  if (!tree_) return "";
  TBranch* b = tree_->GetBranch(name.c_str());
  if (!b) return "";
  TLeaf* l = b->GetLeaf(name.c_str());
  if (!l && b->GetListOfLeaves()->GetEntries() > 0) l = static_cast<TLeaf*>(b->GetListOfLeaves()->At(0));
  if (!l || !l->GetLeafCount()) return "";
  return l->GetLeafCount()->GetName();
}

template <class T>
ROOT::Internal::TTreeReaderValueBase* BranchRegistry::make(const std::string& name, bool asArray) {
  if (asArray) return new TTreeReaderArray<T>(reader_, name.c_str());
  return new TTreeReaderValue<T>(reader_, name.c_str());
}

void* BranchRegistry::find(const std::string& name, std::type_index type, const char* rootType, bool asArray, bool optional) {
  auto it = readers_.find(name);
  if (it != readers_.end()) {
    if (it->second.type != type || it->second.isArray != asArray)
      throw std::runtime_error("BranchRegistry: branch '" + name + "' already registered with a different type/shape");
    return it->second.reader.get();
  }
  if (!hasBranch(name)) {
    if (optional) {
      HARP_DEBUG("BranchRegistry") << "optional branch '" << name << "' not present in input";
      return nullptr;
    }
    throw std::runtime_error("BranchRegistry: branch '" + name + "' not found in input tree '" + std::string(tree_ ? tree_->GetName() : "?") + "'");
  }
  const std::string actual = leafTypeName(name);
  if (actual != rootType)
    throw std::runtime_error("BranchRegistry: branch '" + name + "' has type " + actual + " but was requested as " + rootType);
  const bool actualArray = isArray(name);
  if (actualArray != asArray)
    throw std::runtime_error("BranchRegistry: branch '" + name + "' is " + (actualArray ? "an array" : "a scalar") +
                             " but was requested as " + (asArray ? "an array" : "a scalar"));

  ROOT::Internal::TTreeReaderValueBase* r = nullptr;
  const std::string t = rootType;
  if (t == "Float_t") r = make<float>(name, asArray);
  else if (t == "Double_t") r = make<double>(name, asArray);
  else if (t == "Int_t") r = make<int>(name, asArray);
  else if (t == "UInt_t") r = make<unsigned int>(name, asArray);
  else if (t == "Short_t") r = make<short>(name, asArray);
  else if (t == "UShort_t") r = make<unsigned short>(name, asArray);
  else if (t == "Char_t") r = make<char>(name, asArray);
  else if (t == "UChar_t") r = make<unsigned char>(name, asArray);
  else if (t == "Bool_t") r = make<bool>(name, asArray);
  else if (t == "Long64_t") r = make<long long>(name, asArray);
  else if (t == "ULong64_t") r = make<unsigned long long>(name, asArray);
  else throw std::runtime_error("BranchRegistry: unsupported leaf type " + t + " for branch " + name);

  readers_.emplace(name, Entry{std::unique_ptr<ROOT::Internal::TTreeReaderValueBase>(r), type, asArray});
  HARP_DEBUG("BranchRegistry") << "registered " << (asArray ? "array " : "value ") << name << " (" << t << ")";
  return r;
}

}  // namespace harp
