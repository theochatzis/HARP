#ifndef HARP_CORE_GLOB_H
#define HARP_CORE_GLOB_H
#include <string>
#include <vector>

namespace harp {
// Shell-style matching with '*' and '?'. Used for keep/drop branch lists.
bool globMatch(const std::string& pattern, const std::string& text);
bool globMatchAny(const std::vector<std::string>& patterns, const std::string& text);
// Split "a,b,c" into {"a","b","c"} trimming whitespace.
std::vector<std::string> splitString(const std::string& s, char sep = ',');
}  // namespace harp
#endif
