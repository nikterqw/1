/*
  ФАЙЛ: B1PrimaryGeneratorAction.cc
  Изменения: энергия пучка 80 кэВ для возбуждения K-линий вольфрама (Kα ~59.3 кэВ).
*/

#include "B1PrimaryGeneratorAction.hh"

#include "G4GeneralParticleSource.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"

B1PrimaryGeneratorAction::B1PrimaryGeneratorAction()
  : G4VUserPrimaryGeneratorAction()
  , fGeneralParticleSource(new G4GeneralParticleSource())
{
  auto* particleTable = G4ParticleTable::GetParticleTable();
  auto* particle = particleTable->FindParticle("gamma");
  if (particle) {
    fGeneralParticleSource->SetParticleDefinition(particle);
  }

  auto* source = fGeneralParticleSource->GetCurrentSource();

  // 80 кэВ > K-edge W (69.5 кэВ)
  source->GetEneDist()->SetMonoEnergy(80.0 * keV);

  // Трубка сбоку (x=5 см, z=-5 см)
  source->GetPosDist()->SetPosDisType("Point");
  source->GetPosDist()->SetCentreCoords(G4ThreeVector(5.0 * cm, 0, -5.0 * cm));

  // Фокус на центр сферы
  source->GetAngDist()->SetAngDistType("iso");
  source->GetAngDist()->SetFocusPoint(G4ThreeVector(0, 0, -5.0 * cm));
  source->GetAngDist()->SetFocusAngle(5.0 * deg); // Узкий конус
}

B1PrimaryGeneratorAction::~B1PrimaryGeneratorAction()
{
  delete fGeneralParticleSource;
  fGeneralParticleSource = nullptr;
}

void B1PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
  fGeneralParticleSource->GeneratePrimaryVertex(event);
}
