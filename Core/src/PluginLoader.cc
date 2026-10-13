#include "Core/interface/PluginLoader.h"

#include "Core/interface/Glob.h"
#include "Core/interface/Logger.h"

#include <dlfcn.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

namespace harp {

std::vector<std::string> PluginLoader::defaultDirectories() {
  std::vector<std::string> dirs;
  if (const char* env = std::getenv("HARP_PLUGIN_PATH")) {
    for (const auto& d : splitString(env, ':')) dirs.push_back(d);
  }
  char buf[4096];
  ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n > 0) {
    buf[n] = '\0';
    fs::path exe(buf);
    dirs.push_back((exe.parent_path().parent_path() / "lib" / "plugins").string());
  }
  return dirs;
}

std::vector<std::string> PluginLoader::loadAll(const std::vector<std::string>& dirsIn, const std::string& pattern, const std::string& exclude) {
  std::vector<std::string> dirs = dirsIn.empty() ? defaultDirectories() : dirsIn;
  std::vector<std::string> loaded;
  for (const auto& d : dirs) {
    if (!fs::is_directory(d)) continue;
    std::vector<std::string> libs;
    for (const auto& e : fs::directory_iterator(d))
      if (globMatch(pattern, e.path().filename().string()) && (exclude.empty() || !globMatch(exclude, e.path().filename().string()))) libs.push_back(e.path().string());
    std::sort(libs.begin(), libs.end());
    for (const auto& lib : libs) {
      // skip if already loaded from another directory (same file name)
      const std::string base = fs::path(lib).filename().string();
      bool dup = false;
      for (const auto& l : loaded) dup = dup || fs::path(l).filename().string() == base;
      if (dup) continue;
      load(lib);
      loaded.push_back(lib);
    }
  }
  if (loaded.empty()) {
    std::string msg = "no plugins found in:";
    for (const auto& d : dirs) msg += " " + d;
    HARP_WARNING("PluginLoader") << msg;
  } else {
    HARP_INFO("PluginLoader") << "loaded " << loaded.size() << " plugin librar" << (loaded.size() == 1 ? "y" : "ies");
  }
  return loaded;
}

void PluginLoader::load(const std::string& path) {
  void* h = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
  if (!h) throw std::runtime_error(std::string("PluginLoader: cannot load ") + path + ": " + dlerror());
  HARP_DEBUG("PluginLoader") << "loaded " << path;
}

}  // namespace harp
