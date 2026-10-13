"""Shared module definitions for the Phase-2 single-hadron gun nanos
(the "_cff" naming follows CMSSW: a fragment imported by configurations).

Collections produced (product labels):
  hgcalReader:genParticles, hgcalReader:layerClusters, hgcalReader:tracks
  hgcalReader:clue3d        CLUE3D tracksters             (ticlTrackstersCLUE3DHigh)
  hgcalReader:linked        linked tracksters             (ticlTracksterLinks)
  hgcalReader:candTrk       candidate tracksters          (ticlCandidate)
  hgcalReader:simCP         sim tracksters from CaloParticles
  hgcalReader:simSC         sim tracksters from SimClusters
  hgcalReader:simCP2clue3d, hgcalReader:clue3d2simCP, ... association maps
  hgcalReader:ticlCandidates TICL candidates (linked to candTrk)
"""
from harp.config import Module

ENERGY_BINS = [1, 2, 3, 5, 7, 10, 15, 20, 30, 40, 50, 70, 100, 130, 160, 200]

hgcalReader = Module(
    "HGCalNanoReader",
    genParticles="HGCalGenPart",
    layerClusters="HGCalLayerClusters",
    tracks="GeneralTrack",
    tracksters={
        "clue3d": "ticlTrackstersCLUE3DHigh",
        "linked": "ticlTracksterLinks",
        "candTrk": "ticlCandidate",
        "simCP": "ticlSimTrackstersfromCPs",
        "simSC": "ticlSimTracksters",
    },
    associations={
        "simCP2clue3d": "SimCP2ticlTrackstersCLUE3DHighByHits",
        "clue3d2simCP": "RecoticlTrackstersCLUE3DHigh2SimCPByHits",
        "simCP2linked": "SimCP2ticlTracksterLinksByHits",
        "linked2simCP": "RecoticlTracksterLinks2SimCPByHits",
        "simCP2candTrk": "SimCP2ticlCandidateByHits",
        "candTrk2simCP": "RecoticlCandidate2SimCPByHits",
    },
    ticlCandidates={"ticlCandidates": "TICLCandidates"},
    linkedTracksters="linkedTracksters",
)

# A reader for skims produced by skim_hadron_cfg.py: same nano layout, fewer collections.
hgcalSkimReader = Module(
    "HGCalNanoReader",
    genParticles="HGCalGenPart",
    layerClusters="HGCalLayerClusters",
    tracksters={"clue3d": "ticlTrackstersCLUE3DHigh", "candTrk": "ticlCandidate", "simCP": "ticlSimTrackstersfromCPs"},
    associations={
        "simCP2clue3d": "SimCP2ticlTrackstersCLUE3DHighByHits",
        "clue3d2simCP": "RecoticlTrackstersCLUE3DHigh2SimCPByHits",
        "simCP2candTrk": "SimCP2ticlCandidateByHits",
    },
    ticlCandidates={"ticlCandidates": "TICLCandidates"},
)


def matcher(tracksters, simToReco, candidates="", scoreCut=1.0, minSharedEnergy=0.0):
    """HadronTruthMatcher for a given reco trackster collection."""
    return Module(
        "HadronTruthMatcher",
        genParticles="hgcalReader:genParticles",
        simTracksters="hgcalReader:simCP",
        tracksters=tracksters,
        simToReco=simToReco,
        layerClusters="hgcalReader:layerClusters",
        candidates=candidates,
        genToSim="deltaR",
        maxDeltaR=0.3,
        scoreCut=scoreCut,
        minSharedEnergy=minSharedEnergy,
        genStatus=1,
    )
