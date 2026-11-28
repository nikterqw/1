#include "IdealDetector.hh"
#include "RootManager.hh"

#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4EventManager.hh"
#include "G4Event.hh"
#include "G4Track.hh"

IdealDetector::IdealDetector(const G4String& name)
    : G4VSensitiveDetector(name), fHits(nullptr), fHCID(-1) {
    collectionName.insert("IdealHitCollection");
}

IdealDetector::~IdealDetector() {}

void IdealDetector::Initialize(G4HCofThisEvent* hce) {
    fHits = new IdealHitsCollection(GetName(), collectionName[0]);
    if (fHCID < 0) fHCID = GetCollectionID(0);
    hce->AddHitsCollection(fHCID, fHits);
}

G4bool IdealDetector::ProcessHits(G4Step* step, G4TouchableHistory*) {
    // Берем только те шаги, где частица входит в объем
    if (step->GetTrack()->GetTrackID() > 1) return false; // Игнорируем вторичные частицы
    if (step->GetTrack()->GetDefinition()->GetParticleName() != "gamma") return false;

    // Берем кинетическую энергию гамма-кванта на входе
    G4double ekin = step->GetPreStepPoint()->GetKineticEnergy();
    if (ekin <= 0) return false;

    // Сохраняем хит
    IdealHit* hit = new IdealHit();
    hit->SetEdep(ekin); // Используем поле Edep для хранения энергии гамма-кванта
    fHits->insert(hit);
    
    // Убиваем трек, чтобы он не взаимодействовал дальше в детекторе
    step->GetTrack()->SetTrackStatus(fStopAndKill); 
    
    return true;
}

void IdealDetector::EndOfEvent(G4HCofThisEvent*) {
    if (!fHits || fHits->entries() == 0) return;
    
    const auto* evt = G4EventManager::GetEventManager()->GetConstCurrentEvent();
    const int eventID = evt ? evt->GetEventID() : -1;
    auto* rm = MyROOTManager::GetPointer();

    // В одном событии может прилететь несколько гамма-квантов (например, Compton).
    // Мы записываем их как отдельные кластеры (каждый фотон = 1 кластер = 1 пиксель).
    G4int clusterID = 0;
    for (G4int i = 0; i < fHits->entries(); ++i) {
        IdealHit* hit = (*fHits)[i];
        G4double energy = hit->GetEdep() / keV;
        
        // Записываем в ROOT (используем центральный пиксель 128, 128, так как нам важна только энергия)
        rm->FillPixel(eventID, clusterID, 128, 128, energy);
        rm->FillCluster(eventID, clusterID, 1, energy);
        rm->FillImageE(128, 128, energy);
        rm->FillImageN(128, 128, 1.0);
        
        clusterID++;
    }
    
    rm->FillEvent(eventID, clusterID, clusterID, 0.0);
}
