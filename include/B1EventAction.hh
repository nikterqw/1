/*
  ФАЙЛ: B1EventAction.hh

  Объявление действий на событие: прогресс-лог и опциональная визуализация источника
точками (выборка первичных вершин).
*/

#ifndef B1EventAction_h
#define B1EventAction_h 1

#include "G4UserEventAction.hh"
#include "globals.hh"

class B1RunAction;
class G4Event;

// -----------------------------------------------------------------------------
// B1EventAction
//
// Дополнено:
//  - опциональная визуализация "источника точками":
//    рисуем позицию PrimaryVertex для первых N событий (SPECT_SOURCE_SAMPLE=N).
//    Работает корректно в интерактиве с --vis при запуске в ST.
//
// ВАЖНО: в MT рисование из worker-потоков ненадёжно, поэтому в exampleB1.cc
// при --vis --source-sample мы форсируем ST.
// -----------------------------------------------------------------------------
class B1EventAction : public G4UserEventAction
{
public:
  explicit B1EventAction(B1RunAction* runAction, G4int sourceSampleN = 0);
  ~B1EventAction() override = default;

  void BeginOfEventAction(const G4Event* event) override;
  void EndOfEventAction(const G4Event* event) override;

private:
  B1RunAction* fRunAction = nullptr;

  // Сколько первых событий рисуем как точки источника (0 = выключено)
  G4int fSourceSampleN = 0;
};

#endif
