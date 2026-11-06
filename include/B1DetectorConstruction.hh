/*
  ФАЙЛ: B1DetectorConstruction.hh

  Заголовок построения геометрии. Указатель на логический объём металла
  не нужен, так как SD на металл не назначается.
*/

#ifndef B1DetectorConstruction_h
#define B1DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"

class G4LogicalVolume;
class G4VPhysicalVolume;

class B1DetectorConstruction : public G4VUserDetectorConstruction
{
public:
  B1DetectorConstruction();
  ~B1DetectorConstruction() override;

  G4VPhysicalVolume* Construct() override;
  void ConstructSDandField() override;

private:
  G4LogicalVolume* fLogicTimepix = nullptr;
};

#endif
