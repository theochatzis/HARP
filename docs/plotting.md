# Plotting and fitting

`harp plot configs/plot_hadron_cfg.py histDir=output/hists outputDir=plots tag=v1`
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
* `EventSummaryAnalyzer`: `h_gen_*`, `h_lc_*`, `h_<collection>_n/rawEnergy/nLC`;
* the Processor: `harp/cutflow`.
