#ifndef HARP_CORE_PRODUCTREGISTRY_H
#define HARP_CORE_PRODUCTREGISTRY_H
// Book-keeping of which module produces which product (label -> type).
// Filled by ModuleContext::produces<T>() at module construction, used for
// validation, for --dump and by output modules to know what can be written.
#include <map>
#include <string>
#include <typeindex>
#include <vector>

namespace harp {

struct ProductInfo {
  std::string label;     // full product label as used in Event::get (module[:instance])
  std::string module;    // producing module label
  std::string instance;  // instance name (may be empty)
  std::type_index type;
  std::string typeName;  // demangled C++ type name for messages
};

class ProductRegistry {
 public:
  void add(const ProductInfo& info);
  bool has(const std::string& label) const { return products_.count(label) > 0; }
  const ProductInfo& get(const std::string& label) const;
  std::vector<ProductInfo> all() const;
  // Resolve a label that may omit the instance when the module has one product.
  std::string resolve(const std::string& label) const;

  static std::string makeLabel(const std::string& module, const std::string& instance) {
    return instance.empty() ? module : module + ":" + instance;
  }
  static std::string demangle(const char* name);

 private:
  std::map<std::string, ProductInfo> products_;
};

}  // namespace harp
#endif
