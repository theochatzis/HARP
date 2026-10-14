# Plotting and fitting

`harp plot configs/plot_hadron_cfg.py histDir=output/hists tag=v1   # figures -> /eos/user/t/tchatzis/php-plots/harp/v1/`
builds a JSON plot job and runs `harpPlot`. A plot job is a list of figures,
each drawn by a plotter plugin (`harpPlot --list-plotters`).

```python
from harp.plotcfg import PlotJob, Source, hadron_style
job = PlotJob(outputDir="plots/v1", formats=["pdf", "png"], rootFile="plots/v1/graphs.root")
job.add("ResponseVsEnergy", name="resp_bestRaw",
        sources=[Source("hists/pip_p1to200.root", label="#pi^{+}", color="#E69F00", marker=21, hist="respCLUE3D/pip/h2_bestRaw_vs_E"),
                 Source("hists/Kp_p1to200.root",  label="K^{+}",   color="#56B4E9", marker=22, hist="respCLUE3D/Kp/h2_bestRaw_vs_E")],
        method="gauss", nSigma=1.5, minEntries=30, logx=True, fitResolution=True)
```

A `Source` is one or more files (summed), a legend label, colours and
optionally its own histogram path (otherwise the plot-level `hist`).

## Style

All figures use the CMS TDR style ported from `tdrstyle_mod22.C`
(`Plot/interface/TdrStyle.h`, namespace `harp::tdr`: `setTDRStyle`,
`tdrCanvas`, `tdrDiCanvas`, `tdrHist`, `tdrLeg`, `tdrDraw`, `cmsLumi`).
The labels come from the job: `PlotJob(cmsText="CMS", extraText="Simulation",
rightText="Phase-2 (14 TeV)")`; "CMS" and the italic extra text are drawn
inside the frame (top left), the right text above the frame. Every plotter
accepts `legend=[x1, y1, x2, y2]` and `legendColumns` (default: two columns
across the top) and `text` for a short in-frame annotation.

Detector regions along an axis are drawn with `regions` (x axis of `Overlay1D`
and `StackedHist`) or `yRegions` (y axis of `Profile2D`): a list of
`{"label", "x1", "x2"}`, giving a dashed line at every inner boundary and the
labels just inside the frame. `harp.plotcfg.layer_regions(firstBinEdge)` returns
the CE-E (layers 1-26) / CE-H (27-47) split for a layer axis; pass the lower edge
of the bin holding layer 1 (0.5 for the `0.5..47.5` budget histograms, 1 for the
`0..48` PID / summary histograms). `regionLabelPos`, `regionTextSize`,
`regionLineStyle` and `regionLineColor` tune the cosmetics; `StackedHist` also
takes `headroom` (automatic y maximum = headroom x stack maximum, default 1.6)
to leave room for the labels.

A plot `name` may contain `/`: the figure is written to that sub-directory of
`outputDir`. `plot_hadron_cfg.py` uses this to organise the output by category:

```
<outputDir>/<tag>/
  response/<clue3d|cand>/<definition>/   vsE_scale, vsE_resolution, dist, vsE_<hadron>
  pid/<variable>/                        overlay, vsE_<hadron>
  matching/<variable>/                   vsE_<hadron>, overlay
  summary/                               control distributions
  graphs.root                            scale / resolution graphs of all ResponseVsEnergy plots
```

`categories=response,pid` restricts the job to some categories.

| Plotter | Input | Output |
|---------|-------|--------|
| `Overlay1D` | TH1 per source | overlay, optional normalisation, log axes, ratio panel |
| `ResponseVsEnergy` | TH2 (x = E_gen, y = response) per source | `<name>_scale`: ⟨R⟩ vs E; `<name>_resolution`: σ/⟨R⟩ vs E; optional resolution fit S/√E ⊕ C and per-bin slice canvases; graphs saved in `rootFile` |
| `Profile2D` | one TH2 | COLZ with per-column normalisation and the profile overlaid |
| `StackedHist` | several TProfile / TH2 / TH1 components of one source | stacked histogram (energy budget); TProfiles are stacked by their means, TH2s projected over `projectX`; `normalize` per bin, `normalizeToEntries` per hadron |

## Slice extraction (`Fit/SliceFitter`)

For every x-bin of the TH2 the y-projection is analysed with
`method = "gauss"` (iterative Gaussian fit within ±nσ of the core, default),
`"mean"` (mean / RMS) or `"median"` (median / half 68 % interval). The scale
is the central value, the resolution is width / scale. Bins with fewer than
`minEntries` entries are skipped.

## Resolution parametrisation (`Fit/ResolutionFunction`)

`fitResolution=True` fits σ/⟨R⟩ vs E with √(S²/E + C²) (optionally + N²/E²)
and prints S and C on the figure. The `Fit` library is the place for future
response models (non-linearity corrections, per-hadron calibration
functions); it depends only on ROOT and Core.

## Histogram naming

Analyzers write to `<moduleLabel>/<hadron>/<histogram>` where `<hadron>` is
`pi0, pip, pim, Kp, Km, p, pbar` (or `all` when `perHadronType=False`):

* `ResponseAnalyzer`: `h2_<def>_vs_E`, `p_<def>_vs_E`, `h2_<def>_vs_eta`, `h_<def>` for each response definition;
* `PIDAnalyzer`: `h2_<var>_vs_E`, `h_<var>`, `h2_nLinked_vs_E`, `h2_nTracksters_vs_E`, `h2_candPdg_vs_E`;
* `MatchingQAAnalyzer`: `h2_nLinked_vs_E`, `h2_bestScore_vs_E`, `h2_bestSharedFrac_vs_E`, `h2_linkedSharedFrac_vs_E`, `h2_bestRawOverSim_vs_E`, `h2_recoScore_vs_rawE`, `h2_recoMatched_vs_E`, `h_recoScore`, `h_recoMatched`, `h_hasSim`, `h_hasReco`;
* `EnergyBudgetAnalyzer`: `p_frac_<cat>_vs_E`, `p_resp_<cat>_vs_E`, `p_frac_<EE|HAD>_<cat>_vs_E`, `h2_layer_<cat>_vs_E`, `h_lcEnergy_<cat>`, `h_lcN_<cat>`, `h_nHadrons_vs_E` with `<cat>` = `best`, `other`, `none`;
* `EventSummaryAnalyzer`: `h_gen_*`, `h_lc_*`, `h_<collection>_n/rawEnergy/nLC`;
* the Processor: `harp/cutflow`.
