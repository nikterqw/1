/*
  ФАЙЛ: exampleB1.cc

  Точка входа приложения. Парсит аргументы командной строки, выбирает режим
(batch/interactive), настраивает Geant4 RunManager (MT или ST), инициализирует
физику/геометрию/действия, при --vis поднимает визуализацию и выполняет дополнительные
команды (GPS макрос, отрисовка точек источника/треков).
*/
#include "B1ActionInitialization.hh"
#include "B1DetectorConstruction.hh"

#ifdef G4MULTITHREADED
#include "G4MTRunManager.hh"
#else
#include "G4RunManager.hh"
#endif

#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"
#include "G4Threading.hh"

#include "QBBC.hh"
#include "Randomize.hh"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>

namespace {
  bool HasArg(int argc, char** argv, const char* opt)
  {
    for (int i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], opt) == 0) return true;
    }
    return false;
  }

  const char* GetStrArg(int argc, char** argv, const char* opt, const char* def)
  {
    for (int i = 1; i + 1 < argc; ++i) {
      if (std::strcmp(argv[i], opt) == 0) return argv[i + 1];
    }
    return def;
  }

  int GetIntArg(int argc, char** argv, const char* opt, int def)
  {
    for (int i = 1; i + 1 < argc; ++i) {
      if (std::strcmp(argv[i], opt) == 0) return std::atoi(argv[i + 1]);
    }
    return def;
  }

double GetDoubleArg(int argc, char** argv, const char* opt, double def)
  {
    for (int i = 1; i + 1 < argc; ++i) {
      if (std::strcmp(argv[i], opt) == 0) {
        char* endp = nullptr;
        const double v = std::strtod(argv[i + 1], &endp);
        if (endp != argv[i + 1]) return v;
        return def;
      }
    }
    return def;
  }

  // первый аргумент без '-' считаем макросом (batch)
const char* GetMacroArg(int argc, char** argv)
{
  for (int i = 1; i < argc; ++i) {
    const char* a = argv[i];
    if (!a) continue;

    // Опции без параметра
    if (std::strcmp(a, "--vis") == 0 ||
        std::strcmp(a, "--help") == 0 ||
        std::strcmp(a, "-h") == 0) {
      continue;
    }

    // Опции с параметром: пропускаем и саму опцию, и её значение
    if (std::strcmp(a, "--vis-driver") == 0 ||
        std::strcmp(a, "--vis-macro") == 0 ||
        std::strcmp(a, "--tracks") == 0 ||
        std::strcmp(a, "--source-sample") == 0 ||
        std::strcmp(a, "--clog") == 0 ||
        std::strcmp(a, "--image-txt") == 0 ||
        std::strcmp(a, "--energy-txt") == 0 ||
        std::strcmp(a, "--threads") == 0 ||
        std::strcmp(a, "--threshold-kev") == 0) {
      ++i; // skip value
      continue;
    }

    // Любой другой аргумент без '-' считаем batch-макросом
    if (a[0] != '-') {
      return a;
    }
  }
  return nullptr;
}

  void SetEnvVar(const char* key, const char* value)
  {
#if defined(_WIN32)
    _putenv_s(key, value);
#else
    setenv(key, value, 1);
#endif
  }

  void PrintHelp(const char* prog)
  {
    std::cout << "Usage:\n"
              << "  " << prog << " [run.mac] [options]\n\n"
              << R"HELP(Modes:
  Batch mode:
    exampleB1 run.mac
  Interactive mode (UI):
    exampleB1 [options]

Core idea:
  Geant4 simulation of "mask + Timepix(CdTe)".
  Output is Timepix-like:
    - energy deposition per pixel (256x256, pitch 55um) aggregated per event
    - per-event clustering of fired pixels (multiple clusters)
    - ROOT output + PIXet-like CLOG output

Registered event (by design):
  An event is considered registered if at least one pixel passes threshold:
    E_pixel >= 6 keV
  Events without fired pixels are NOT stored.

Outputs (ST):
  SPECT.root
  SPECT.clog

Outputs (MT):
  workers write:
    SPECT_t<ID>.root
    SPECT_t<ID>.clog
  master merges at end:
    SPECT.root
    SPECT.clog (or custom name via --clog)

ROOT content:
  PixelTree (one entry = one fired pixel)
    EventID (int)       - Geant4 event id
    ClusterID (int)     - cluster index within event (0..)
    X,Y (0..255)        - pixel coordinates
    E_keV (double)      - energy in pixel (sum over the event)
  ClusterTree (one entry = one cluster)
    EventID, ClusterID, nPix, SumE_keV
  EventTree (one entry = one registered event)
    EventID, nClusters, nPixTotal, SumE_keV   (SumE is after pixel threshold)
  imageE (TH2D) integrated energy map (sum E_keV over run)
  imageN (TH2D) integrated occupancy map (count of fired pixels over run)

CLOG content (PIXet-like):
  For each registered event:
    Frame <EventID> 0 0
    then N lines (one per cluster):
      [X,Y,value] [X,Y,value] ...
    value = E_keV for that pixel.

Options:
  --help, -h
      Print this help and exit.

  --vis
      Enable visualization (default: OFF).

  --vis-driver <driver>
      Visualization driver (default: OGLSQt).
      
  --energy-txt <file>
      Export integrated energy map (TH2D imageE) to text 256x256.

  --threshold-kev <N>
      Pixel threshold in keV (default 6). Floating values allowed. Below threshold is not registered.

  --threads <N>
      Number of worker threads in MT batch mode (default: number of cores).

Notes:
  - In interactive visualization with --tracks or --source-sample the program forces single-threaded mode.
    Any --threads value is ignored in this case.

Examples: OGLSQt, OGLIQt, OGLS, OGLI, ...

  --vis-macro <file.mac>
      Execute visualization macro after opening driver and drawing volume.

  --tracks N
      In interactive --vis mode: enable trajectories and automatically do /run/beamOn N.
      This is the standard way to "see sources" (GPS itself is not geometry).

  --source-sample N
      Draw primary vertex positions as points for first N events (universal GPS visualization).
      Also automatically performs /run/beamOn N in --vis mode.

  --clog <file>
      Set name of FINAL merged CLOG file (default: SPECT.clog).

  --image-txt <file>
      Save integrated hit-count image (TH2D imageN) as a text matrix 256x256.
      Default: imageN.txt

)HELP";
  }
} // namespace

int main(int argc, char** argv)
{
  if (HasArg(argc, argv, "--help") || HasArg(argc, argv, "-h")) {
    PrintHelp(argv[0]);
    return 0;
  }

  const bool enableVis      = HasArg(argc, argv, "--vis");
  const int  visTracks      = GetIntArg(argc, argv, "--tracks", 0);
  const int  sourceSampleN  = GetIntArg(argc, argv, "--source-sample", 0);

  const char* visDriver     = GetStrArg(argc, argv, "--vis-driver", "OGLSQt");
  const char* visMacro      = GetStrArg(argc, argv, "--vis-macro", nullptr);

  // Управление числом потоков (только для MT batch режима)
  const int threadsArg = GetIntArg(argc, argv, "--threads", -1);

  // Порог по пикселю в keV (будет прочитан TimepixDetector через env)
  const double thresholdKeV = GetDoubleArg(argc, argv, "--threshold-kev", -1.0);
  
  // Выходные файлы
  const char* clogOut      = GetStrArg(argc, argv, "--clog", "SPECT.clog");
  const char* imgTxtOut    = GetStrArg(argc, argv, "--image-txt", "imageN.txt");
  const char* energyTxtOut = GetStrArg(argc, argv, "--energy-txt", "imageE.txt");
  SetEnvVar("SPECT_CLOG_OUT", clogOut);
  SetEnvVar("SPECT_IMAGETXT_OUT", imgTxtOut);
  SetEnvVar("SPECT_ENERGYTXT_OUT", energyTxtOut);

  // Порог по пикселю (keV). Допускаются дробные значения.
  if (thresholdKeV > 0.0) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(6);
    oss << thresholdKeV;
    SetEnvVar("SPECT_THRESHOLD_KEV", oss.str().c_str());
  }

  // Передаём sampleN в EventAction через env
  {
    std::string s = std::to_string(sourceSampleN);
    SetEnvVar("SPECT_SOURCE_SAMPLE", s.c_str());
  }

  const char* macro = GetMacroArg(argc, argv);
  const bool interactive = (macro == nullptr);

  std::unique_ptr<G4UIExecutive> ui;
  if (interactive) ui = std::make_unique<G4UIExecutive>(argc, argv);

  G4Random::setTheEngine(new CLHEP::RanecuEngine);

  // ------------------------
  // RunManager selection:
  //   - batch: keep MT if compiled
  //   - interactive + vis + (tracks or source-sample): force ST for stable drawing
  // ------------------------
  G4RunManager* runManager = nullptr;

#ifdef G4MULTITHREADED
  const bool forceSTForVis = (interactive && enableVis && (visTracks > 0 || sourceSampleN > 0));
  if (forceSTForVis) {
    G4cout << "Interactive visualization: forcing ST for stable drawing." << G4endl;
    if (threadsArg > 1) {
      G4cout << "NOTE: --threads is ignored in interactive visualization mode; using 1 thread." << G4endl;
    }
  }
  if (!forceSTForVis) {
    auto* mt = new G4MTRunManager;
    mt->SetNumberOfThreads((threadsArg > 0) ? threadsArg : static_cast<int>(G4Threading::G4GetNumberOfCores()));
    runManager = mt;
  } else {
    runManager = new G4RunManager;
  }
#else
  runManager = new G4RunManager;
#endif

  runManager->SetUserInitialization(new B1DetectorConstruction());

  // QBBC включает флуоресценцию по умолчанию в Geant4 10.5+
  auto* physicsList = new QBBC;
  physicsList->SetVerboseLevel(1);
  runManager->SetUserInitialization(physicsList);

  runManager->SetUserInitialization(new B1ActionInitialization());

  auto* UImanager = G4UImanager::GetUIpointer();

  // Batch
  if (!interactive) {
    UImanager->ApplyCommand(G4String("/control/execute ") + G4String(macro));
    delete runManager;
    return 0;
  }

  // Interactive
  std::unique_ptr<G4VisManager> visManager;
  if (enableVis) {
    visManager = std::make_unique<G4VisExecutive>();
    visManager->Initialize();

    UImanager->ApplyCommand("/run/initialize");
    UImanager->ApplyCommand(std::string("/vis/open ") + visDriver);
    UImanager->ApplyCommand("/vis/drawVolume");
    UImanager->ApplyCommand("/vis/viewer/set/autoRefresh true");
    UImanager->ApplyCommand("/vis/scene/add/axes 0 0 0 5 cm");

    // чтобы точки/треки оставались на экране после каждого события
    if (visTracks > 0 || sourceSampleN > 0) {
      UImanager->ApplyCommand("/vis/scene/endOfEventAction accumulate");
    }

    if (visMacro && std::strlen(visMacro) > 0) {
      UImanager->ApplyCommand(std::string("/control/execute ") + visMacro);
    }

    // ВАЖНО: включаем сохранение траекторий для ЛЮБОГО режима визуализации,
    // чтобы видеть треки вторичных фотонов из металла.
    if (visTracks > 0 || sourceSampleN > 0) {
      UImanager->ApplyCommand("/tracking/storeTrajectory 1");
      UImanager->ApplyCommand("/vis/scene/add/trajectories smooth");
      UImanager->ApplyCommand("/vis/trajectory/setDrawStepPts true");
      UImanager->ApplyCommand("/vis/trajectory/setStepPtsSize 1");
      UImanager->ApplyCommand("/vis/trajectory/setLineWidth 1");
    }

    if (visTracks > 0) {
      UImanager->ApplyCommand(std::string("/run/beamOn ") + std::to_string(visTracks));
      UImanager->ApplyCommand("/vis/viewer/update");
      UImanager->ApplyCommand("/vis/viewer/flush");
    } else if (sourceSampleN > 0) {
      UImanager->ApplyCommand(std::string("/run/beamOn ") + std::to_string(sourceSampleN));
      UImanager->ApplyCommand("/vis/viewer/flush");
      UImanager->ApplyCommand("/vis/viewer/refresh");
    }
  }

  ui->SessionStart();

  delete runManager;
  return 0;
}
