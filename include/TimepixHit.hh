/*
  ФАЙЛ: TimepixHit.hh

  Класс hit для Geant4: хранит энерговыделение и координаты взаимодействия в сенсоре (для
последующей пикселизации).
*/

#ifndef TimepixHit_h
#define TimepixHit_h 1

#include "G4VHit.hh"
#include "globals.hh"

// Храним локальные координаты в системе детектора + edep/ekin
class TimepixHit : public G4VHit
{
public:
  TimepixHit() = default;
  ~TimepixHit() override = default;

  void Print() override {}
  void Draw() override {}

  void SetLocalX(G4double v) { local_x = v; }
  void SetLocalY(G4double v) { local_y = v; }
  void SetEdep(G4double v)   { edep = v; }
  void SetEkin(G4double v)   { ekin = v; }

  G4double GetLocalX() const { return local_x; }
  G4double GetLocalY() const { return local_y; }
  G4double GetEdep()   const { return edep; }
  G4double GetEkin()   const { return ekin; }

private:
  G4double local_x{0.0};
  G4double local_y{0.0};
  G4double edep{0.0};
  G4double ekin{0.0};
};

#endif
