/*
  ФАЙЛ: B1ActionInitialization.cc

  Реализация регистрации действий Geant4. Читает настройки из переменных окружения
(например SPECT_SOURCE_SAMPLE) и передаёт их в EventAction/SteppingAction.
*/

#include "B1ActionInitialization.hh"

#include "B1EventAction.hh"
#include "B1PrimaryGeneratorAction.hh"
#include "B1RunAction.hh"
#include "B1SteppingAction.hh"

#include <cstdlib>

namespace {
  int ReadEnvInt(const char* key, int defVal)
  {
    const char* s = std::getenv(key);
    if (!s) return defVal;
    char* end = nullptr;
    long v = std::strtol(s, &end, 10);
    if (end == s) return defVal;
    if (v < 0) v = 0;
    if (v > 1000000000L) v = 1000000000L;
    return static_cast<int>(v);
  }
}

void B1ActionInitialization::BuildForMaster() const
{
  SetUserAction(new B1RunAction());
}

void B1ActionInitialization::Build() const
{
  SetUserAction(new B1PrimaryGeneratorAction());

  auto* runAction = new B1RunAction();
  SetUserAction(runAction);

  // Сколько точек источника рисуем (берём из env, который выставляет exampleB1.cc)
  const int sampleN = ReadEnvInt("SPECT_SOURCE_SAMPLE", 0);

  auto* eventAction = new B1EventAction(runAction, sampleN);
  SetUserAction(eventAction);

  SetUserAction(new B1SteppingAction(eventAction));
}
