"""Plots from hist_hadron_cfg.py outputs, organised by category:

    <outputDir>/<tag>/
      response/<analyzer>/<definition>/{scale,resolution,dist}   money plots: scale and resolution vs E_gen, integrated response
      pid/<variable>/{overlay, vsE_<hadron>}                      shower-shape variables per hadron type
      matching/<variable>/vsE_<hadron>, matching/{recoScore,recoMatched}   sim-matching quality
      summary/...                                                 control distributions

    harp plot configs/plot_hadron_cfg.py histDir=output/hists tag=v1
    harp plot configs/plot_hadron_cfg.py files=pip_p1to200:hists_pip.root,Kp_p1to200:hists_Kp.root outputDir=plots/test

The hist files are looked up as <histDir>/<sample>.root unless 'files' is given.
A plot name may contain '/', which becomes a sub-directory of the output directory.
"""
import os
from harp.config import VarParsing
from harp.plotcfg import PlotJob, Source, hadron_style
from harp import samples

options = VarParsing()
options.register("histDir", "output/hists", str, "directory with <sample>.root histogram files")
options.register("files", [], list, "explicit sample:file pairs")
options.register("samples", ["pi0_p1to200", "pip_p1to200", "pim_p1to200", "Kp_p1to200", "Km_p1to200", "p_p1to200", "pbar_p1to200"], list, "samples to overlay")
options.register("outputDir", "plots", str, "where to write the figures")
options.register("tag", "", str, "sub-directory of outputDir")
options.register("method", "gauss", str, "slice extraction: gauss | mean | median")
options.register("formats", ["pdf", "png"], list, "figure formats")
options.register("drawSlices", False, bool, "also draw the per-energy-bin response distributions")
options.register("categories", ["response", "pid", "matching", "summary"], list, "which plot categories to produce")
options.parse()

# sample -> hist file
hist_files = {}
if options.files:
    for item in options.files:
        name, path = item.split(":", 1)
        hist_files[name] = path
else:
    for name in options.samples:
        path = os.path.join(options.histDir, name + ".root")
        if os.path.exists(path):
            hist_files[name] = path
if not hist_files:
    raise SystemExit("no histogram files found (histDir=%s)" % options.histDir)

out = os.path.join(options.outputDir, options.tag) if options.tag else options.outputDir
job = PlotJob(outputDir=out, formats=options.formats, rootFile=os.path.join(out, "graphs.root"))


def sources(hist_template):
    """One Source per hadron sample; {hadron} in the template is replaced by the pdg key."""
    out = []
    for name, path in hist_files.items():
        st = hadron_style(samples.info(name)["pdgId"])
        out.append(Source(path, label=st["label"], color=st["color"], marker=st["marker"], hist=hist_template.format(hadron=st["key"])))
    return out


def per_hadron(category, var, hist_template, **opts):
    """One Profile2D per hadron sample: <category>/<var>/vsE_<hadron>."""
    for name, path in hist_files.items():
        st = hadron_style(samples.info(name)["pdgId"])
        job.add("Profile2D", name="%s/%s/vsE_%s" % (category, var, st["key"]), sources=[Source(path, label=st["label"])],
                hist=hist_template.format(hadron=st["key"]), normalizeColumns=True, logx=True, **opts)


common = dict(method=options.method, logx=True, minEntries=30, drawSlices=options.drawSlices)

# --- response: scale and resolution vs E_gen, integrated distributions ------
if "response" in options.categories:
    for analyzer, defs in [("respCLUE3D", ["bestRaw", "linkedRaw", "allRaw", "simRaw", "layerClusters"]),
                           ("respCand", ["bestRaw", "candidate", "candidateRaw", "allRaw"])]:
        tag = analyzer.replace("resp", "").lower()          # clue3d, cand
        for d in defs:
            base = "response/%s/%s" % (tag, d)
            job.add("ResponseVsEnergy", name=base + "/vsE", sources=sources(analyzer + "/{hadron}/h2_%s_vs_E" % d),
                    text="%s: %s" % (tag, d), fitResolution=(d in ("bestRaw", "candidate", "simRaw", "allRaw")), **common)
            job.add("Overlay1D", name=base + "/dist", sources=sources(analyzer + "/{hadron}/h_%s" % d), normalize=True, text="%s: %s" % (tag, d))
            per_hadron("response/%s/%s" % (tag, d), "", analyzer + "/{hadron}/h2_%s_vs_E" % d, yTitle="E_{reco}/E_{gen}")

# --- pid: shower-shape variables ---------------------------------------------
if "pid" in options.categories:
    for var in ["nLayers", "firstLayer", "lastLayer", "layerOfMaxEnergy", "length", "fractionEE", "emFraction",
                "sigma2", "sigma3", "rmsLayer", "maxLayerEnergyFraction", "nLayerClusters"]:
        job.add("Overlay1D", name="pid/%s/overlay" % var, sources=sources("pidCLUE3D/{hadron}/h_%s" % var), normalize=True, text=var)
        per_hadron("pid", var, "pidCLUE3D/{hadron}/h2_%s_vs_E" % var)
    per_hadron("pid", "candidatePdgId", "pidCLUE3D/{hadron}/h2_candPdg_vs_E")

# --- matching: sim <-> reco quality ------------------------------------------
if "matching" in options.categories:
    for var in ["nLinked", "nTracksters", "bestScore", "bestSharedFrac", "linkedSharedFrac", "bestRawOverSim", "recoMatched"]:
        per_hadron("matching", var, "qaCLUE3D/{hadron}/h2_%s_vs_E" % var)
    job.add("Overlay1D", name="matching/recoScore/overlay", sources=sources("qaCLUE3D/{hadron}/h_recoScore"), normalize=True, logy=True)
    job.add("Overlay1D", name="matching/recoMatched/overlay", sources=sources("qaCLUE3D/{hadron}/h_recoMatched"), normalize=True)
    job.add("Overlay1D", name="matching/hasReco/overlay", sources=sources("qaCLUE3D/{hadron}/h_hasReco"), normalize=True)

# --- summary: control distributions ------------------------------------------
if "summary" in options.categories:
    job.add("Overlay1D", name="summary/gen_energy", sources=sources("summary/h_gen_energy"))
    job.add("Overlay1D", name="summary/gen_eta", sources=sources("summary/h_gen_eta"))
    job.add("Overlay1D", name="summary/lc_n", sources=sources("summary/h_lc_n"), normalize=True)
    job.add("Overlay1D", name="summary/lc_layer", sources=sources("summary/h_lc_layer"), normalize=True)
    job.add("Overlay1D", name="summary/clue3d_n", sources=sources("summary/h_clue3d_n"), normalize=True)
    job.add("Overlay1D", name="summary/clue3d_rawEnergy", sources=sources("summary/h_clue3d_rawEnergy"), normalize=True, logy=True)
    job.add("Overlay1D", name="summary/candTrk_n", sources=sources("summary/h_candTrk_n"), normalize=True)
