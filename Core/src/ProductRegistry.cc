#include "Core/interface/ProductRegistry.h"

#include <cxxabi.h>

#include <memory>
#include <stdexcept>

namespace harp {

void ProductRegistry::add(const ProductInfo& info) {
  if (products_.count(info.label))
    throw std::runtime_error("ProductRegistry: product '" + info.label + "' declared twice");
  products_.emplace(info.label, info);
}

const ProductInfo& ProductRegistry::get(const std::string& label) const {
  auto it = products_.find(label);
  if (it == products_.end()) throw std::runtime_error("ProductRegistry: unknown product '" + label + "'");
  return it->second;
}

std::vector<ProductInfo> ProductRegistry::all() const {
  std::vector<ProductInfo> out;
  for (const auto& kv : products_) out.push_back(kv.second);
  return out;
}

std::string ProductRegistry::resolve(const std::string& label) const {
  if (products_.count(label)) return label;
  std::string found;
  int n = 0;
  for (const auto& kv : products_)
    if (kv.second.module == label) { found = kv.first; ++n; }
  if (n == 1) return found;
  return label;
}

std::string ProductRegistry::demangle(const char* name) {
  int status = 0;
  std::unique_ptr<char, void (*)(void*)> res{abi::__cxa_demangle(name, nullptr, nullptr, &status), std::free};
  return status == 0 ? res.get() : name;
}

}  // namespace harp
