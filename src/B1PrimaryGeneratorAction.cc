/*
  ФАЙЛ: B1PrimaryGeneratorAction.cc
  Изменения: убрано дублирование настроек GPS. Все параметры источника
  задаются в GPS.mac. Здесь только создаётся GPS и выбирается частица gamma.
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
