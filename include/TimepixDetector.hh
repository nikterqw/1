/*
  ФАЙЛ: TimepixDetector.hh

  Объявление чувствительного детектора (G4VSensitiveDetector): принимает hits в сенсоре,
агрегирует по пикселям, применяет порог и кластеризацию, передаёт в MyROOTManager.
*/

#ifndef TIMEPIX_DETECTOR_HH
#define TIMEPIX_DETECTOR_HH

#include "G4VSensitiveDetector.hh"
#include "G4THitsCollection.hh"
#include "globals.hh"

#include "TimepixHit.hh"

class G4HCofThisEvent;
class G4Step;
class G4TouchableHistory;

using TimepixHitCollection = G4THitsCollection<TimepixHit>;

// -----------------------------------------------------------------------------
// TimepixDetector
//
// Реализация "как Timepix" (frame-based для Geant4 события):
//  - Накапливаем edep по пикселям 256x256 (pitch 55um) В РАМКАХ ОДНОГО События.
//  - Применяем порог 6 keV на пиксель: ниже порога пиксель не считается сработавшим.
//  - В ROOT пишем:
//      PixelTree: (EventID, X, Y, E_keV) только для fired pixels
//      EventTree: (EventID, nPix, SumE_keV) только для зарегистрированных событий (nPix>0)
//      imageE/imageN: интегральные карты по всем зарегистрированным событиям
//  - В *.clog пишем по одному "Frame" на зарегистрированное событие.
// -----------------------------------------------------------------------------
class TimepixDetector : public G4VSensitiveDetector
{
public:
  explicit TimepixDetector(const G4String& name);
  ~TimepixDetector() override = default;

  void Initialize(G4HCofThisEvent* hce) override;
  G4bool ProcessHits(G4Step* step, G4TouchableHistory* hist) override;
  void EndOfEvent(G4HCofThisEvent* hce) override;

private:
  // Порог срабатывания пикселя (keV). По умолчанию 6 keV,
  // может быть переопределён через env SPECT_THRESHOLD_KEV / CLI --threshold-kev.
  G4double fThresholdKeV = 6.0;

  TimepixHitCollection* fHits = nullptr;
  G4int fHCID = -1;
};

#endif
