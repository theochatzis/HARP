# HARP — Hadron Analysis & Response Performance

HARP is a small, CMSSW-flavoured C++ framework to study hadrons in the CMS
Phase-2 HGCal (and their implications for jets) from nano-like inputs:
low-level (layer clusters, tracksters, sim matching) and high-level (response,
particle identification) performance in the same tool chain.

```
nano-like input  ──▶  Skim  ──▶  Hist  ──▶  Plot / Fit
(specialised nanos)   select events,  histograms   figures, slice fits,
                      keep/drop/add   on nano or   response parametrisations
                      branches        skim
```

All event processing is C++ (plugin modules on an event data model); python is
only used to describe jobs (modules, parameters, paths), like `cmsRun` configs.

## Quick start (lxplus, EL9)

```bash
git clone <this repo> HARP && cd HARP
source env/setup.sh                 # LCG_107a view from cvmfs: ROOT 6.34, gcc 13, cmake, nlohmann-json
mkdir -p build && cd build && cmake .. && make -j8 && cd ..
harp samples scan                   # cache the file lists of samples/phase2_gun.json

# 1) skim: 1 file of the pi+ gun, 300 events -> slim nano + matching tables
harp run configs/skim_hadron_cfg.py sample=pip_p1to200 maxFiles=1 maxEvents=300 outputFile=skim_pip.root
harpInspect skim_pip.root -c -e 2   # what is inside

# 2) histograms (directly on nanos, or on a skim)
harp run configs/hist_hadron_cfg.py sample=pip_p1to200 maxFiles=4 histFile=output/hists/pip_p1to200.root
harp run configs/hist_hadron_cfg.py inputFiles=skim_pip.root histFile=hists_from_skim.root

# 3) plots (response vs E_gen for all hadron types, PID variables, matching QA)
harp plot configs/plot_hadron_cfg.py histDir=output/hists outputDir=plots tag=v1

# 4) scale up with condor
harp submit --cfg configs/hist_hadron_cfg.py --hist-only --sample pip_p1to200 Kp_p1to200 \
            --output-dir /eos/user/$USER/HARP/hists_v1 --tag hists_v1 --files-per-job 10
harp check --tag hists_v1 && harp merge --tag hists_v1
```

Every executable and every configuration prints its usage with `-h`
(`harp -h`, `harp run -h`, `harpRun -h`, `harp run configs/hist_hadron_cfg.py help`).

## Repository layout

| Directory | Content | Builds |
|-----------|---------|--------|
| `Core/` | framework kernel: Event, Module base classes, plugin factory, branch access, histogram service, skim writer, scheduler | `libHarpCore.so` |
| `DataFormats/` | event data model: `GenParticle`, `LayerCluster`, `Trackster`, `TICLCandidate`, `Track`, `AssociationMap`, `HadronMatch`, `ShowerVariables`, HGCal layer table | `libHarpDataFormats.so` |
| `Modules/<Pkg>/plugins/` | modules (producers / filters / analyzers); one plugin library per package, auto-discovered by CMake and loaded at run time | `lib/plugins/libHarp<Pkg>Plugin.so` |
| `Fit/` | slice fitting (scale / resolution vs E), resolution parametrisation | `libHarpFit.so` |
| `Plot/` | style, histogram loading, plotter plugins (`Overlay1D`, `ResponseVsEnergy`, `Profile2D`) | `libHarpPlot.so`, `libHarpPlottersPlugin.so` |
| `bin/` | `harpRun` (process), `harpPlot` (figures), `harpInspect` (file structure) | executables |
| `python/harp/` | `config` (Process/Module/VarParsing), `plotcfg`, `samples`, `condor`, `cli` | — |
| `configs/` | job descriptions: `skim_hadron_cfg.py`, `hist_hadron_cfg.py`, `plot_hadron_cfg.py`, `hgcal_reader_cff.py` | — |
| `samples/` | sample registry (`phase2_gun.json`) and cached file lists | — |
| `condor/` | submission notes and templates (driven by `harp submit`) | — |
| `docs/` | design, configuration, how to add a module, data model, condor, plotting | — |
| `test/` | `run_quick_test.sh`: skim + hist + plot on a few files | — |

## The three steps in one picture

```
             python configs/*.py  ──(harp run)──▶  JSON  ──▶  harpRun
                                                             │
    ┌────────────────────────────────────────────────────────┴───────────────┐
    │ Processor: for each event, run the path                                │
    │   HGCalNanoReader (Producer)  nano branches ─▶ EDM collections         │
    │   GenParticleFilter (Filter)  stop the path if the event fails         │
    │   HadronTruthMatcher (Producer)  gen ─▶ sim trackster ─▶ reco tracksters│
    │   ShowerVariablesProducer (Producer)  per-trackster PID variables      │
    │   ResponseAnalyzer / PIDAnalyzer / MatchingQAAnalyzer (Analyzers)      │
    │   NanoSkimWriter (Analyzer)  kept branches + products ─▶ skim tree     │
    └────────────────────────────────────────────────────────────────────────┘
```

Products are exchanged through the `Event` by label (`"hgcalReader:clue3d"`,
`"matchCLUE3D"`), histograms are booked in a per-module directory of the
output file, and anything a producer makes can be persisted in a skim as a
nano-style flat table (`nHadronMatchCLUE3D`, `HadronMatchCLUE3D_bestRawEnergy[...]`).

## Deliverables already wired in

1. **Response vs true hadron energy** for every hadron type of the gun samples
   (π0, π±, K±, p, p̄) and several reco energy definitions (best trackster raw /
   regressed, all linked tracksters, all tracksters, TICL candidate, sim
   trackster, all layer clusters): `ResponseAnalyzer` → `ResponseVsEnergy`
   plotter (Gaussian / mean / median slice extraction, resolution fit).
2. **Particle identification inputs**: per-trackster shower variables
   (number of layers, first / last layer, length, CE-E fraction, EM fraction,
   PCA widths, longitudinal RMS, …) versus energy and per hadron type, plus the
   TICL candidate pdgId assignment: `ShowerVariablesProducer` + `PIDAnalyzer`.
3. **Matching quality with sim information**: number of tracksters linked to
   the hadron, best association score, shared energy fractions, reco→sim purity:
   `MatchingQAAnalyzer`.

## Where to go next

* `docs/design.md` — architecture and the reasoning behind it
* `docs/adding_a_module.md` — write a new producer / filter / analyzer in 20 lines
* `docs/configuration.md` — python → JSON configuration, options, products
* `docs/edm.md` — the data model and how the nano branches map to it
* `docs/plotting.md` — plot jobs, plotters, slice fits
* `docs/condor.md` — batch processing
* `docs/nano_schema.md` — what the Phase-2 gun nanos contain
