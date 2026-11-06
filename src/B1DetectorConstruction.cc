/*
  ФАЙЛ: B1DetectorConstruction.cc
  Изменения: сфера из вольфрама (W), визуализация, удален лишний SD.
*/

#include "B1DetectorConstruction.hh"
#include "RootManager.hh"

#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4Tubs.hh"
#include "G4Box.hh"
#include "G4Sphere.hh"
#include "G4LogicalVolume.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include "TimepixDetector.hh"

#include <cstdio>
#include <string>
#include <vector>

namespace {
  std::FILE* TryOpenMaskFile(const char* fname)
  {
    return std::fopen(fname, "r");
  }
}

B1DetectorConstruction::B1DetectorConstruction()
  : G4VUserDetectorConstruction()
{
}

B1DetectorConstruction::~B1DetectorConstruction() = default;

G4VPhysicalVolume* B1DetectorConstruction::Construct()
{
  auto* nist = G4NistManager::Instance();
  const G4bool checkOverlaps = true;

  // -----------------------------
  // World
  // -----------------------------
  const G4double world_sizeXY = 10.0 * cm;
  const G4double world_sizeZ  = 50.0 * cm;

  auto* world_mat  = nist->FindOrBuildMaterial("G4_AIR");
  auto* solidWorld = new G4Box("World", 0.5 * world_sizeXY, 0.5 * world_sizeXY, 0.5 * world_sizeZ);
  auto* logicWorld = new G4LogicalVolume(solidWorld, world_mat, "World");
  auto* physWorld  = new G4PVPlacement(nullptr, {}, logicWorld, "World", nullptr, false, 0, checkOverlaps);

  // -----------------------------
  // Mask pattern from file
  // -----------------------------
  constexpr int kMaskSize = 61;
  constexpr int kInversion = 0;
  MyROOTManager::SetInversion(kInversion);

  std::FILE* mask_conf = TryOpenMaskFile("mask-mura-61.txt");
  if (!mask_conf) mask_conf = TryOpenMaskFile("../mask-mura-61.txt");
  if (!mask_conf) {
    G4Exception("B1DetectorConstruction::Construct", "MASK_FILE_NOT_FOUND", FatalException,
                "Cannot open mask-mura-61.txt (tried ./ and ../).");
  }

  std::vector<int> mask_pattern(static_cast<size_t>(kMaskSize * kMaskSize), 0);
  int tmp = 0;
  for (int i = 0; i < kMaskSize; ++i) {
    for (int j = 0; j < kMaskSize; ++j) {
      if (std::fscanf(mask_conf, "%d", &tmp) != 1) {
        std::fclose(mask_conf);
        G4Exception("B1DetectorConstruction::Construct", "MASK_FILE_READ_ERROR", FatalException,
                    "Failed reading mask-mura-61.txt");
      }
      mask_pattern[static_cast<size_t>(i * kMaskSize + j)] = tmp;
    }
  }
  std::fclose(mask_conf);

  // -----------------------------
  // Disc (Mask plate)
  // -----------------------------
  auto* mask_mat = nist->FindOrBuildMaterial("G4_W");
  const G4ThreeVector mask_pos(0, 0, 0);

  const G4double mask_outer_radius = 2.203869 * cm;
  const G4double mask_inner_radius = 0.0 * cm;
  const G4double mask_thickness    = 2.0 * mm; // Оставил 2 мм для лучшего поглощения 60 кэВ

  auto* solidMask = new G4Tubs("sMask",
                              mask_inner_radius,
                              mask_outer_radius,
                              mask_thickness * 0.5,
                              0.0 * deg,
                              360.0 * deg);

  auto* logicMask = new G4LogicalVolume(solidMask, mask_mat, "lMask");
  G4VisAttributes* maskVis = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5, 0.5)); // серый, полупрозрачный
  maskVis->SetVisibility(true);
  logicMask->SetVisAttributes(maskVis);

  (void)new G4PVPlacement(nullptr, mask_pos, logicMask, "pMask", logicWorld, false, 0, checkOverlaps);

  // -----------------------------
  // Hole (air cylinder) + placement
  // -----------------------------
  auto* hole_mat = nist->FindOrBuildMaterial("G4_AIR");
  const G4double hole_outer_radius = 0.015 * cm;
  const G4double hole_inner_radius = 0.0  * cm;
  const G4double hole_thickness    = mask_thickness;
  const G4double hole_step         = 0.036129 * cm;

  auto* solidHole = new G4Tubs("sHole", hole_inner_radius, hole_outer_radius, hole_thickness * 0.5, 0.0 * deg, 360.0 * deg);
  auto* logicHole = new G4LogicalVolume(solidHole, hole_mat, "lHole");
  G4VisAttributes* holeVis = new G4VisAttributes(G4Colour(1.0, 1.0, 1.0, 0.0)); // невидимый
  logicHole->SetVisAttributes(holeVis);

  constexpr G4double kMaskRotationZ = 0.0;
  for (int i = 0; i < kMaskSize; ++i) {
    for (int j = 0; j < kMaskSize; ++j) {
      const G4double hole_pos_x = (j - (kMaskSize - 1) / 2.0) * hole_step;
      const G4double hole_pos_y = ((kMaskSize - 1) / 2.0 - i) * hole_step;
      G4ThreeVector hole_pos(hole_pos_x, hole_pos_y, 0.0);
      hole_pos.rotateZ(kMaskRotationZ);
      if (mask_pattern[static_cast<size_t>(i * kMaskSize + j)] == kInversion) {
        (void)new G4PVPlacement(nullptr, hole_pos, logicHole, "pHole", logicMask, false, i * kMaskSize + j, checkOverlaps);
      }
    }
  }

  // -----------------------------
  // Detector plate (Timepix)
  // -----------------------------
  auto* timepix_mat = nist->FindOrBuildMaterial("G4_CADMIUM_TELLURIDE");
  const G4ThreeVector timepix_pos(0, 0, 4.4216 * cm);
  const G4double timepix_width     = 14.08 * mm;
  const G4double timepix_thickness = 2.0   * mm;

  auto* solidTimepix = new G4Box("sTimepix", timepix_width * 0.5, timepix_width * 0.5, timepix_thickness * 0.5);
  auto* logicTimepix = new G4LogicalVolume(solidTimepix, timepix_mat, "lTimepix");
  
  G4VisAttributes* timepixVis = new G4VisAttributes(G4Colour(0.0, 0.0, 1.0, 0.5)); // синий, полупрозрачный
  timepixVis->SetVisibility(true);
  logicTimepix->SetVisAttributes(timepixVis);

  (void)new G4PVPlacement(nullptr, timepix_pos, logicTimepix, "pTimepix", logicWorld, false, 0, checkOverlaps);
  fLogicTimepix = logicTimepix;

  // -----------------------------
  // Metal Sphere (Tungsten)
  // -----------------------------
  auto* metal_mat = nist->FindOrBuildMaterial("G4_W"); // Вольфрам
  const G4ThreeVector metal_pos(0, 0, -5.0 * cm); // 5 см перед маской
  const G4double metal_radius = 2.0 * mm;

  auto* solidMetal = new G4Sphere("sMetal", 0.0, metal_radius, 0.0 * deg, 360.0 * deg, 0.0 * deg, 180.0 * deg);
  auto* logicMetal = new G4LogicalVolume(solidMetal, metal_mat, "lMetal");
  
  G4VisAttributes* metalVis = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0)); // красный
  metalVis->SetVisibility(true);
  metalVis->SetForceSolid(true);
  logicMetal->SetVisAttributes(metalVis);

  (void)new G4PVPlacement(nullptr, metal_pos, logicMetal, "pMetal", logicWorld, false, 0, checkOverlaps);

  return physWorld;
}

void B1DetectorConstruction::ConstructSDandField()
{
  auto* sdm = G4SDManager::GetSDMpointer();

  // Только детектор Timepix! Лишний SD для металла убран.
  auto* timepixSD = new TimepixDetector("Timepix1");
  sdm->AddNewDetector(timepixSD);
  if (fLogicTimepix) {
    fLogicTimepix->SetSensitiveDetector(timepixSD);
  } else {
    G4Exception("B1DetectorConstruction::ConstructSDandField",
                "TIMEPIX_LV_NULL",
                JustWarning,
                "fLogicTimepix is null. Sensitive detector not set.");
  }
}
