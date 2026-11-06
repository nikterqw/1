/*
  ФАЙЛ: B1SteppingAction.cc

  Реализация stepping action: фильтрует шаги по объёму сенсора, снимает edep и координаты
и передаёт их в менеджер/SD.
*/

#include "B1SteppingAction.hh"

#include "B1EventAction.hh"

#include "G4Step.hh"

B1SteppingAction::B1SteppingAction(B1EventAction* eventAction)
  : G4UserSteppingAction()
  , fEventAction(eventAction)
{
}

void B1SteppingAction::UserSteppingAction(const G4Step*)
{
  // Пусто: используем SensitiveDetector(TimepixDetector) для сбора информации.
  // Если понадобится, тут можно добавить логику по шагам.
}
