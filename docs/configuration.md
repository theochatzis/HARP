# Configuration

A job is a python file that builds a `harp.config.Process` (or, for plots, a
`harp.plotcfg.PlotJob`) and is executed by `harp run` / `harp plot`:

```
harp run configs/hist_hadron_cfg.py sample=pip_p1to200 maxFiles=3 histFile=h.root [--workdir work] [--dry-run] [-- harpRun options]
```

1. the python file runs with `sys.argv` = the `key=value` options;
2. `process.to_json()` is written to `work/<cfg>.json`;
3. `harpRun work/<cfg>.json <harpRun options>` is executed.

## Process

```python
from harp.config import Process, Module, VarParsing
from harp import samples

options = VarParsing()
options.register("sample", "", str, "sample from samples/*.json")
options.register("maxFiles", -1, int, "files to use")
options.register("histFile", "hists.root", str, "output")
options.parse()                                   # 'help' prints the options

process = Process("MYJOB", files=samples.files(options.sample, options.maxFiles), histFile=options.histFile, maxEvents=-1)
process.reader = Module("HGCalNanoReader", genParticles="HGCalGenPart", tracksters={"clue3d": "ticlTrackstersCLUE3DHigh"})
process.sel    = Module("GenParticleFilter", src="reader:genParticles", minEnergy=5)
process.path   = ["reader", "sel"]
```

* Attributes assigned a `Module` become entries of the `modules` block; the
  attribute name is the module label.
* `process.path` is the ordered list of labels. Filters stop the path.
* `Module.clone(**overrides)` makes variants; `Module` parameters can be any
  JSON value (numbers, strings, lists, dicts).
* Shared fragments live in `configs/*_cff.py` and are imported.

Resulting JSON (what `harp dump` prints):

```json
{
  "process": "MYJOB",
  "input":   {"files": ["..."], "treeName": "Events", "maxEvents": -1, "skipEvents": 0},
  "output":  {"histFile": "hists.root"},
  "options": {"logLevel": "INFO", "reportEvery": 1000, "pluginDirs": []},
  "modules": {"reader": {"@type": "HGCalNanoReader", "genParticles": "HGCalGenPart", ...}, "sel": {...}},
  "path":    ["reader", "sel"]
}
```

`harpRun` options override the JSON: `-n` (maxEvents), `-s` (skipEvents),
`-i` (files), `-o` (histFile), `-v`, `--dump`.

## Input files

`input.files` entries may be files, `@list.txt` (one path per line) or a
directory (all `*.root`). `/eos/...` paths work through the fuse mount; the
nanos open through xrootd automatically.

## Samples

`samples/phase2_gun.json` maps a name to a directory and metadata
(`pdgId`, legend label). `harp samples scan` caches the file lists in
`samples/files/<name>.txt`; `harp.samples.files(name, max_files)` is what the
configurations call.

## Products and labels

`HGCalNanoReader` produces one product per collection: `reader:genParticles`,
`reader:clue3d`, ... (the dict keys). Single-product modules use their label
(`matchCLUE3D`). `Event::get` resolves `module` to `module:instance` when the
module has exactly one product.

## Options of the shipped configurations

| Config | Options |
|--------|---------|
| `skim_hadron_cfg.py` | `inputFiles`, `sample`, `maxFiles`, `firstFile`, `maxEvents`, `outputFile`, `histFile`, `minEnergy`, `minAbsEta`, `maxAbsEta` |
| `hist_hadron_cfg.py` | `inputFiles`, `sample`, `maxFiles`, `firstFile`, `maxEvents`, `histFile`, `linkScoreCut`, `minSharedEnergy`, `purityScoreCut`, `perHadronType` |
| `plot_hadron_cfg.py` | `histDir`, `files`, `samples`, `outputDir`, `tag`, `method`, `formats`, `drawSlices` |

Run `harp run <cfg> help` to see them with defaults.
