"""Plot job description for harpPlot.

    from harp.plotcfg import PlotJob, Source
    job = PlotJob(outputDir="plots/v1", formats=["pdf", "png"], rootFile="plots/v1/graphs.root")
    job.add("ResponseVsEnergy", name="resp_bestRaw", hist="respCLUE3D/{hadron}/h2_bestRaw_vs_E",
            sources=[Source("hists_pip.root", label="#pi^{+}", color="#E69F00", hist="respCLUE3D/pip/h2_bestRaw_vs_E"), ...])
"""
import json
from collections import OrderedDict


class Source:
    def __init__(self, files, label="", color=None, marker=None, hist=None, lineStyle=None):
        self.files = [files] if isinstance(files, str) else list(files)
        self.label = label
        self.color = color
        self.marker = marker
        self.hist = hist
        self.lineStyle = lineStyle

    def to_dict(self):
        d = OrderedDict(files=self.files, label=self.label)
        for k in ("color", "marker", "hist", "lineStyle"):
            v = getattr(self, k)
            if v is not None:
                d[k] = v
        return d


class PlotJob:
    def __init__(self, outputDir="plots", formats=("pdf", "png"), rootFile="", cmsText="CMS", extraText="Simulation", rightText="Phase-2 (14 TeV)"):
        self.outputDir = outputDir
        self.formats = list(formats)
        self.rootFile = rootFile
        # TDR-style labels: "CMS" bold + italic extra text in the frame, right text above the frame
        self.label = OrderedDict(cms=cmsText, extra=extraText, right=rightText)
        self.plots = []

    def add(self, type_, name, sources, **options):
        if not sources:
            raise ValueError("plot %s has no sources" % name)
        d = OrderedDict()
        d["@type"] = type_
        d["name"] = name
        d["sources"] = [s.to_dict() if isinstance(s, Source) else s for s in sources]
        d.update(options)
        self.plots.append(d)
        return d

    def to_dict(self):
        return OrderedDict(outputDir=self.outputDir, formats=self.formats, rootFile=self.rootFile, label=self.label, plots=self.plots)

    def to_json(self, indent=2):
        return json.dumps(self.to_dict(), indent=indent)

    def dump(self, path):
        with open(path, "w") as f:
            f.write(self.to_json())
        return path

    def summary(self):
        lines = ["PlotJob -> %s (%s)" % (self.outputDir, ",".join(self.formats))]
        for p in self.plots:
            lines.append("  %-18s %s  [%d source(s)]" % (p["@type"], p["name"], len(p["sources"])))
        return "\n".join(lines)


# Shared cosmetics for the gun hadrons (pdgId -> legend label / colour).
HADRONS = OrderedDict([
    (111, dict(key="pi0", label="#pi^{0}", color="#000000", marker=20)),
    (211, dict(key="pip", label="#pi^{+}", color="#E69F00", marker=21)),
    (-211, dict(key="pim", label="#pi^{-}", color="#D55E00", marker=25)),
    (321, dict(key="Kp", label="K^{+}", color="#56B4E9", marker=22)),
    (-321, dict(key="Km", label="K^{-}", color="#0072B2", marker=26)),
    (2212, dict(key="p", label="p", color="#009E73", marker=23)),
    (-2212, dict(key="pbar", label="#bar{p}", color="#CC79A7", marker=32)),
])


def hadron_style(pdgId):
    return HADRONS.get(pdgId, dict(key="pdg%d" % pdgId, label=str(pdgId), color=None, marker=None))


# HGCal compartments in layer numbers (DataFormats/interface/HGCalLayers.h): 26 CE-E + 21 CE-H planes.
N_LAYERS_EE = 26
N_LAYERS = 47


def layer_regions(firstBinEdge=0.5, nEE=N_LAYERS_EE, nLayers=N_LAYERS):
    """CE-E / CE-H regions for a layer axis, for the 'regions' (x) / 'yRegions' (y) plot options.

    firstBinEdge is the lower edge of the bin holding layer 1: 0.5 for a 0.5..47.5 axis (bin centre = layer),
    1 for a 0..48 axis where layer l fills [l, l+1)."""
    boundary = firstBinEdge + nEE
    return [dict(label="CE-E", x1=firstBinEdge, x2=boundary), dict(label="CE-H", x1=boundary, x2=firstBinEdge + nLayers)]
