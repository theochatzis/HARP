#include "Core/interface/Module.h"

namespace harp {
std::string ModuleBase::kindName() const {
  switch (kind()) {
    case ModuleKind::Producer: return "Producer";
    case ModuleKind::Filter: return "Filter";
    case ModuleKind::Analyzer: return "Analyzer";
  }
  return "?";
}
}  // namespace harp
