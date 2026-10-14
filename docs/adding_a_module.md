# Adding a module

A module is one `.cc` file in `Modules/<Package>/plugins/`. Three kinds:

* `Producer` — `produce(Event&)`: creates products (`ev.put(label, obj)`);
* `Filter` — `filter(Event&)` returns `bool`; `false` stops the path;
* `Analyzer` — `analyze(const Event&)`: fills histograms or writes output.

## 1. Write the class

```cpp
// Modules/Hadron/plugins/TracksterEnergyFractionAnalyzer.cc
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/HadronMatch.h"
#include "DataFormats/interface/Trackster.h"
#include <TH2D.h>

namespace harp {

class TracksterEnergyFractionAnalyzer : public Analyzer {
 public:
  TracksterEnergyFractionAnalyzer(const ParameterSet& ps, ModuleContext& ctx)
      : Analyzer(ctx),
        matches_(ctx.consumesProduct(ps.get<std::string>("matches"))),      // declared dependency
        tracksters_(ctx.consumesProduct(ps.get<std::string>("tracksters"))) {
    h_ = ctx.hists().book2D("h2_leadFraction_vs_E", ";E_{gen} [GeV];E_{lead}/#sum E", 50, 0, 200, 50, 0, 1.0001);
  }
  void analyze(const Event& ev) override {
    const auto& matches = ev.get<HadronMatchCollection>(matches_);
    const auto& trk = ev.get<TracksterCollection>(tracksters_);
    for (const auto& m : matches) {
      if (!m.hasReco() || m.allRawEnergy <= 0) continue;
      h_->Fill(m.genEnergy, trk[m.bestIndex].rawEnergy / m.allRawEnergy);
    }
  }
 private:
  std::string matches_, tracksters_;
  TH2D* h_ = nullptr;
};

}  // namespace harp
HARP_REGISTER_MODULE(harp::TracksterEnergyFractionAnalyzer);
```

Reading a raw nano branch instead of a product:

```cpp
pt_ = ctx.consumesArray<float>("HGCalGenPart_pt");          // type must match the leaf (Float_t)
n_  = ctx.consumesValue<int>("nHGCalGenPart");
opt_ = ctx.consumesArray<float>("Maybe_missing", /*optional=*/true);  // opt_.valid() tells
...
for (float pt : pt_) ...;   pt_.size();  pt_[i];
```

Producing a product:

```cpp
out_ = ctx.produces<std::vector<float>>("fractions");   // label "<moduleLabel>:fractions"
...
ev.put(out_, std::move(values));
```

`std::vector<float/int/double>`, scalars, `FlatTable`, `HadronMatchCollection`
and `ShowerVariablesCollection` can be written to skims out of the box. For a
new struct, register a converter (see `DataFormats/src/Persistence.cc`).

## 2. Add it to the build

Append the file to `Modules/Hadron/CMakeLists.txt` (or create a new package
directory with its own `CMakeLists.txt`):

```cmake
harp_add_plugin(Hadron
  SOURCES ... plugins/TracksterEnergyFractionAnalyzer.cc
  DEPENDS HarpDataFormats)
```

`cd build && make -j8`. `harpRun --list-modules` must now show it.

## 3. Use it

```python
process.leadFrac = Module("TracksterEnergyFractionAnalyzer", matches="matchCLUE3D", tracksters="hgcalReader:clue3d")
process.path.append("leadFrac")
```

`harp run configs/hist_hadron_cfg.py sample=pip_p0p2to200 maxFiles=1 -- --dump`
prints the module table and products without processing events.

## Checklist for a good module

* all parameters with defaults in the constructor, documented in the header comment;
* throw `std::runtime_error` with a helpful message for invalid configuration
  (the Processor prefixes it with the module label);
* histograms booked in the constructor (never in `analyze`);
* no static state: several instances of the same type with different labels
  must coexist (`matchCLUE3D`, `matchCand`).

## Adding a plotter

Same pattern in `Plot/plugins/`: derive from `harp::Plotter`, implement
`draw()`, register with `HARP_REGISTER_PLOTTER(harp::MyPlotter)`, add the
file to `Plot/CMakeLists.txt`. `ctx_.loader()` fetches histograms,
`ctx_.save(canvas, name)` writes all configured formats.
