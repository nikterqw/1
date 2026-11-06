/*
  ФАЙЛ: B1PrimaryGeneratorAction.hh

  Объявление генератора первичных частиц. В текущем проекте фактически используется GPS,
но класс сохраняется для совместимости/расширения.
*/

#ifndef B1PrimaryGeneratorAction_h
#define B1PrimaryGeneratorAction_h 1

#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"

class G4Event;
class G4GeneralParticleSource;

// -----------------------------------------------------------------------------
// B1PrimaryGeneratorAction
//
// Используем G4GeneralParticleSource (GPS), потому что она уже умеет:
//  - несколько источников (/gps/source/add ...),
//  - различные распределения энергии/угла/позиции,
//  - настройку через макросы и UI.
//
// ВАЖНО про владение:
//  - fGeneralParticleSource создаётся в конструкторе и удаляется в деструкторе.
//  - Geant4 владеет объектом B1PrimaryGeneratorAction (через RunManager).
// -----------------------------------------------------------------------------
class B1PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:
  B1PrimaryGeneratorAction();
  ~B1PrimaryGeneratorAction() override;

  void GeneratePrimaries(G4Event* event) override;

  const G4GeneralParticleSource* GetGeneralParticleSource() const { return fGeneralParticleSource; }

  // Запрещаем копирование/перемещение: внутри сырой указатель на GPS.
  B1PrimaryGeneratorAction(const B1PrimaryGeneratorAction&) = delete;
  B1PrimaryGeneratorAction& operator=(const B1PrimaryGeneratorAction&) = delete;
  B1PrimaryGeneratorAction(B1PrimaryGeneratorAction&&) = delete;
  B1PrimaryGeneratorAction& operator=(B1PrimaryGeneratorAction&&) = delete;

private:
  G4GeneralParticleSource* fGeneralParticleSource = nullptr;
};

#endif
