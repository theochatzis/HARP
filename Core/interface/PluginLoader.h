#ifndef HARP_CORE_PLUGINLOADER_H
#define HARP_CORE_PLUGINLOADER_H
// dlopen's every libHarp*Plugin.so found in the plugin directories so that the
// static HARP_REGISTER_* registrations run. Directories come from (in order):
// explicit argument, $HARP_PLUGIN_PATH (colon separated), <exe dir>/../lib/plugins.
#include <string>
#include <vector>

namespace harp {
class PluginLoader {
 public:
  static std::vector<std::string> defaultDirectories();
  // Returns the list of loaded libraries.
  static std::vector<std::string> loadAll(const std::vector<std::string>& dirs = {}, const std::string& pattern = "libHarp*Plugin.so", const std::string& exclude = "");
  static void load(const std::string& path);
};
}  // namespace harp
#endif
