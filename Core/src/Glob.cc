#include "Core/interface/Glob.h"

#include <fnmatch.h>

namespace harp {

bool globMatch(const std::string& pattern, const std::string& text) {
  return fnmatch(pattern.c_str(), text.c_str(), 0) == 0;
}

bool globMatchAny(const std::vector<std::string>& patterns, const std::string& text) {
  for (const auto& p : patterns)
    if (globMatch(p, text)) return true;
  return false;
}

std::vector<std::string> splitString(const std::string& s, char sep) {
  std::vector<std::string> out;
  std::string cur;
  auto flush = [&]() {
    size_t a = cur.find_first_not_of(" \t\n");
    size_t b = cur.find_last_not_of(" \t\n");
    if (a != std::string::npos) out.push_back(cur.substr(a, b - a + 1));
    cur.clear();
  };
  for (char c : s) {
    if (c == sep) flush();
    else cur += c;
  }
  flush();
  return out;
}

}  // namespace harp
