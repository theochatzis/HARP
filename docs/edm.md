# Event data model

All types are plain structs in `DataFormats/interface/`, collections are
`std::vector`s. They are filled from the nano by `HGCalNanoReader`
(`Modules/HGCalReaders`).

| Type | Nano source (default prefixes) | Notes |
|------|-------------------------------|-------|
| `GenParticle` | `HGCalGenPart_*` | pdgId, status, energy, pt, eta, phi, vertex; `hadronName(pdgId)` gives `pip`, `Km`, ... used as histogram directories |
| `LayerCluster` | `HGCalLayerClusters_*` | energy, position, time, nHits, algoId; `layer` (1..47) computed from \|z\| with `HGCalLayers::layerFromZ` |
| `Trackster` | `ticlTrackstersCLUE3DHigh_*`, `ticlTracksterLinks_*`, `ticlCandidate_*`, `ticlSimTracksters*_*` | raw / EM / regressed energy, barycenter, PCA eigenvalues and axis, time, `vertices` (indices into the layer clusters, resolved from the nested `..._n/o..vertices` + `..vertices_vertices` branches); sim-only fields (`caloParticleIdx`, boundary position/momentum, `isPU`) when present |
| `TICLCandidate` | `TICLCandidates_*` + `linkedTracksters_tracksterIndex` | pdgId, charge, track index, energies, `tracksterIndices` into the `ticlCandidate` trackster collection |
| `Track` | `GeneralTrack_*` | kinematics, hits, position propagated to the HGCal front face |
| `AssociationMap` | `<Name>_n<Name>Links` + `<Name>Links_{index,score,sharedEnergy}` | `links[from]` = vector of (`index`, `score`, `sharedEnergy`); `best(from)` = lowest score. Score 0 = perfect, 1 = none |
| `HadronMatch` | produced by `HadronTruthMatcher` | per generated hadron: sim trackster, best / linked / all reco tracksters energies, TICL candidate, total layer-cluster energy |
| `ShowerVariables` | produced by `ShowerVariablesProducer` | per trackster: number of layers, first/last layer, length, CE-E / EM fractions, PCA widths, longitudinal RMS |

## Matching chain (gun samples)

```
GenParticle ──same energy and eta (sim regressed_energy = CaloParticle energy)──▶ sim trackster (CaloParticle)
                                                       │  AssociationMap  SimCP2ticlTrackstersCLUE3DHighByHits
                                                       ▼
                                             reco tracksters (best = lowest score, linked = score < cut)
                                                       │  TICLCandidate.tracksterIndices (for the ticlCandidate collection)
                                                       ▼
                                                 TICL candidate (pdgId, energy)
```

Details and the quality criteria are in `docs/matching.md`. The same `HadronTruthMatcher` serves any reco trackster collection; the
shipped configs run it for CLUE3D tracksters (`matchCLUE3D`) and for the
candidate tracksters with the TICL candidate look-up (`matchCand`).

## HGCal layers

`HGCalLayers` holds the 47 |z| planes (cm) measured from the layer clusters
of the gun nanos (26 CE-E + 21 CE-H, first plane at 322.2 cm, last at
513.2 cm). If the geometry changes, regenerate the table (unique rounded |z|
of `HGCalLayerClusters_position_z`) and update `DataFormats/src/HGCalLayers.cc`.

## Persistence

`DataFormats/src/Persistence.cc` registers the `FlatTable` converters of
`HadronMatchCollection` and `ShowerVariablesCollection`. In a skim they appear as

```
nHadronMatchCLUE3D, HadronMatchCLUE3D_genEnergy[nHadronMatchCLUE3D], HadronMatchCLUE3D_bestRawEnergy[...], ...
nShowerCLUE3D, ShowerCLUE3D_nLayers[nShowerCLUE3D], ...
```

(`harpInspect skim.root -b 'HadronMatch*'` lists them with their documentation).
