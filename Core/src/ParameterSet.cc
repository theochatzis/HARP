#include "Core/interface/ParameterSet.h"

#include <fstream>

namespace harp {

ParameterSet ParameterSet::fromFile(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("ParameterSet: cannot open " + path);
  nlohmann::json j;
  try {
    in >> j;
  } catch (const std::exception& e) {
    throw std::runtime_error("ParameterSet: JSON parse error in " + path + ": " + e.what());
  }
  ParameterSet ps(j);
  ps.setContext(path);
  return ps;
}

ParameterSet ParameterSet::fromString(const std::string& text) {
  ParameterSet ps(nlohmann::json::parse(text));
  ps.setContext("<string>");
  return ps;
}

ParameterSet ParameterSet::getPSet(const std::string& key) const {
  if (!json_.contains(key) || !json_.at(key).is_object())
    throw std::runtime_error("ParameterSet: missing nested parameter set '" + key + "' in " + context_);
  ParameterSet ps(json_.at(key));
  ps.setContext(context_ + "." + key);
  return ps;
}

ParameterSet ParameterSet::getPSet(const std::string& key, const ParameterSet& def) const {
  if (!json_.contains(key) || !json_.at(key).is_object()) return def;
  return getPSet(key);
}

std::vector<ParameterSet> ParameterSet::getVPSet(const std::string& key) const {
  std::vector<ParameterSet> out;
  if (!json_.contains(key)) return out;
  if (!json_.at(key).is_array()) throw std::runtime_error("ParameterSet: '" + key + "' is not a list in " + context_);
  int i = 0;
  for (const auto& item : json_.at(key)) {
    ParameterSet ps(item);
    ps.setContext(context_ + "." + key + "[" + std::to_string(i++) + "]");
    out.push_back(ps);
  }
  return out;
}

std::vector<std::string> ParameterSet::keys() const {
  std::vector<std::string> out;
  for (auto it = json_.begin(); it != json_.end(); ++it) out.push_back(it.key());
  return out;
}

}  // namespace harp
