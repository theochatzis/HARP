#ifndef HARP_CORE_BRANCHREGISTRY_H
#define HARP_CORE_BRANCHREGISTRY_H
// Access to the branches of the input (nano-like) tree.
//
// Modules declare which branches they read in their constructor through
// ModuleContext::consumesArray<T>/consumesValue<T>; the registry creates the
// corresponding TTreeReaderArray/Value *before* the event loop starts (a ROOT
// requirement) and hands back lightweight handles that stay valid for the job.
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TTreeReaderValue.h>

#include <map>
#include <memory>
#include <string>
#include <typeindex>
#include <vector>

namespace harp {

// Name of the ROOT leaf type for a C++ type, used to validate requested types.
template <class T> struct RootTypeName;
template <> struct RootTypeName<float> { static constexpr const char* value = "Float_t"; };
template <> struct RootTypeName<double> { static constexpr const char* value = "Double_t"; };
template <> struct RootTypeName<int> { static constexpr const char* value = "Int_t"; };
template <> struct RootTypeName<unsigned int> { static constexpr const char* value = "UInt_t"; };
template <> struct RootTypeName<short> { static constexpr const char* value = "Short_t"; };
template <> struct RootTypeName<unsigned short> { static constexpr const char* value = "UShort_t"; };
template <> struct RootTypeName<char> { static constexpr const char* value = "Char_t"; };
template <> struct RootTypeName<unsigned char> { static constexpr const char* value = "UChar_t"; };
template <> struct RootTypeName<bool> { static constexpr const char* value = "Bool_t"; };
template <> struct RootTypeName<long long> { static constexpr const char* value = "Long64_t"; };
template <> struct RootTypeName<unsigned long long> { static constexpr const char* value = "ULong64_t"; };

// Read-only view on a variable-size array branch (e.g. HGCalGenPart_pt[nHGCalGenPart]).
template <class T>
class ArrayHandle {
 public:
  ArrayHandle() = default;
  explicit ArrayHandle(TTreeReaderArray<T>* a) : arr_(a) {}
  bool valid() const { return arr_ != nullptr; }
  std::size_t size() const { return arr_ ? arr_->GetSize() : 0; }
  bool empty() const { return size() == 0; }
  const T& operator[](std::size_t i) const { return (*arr_)[i]; }
  const T& at(std::size_t i) const { return arr_->At(i); }
  std::vector<T> toVector() const {
    std::vector<T> v;
    v.reserve(size());
    for (std::size_t i = 0; i < size(); ++i) v.push_back((*arr_)[i]);
    return v;
  }
  class iterator {
   public:
    iterator(const ArrayHandle* h, std::size_t i) : h_(h), i_(i) {}
    const T& operator*() const { return (*h_)[i_]; }
    iterator& operator++() { ++i_; return *this; }
    bool operator!=(const iterator& o) const { return i_ != o.i_; }
   private:
    const ArrayHandle* h_;
    std::size_t i_;
  };
  iterator begin() const { return iterator(this, 0); }
  iterator end() const { return iterator(this, size()); }

 private:
  TTreeReaderArray<T>* arr_ = nullptr;
};

// Read-only access to a scalar branch (e.g. nHGCalGenPart, run).
template <class T>
class ValueHandle {
 public:
  ValueHandle() = default;
  explicit ValueHandle(TTreeReaderValue<T>* v) : val_(v) {}
  bool valid() const { return val_ != nullptr; }
  T get() const { return **val_; }
  T operator*() const { return **val_; }

 private:
  TTreeReaderValue<T>* val_ = nullptr;
};

class BranchRegistry {
 public:
  BranchRegistry(TTreeReader& reader, TTree* tree);

  bool hasBranch(const std::string& name) const;
  std::vector<std::string> branchNames() const;
  // ROOT type name of the (single) leaf of a branch, "" if the branch does not exist.
  std::string leafTypeName(const std::string& name) const;
  // Name of the counter branch of an array branch ("" for scalars).
  std::string counterBranch(const std::string& name) const;
  bool isArray(const std::string& name) const { return !counterBranch(name).empty(); }
  TTree* tree() const { return tree_; }
  TTreeReader& reader() { return reader_; }

  // Register (or reuse) a reader for a branch. With optional=true a missing
  // branch yields an invalid handle instead of an exception.
  template <class T>
  ArrayHandle<T> array(const std::string& name, bool optional = false) {
    auto* r = reinterpret_cast<TTreeReaderArray<T>*>(find(name, typeid(T), RootTypeName<T>::value, true, optional));
    if (!r) return ArrayHandle<T>();
    return ArrayHandle<T>(r);
  }
  template <class T>
  ValueHandle<T> value(const std::string& name, bool optional = false) {
    auto* r = reinterpret_cast<TTreeReaderValue<T>*>(find(name, typeid(T), RootTypeName<T>::value, false, optional));
    if (!r) return ValueHandle<T>();
    return ValueHandle<T>(r);
  }
  std::size_t nRegistered() const { return readers_.size(); }

 private:
  struct Entry {
    std::unique_ptr<ROOT::Internal::TTreeReaderValueBase> reader;
    std::type_index type;
    bool isArray;
  };
  void* find(const std::string& name, std::type_index type, const char* rootType, bool asArray, bool optional);
  template <class T>
  ROOT::Internal::TTreeReaderValueBase* make(const std::string& name, bool asArray);

  TTreeReader& reader_;
  TTree* tree_;
  std::map<std::string, Entry> readers_;
};

}  // namespace harp
#endif
