/*
  ФАЙЛ: TimepixDetector.cc

  Реализация TimepixDetector: обработка hits, расчёт индекса пикселя, суммирование
  энергии, порог (по умолчанию 6 keV, настраивается), кластеризация связных
  компонентов и запись в ROOT/CLOG.
*/

#include "TimepixDetector.hh"
#include "RootManager.hh"

#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4TouchableHandle.hh"
#include "G4AffineTransform.hh"
#include "G4EventManager.hh"
#include "G4Event.hh"

#include <unordered_map>
#include <cstdlib>
#include <cstring>
#include <unordered_set>
#include <vector>
#include <algorithm>

namespace {
  constexpr G4double kPitch = 0.055 * mm;        // 55 um
  constexpr G4double kHalf  = 14.08 * mm * 0.5;  // half width
  // 8-connected neighborhood
  const int dx8[8] = {-1,0,1,-1,1,-1,0,1};
  const int dy8[8] = {-1,-1,-1,0,0,1,1,1};

  inline int clamp255(int v) { return std::min(255, std::max(0, v)); }
  inline int key(int ix, int iy) { return ix + 256 * iy; }
}

TimepixDetector::TimepixDetector(const G4String& name)
  : G4VSensitiveDetector(name)
{
  // Читаем порог из окружения (keV). Это позволяет удобно управлять
  // детектором без перекомпиляции: --threshold-kev N выставляет env.
  if (const char* s = std::getenv("SPECT_THRESHOLD_KEV")) {
    char* end = nullptr;
    const double v = std::strtod(s, &end);
    if (end != s && v > 0.0) {
      fThresholdKeV = v;
    }
  }

  collectionName.insert("TimepixHitCollection");
}

void TimepixDetector::Initialize(G4HCofThisEvent* hce)
{
  fHits = new TimepixHitCollection(GetName(), collectionName[0]);
  if (fHCID < 0) fHCID = GetCollectionID(0);
  hce->AddHitsCollection(fHCID, fHits);
}

G4bool TimepixDetector::ProcessHits(G4Step* step, G4TouchableHistory*)
{
  const G4double edep = step->GetTotalEnergyDeposit();
  if (edep <= 0.0) return false;

  const auto* pre = step->GetPreStepPoint();
  const auto posGlobal = pre->GetPosition();

  // global -> local (sensor coords)
  // ВАЖНО: GetTopTransform() возвращает преобразование из глобальных
  // координат в локальные. Инвертировать его не нужно.
  const auto touch = pre->GetTouchableHandle();
  const auto posLocal = touch->GetHistory()->GetTopTransform().TransformPoint(posGlobal);

  auto* hit = new TimepixHit();
  hit->SetLocalX(posLocal.x());
  hit->SetLocalY(posLocal.y());
  hit->SetEdep(edep);
  hit->SetEkin(step->GetTrack()->GetKineticEnergy());

  fHits->insert(hit);
  return true;
}

void TimepixDetector::EndOfEvent(G4HCofThisEvent*)
{
  const auto* evt = G4EventManager::GetEventManager()->GetConstCurrentEvent();
  const int eventID = evt ? evt->GetEventID() : -1;

  if (!fHits) return;
  const G4int n = fHits->entries();
  if (n <= 0) return;

  // 1) accumulate edep per pixel within event
  std::unordered_map<int, double> pixE; // key -> E_keV
  pixE.reserve(static_cast<std::size_t>(n));

  for (G4int i = 0; i < n; ++i) {
    const auto* h = (*fHits)[i];
    const double edep_keV = h->GetEdep() / keV;
    if (edep_keV <= 0.0) continue;

    const G4double x = h->GetLocalX();
    const G4double y = h->GetLocalY();

    int ix = static_cast<int>((x + kHalf) / kPitch);
    int iy = static_cast<int>((y + kHalf) / kPitch);
    ix = clamp255(ix);
    iy = clamp255(iy);

    pixE[key(ix, iy)] += edep_keV;
  }

  // 2) apply threshold: keep only fired pixels
  const double kThr_keV = fThresholdKeV;
  std::unordered_map<int, double> fired;
  fired.reserve(pixE.size());
  double sumE_keV = 0.0;

  for (const auto& kv : pixE) {
    if (kv.second >= kThr_keV) {
      fired.emplace(kv.first, kv.second);
      sumE_keV += kv.second;
    }
  }

  if (fired.empty()) {
    // “нулевые” (без fired pixels) ты не хочешь
    return;
  }

  // 3) clustering on fired pixels (8-connected)
  std::unordered_set<int> unvisited;
  unvisited.reserve(fired.size());
  for (const auto& kv : fired) unvisited.insert(kv.first);

  std::vector<std::vector<std::pair<int,double>>> clusters; // each cluster: (key, E_keV)
  clusters.reserve(8);

  while (!unvisited.empty()) {
    const int start = *unvisited.begin();
    unvisited.erase(start);

    std::vector<int> stack;
    stack.push_back(start);

    std::vector<std::pair<int,double>> cluster;
    cluster.reserve(16);

    while (!stack.empty()) {
      const int cur = stack.back();
      stack.pop_back();

      auto itE = fired.find(cur);
      if (itE == fired.end()) continue;
      cluster.emplace_back(cur, itE->second);

      const int cx = cur % 256;
      const int cy = cur / 256;

      for (int k = 0; k < 8; ++k) {
        const int nx = cx + dx8[k];
        const int ny = cy + dy8[k];
        if (nx < 0 || nx > 255 || ny < 0 || ny > 255) continue;
        const int nk = key(nx, ny);

        auto it = unvisited.find(nk);
        if (it != unvisited.end()) {
          unvisited.erase(it);
          stack.push_back(nk);
        }
      }
    }

    if (!cluster.empty()) clusters.push_back(std::move(cluster));
  }

  // 4) write ROOT + update integrated images
  auto* rm = MyROOTManager::GetPointer();

  int nPixTotal = 0;
  for (int cid = 0; cid < static_cast<int>(clusters.size()); ++cid) {
    double clSum = 0.0;
    const auto& cl = clusters[cid];

    for (const auto& p : cl) {
      const int k = p.first;
      const double e = p.second;
      const int ix = k % 256;
      const int iy = k / 256;

      rm->FillPixel(eventID, cid, static_cast<std::uint16_t>(ix), static_cast<std::uint16_t>(iy), e);
      rm->FillImageE(ix, iy, e);
      rm->FillImageN(ix, iy, 1.0);

      clSum += e;
      ++nPixTotal;
    }

    rm->FillCluster(eventID, cid, static_cast<int>(cl.size()), clSum);
  }

  rm->FillEvent(eventID, static_cast<int>(clusters.size()), nPixTotal, sumE_keV);

  // 5) write CLOG frame with multiple cluster rows (PIXet format)
  rm->WriteCLOGFrame(eventID, clusters);
}
