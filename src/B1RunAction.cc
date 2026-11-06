/*
  ФАЙЛ: B1RunAction.cc

  Реализация Begin/EndOfRun. В MT закрывает per-thread файлы и выполняет merge на master.
После завершения сохраняет imageN в текстовую матрицу 256x256.
*/

#include "B1RunAction.hh"
#include "RootManager.hh"

#include "G4Threading.hh"
#include "G4Run.hh"
#include "Randomize.hh"

#include <cstdlib>
#include <ctime>

namespace {
  bool TryGetBaseSeedFromEnv(long& outBaseSeed)
  {
    const char* s = std::getenv("G4SEED");
    if (!s) return false;
    char* end = nullptr;
    const long v = std::strtol(s, &end, 10);
    if (end == s) return false;
    outBaseSeed = v;
    return true;
  }
}

void B1RunAction::BeginOfRunAction(const G4Run*)
{
  // ---- RNG seeding (опционально, но полезно для воспроизводимости)
  long baseSeed = 0;
  const bool hasUserSeed = TryGetBaseSeedFromEnv(baseSeed);
  if (!hasUserSeed) baseSeed = static_cast<long>(std::time(nullptr));

  const auto tid = G4Threading::G4GetThreadId(); // master:-1, worker:0..N-1
  long seeds[2];
  seeds[0] = baseSeed + 12345L * (tid + 1);
  seeds[1] = (baseSeed ^ 0x9E3779B97F4A7C15ULL) + 67890L * (tid + 1);
  CLHEP::HepRandom::getTheEngine()->setSeeds(seeds, -1);

  // ---- ROOT + CLOG output
#ifdef G4MULTITHREADED
  if (G4Threading::IsMultithreadedApplication()) {
    // MT: master не пишет, worker пишет
    if (G4Threading::IsMasterThread()) return;
    MyROOTManager::GetPointer()->Initialize();
    return;
  }
#endif

  // ST запуск (даже если бинарь собран как MT): пишем напрямую
  MyROOTManager::GetPointer()->Initialize();
}

void B1RunAction::EndOfRunAction(const G4Run*)
{
#ifdef G4MULTITHREADED
  if (G4Threading::IsMultithreadedApplication()) {
    // MT: worker закрывает свой ROOT + CLOG
    if (!G4Threading::IsMasterThread()) {
      MyROOTManager::GetPointer()->Finalize();
      return;
    }

    // MT: master мерджит ROOT и CLOG
    MyROOTManager::MergeAll("SPECT.root", /*deletePartials=*/true);
    const char* out = std::getenv("SPECT_CLOG_OUT");
	MyROOTManager::MergeAllCLOG(out ? out : "SPECT.clog", true);
    // Экспорт интегральной карты hit-count (imageN) в текстовый файл.
    const char* imgTxt = std::getenv("SPECT_IMAGETXT_OUT");
    MyROOTManager::ExportImageNText("SPECT.root", imgTxt ? imgTxt : "imageN.txt");
    const char* eTxt = std::getenv("SPECT_ENERGYTXT_OUT");
    MyROOTManager::ExportImageEText("SPECT.root", eTxt ? eTxt : "imageE.txt");
    return;
  }
#endif

  // ST: просто закрываем один ROOT + один CLOG
  MyROOTManager::GetPointer()->Finalize();
  const char* imgTxt = std::getenv("SPECT_IMAGETXT_OUT");
  MyROOTManager::ExportImageNText("SPECT.root", imgTxt ? imgTxt : "imageN.txt");
  const char* eTxt = std::getenv("SPECT_ENERGYTXT_OUT");
  MyROOTManager::ExportImageEText("SPECT.root", eTxt ? eTxt : "imageE.txt");
}
