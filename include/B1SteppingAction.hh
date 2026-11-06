/*
  ФАЙЛ: B1SteppingAction.hh

  Объявление stepping action: перехватывает шаги трека для накопления энерговыделения в
сенсоре и передачи в TimepixDetector/ROOT менеджер.
*/

#ifndef B1SteppingAction_h
#define B1SteppingAction_h 1

#include "G4UserSteppingAction.hh"
#include "globals.hh"

class B1EventAction;
class G4Step;

// -----------------------------------------------------------------------------
// B1SteppingAction
//
// Действие на каждом шаге трека. Обычно тут делают:
//  - накопление энерговыделения в объёме,
//  - триггеры/фильтры по частице или материалу,
//  - пользовательские счётчики.
//
// Сейчас оставлено пустым, т.к. сбор хитов происходит в SensitiveDetector.
// -----------------------------------------------------------------------------
class B1SteppingAction : public G4UserSteppingAction
{
public:
  explicit B1SteppingAction(B1EventAction* eventAction);
  ~B1SteppingAction() override = default;

  void UserSteppingAction(const G4Step* step) override;

private:
  B1EventAction* fEventAction = nullptr; // не владеем
};

#endif
