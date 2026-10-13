// HGCalNanoReader: builds the event data model (GenParticle, LayerCluster,
// Trackster, TICLCandidate, Track, AssociationMap collections) from the flat
// branches of an HGCal nano. Which collections are read, and their branch
// prefixes, is fully configurable so the same module serves different
// "specialised" nanos.
//
// Configuration (python):
//   process.hgcalReader = Module("HGCalNanoReader",
//       genParticles   = "HGCalGenPart",               # -> product "hgcalReader:genParticles"
//       layerClusters  = "HGCalLayerClusters",         # -> "hgcalReader:layerClusters"
//       tracksters     = {"clue3d": "ticlTrackstersCLUE3DHigh", "simCP": "ticlSimTrackstersfromCPs"},
//       associations   = {"simCP2clue3d": "SimCP2ticlTrackstersCLUE3DHighByHits"},
//       ticlCandidates = {"ticlCandidates": "TICLCandidates"},   # optional, needs linkedTracksters
//       linkedTracksters = "linkedTracksters",
//       tracks         = "GeneralTrack")
#include "Core/interface/Logger.h"
#include "Core/interface/ModuleFactory.h"
#include "DataFormats/interface/Association.h"
#include "DataFormats/interface/GenParticle.h"
#include "DataFormats/interface/HGCalLayers.h"
#include "DataFormats/interface/LayerCluster.h"
#include "DataFormats/interface/TICLCandidate.h"
#include "DataFormats/interface/Track.h"
#include "DataFormats/interface/Trackster.h"

#include <map>
#include <memory>
#include <string>

namespace harp {

namespace {

// ---- helpers reading one nano "table" each -------------------------------

struct GenReader {
  std::string label;
  ArrayHandle<int> pdgId, status;
  ArrayHandle<float> energy, pt, eta, phi, mass, vx, vy, vz;
  GenReader(ModuleContext& ctx, const std::string& p, const std::string& instance) : label(ctx.produces<GenParticleCollection>(instance)) {
    pdgId = ctx.consumesArray<int>(p + "_pdgId");
    status = ctx.consumesArray<int>(p + "_status");
    energy = ctx.consumesArray<float>(p + "_energy");
    pt = ctx.consumesArray<float>(p + "_pt");
    eta = ctx.consumesArray<float>(p + "_eta");
    phi = ctx.consumesArray<float>(p + "_phi");
    mass = ctx.consumesArray<float>(p + "_mass");
    vx = ctx.consumesArray<float>(p + "_vx", true);
    vy = ctx.consumesArray<float>(p + "_vy", true);
    vz = ctx.consumesArray<float>(p + "_vz", true);
  }
  void fill(Event& ev) const {
    GenParticleCollection out(pdgId.size());
    for (std::size_t i = 0; i < out.size(); ++i) {
      GenParticle& g = out[i];
      g.pdgId = pdgId[i]; g.status = status[i];
      g.energy = energy[i]; g.pt = pt[i]; g.eta = eta[i]; g.phi = phi[i]; g.mass = mass[i];
      if (vx.valid()) { g.vx = vx[i]; g.vy = vy[i]; g.vz = vz[i]; }
    }
    ev.put(label, std::move(out));
  }
};

struct LayerClusterReader {
  std::string label;
  ArrayHandle<int> algoId, nHits;
  ArrayHandle<float> energy, correctedEnergy, x, y, z, eta, phi, time, timeError;
  LayerClusterReader(ModuleContext& ctx, const std::string& p, const std::string& instance) : label(ctx.produces<LayerClusterCollection>(instance)) {
    algoId = ctx.consumesArray<int>(p + "_algoID", true);
    nHits = ctx.consumesArray<int>(p + "_nHits", true);
    energy = ctx.consumesArray<float>(p + "_energy");
    correctedEnergy = ctx.consumesArray<float>(p + "_correctedEnergy", true);
    x = ctx.consumesArray<float>(p + "_position_x");
    y = ctx.consumesArray<float>(p + "_position_y");
    z = ctx.consumesArray<float>(p + "_position_z");
    eta = ctx.consumesArray<float>(p + "_position_eta");
    phi = ctx.consumesArray<float>(p + "_position_phi");
    time = ctx.consumesArray<float>(p + "_time", true);
    timeError = ctx.consumesArray<float>(p + "_timeError", true);
  }
  void fill(Event& ev) const {
    LayerClusterCollection out(energy.size());
    for (std::size_t i = 0; i < out.size(); ++i) {
      LayerCluster& c = out[i];
      c.energy = energy[i];
      c.correctedEnergy = correctedEnergy.valid() ? correctedEnergy[i] : energy[i];
      c.x = x[i]; c.y = y[i]; c.z = z[i]; c.eta = eta[i]; c.phi = phi[i];
      if (time.valid()) { c.time = time[i]; c.timeError = timeError[i]; }
      if (algoId.valid()) c.algoId = algoId[i];
      if (nHits.valid()) c.nHits = nHits[i];
      c.layer = HGCalLayers::layerFromZ(c.z);
    }
    ev.put(label, std::move(out));
  }
};

struct TracksterReader {
  std::string label;
  ArrayHandle<unsigned short> nVert, oVert;
  ArrayHandle<unsigned int> vertices;
  ArrayHandle<float> vertexMult;
  ArrayHandle<float> rawE, rawEmE, rawPt, regE, bx, by, bz, beta, bphi, ev1, ev2, ev3, e0x, e0y, e0z, time, timeErr;
  // sim-only
  ArrayHandle<int> cpIdx, isPU;
  ArrayHandle<float> bX, bY, bZ, bEta, bPhi, bPx, bPy, bPz, simTime, genPt, mass;
  TracksterReader(ModuleContext& ctx, const std::string& p, const std::string& instance) : label(ctx.produces<TracksterCollection>(instance)) {
    nVert = ctx.consumesArray<unsigned short>(p + "_n" + p + "vertices");
    oVert = ctx.consumesArray<unsigned short>(p + "_o" + p + "vertices", true);
    vertices = ctx.consumesArray<unsigned int>(p + "vertices_vertices");
    vertexMult = ctx.consumesArray<float>(p + "vertices_vertex_mult", true);
    rawE = ctx.consumesArray<float>(p + "_raw_energy");
    rawEmE = ctx.consumesArray<float>(p + "_raw_em_energy");
    rawPt = ctx.consumesArray<float>(p + "_raw_pt");
    regE = ctx.consumesArray<float>(p + "_regressed_energy");
    bx = ctx.consumesArray<float>(p + "_barycenter_x");
    by = ctx.consumesArray<float>(p + "_barycenter_y");
    bz = ctx.consumesArray<float>(p + "_barycenter_z");
    beta = ctx.consumesArray<float>(p + "_barycenter_eta");
    bphi = ctx.consumesArray<float>(p + "_barycenter_phi");
    ev1 = ctx.consumesArray<float>(p + "_EV1");
    ev2 = ctx.consumesArray<float>(p + "_EV2");
    ev3 = ctx.consumesArray<float>(p + "_EV3");
    e0x = ctx.consumesArray<float>(p + "_eVector0_x");
    e0y = ctx.consumesArray<float>(p + "_eVector0_y");
    e0z = ctx.consumesArray<float>(p + "_eVector0_z");
    time = ctx.consumesArray<float>(p + "_time");
    timeErr = ctx.consumesArray<float>(p + "_timeError");
    cpIdx = ctx.consumesArray<int>(p + "_caloParticleIdx", true);
    isPU = ctx.consumesArray<int>(p + "_isPU", true);
    bX = ctx.consumesArray<float>(p + "_boundaryX", true);
    bY = ctx.consumesArray<float>(p + "_boundaryY", true);
    bZ = ctx.consumesArray<float>(p + "_boundaryZ", true);
    bEta = ctx.consumesArray<float>(p + "_boundaryEta", true);
    bPhi = ctx.consumesArray<float>(p + "_boundaryPhi", true);
    bPx = ctx.consumesArray<float>(p + "_boundaryPx", true);
    bPy = ctx.consumesArray<float>(p + "_boundaryPy", true);
    bPz = ctx.consumesArray<float>(p + "_boundaryPz", true);
    simTime = ctx.consumesArray<float>(p + "_simTime", true);
    genPt = ctx.consumesArray<float>(p + "_genPt", true);
    mass = ctx.consumesArray<float>(p + "_mass", true);
  }
  void fill(Event& ev) const {
    TracksterCollection out(rawE.size());
    std::size_t offset = 0;
    for (std::size_t i = 0; i < out.size(); ++i) {
      Trackster& t = out[i];
      t.rawEnergy = rawE[i]; t.rawEmEnergy = rawEmE[i]; t.rawPt = rawPt[i]; t.regressedEnergy = regE[i];
      t.barycenterX = bx[i]; t.barycenterY = by[i]; t.barycenterZ = bz[i]; t.barycenterEta = beta[i]; t.barycenterPhi = bphi[i];
      t.ev1 = ev1[i]; t.ev2 = ev2[i]; t.ev3 = ev3[i];
      t.eVector0X = e0x[i]; t.eVector0Y = e0y[i]; t.eVector0Z = e0z[i];
      t.time = time[i]; t.timeError = timeErr[i];
      const std::size_t n = nVert[i];
      const std::size_t start = oVert.valid() ? static_cast<std::size_t>(oVert[i]) : offset;
      t.vertices.reserve(n);
      if (vertexMult.valid()) t.vertexMultiplicity.reserve(n);
      for (std::size_t k = 0; k < n && start + k < vertices.size(); ++k) {
        t.vertices.push_back(vertices[start + k]);
        if (vertexMult.valid()) t.vertexMultiplicity.push_back(vertexMult[start + k]);
      }
      offset += n;
      if (cpIdx.valid()) {
        t.hasSimInfo = true;
        t.caloParticleIdx = cpIdx[i];
        if (isPU.valid()) t.isPU = isPU[i];
        if (bX.valid()) { t.boundaryX = bX[i]; t.boundaryY = bY[i]; t.boundaryZ = bZ[i]; t.boundaryEta = bEta[i]; t.boundaryPhi = bPhi[i]; t.boundaryPx = bPx[i]; t.boundaryPy = bPy[i]; t.boundaryPz = bPz[i]; }
        if (simTime.valid()) t.simTime = simTime[i];
        if (genPt.valid()) t.genPt = genPt[i];
        if (mass.valid()) t.mass = mass[i];
      }
    }
    ev.put(label, std::move(out));
  }
};

struct AssociationReader {
  std::string label;
  ArrayHandle<unsigned short> nLinks;
  ArrayHandle<unsigned int> index;
  ArrayHandle<float> score, sharedEnergy;
  AssociationReader(ModuleContext& ctx, const std::string& p, const std::string& instance) : label(ctx.produces<AssociationMap>(instance)) {
    nLinks = ctx.consumesArray<unsigned short>(p + "_n" + p + "Links");
    index = ctx.consumesArray<unsigned int>(p + "Links_index");
    score = ctx.consumesArray<float>(p + "Links_score");
    sharedEnergy = ctx.consumesArray<float>(p + "Links_sharedEnergy");
  }
  void fill(Event& ev) const {
    AssociationMap out;
    out.links.resize(nLinks.size());
    std::size_t k = 0;
    for (std::size_t i = 0; i < nLinks.size(); ++i) {
      const std::size_t n = nLinks[i];
      out.links[i].reserve(n);
      for (std::size_t j = 0; j < n && k < index.size(); ++j, ++k) out.links[i].push_back(AssociationLink{index[k], score[k], sharedEnergy[k]});
    }
    ev.put(label, std::move(out));
  }
};

struct CandidateReader {
  std::string label;
  ArrayHandle<int> pdgId, charge, trackIdx;
  ArrayHandle<float> energy, rawEnergy, pt, eta, phi, p, mass, px, py, pz, time, timeErr, mtdTime, mtdTimeErr;
  ArrayHandle<unsigned short> nLinked;
  ArrayHandle<unsigned int> linked;
  CandidateReader(ModuleContext& ctx, const std::string& p_, const std::string& linkedBranch, const std::string& instance) : label(ctx.produces<TICLCandidateCollection>(instance)) {
    pdgId = ctx.consumesArray<int>(p_ + "_pdgID");
    charge = ctx.consumesArray<int>(p_ + "_charge");
    trackIdx = ctx.consumesArray<int>(p_ + "_trackIdx");
    energy = ctx.consumesArray<float>(p_ + "_energy");
    rawEnergy = ctx.consumesArray<float>(p_ + "_raw_energy");
    pt = ctx.consumesArray<float>(p_ + "_pt");
    eta = ctx.consumesArray<float>(p_ + "_eta");
    phi = ctx.consumesArray<float>(p_ + "_phi");
    p = ctx.consumesArray<float>(p_ + "_p");
    mass = ctx.consumesArray<float>(p_ + "_mass");
    px = ctx.consumesArray<float>(p_ + "_px");
    py = ctx.consumesArray<float>(p_ + "_py");
    pz = ctx.consumesArray<float>(p_ + "_pz");
    time = ctx.consumesArray<float>(p_ + "_time");
    timeErr = ctx.consumesArray<float>(p_ + "_timeError");
    mtdTime = ctx.consumesArray<float>(p_ + "_MTDtime", true);
    mtdTimeErr = ctx.consumesArray<float>(p_ + "_MTDtimeError", true);
    nLinked = ctx.consumesArray<unsigned short>(p_ + "_nLinkedTracksters", true);
    if (!linkedBranch.empty()) linked = ctx.consumesArray<unsigned int>(linkedBranch + "_tracksterIndex", true);
  }
  void fill(Event& ev) const {
    TICLCandidateCollection out(pdgId.size());
    std::size_t k = 0;
    for (std::size_t i = 0; i < out.size(); ++i) {
      TICLCandidate& c = out[i];
      c.pdgId = pdgId[i]; c.charge = charge[i];
      c.trackIdx = trackIdx[i] < 0 ? -1 : trackIdx[i];  // nano stores INT_MIN for "no track"
      c.energy = energy[i]; c.rawEnergy = rawEnergy[i]; c.pt = pt[i]; c.eta = eta[i]; c.phi = phi[i]; c.p = p[i]; c.mass = mass[i];
      c.px = px[i]; c.py = py[i]; c.pz = pz[i]; c.time = time[i]; c.timeError = timeErr[i];
      if (mtdTime.valid()) { c.mtdTime = mtdTime[i]; c.mtdTimeError = mtdTimeErr[i]; }
      if (nLinked.valid() && linked.valid()) {
        const std::size_t n = nLinked[i];
        for (std::size_t j = 0; j < n && k < linked.size(); ++j, ++k) c.tracksterIndices.push_back(linked[k]);
      }
    }
    ev.put(label, std::move(out));
  }
};

struct TrackReader {
  std::string label;
  ArrayHandle<int> charge;
  ArrayHandle<unsigned short> nhits;
  ArrayHandle<unsigned char> missingOuter;
  ArrayHandle<float> pt, eta, phi, p, ptErr, vx, vy, vz, hx, hy, hz, heta, hphi;
  TrackReader(ModuleContext& ctx, const std::string& p_, const std::string& instance) : label(ctx.produces<TrackCollection>(instance)) {
    charge = ctx.consumesArray<int>(p_ + "_charge");
    nhits = ctx.consumesArray<unsigned short>(p_ + "_nhits", true);
    missingOuter = ctx.consumesArray<unsigned char>(p_ + "_missingOuterHits", true);
    pt = ctx.consumesArray<float>(p_ + "_pt");
    eta = ctx.consumesArray<float>(p_ + "_eta");
    phi = ctx.consumesArray<float>(p_ + "_phi");
    p = ctx.consumesArray<float>(p_ + "_p");
    ptErr = ctx.consumesArray<float>(p_ + "_ptErr", true);
    vx = ctx.consumesArray<float>(p_ + "_vx", true);
    vy = ctx.consumesArray<float>(p_ + "_vy", true);
    vz = ctx.consumesArray<float>(p_ + "_vz", true);
    hx = ctx.consumesArray<float>(p_ + "_hgcal_x", true);
    hy = ctx.consumesArray<float>(p_ + "_hgcal_y", true);
    hz = ctx.consumesArray<float>(p_ + "_hgcal_z", true);
    heta = ctx.consumesArray<float>(p_ + "_hgcal_eta", true);
    hphi = ctx.consumesArray<float>(p_ + "_hgcal_phi", true);
  }
  void fill(Event& ev) const {
    TrackCollection out(pt.size());
    for (std::size_t i = 0; i < out.size(); ++i) {
      Track& t = out[i];
      t.charge = charge[i]; t.pt = pt[i]; t.eta = eta[i]; t.phi = phi[i]; t.p = p[i];
      if (nhits.valid()) t.nHits = nhits[i];
      if (missingOuter.valid()) t.missingOuterHits = missingOuter[i];
      if (ptErr.valid()) t.ptErr = ptErr[i];
      if (vx.valid()) { t.vx = vx[i]; t.vy = vy[i]; t.vz = vz[i]; }
      if (hx.valid()) { t.hgcalX = hx[i]; t.hgcalY = hy[i]; t.hgcalZ = hz[i]; t.hgcalEta = heta[i]; t.hgcalPhi = hphi[i]; }
    }
    ev.put(label, std::move(out));
  }
};

}  // namespace

class HGCalNanoReader : public Producer {
 public:
  HGCalNanoReader(const ParameterSet& ps, ModuleContext& ctx) : Producer(ctx) {
    const std::string gen = ps.get<std::string>("genParticles", "");
    if (!gen.empty()) gen_ = std::make_unique<GenReader>(ctx, gen, "genParticles");
    const std::string lc = ps.get<std::string>("layerClusters", "");
    if (!lc.empty()) lc_ = std::make_unique<LayerClusterReader>(ctx, lc, "layerClusters");
    for (const auto& kv : ps.get<std::map<std::string, std::string>>("tracksters", {}))
      tracksters_.push_back(std::make_unique<TracksterReader>(ctx, kv.second, kv.first));
    for (const auto& kv : ps.get<std::map<std::string, std::string>>("associations", {}))
      assocs_.push_back(std::make_unique<AssociationReader>(ctx, kv.second, kv.first));
    const std::string linked = ps.get<std::string>("linkedTracksters", "linkedTracksters");
    for (const auto& kv : ps.get<std::map<std::string, std::string>>("ticlCandidates", {}))
      candidates_.push_back(std::make_unique<CandidateReader>(ctx, kv.second, linked, kv.first));
    const std::string trk = ps.get<std::string>("tracks", "");
    if (!trk.empty()) tracks_ = std::make_unique<TrackReader>(ctx, trk, "tracks");
    HARP_INFO(label()) << "reading " << (gen_ ? 1 : 0) + (lc_ ? 1 : 0) + (tracks_ ? 1 : 0) + tracksters_.size() + assocs_.size() + candidates_.size() << " collections";
  }

  void produce(Event& ev) override {
    if (gen_) gen_->fill(ev);
    if (lc_) lc_->fill(ev);
    for (auto& r : tracksters_) r->fill(ev);
    for (auto& r : assocs_) r->fill(ev);
    for (auto& r : candidates_) r->fill(ev);
    if (tracks_) tracks_->fill(ev);
  }

 private:
  std::unique_ptr<GenReader> gen_;
  std::unique_ptr<LayerClusterReader> lc_;
  std::vector<std::unique_ptr<TracksterReader>> tracksters_;
  std::vector<std::unique_ptr<AssociationReader>> assocs_;
  std::vector<std::unique_ptr<CandidateReader>> candidates_;
  std::unique_ptr<TrackReader> tracks_;
};

}  // namespace harp

HARP_REGISTER_MODULE(harp::HGCalNanoReader);
