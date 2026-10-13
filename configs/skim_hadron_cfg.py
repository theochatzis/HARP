"""Skim configuration: nano -> slimmer nano with the hadron matching products.

    harp run configs/skim_hadron_cfg.py sample=pip_p1to200 maxFiles=2 maxEvents=1000 outputFile=skim_pip.root
    harp run configs/skim_hadron_cfg.py inputFiles=a.root,b.root outputFile=skim.root
"""
from harp.config import Process, Module, VarParsing
from harp import samples
from hgcal_reader_cff import hgcalReader, matcher

options = VarParsing()
options.register("inputFiles", [], list, "explicit input files (comma separated or @list.txt)")
options.register("sample", "", str, "sample name from samples/*.json (used if inputFiles is empty)")
options.register("maxFiles", -1, int, "limit the number of files taken from the sample")
options.register("firstFile", 0, int, "first file index taken from the sample")
options.register("maxEvents", -1, int, "events to process (-1: all)")
options.register("outputFile", "skim.root", str, "skim output file")
options.register("histFile", "skim_hists.root", str, "control histograms")
options.register("minEnergy", 0.0, float, "minimum generated energy [GeV]")
options.register("maxAbsEta", 3.0, float, "maximum |eta| of the generated hadron")
options.register("minAbsEta", 1.5, float, "minimum |eta| of the generated hadron")
options.parse()

process = Process("HADRONSKIM",
                  files=samples.resolve_inputs(options.inputFiles, options.sample, options.maxFiles, options.firstFile),
                  maxEvents=options.maxEvents, histFile=options.histFile)

process.hgcalReader = hgcalReader

# event selection: exactly one generated hadron in the HGCal acceptance
process.genFilter = Module("GenParticleFilter", src="hgcalReader:genParticles", status=1,
                           minEnergy=options.minEnergy, minAbsEta=options.minAbsEta, maxAbsEta=options.maxAbsEta, exactCount=1)
process.hasTrackster = Module("CollectionSizeFilter", src="hgcalReader:clue3d", kind="tracksters", min=1)

# derived products, persisted in the skim as flat tables
process.matchCLUE3D = matcher("hgcalReader:clue3d", "hgcalReader:simCP2clue3d")
process.matchCand = matcher("hgcalReader:candTrk", "hgcalReader:simCP2candTrk", candidates="hgcalReader:ticlCandidates")
process.showerCLUE3D = Module("ShowerVariablesProducer", tracksters="hgcalReader:clue3d", layerClusters="hgcalReader:layerClusters")

process.summary = Module("EventSummaryAnalyzer", genParticles="hgcalReader:genParticles",
                         layerClusters="hgcalReader:layerClusters", tracksters={"clue3d": "hgcalReader:clue3d"})

process.skimWriter = Module(
    "NanoSkimWriter",
    fileName=options.outputFile,
    treeName="Events",
    keep=[
        "run", "luminosityBlock", "event",
        "nHGCalGenPart", "HGCalGenPart_*",
        "nHGCalLayerClusters", "HGCalLayerClusters_*",
        "nticlTrackstersCLUE3DHigh*", "ticlTrackstersCLUE3DHigh*",
        "nticlCandidate*", "ticlCandidate*",
        "nticlSimTrackstersfromCPs*", "ticlSimTrackstersfromCPs*",
        "nSimCP2ticlTrackstersCLUE3DHighByHits*", "SimCP2ticlTrackstersCLUE3DHighByHits*",
        "nRecoticlTrackstersCLUE3DHigh2SimCPByHits*", "RecoticlTrackstersCLUE3DHigh2SimCPByHits*",
        "nSimCP2ticlCandidateByHits*", "SimCP2ticlCandidateByHits*",
        "nTICLCandidates", "TICLCandidates_*", "nlinkedTracksters", "linkedTracksters_*",
    ],
    drop=["HGCalLayerClusters_time*"],
    products={"matchCLUE3D": "HadronMatchCLUE3D", "matchCand": "HadronMatchCand", "showerCLUE3D": "ShowerCLUE3D"},
)

process.path = ["hgcalReader", "genFilter", "hasTrackster", "matchCLUE3D", "matchCand", "showerCLUE3D", "summary", "skimWriter"]
