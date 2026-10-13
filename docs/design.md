# Design

## Goals

* One tool chain for **low-level** (layer clusters, tracksters, sim matching)
  and **high-level** (response, identification) hadron performance studies.
* **Plug-and-play**: a new study = a new module file (and a few lines of
  python), never a change to the framework.
* **C++ for everything that touches events**, python only to describe jobs.
* Same binaries run interactively, on a skim, or on condor.

## Concepts (the CMSSW analogy)

| CMSSW | HARP | Where |
|-------|------|-------|
| `edm::Event` | `harp::Event` — per-entry store of products keyed by label | `Core/interface/Event.h` |
| `EDProducer` / `EDFilter` / `EDAnalyzer` | `harp::Producer` / `Filter` / `Analyzer` | `Core/interface/Module.h` |
| `consumes<T>(InputTag)` | `ctx.consumesArray<T>("branch")` (raw nano branch) / `ctx.consumesProduct("label")` | `ModuleContext` |
| `produces<T>()` | `ctx.produces<T>("instance")` → label `module:instance` | `ModuleContext` |
| `DEFINE_FWK_MODULE` | `HARP_REGISTER_MODULE(Class)` | `Core/interface/ModuleFactory.h` |
| `TFileService` | `ctx.hists()` → `HistDir` per module | `Core/interface/HistService.h` |
| `PoolOutputModule` + `keep/drop` | `NanoSkimWriter` + `OutputTree` (glob keep/drop + products as flat tables) | `Modules/Output`, `Core/interface/OutputTree.h` |
| `cms.Process`, `cms.Path` | `harp.config.Process`, `process.path = [...]` | `python/harp/config.py` |
| `cmsRun cfg.py opt=value` | `harp run cfg.py opt=value` (→ JSON → `harpRun`) | `python/harp/cli.py` |
| plugin manager | `PluginLoader` dlopen's `lib/plugins/libHarp*Plugin.so` | `Core/interface/PluginLoader.h` |

## Event loop

`Processor` (Core/src/Processor.cc):

1. parse the JSON, load the plugins, open the input `TChain` with a `TTreeReader`;
2. construct the modules of the `path` in order. In their constructors the
   modules register the branches they read (`consumesArray<T>`) — the
   `BranchRegistry` creates the `TTreeReaderArray/Value` objects *before* the
   loop, as ROOT requires, and validates branch existence and type with a
   clear error message — and declare their products;
3. warn about products consumed before they are produced;
4. loop: `event.clear()`, run the modules; a `Filter` returning `false` ends
   the path for that event; a `harp/cutflow` histogram counts the survivors
   of every step (used by `harp check`);
5. `endJob()`, write histograms, print the pass table.

## Data flow

Raw branches are only read by *reader* producers (`HGCalNanoReader`), which
build the EDM collections (`DataFormats/`). Every other module works on EDM
products, so the physics code is independent of the branch layout: a nano
with different prefixes only needs a different reader configuration.

Products are owned by the `Event` (type-erased holders with run-time type
checking). Any product type with a registered `FlatTable` converter
(`Core/interface/Persistence.h`) can be written to a skim as nano-style
branches, and read back by the same reader machinery (the skim keeps the
nano layout, so `hist_hadron_cfg.py` runs unchanged on nanos and on skims).

## Packages and dependencies

```
Core  <─ DataFormats <─ Modules/* (plugins)
Core  <─ Fit <─ Plot <─ Plot/plugins
bin/harpRun -> Core, DataFormats (+ plugins at run time)
bin/harpPlot -> Plot, Fit (+ plotter plugins at run time)
```

`Modules/` is scanned by CMake: a directory with a `CMakeLists.txt` calling
`harp_add_plugin(...)` becomes a plugin library. Nothing else has to be
registered anywhere.

## Why not RDataFrame?

RDataFrame is excellent for flat, declarative histogramming, but the studies
here need per-event object logic (matching gen → sim → reco through
association maps, shower reconstruction from layer clusters, candidate
look-ups) and want the result persisted as new nano branches. An explicit
object-oriented event loop keeps that logic readable and testable, and the
skims it produces are still plain nanos that RDataFrame (or `uproot`) can
consume for quick looks.

## Conventions

* namespace `harp`, headers in `<Pkg>/interface`, sources in `<Pkg>/src`,
  modules in `<Pkg>/plugins`, one class per file, `HARP_REGISTER_MODULE` at
  the end of the file;
* parameters are read in the constructor with explicit defaults
  (`ps.get<double>("scoreCut", 1.0)`), so `--dump` shows the resolved values;
* histogram names are stable identifiers (`h2_<def>_vs_E`) that plot configs
  rely on; add, do not rename;
* product labels: `module` for single-product modules, `module:instance` for
  readers;
* energies in GeV, lengths in cm, layers 1..47 (CE-E 1..26, CE-H 27..47).
