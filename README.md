# HARP — Hadron Analysis & Response Performance

C++ framework (python only for job configuration) to study hadrons in the CMS
Phase-2 HGCal from nano-like inputs: energy response, particle identification
and sim-matching quality, from single-particle guns to jets.

## Setup and build (lxplus, EL9)

```bash
git clone <repo> HARP
cd HARP
source env/setup.sh          # LCG_107a: ROOT 6.34, gcc 13, cmake
mkdir -p build
cd build
cmake ..
make -j8
cd ..
harp samples scan            # cache the file lists of samples/phase2_gun.json
```

## Working example

```bash
# histograms for one gun sample (4 files), then for all of them
harp run configs/hist_hadron_cfg.py sample=pip_p1to200 maxFiles=4 histFile=output/hists/pip_p1to200.root
for s in $(harp samples list | awk '{print $1}'); do
  harp run configs/hist_hadron_cfg.py sample=$s histFile=output/hists/$s.root
done

# figures -> /eos/user/t/tchatzis/php-plots/harp/hadrons_noPU/{response,pid,matching,budget,summary}
harp plot configs/plot_hadron_cfg.py histDir=output/hists tag=hadrons_noPU

# optional: a skim (slim nano + matching tables) and histograms from it
harp run configs/skim_hadron_cfg.py sample=pip_p1to200 maxFiles=1 outputFile=skim_pip.root
harp run configs/hist_hadron_cfg.py inputFiles=skim_pip.root histFile=hists_from_skim.root
harpInspect skim_pip.root -c

# condor for the full samples
harp submit --cfg configs/hist_hadron_cfg.py --hist-only --sample pip_p1to200 Kp_p1to200 \
            --output-dir /eos/user/$USER/HARP/v1 --tag v1 --files-per-job 10
harp check --tag v1
harp merge --tag v1
```

Every command answers `-h`; configuration options are listed with
`harp run configs/hist_hadron_cfg.py help`. `plots/README.txt` explains every figure.

## How it works

A job is a python `Process`: a list of modules run in order on every event.
Modules are C++ plugins of three kinds, producers (make objects: `HGCalNanoReader`
reads the nano into the data model, `HadronTruthMatcher` matches gen → sim → reco,
`ShowerVariablesProducer`), filters (`GenParticleFilter`) and analyzers
(`ResponseAnalyzer`, `PIDAnalyzer`, `MatchingQAAnalyzer`, `EnergyBudgetAnalyzer`,
`NanoSkimWriter`). Objects are exchanged through the event by label, histograms go
to one directory per module, and products can be written into skims as nano branches.
Adding a study means adding one `.cc` file under `Modules/` and a few lines of config.

| Directory | Content |
|-----------|---------|
| `Core/`, `DataFormats/` | framework kernel and event data model (`libHarpCore`, `libHarpDataFormats`) |
| `Modules/<Pkg>/plugins/` | analysis modules, one plugin library per package |
| `Fit/`, `Plot/` | slice fits, resolution function, TDR style, plotter plugins |
| `bin/` | `harpRun`, `harpPlot`, `harpInspect` |
| `python/harp/`, `scripts/harp` | configuration layer, sample registry, condor, the `harp` CLI |
| `configs/` | `hist_hadron_cfg.py`, `skim_hadron_cfg.py`, `plot_hadron_cfg.py`, `hgcal_reader_cff.py` |
| `samples/`, `condor/`, `docs/`, `test/` | sample registry, batch notes, documentation, smoke test |

## Documentation

`docs/design.md` (architecture), `docs/adding_a_module.md` (new module in 20 lines),
`docs/configuration.md`, `docs/edm.md` (data model and nano mapping),
`docs/plotting.md`, `docs/condor.md`, `docs/nano_schema.md` (what the gun nanos contain).
