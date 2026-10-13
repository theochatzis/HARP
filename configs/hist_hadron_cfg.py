"""Histogram configuration: response, PID inputs and matching QA.

Runs either directly on the gun nanos or on skims from skim_hadron_cfg.py
(the skim keeps the collections this configuration needs).

    harp run configs/hist_hadron_cfg.py sample=pip_p1to200 maxFiles=5 histFile=hists_pip.root
    harp run configs/hist_hadron_cfg.py inputFiles=skim_pip.root histFile=hists_pip.root
"""
from harp.config import Process, Module, VarParsing
from harp import samples
from hgcal_reader_cff import hgcalSkimReader, matcher, ENERGY_BINS

options = VarParsing()
options.register("inputFiles", [], list, "explicit input files (comma separated or @list.txt)")
options.register("sample", "", str, "sample name from samples/*.json (used if inputFiles is empty)")
options.register("maxFiles", -1, int, "limit the number of files taken from the sample")
options.register("firstFile", 0, int, "first file index taken from the sample")
options.register("maxEvents", -1, int, "events to process (-1: all)")
options.register("histFile", "hists.root", str, "histogram output file")
options.register("linkScoreCut", 1.0, float, "sim->reco score below which a reco trackster counts as linked to the hadron (0 = perfect, 1 = none)")
options.register("minSharedEnergy", 0.0, float, "minimum energy [GeV] a linked trackster must share with the sim trackster")
options.register("purityScoreCut", 0.6, float, "reco->sim score below which a reco trackster is 'correctly matched' (matching QA)")
options.register("perHadronType", True, bool, "one histogram directory per hadron pdgId")
options.parse()

process = Process("HADRONHIST",
                  files=samples.resolve_inputs(options.inputFiles, options.sample, options.maxFiles, options.firstFile),
                  maxEvents=options.maxEvents, histFile=options.histFile)

process.hgcalReader = hgcalSkimReader
process.genFilter = Module("GenParticleFilter", src="hgcalReader:genParticles", status=1, minAbsEta=1.5, maxAbsEta=3.0, exactCount=1)

process.matchCLUE3D = matcher("hgcalReader:clue3d", "hgcalReader:simCP2clue3d", scoreCut=options.linkScoreCut, minSharedEnergy=options.minSharedEnergy)
process.matchCand = matcher("hgcalReader:candTrk", "hgcalReader:simCP2candTrk", candidates="hgcalReader:ticlCandidates", scoreCut=options.linkScoreCut, minSharedEnergy=options.minSharedEnergy)
process.showerCLUE3D = Module("ShowerVariablesProducer", tracksters="hgcalReader:clue3d", layerClusters="hgcalReader:layerClusters")

process.summary = Module("EventSummaryAnalyzer", genParticles="hgcalReader:genParticles",
                         layerClusters="hgcalReader:layerClusters", tracksters={"clue3d": "hgcalReader:clue3d", "candTrk": "hgcalReader:candTrk"})

# 1) response vs true energy for several reco energy definitions
process.respCLUE3D = Module("ResponseAnalyzer", matches="matchCLUE3D", energyBins=ENERGY_BINS, perHadronType=options.perHadronType,
                            definitions=["bestRaw", "bestRegressed", "linkedRaw", "linkedRegressed", "allRaw", "allRegressed", "simRaw", "layerClusters"])
process.respCand = Module("ResponseAnalyzer", matches="matchCand", energyBins=ENERGY_BINS, perHadronType=options.perHadronType,
                          definitions=["bestRaw", "bestRegressed", "candidate", "candidateRaw", "allRaw"])
# 2) particle identification inputs
process.pidCLUE3D = Module("PIDAnalyzer", matches="matchCLUE3D", showerVariables="showerCLUE3D", energyBins=ENERGY_BINS, perHadronType=options.perHadronType)
# 3) matching quality with the sim information
process.qaCLUE3D = Module("MatchingQAAnalyzer", matches="matchCLUE3D", tracksters="hgcalReader:clue3d", recoToSim="hgcalReader:clue3d2simCP",
                          scoreCut=options.purityScoreCut, energyBins=ENERGY_BINS, perHadronType=options.perHadronType)

process.path = ["hgcalReader", "genFilter", "matchCLUE3D", "matchCand", "showerCLUE3D",
                "summary", "respCLUE3D", "respCand", "pidCLUE3D", "qaCLUE3D"]
