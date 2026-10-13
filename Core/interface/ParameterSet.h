#ifndef HARP_CORE_PARAMETERSET_H
#define HARP_CORE_PARAMETERSET_H
// JSON-backed configuration object handed to every module.
// Produced by the python layer (harp.config.Process) and parsed by harpRun.
#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>
#include <vector>

namespace harp {

class ParameterSet {
 public:
  ParameterSet() : json_(nlohmann::json::object()) {}
  explicit ParameterSet(nlohmann::json j) : json_(std::move(j)) {}

  static ParameterSet fromFile(const std::string& path);
  static ParameterSet fromString(const std::string& text);

  bool has(const std::string& key) const { return json_.contains(key); }

  // Mandatory parameter: throws a descriptive error if missing or of wrong type.
  template <class T>
  T get(const std::string& key) const {
    if (!json_.contains(key)) throw std::runtime_error("ParameterSet: missing parameter '" + key + "' in " + context_);
    try {
      return json_.at(key).get<T>();
    } catch (const std::exception& e) {
      throw std::runtime_error("ParameterSet: parameter '" + key + "' in " + context_ + " has wrong type: " + e.what());
    }
  }
  // Optional parameter with default.
  template <class T>
  T get(const std::string& key, const T& def) const {
    if (!json_.contains(key) || json_.at(key).is_null()) return def;
    return get<T>(key);
  }
  std::string getString(const std::string& key, const std::string& def) const { return get<std::string>(key, def); }

  ParameterSet getPSet(const std::string& key) const;
  ParameterSet getPSet(const std::string& key, const ParameterSet& def) const;
  std::vector<ParameterSet> getVPSet(const std::string& key) const;
  std::vector<std::string> keys() const;

  const nlohmann::json& json() const { return json_; }
  nlohmann::json& json() { return json_; }
  std::string dump(int indent = 2) const { return json_.dump(indent); }

  void setContext(const std::string& c) { context_ = c; }
  const std::string& context() const { return context_; }

 private:
  nlohmann::json json_;
  std::string context_ = "<unnamed>";
};

}  // namespace harp
#endif
