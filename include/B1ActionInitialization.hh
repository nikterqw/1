/*
  ФАЙЛ: B1ActionInitialization.hh

  Объявление класса инициализации пользовательских действий Geant4 (генератор первичных
частиц, run/event/stepping actions). В MT обеспечивает отдельную конфигурацию для master и
worker потоков.
*/

#ifndef B1ActionInitialization_h
#define B1ActionInitialization_h 1

#include "G4VUserActionInitialization.hh"

// -----------------------------------------------------------------------------
// B1ActionInitialization
//
// В MT:
//  - BuildForMaster() вызывается один раз на master потоке
//  - Build() вызывается для каждого worker потока
//
// Здесь важно не хранить глобальные указатели на actions.
// Geant4 берёт владение переданными объектами через SetUserAction().
// -----------------------------------------------------------------------------
class B1ActionInitialization : public G4VUserActionInitialization
{
public:
  B1ActionInitialization() = default;
  ~B1ActionInitialization() override = default;

  void BuildForMaster() const override;
  void Build() const override;
};

#endif
