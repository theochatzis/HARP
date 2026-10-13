#include "Core/interface/OutputTree.h"

#include "Core/interface/Glob.h"
#include "Core/interface/Logger.h"

#include <TBranch.h>

#include <algorithm>
#include <cstring>
#include <set>
#include <filesystem>
#include <stdexcept>

namespace harp {

namespace {

template <class T> struct LeafCode;
template <> struct LeafCode<float> { static constexpr char value = 'F'; };
template <> struct LeafCode<double> { static constexpr char value = 'D'; };
template <> struct LeafCode<int> { static constexpr char value = 'I'; };
template <> struct LeafCode<unsigned int> { static constexpr char value = 'i'; };
template <> struct LeafCode<short> { static constexpr char value = 'S'; };
template <> struct LeafCode<unsigned short> { static constexpr char value = 's'; };
template <> struct LeafCode<char> { static constexpr char value = 'B'; };
template <> struct LeafCode<unsigned char> { static constexpr char value = 'b'; };
template <> struct LeafCode<bool> { static constexpr char value = 'O'; };
template <> struct LeafCode<long long> { static constexpr char value = 'L'; };
template <> struct LeafCode<unsigned long long> { static constexpr char value = 'l'; };

// Growable raw buffer bound to an output branch (re-binds the address on growth).
template <class T>
class OutputBuffer {
 public:
  OutputBuffer(TTree* tree, const std::string& name, const std::string& counter, const std::string& title) {
    capacity_ = 16;
    data_.reset(new T[capacity_]);
    std::string leaflist = name;
    if (!counter.empty()) leaflist += "[" + counter + "]";
    leaflist += std::string("/") + LeafCode<T>::value;
    branch_ = tree->Branch(name.c_str(), data_.get(), leaflist.c_str());
    if (!branch_) throw std::runtime_error("OutputTree: cannot create branch " + name);
    if (!title.empty()) branch_->SetTitle(title.c_str());
  }
  void ensure(std::size_t n) {
    if (n <= capacity_) return;
    while (capacity_ < n) capacity_ *= 2;
    data_.reset(new T[capacity_]);
    branch_->SetAddress(data_.get());
  }
  T* data() { return data_.get(); }

 private:
  std::size_t capacity_ = 0;
  std::unique_ptr<T[]> data_;
  TBranch* branch_ = nullptr;
};

template <class T>
class ScalarCopier : public BranchCopierBase {
 public:
  ScalarCopier(TTree* tree, BranchRegistry& reg, const std::string& name)
      : name_(name), in_(reg.value<T>(name)), out_(tree, name, "", reg.tree()->GetBranch(name.c_str())->GetTitle()) {}
  void copy() override { out_.data()[0] = in_.get(); }
  const std::string& name() const override { return name_; }

 private:
  std::string name_;
  ValueHandle<T> in_;
  OutputBuffer<T> out_;
};

template <class T>
class ArrayCopier : public BranchCopierBase {
 public:
  ArrayCopier(TTree* tree, BranchRegistry& reg, const std::string& name, const std::string& counter)
      : name_(name), in_(reg.array<T>(name)), out_(tree, name, counter, reg.tree()->GetBranch(name.c_str())->GetTitle()) {}
  void copy() override {
    const std::size_t n = in_.size();
    out_.ensure(n);
    T* d = out_.data();
    for (std::size_t i = 0; i < n; ++i) d[i] = in_[i];
  }
  const std::string& name() const override { return name_; }

 private:
  std::string name_;
  ArrayHandle<T> in_;
  OutputBuffer<T> out_;
};

template <class T>
std::unique_ptr<BranchCopierBase> makeTyped(TTree* tree, BranchRegistry& reg, const std::string& name, const std::string& counter) {
  if (counter.empty()) return std::make_unique<ScalarCopier<T>>(tree, reg, name);
  return std::make_unique<ArrayCopier<T>>(tree, reg, name, counter);
}

}  // namespace

// Output buffers of one FlatTable: counter + one buffer per column.
class TableBranches {
 public:
  TableBranches(TTree* tree, const FlatTable& t) : name_(t.name()), singleton_(t.singleton()) {
    const std::string counter = singleton_ ? "" : "n" + name_;
    if (!singleton_) count_ = std::make_unique<OutputBuffer<int>>(tree, counter, "", "number of " + name_);
    for (const auto& c : t.columns()) {
      const std::string bname = name_ + "_" + c.name;
      switch (c.type) {
        case FlatTable::ColumnType::Float: f_.emplace_back(c.name, std::make_unique<OutputBuffer<float>>(tree, bname, counter, c.doc)); break;
        case FlatTable::ColumnType::Int: i_.emplace_back(c.name, std::make_unique<OutputBuffer<int>>(tree, bname, counter, c.doc)); break;
        case FlatTable::ColumnType::Bool: b_.emplace_back(c.name, std::make_unique<OutputBuffer<bool>>(tree, bname, counter, c.doc)); break;
      }
    }
  }
  void fill(const FlatTable& t) {
    const std::size_t n = singleton_ ? 1 : t.nRows();
    if (count_) count_->data()[0] = static_cast<int>(n);
    std::size_t nf = 0, ni = 0, nb = 0;
    for (const auto& c : t.columns()) {
      switch (c.type) {
        case FlatTable::ColumnType::Float: {
          auto& o = col(f_, nf++, c.name); o->ensure(n);
          std::copy(c.f.begin(), c.f.begin() + n, o->data()); break;
        }
        case FlatTable::ColumnType::Int: {
          auto& o = col(i_, ni++, c.name); o->ensure(n);
          std::copy(c.i.begin(), c.i.begin() + n, o->data()); break;
        }
        case FlatTable::ColumnType::Bool: {
          auto& o = col(b_, nb++, c.name); o->ensure(n);
          for (std::size_t k = 0; k < n; ++k) o->data()[k] = c.b[k] != 0;
          break;
        }
      }
    }
  }

 private:
  template <class V>
  typename V::value_type::second_type& col(V& v, std::size_t idx, const std::string& name) {
    if (idx >= v.size() || v[idx].first != name)
      throw std::runtime_error("OutputTree: FlatTable '" + name_ + "' changed its column layout (column '" + name + "')");
    return v[idx].second;
  }
  std::string name_;
  bool singleton_;
  std::unique_ptr<OutputBuffer<int>> count_;
  std::vector<std::pair<std::string, std::unique_ptr<OutputBuffer<float>>>> f_;
  std::vector<std::pair<std::string, std::unique_ptr<OutputBuffer<int>>>> i_;
  std::vector<std::pair<std::string, std::unique_ptr<OutputBuffer<bool>>>> b_;
};

OutputTree::OutputTree(const std::string& fileName, const std::string& treeName, const std::string& title) {
  // remove a previous copy first: overwriting on EOS leaves a hidden .sys.v#.<file>/ version directory
  std::error_code ec;
  std::filesystem::remove(fileName, ec);
  file_.reset(TFile::Open(fileName.c_str(), "RECREATE"));
  if (!file_ || file_->IsZombie()) throw std::runtime_error("OutputTree: cannot create " + fileName);
  tree_ = new TTree(treeName.c_str(), title.empty() ? treeName.c_str() : title.c_str());
  tree_->SetDirectory(file_.get());
  tree_->SetAutoFlush(-30000000);
  HARP_INFO("OutputTree") << "output file " << fileName << " tree " << treeName;
}

OutputTree::~OutputTree() = default;

std::unique_ptr<BranchCopierBase> OutputTree::makeCopier(BranchRegistry& reg, const std::string& name) {
  const std::string t = reg.leafTypeName(name);
  const std::string counter = reg.counterBranch(name);
  if (t == "Float_t") return makeTyped<float>(tree_, reg, name, counter);
  if (t == "Double_t") return makeTyped<double>(tree_, reg, name, counter);
  if (t == "Int_t") return makeTyped<int>(tree_, reg, name, counter);
  if (t == "UInt_t") return makeTyped<unsigned int>(tree_, reg, name, counter);
  if (t == "Short_t") return makeTyped<short>(tree_, reg, name, counter);
  if (t == "UShort_t") return makeTyped<unsigned short>(tree_, reg, name, counter);
  if (t == "Char_t") return makeTyped<char>(tree_, reg, name, counter);
  if (t == "UChar_t") return makeTyped<unsigned char>(tree_, reg, name, counter);
  if (t == "Bool_t") return makeTyped<bool>(tree_, reg, name, counter);
  if (t == "Long64_t") return makeTyped<long long>(tree_, reg, name, counter);
  if (t == "ULong64_t") return makeTyped<unsigned long long>(tree_, reg, name, counter);
  throw std::runtime_error("OutputTree: unsupported branch type " + t + " for " + name);
}

std::vector<std::string> OutputTree::addInputBranches(BranchRegistry& reg, const std::vector<std::string>& keep, const std::vector<std::string>& drop) {
  std::set<std::string> selected;
  for (const auto& name : reg.branchNames()) {
    if (!globMatchAny(keep, name)) continue;
    if (globMatchAny(drop, name)) continue;
    selected.insert(name);
  }
  // counters of kept arrays must be present and written first
  std::vector<std::string> scalars, arrays;
  for (const auto& name : selected) {
    const std::string counter = reg.counterBranch(name);
    if (counter.empty()) {
      scalars.push_back(name);
    } else {
      arrays.push_back(name);
      if (!selected.count(counter) && std::find(scalars.begin(), scalars.end(), counter) == scalars.end()) {
        HARP_DEBUG("OutputTree") << "adding counter branch " << counter << " needed by " << name;
        scalars.push_back(counter);
      }
    }
  }
  std::vector<std::string> ordered = scalars;
  ordered.insert(ordered.end(), arrays.begin(), arrays.end());
  for (const auto& name : ordered) copiers_.push_back(makeCopier(reg, name));
  HARP_INFO("OutputTree") << "copying " << ordered.size() << " input branches (" << scalars.size() << " scalars, " << arrays.size() << " arrays)";
  return ordered;
}

void OutputTree::copyInputBranches() {
  for (auto& c : copiers_) c->copy();
}

void OutputTree::fillTable(const FlatTable& table) {
  auto it = tables_.find(table.name());
  if (it == tables_.end()) {
    if (tree_->GetEntries() > 0)
      HARP_WARNING("OutputTree") << "table '" << table.name() << "' appears after " << tree_->GetEntries() << " entries were already filled";
    it = tables_.emplace(table.name(), std::make_unique<TableBranches>(tree_, table)).first;
    HARP_INFO("OutputTree") << "added table '" << table.name() << "' with " << table.columns().size() << " columns";
  }
  it->second->fill(table);
}

void OutputTree::fill() { tree_->Fill(); }

void OutputTree::write() {
  if (!file_) return;
  file_->cd();
  tree_->Write(nullptr, TObject::kOverwrite);
  HARP_INFO("OutputTree") << "wrote " << tree_->GetEntries() << " entries to " << file_->GetName();
  file_->Close();
  file_.reset();
  tree_ = nullptr;
}

}  // namespace harp
