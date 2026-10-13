# The Phase-2 gun nanos (`/eos/user/t/tchatzis/phase2_gun/`)

Seven samples, ~190–200 files × 500 events each, one generated hadron per event
with flat momentum 1–200 GeV in the HGCal acceptance:

| Sample | pdgId | | Sample | pdgId |
|--------|-------|-|--------|-------|
| `pi0_p1to200` | 111 | | `Km_p1to200` | −321 |
| `pip_p1to200` | 211 | | `p_p1to200` | 2212 |
| `pim_p1to200` | −211 | | `pbar_p1to200` | −2212 |
| `Kp_p1to200` | 321 | | | |

`harpInspect <file> -c` prints the collections; the ones HARP uses:

| Collection (counter) | Content |
|----------------------|---------|
| `nHGCalGenPart` | generated particle(s) |
| `nHGCalLayerClusters` | CLUE layer clusters (energy, position, time, nHits, algoID) |
| `nticlTrackstersCLUE3DHigh` (+`...vertices`) | CLUE3D tracksters, with their layer-cluster indices |
| `nticlTracksterLinks` (+`...vertices`) | tracksters after linking |
| `nticlCandidate` (+`...vertices`) | final tracksters, one per TICL candidate |
| `nTICLCandidates`, `nlinkedTracksters`, `nTICLCandidatesExtra` | TICL candidates (pdgID, energy, track index) and their trackster indices |
| `nticlSimTrackstersfromCPs` | sim tracksters from CaloParticles (the truth of the gun hadron) |
| `nticlSimTracksters` | sim tracksters from SimClusters |
| `nSimCP2ticlTrackstersCLUE3DHighByHits` (+`...Links`) | sim CaloParticle → CLUE3D trackster association (index, score, sharedEnergy) |
| `nRecoticlTrackstersCLUE3DHigh2SimCPByHits` (+`...Links`) | the reverse association |
| `...TracksterLinks...`, `...ticlCandidate...`, `...SuperclusteringDNN...` | the same association pairs for the other trackster collections, and for SimClusters (`SimSC`) |
| `nGeneralTrack`, `nGSFTrack` | tracks (with HGCal front-face propagation) |
| `HLT_*`, `L1*` | trigger bits (unused) |

Observations that matter for the analysis:

* `ticlTrackstersCLUE3DHigh_regressed_energy` is 0 in these samples; the
  regressed energy is only available for the candidate tracksters
  (`ticlCandidate`) and `TICLCandidates_energy`. Use `bestRaw` for CLUE3D.
* Hadron showers are split into several CLUE3D tracksters; the best-matched
  one carries a fraction of the energy and its association score is often
  above 0.5. Hence the `linked*` and `all*` response definitions.
* Association scores: 0 = perfect, 1 = no overlap; `sharedEnergy` is in GeV.
* `TICLCandidates_trackIdx` is INT_MIN when there is no track (mapped to −1).
* Layer-cluster z positions fall on 47 planes (see `docs/edm.md`).
