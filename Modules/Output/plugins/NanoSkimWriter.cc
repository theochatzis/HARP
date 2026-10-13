// NanoSkimWriter: writes a skimmed nano-like tree with
//   * the input branches selected by keep/drop glob patterns,
//   * products of this process (converted to flat tables) as new branches.
// Place it at the end of the path: events rejected by a filter are not written.
//
//   process.skimWriter = Module("NanoSkimWriter",
//       fileName = "skim.root", treeName = "Events",
//       keep = ["HGCalGenPart_*", "ticlTrackstersCLUE3DHigh*", "SimCP2ticlTrackstersCLUE3DHighByHits*"],
//       drop = ["*_time*"],
//       products = {"hadronMatcher": "HadronMatch", "showerVars": "ShowerVars"})   # product label -> table name
#include "Core/interface/ModuleFactory.h"
#include "Core/interface/OutputTree.h"
#include "Core/interface/Persistence.h"

#include <map>

namespace harp {

class NanoSkimWriter : public Analyzer {
 public:
  NanoSkimWriter(const ParameterSet& ps, ModuleContext& ctx) : Analyzer(ctx) {
    fileName_ = ps.get<std::string>("fileName", "skim.root");
    treeName_ = ps.get<std::string>("treeName", "Events");
    keep_ = ps.get<std::vector<std::string>>("keep", {});
    drop_ = ps.get<std::vector<std::string>>("drop", {});
    products_ = ps.get<std::map<std::string, std::string>>("products", {});
    for (const auto& kv : products_) ctx.consumesProduct(kv.first);
    out_ = std::make_unique<OutputTree>(fileName_, treeName_);
    if (!keep_.empty()) out_->addInputBranches(ctx.branches(), keep_, drop_);
    // validate product types early if they are declared
    for (const auto& kv : products_) {
      const std::string label = ctx.productRegistry().resolve(kv.first);
      if (ctx.productRegistry().has(label)) {
        const auto& info = ctx.productRegistry().get(label);
        if (!PersistenceRegistry::instance().canPersist(info.type))
          throw std::runtime_error("NanoSkimWriter: product '" + kv.first + "' of type " + info.typeName + " has no FlatTable converter (see Core/interface/Persistence.h)");
      }
    }
  }

  void analyze(const Event& ev) override {
    out_->copyInputBranches();
    for (const auto& kv : products_) {
      const std::string label = ev.registry().resolve(kv.first);
      const ProductHolderBase* h = ev.holder(label);
      if (!h) throw std::runtime_error("NanoSkimWriter: product '" + kv.first + "' not present in event");
      out_->fillTable(PersistenceRegistry::instance().make(*h, kv.second));
    }
    out_->fill();
  }

  void endJob() override { out_->write(); }

 private:
  std::string fileName_, treeName_;
  std::vector<std::string> keep_, drop_;
  std::map<std::string, std::string> products_;
  std::unique_ptr<OutputTree> out_;
};

}  // namespace harp
HARP_REGISTER_MODULE(harp::NanoSkimWriter);
