/*
  ФАЙЛ: B1RunAction.hh

  Объявление действий на запуск: управление seed'ами, инициализация и финализация
ROOT/CLOG через MyROOTManager, merge в MT.
*/

#ifndef B1RunAction_h
#define B1RunAction_h 1

#include "G4UserRunAction.hh"
#include "globals.hh"

class G4Run;

// -----------------------------------------------------------------------------
// B1RunAction
//
// Используем для:
//  - инициализации/финализации ROOT-вывода в worker потоках,
//  - (опционально) задания seed'ов генератора случайных чисел.
//
// ВАЖНО про MT и seeds:
//  - Для воспроизводимости лучше задавать seed'ы детерминированно (через макрос
//    /random/setSeeds или через CLHEP/Geant4 API).
//  - Ниже мы оставляем безопасную логику: если пользователь НЕ задал seed сам,
//    можно сгенерировать разные seed'ы на разные потоки.
// -----------------------------------------------------------------------------
class B1RunAction : public G4UserRunAction
{
public:
  B1RunAction() = default;
  ~B1RunAction() override = default;

  void BeginOfRunAction(const G4Run*) override;
  void EndOfRunAction(const G4Run*) override;
};

#endif
