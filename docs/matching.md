# Matching: generated hadron → sim trackster → reco tracksters → candidate

All truth matching in HARP happens in one producer, `HadronTruthMatcher`
(`Modules/Hadron/plugins/HadronTruthMatcher.cc`), which fills one
`HadronMatch` per generated hadron. The quality criteria follow the CMSSW
HGCal validation (`Validation/HGCalValidation`, `HGVHistoProducerAlgo` and
`TICLCandidateValidator`) so that HARP numbers can be compared with the
official validation plots.

## 1. Generated hadron → sim trackster (CaloParticle)

The nano has no gen→CaloParticle link, so the matcher picks the sim trackster
from CaloParticles (`ticlSimTrackstersfromCPs`) itself. Three methods exist
(`genToSim` parameter):

| Method | Criterion | Use |
|---|---|---|
| `energyEta` (default) | sim trackster whose `regressed_energy` (= the CaloParticle energy, i.e. the generated energy) agrees with E_gen within `maxEnergyDiff` (5 %) and whose η at the HGCal front face agrees with the gen η within `maxDeltaEta` (0.2); the smallest energy difference wins; pile-up sim tracksters are skipped | any sample |
| `deltaR` | closest sim trackster in ΔR between the gen direction and the sim trackster's HGCal front-face position, within `maxDeltaR` (0.3) | neutral particles only |
| `index` | sim trackster *i* ↔ gen particle *i* | single-particle guns |

`energyEta` is the default because the magnetic field bends charged hadrons
in φ before they reach HGCal: the ΔR between the gen direction and the
front-face position is 0.1 at 15 GeV, 0.4 at 4 GeV and above 1 below 3 GeV,
so a ΔR cut loses the low-energy charged hadrons that are precisely the
interesting ones. Energy and η are not affected by the bending.

The sim trackster is absent when the hadron never produced a CaloParticle
with HGCal hits (decayed or interacted before HGCal, left through the gap).
`matching/hasSim/vsE` shows that fraction.

## 2. Sim trackster → reco tracksters

Nothing is computed here: the matcher reads the hit-based association map
produced in CMSSW by `AllTracksterToSimTracksterAssociatorsByHitsProducer`
(`SimCalorimetry/HGCalAssociatorProducers`), e.g.
`SimCP2ticlTrackstersCLUE3DHighByHits` for CLUE3D tracksters. For a sim
trackster it lists every reco trackster sharing at least one rec-hit, each
with

* `sharedEnergy`: energy of the rec-hits in both objects (GeV);
* `score`: Σ_hits (f_sim − f_reco)² E_hit² / Σ_hits f_sim² E_hit², with f the
  fraction of the hit's energy assigned to the object and the sum running over
  the rec-hits of the sim trackster. The reco→sim map uses the mirrored
  formula, normalised to the reco trackster's hits (it penalises reco hits
  absent from the sim trackster).

### What the score is, and is not

The score is deterministic hit bookkeeping, not the output of a classifier
and not a probability. A hit of the sim trackster that the reco trackster
does not contain contributes its full f_sim² E², a hit present in both with
the same fraction contributes nothing; the normalisation makes the result
bounded between 0 (the reco trackster contains every hit of the sim trackster
with the right fractions) and 1 (nothing in common). For hits that are either
fully inside or fully outside the reco trackster it reduces to

    score(S→R) ≈ 1 − (E²-weighted fraction of S's hits captured by R)

so `score < 0.2` reads "one trackster captures about 80 % or more of the
hadron's energy-weighted hits". Two consequences for hadron studies:

* the E² weighting is dominated by the highest-energy hits: a trackster can
  hold half of the raw energy in many small layer clusters and still have a
  poor score;
* the purity criterion (one trackster with score < 0.2) is a statement about
  containment in a *single* object, designed for compact EM showers. A hadron
  shower split into several well-reconstructed tracksters fails it even
  though all of its energy is clustered. That is why the CMSSW efficiency and
  purity curves for charged hadrons stay low while `hasReco` reaches 1, and
  why the linking is better judged by comparing collections (CLUE3D, linked,
  candidate tracksters) at equal criteria and by the energy budget.

The only machine-learning quantities in the chain are the TICL candidate
pdgId and the regressed energy; the association maps and everything built on
them (best trackster, efficiency, purity, fake rate) are pure hit overlap.

From that list the matcher stores

* **best**: the link with the lowest score (`bestIndex`, `bestScore`,
  `bestSharedEnergy`, `bestRawEnergy`, `bestRegressedEnergy`);
* **linked**: all links with `score < scoreCut` (default 1.0, i.e. every
  trackster sharing hits) and `sharedEnergy ≥ minSharedEnergy` (default 0):
  `nLinked`, `linkedRawEnergy`, `linkedRegressedEnergy`, `linkedSharedEnergy`;
* **all**: sums over every trackster of the collection and over every layer
  cluster of the event, for comparison.

## 3. Best trackster → TICL candidate

When a candidate collection is given, the candidate whose `tracksterIndices`
contains the best trackster is attached (`candidateIndex`, `candidatePdgId`,
`candidateEnergy`, `candidateRawEnergy`, `candidateHasTrack`). This is only
meaningful when the matcher runs on the candidate trackster collection
(`ticlCandidate`), which is what `matchCand` does in `hist_hadron_cfg.py`;
`matchCLUE3D` runs the same code on CLUE3D tracksters.

## Quality definitions (`MatchingQAAnalyzer`)

Taken from `HGVHistoProducerAlgo` and its defaults in `HGCalValidator.cc`:

| Quantity | Definition | CMSSW parameter (default) | HARP parameter | Histogram |
|---|---|---|---|---|
| efficiency | the sim trackster has ≥ 1 reco trackster with `sharedEnergy / E_sim ≥ 0.5` | `minTSTSharedEneFracEfficiency` (0.5) | `effSharedFraction` | `p_efficient_vs_E` |
| purity | ≥ 1 reco trackster with sim→reco `score < 0.2` | `maxSimToRecoScoreForPurity` (0.2) | `purityScoreCut` | `p_pure_vs_E` |
| duplicate rate | ≥ 2 reco tracksters with sim→reco `score < 0.2` | `maxSimToRecoScoreForDuplicate` (0.2) | `purityScoreCut` | `p_duplicate_vs_E` |
| non-fake (per reco trackster) | ≥ 1 sim trackster with reco→sim `score < 0.6` | `maxRecoToSimScoreForNonFake` (0.6) | `fakeScoreCut` | `p_nonFake_vs_rawE`, `p_nonFake_vs_E`, `h_recoMatched` |
| candidate energy match | shared energy of the best candidate trackster / sim candidate raw energy > 0.5 | hard-coded in `TICLCandidateValidator` | (same as efficiency with `matchCand`) | `h2_bestSharedFrac_vs_E` |
| candidate PID | reco candidate pdgId = sim candidate pdgId | — | — | `h2_candPdg_vs_E` (PIDAnalyzer) |

Two HARP-specific quantities complete the picture: `p_hasReco_vs_E`, the
fraction of hadrons with any link at all (a lower bar than the CMSSW
efficiency), and `p_hasSim_vs_E`, the gen→sim matching efficiency of step 1.
For layer clusters CMSSW uses a score cut of 0.1 in both directions
(`ScoreCutLCtoCP_`, `ScoreCutCPtoLC_`); HARP does not evaluate layer-cluster
associations, the `EnergyBudgetAnalyzer` uses the trackster vertex lists instead.

## Reading the numbers

The CMSSW efficiency (shared fraction ≥ 0.5 in *one* trackster) is a stricter
statement than "the hadron has a trackster": a hadron whose energy is split
into three equal tracksters has `hasReco = 1` but `efficient = 0`. For
hadrons this is the normal situation at low energy, so the two curves differ
a lot; for π0 they coincide above 10 GeV. The response definitions
(`bestRaw`, `linkedRaw`, `allRaw`) are the energy-side view of the same
fragmentation.
