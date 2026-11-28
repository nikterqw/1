#ifndef IDEAL_DETECTOR_HH
#define IDEAL_DETECTOR_HH

#include "G4VSensitiveDetector.hh"
#include "G4THitsCollection.hh"
#include "globals.hh"

class G4Step;
class G4HCofThisEvent;

// Класс "Хита" для идеального детектора (хранит только энергию)
class IdealHit : public G4VHit {
public:
    IdealHit() : fEdep(0) {}
    virtual ~IdealHit() {}
    void SetEdep(G4double e) { fEdep = e; }
    G4double GetEdep() const { return fEdep; }
private:
    G4double fEdep;
};

using IdealHitsCollection = G4THitsCollection<IdealHit>;

class IdealDetector : public G4VSensitiveDetector {
public:
    IdealDetector(const G4String& name);
    virtual ~IdealDetector();
    
    virtual void Initialize(G4HCofThisEvent* hce) override;
    virtual G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
    virtual void EndOfEvent(G4HCofThisEvent* hce) override;

private:
    IdealHitsCollection* fHits;
    G4int fHCID;
};

#endif
