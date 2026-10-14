"""Plots from hist_hadron_cfg.py outputs, organised by category:

    <outputDir>/<tag>/
      response/<analyzer>/<definition>/{scale,resolution,dist}   money plots: scale and resolution vs E_gen, integrated response
      pid/<variable>/{overlay, vsE_<hadron>}                      shower-shape variables per hadron type
      matching/<variable>/vsE_<hadron>, matching/{recoScore,recoMatched}   sim-matching quality
      budget/<hadron>/...                                         where the layer-cluster energy ends up (stacked)
      summary/...                                                 control distributions

    harp plot configs/plot_hadron_cfg.py histDir=output/hists tag=v1              # -> /eos/user/t/tchatzis/php-plots/harp/v1/
    harp plot configs/plot_hadron_cfg.py files=pip_p1to200:hists_pip.root,Kp_p1to200:hists_Kp.root outputDir=plots/test

The figures go to outputDir (default: the php-plots web directory); the description of
every figure is kept in the repository at plots/README.txt.

The hist files are looked up as <histDir>/<sample>.root unless 'files' is given.
A plot name may contain '/', which becomes a sub-directory of the output directory.
"""
import os
from harp.config import VarParsing
from harp.plotcfg import PlotJob, Source, hadron_style, layer_regions
from harp import samples

options = VarParsing()
options.register("histDir", "output/hists", str, "directory with <sample>.root histogram files")
options.register("files", [], list, "explicit sample:file pairs")
options.register("samples", ["pi0_p1to200", "pip_p1to200", "pim_p1to200", "Kp_p1to200", "Km_p1to200", "p_p1to200", "pbar_p1to200"], list, "samples to overlay")
options.register("outputDir", "/eos/user/t/tchatzis/php-plots/harp", str, "where to write the figures (<outputDir>/<tag>/...)")
options.register("tag", "", str, "sub-directory of outputDir")
options.register("method", "gauss", str, "slice extraction: gauss | mean | median")
options.register("formats", ["pdf", "png"], list, "figure formats")
options.register("drawSlices", False, bool, "also draw the per-energy-bin response distributions")
options.register("categories", ["response", "pid", "matching", "budget", "summary"], list, "which plot categories to produce")
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
    LAYER_VARS = ["firstLayer", "lastLayer", "layerOfMaxEnergy"]       # histogrammed on a 0..48 axis
    for var in ["nLayers", "firstLayer", "lastLayer", "layerOfMaxEnergy", "length", "fractionEE", "emFraction",
                "sigma2", "sigma3", "rmsLayer", "maxLayerEnergyFraction", "nLayerClusters"]:
        reg = dict(regions=layer_regions(firstBinEdge=1), legend=[0.40, 0.64, 0.93, 0.86]) if var in LAYER_VARS else {}
        job.add("Overlay1D", name="pid/%s/overlay" % var, sources=sources("pidCLUE3D/{hadron}/h_%s" % var), normalize=True, text=var, **reg)
        per_hadron("pid", var, "pidCLUE3D/{hadron}/h2_%s_vs_E" % var, **(dict(yRegions=layer_regions(firstBinEdge=1)) if var in LAYER_VARS else {}))
    per_hadron("pid", "candidatePdgId", "pidCLUE3D/{hadron}/h2_candPdg_vs_E")

# --- matching: sim <-> reco quality ------------------------------------------
if "matching" in options.categories:
    for var in ["nLinked", "nTracksters", "bestScore", "bestSharedFrac", "linkedSharedFrac", "bestRawOverSim", "recoMatched"]:
        per_hadron("matching", var, "qaCLUE3D/{hadron}/h2_%s_vs_E" % var)
    job.add("Overlay1D", name="matching/recoScore/overlay", sources=sources("qaCLUE3D/{hadron}/h_recoScore"), normalize=True, logy=True)
    job.add("Overlay1D", name="matching/recoMatched/overlay", sources=sources("qaCLUE3D/{hadron}/h_recoMatched"), normalize=True)
    job.add("Overlay1D", name="matching/hasReco/overlay", sources=sources("qaCLUE3D/{hadron}/h_hasReco"), normalize=True)

# --- budget: where the layer-cluster energy ends up ---------------------------
if "budget" in options.categories:
    CATS = [("best", "best trackster", "#0072B2"), ("other", "other tracksters", "#56B4E9"), ("none", "no trackster", "#D55E00")]
    COMPS = [("EE", "CE-E", ["#0072B2", "#56B4E9", "#D55E00"]), ("HAD", "CE-H", ["#004466", "#2C8FBF", "#8C3A00"])]
    E_WINDOWS = [(10, 20), (40, 70), (100, 200)]

    def comps(hadron, template, cats=CATS):
        return [dict(hist=template.format(hadron=hadron, cat=c), label=l, color=col) for c, l, col in cats]

    for name, path in hist_files.items():
        st = hadron_style(samples.info(name)["pdgId"])
        h, src = st["key"], [Source(path, label=st["label"])]
        base = "budget/%s/" % h
        # fraction of the layer-cluster energy per category vs E_gen (stacks to 1)
        job.add("StackedHist", name=base + "fraction_vsE", sources=src, components=comps(h, "budgetCLUE3D/{hadron}/p_frac_{cat}_vs_E"),
                logx=True, yRange=[0, 1.45], yTitle="Fraction of layer-cluster energy", text=st["label"])
        # the same divided by E_gen: stacks to the layer-cluster response, gap to 1 = energy never clustered
        job.add("StackedHist", name=base + "response_vsE", sources=src, components=comps(h, "budgetCLUE3D/{hadron}/p_resp_{cat}_vs_E"),
                logx=True, yRange=[0, 1.45], yTitle="E / E_{gen}", unityLine=True, text=st["label"])
        # split by compartment: 6 components
        cc = []
        for comp, clabel, cols in COMPS:
            cc += [dict(hist="budgetCLUE3D/%s/p_frac_%s_%s_vs_E" % (h, comp, c), label="%s, %s" % (clabel, l), color=col) for (c, l, _), col in zip(CATS, cols)]
        job.add("StackedHist", name=base + "compartment_vsE", sources=src, components=cc, logx=True, yRange=[0, 1.7],
                yTitle="Fraction of layer-cluster energy", legend=[0.50, 0.62, 0.93, 0.92], legendTextSize=0.034, text=st["label"])
        # per layer, in E_gen windows: energy fraction per layer per category
        layer_opts = dict(xTitle="Layer", regions=layer_regions(), legend=[0.55, 0.62, 0.93, 0.84])
        for lo, hi in E_WINDOWS:
            job.add("StackedHist", name=base + "layer_E%d-%d" % (lo, hi), sources=src, components=comps(h, "budgetCLUE3D/{hadron}/h2_layer_{cat}_vs_E"),
                    projectX=[lo, hi], normalizeToEntries="budgetCLUE3D/%s/h_nHadrons_vs_E" % h, yTitle="Mean energy per hadron (GeV)", headroom=1.9,
                    text="%s, %d < E_{gen} < %d GeV" % (st["label"], lo, hi), **layer_opts)
            job.add("StackedHist", name=base + "layerFraction_E%d-%d" % (lo, hi), sources=src, components=comps(h, "budgetCLUE3D/{hadron}/h2_layer_{cat}_vs_E"),
                    projectX=[lo, hi], normalize=True, yTitle="Fraction of layer energy", yRange=[0, 1.6], text="%s, %d < E_{gen} < %d GeV" % (st["label"], lo, hi), **layer_opts)
        # which layer-cluster energies are lost: energy-weighted spectrum and counts
        job.add("StackedHist", name=base + "lcEnergy", sources=src, components=comps(h, "budgetCLUE3D/{hadron}/h_lcEnergy_{cat}"),
                logx=True, xTitle="E_{LC} (GeV)", yTitle="Energy (GeV)", text=st["label"])
        job.add("StackedHist", name=base + "lcEnergyFraction", sources=src, components=comps(h, "budgetCLUE3D/{hadron}/h_lcEnergy_{cat}"),
                logx=True, normalize=True, xTitle="E_{LC} (GeV)", yTitle="Fraction of energy", yRange=[0, 1.45], text=st["label"])
        job.add("StackedHist", name=base + "lcCount", sources=src, components=comps(h, "budgetCLUE3D/{hadron}/h_lcN_{cat}"),
                logx=True, logy=True, xTitle="E_{LC} (GeV)", yTitle="Layer clusters", text=st["label"])
    # all hadrons overlaid: fraction of energy in no trackster, and in the best trackster
    for cat, label in [("none", "no trackster"), ("best", "best trackster"), ("other", "other tracksters")]:
        job.add("Overlay1D", name="budget/overlay_fraction_%s_vsE" % cat, sources=sources("budgetCLUE3D/{hadron}/p_frac_%s_vs_E" % cat),
                logx=True, yRange=[0, 1.45], yTitle="Fraction of E_{LC} in %s" % label, xTitle="E_{gen} (GeV)", drawOption="PE")
    for comp, clabel, _ in COMPS:
        job.add("Overlay1D", name="budget/overlay_fraction_%s_none_vsE" % comp, sources=sources("budgetCLUE3D/{hadron}/p_frac_%s_none_vs_E" % comp),
                logx=True, yRange=[0, 1.0], yTitle="Fraction of E_{LC} in no trackster, %s" % clabel, xTitle="E_{gen} (GeV)", drawOption="PE")

# --- summary: control distributions ------------------------------------------
if "summary" in options.categories:
    job.add("Overlay1D", name="summary/gen_energy", sources=sources("summary/h_gen_energy"))
    job.add("Overlay1D", name="summary/gen_eta", sources=sources("summary/h_gen_eta"))
    job.add("Overlay1D", name="summary/lc_n", sources=sources("summary/h_lc_n"), normalize=True)
    job.add("Overlay1D", name="summary/lc_layer", sources=sources("summary/h_lc_layer"), normalize=True, regions=layer_regions(firstBinEdge=1), legend=[0.40, 0.64, 0.93, 0.86])
    job.add("Overlay1D", name="summary/clue3d_n", sources=sources("summary/h_clue3d_n"), normalize=True)
    job.add("Overlay1D", name="summary/clue3d_rawEnergy", sources=sources("summary/h_clue3d_rawEnergy"), normalize=True, logy=True)
    job.add("Overlay1D", name="summary/candTrk_n", sources=sources("summary/h_candTrk_n"), normalize=True)
