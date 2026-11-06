/*
  ФАЙЛ: RootManager.hh

  Интерфейс менеджера записи данных: ROOT деревья (PixelTree/ClusterTree/EventTree), TH2D
карты (imageE/imageN), CLOG и MT merge. Потокобезопасность обеспечивается через thread-
local инстанс и mutex на merge.
*/

#pragma once

#include "globals.hh"
#include "G4AutoLock.hh"
#include "G4Threading.hh"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

// ROOT forward declarations
class TFile;
class TTree;
class TH2D;

// -----------------------------------------------------------------------------
// MyROOTManager
//
// Цель: сохранить данные "как Timepix":
//  1) PixelTree: разреженный список сработавших пикселей для каждого события
//       EventID (I), X (uint16), Y (uint16), E_keV (double)
//     Записываются ТОЛЬКО пиксели, прошедшие порог (в SD).
//
//  2) EventTree: агрегированная запись по событию (только "зарегистрированные"
//     события, то есть те, где есть хотя бы один пиксель выше порога):
//       EventID (I), nPix (I), SumE_keV (double)
//
//  3) imageE: TH2D 256x256 — интегральная карта (сумма E_keV по всем событиям)
//     imageN: TH2D 256x256 — интегральная карта (кол-во срабатываний пикселя)
//
// Дополнительно: отдельный текстовый файл *.clog (Cluster log) в стиле PIXET Pro:
//  - Строка "Frame <id> <timestamp> <exposure>"
//  - Далее строка-кластер: список пикселей "[X,Y,value]" через пробел
//  Для нас "кластер" = все fired pixels данного события (одна строка на событие).
//
// Потоки/MT:
//  - В реальном MT каждый worker пишет в свой ROOT-файл и свой CLOG-файл:
//       SPECT_t<ID>.root, SPECT_t<ID>.clog
//    Master в конце мерджит ROOT и конкатенирует CLOG в единый SPECT.clog.
//  - В ST режиме пишем сразу SPECT.root и SPECT.clog.
// -----------------------------------------------------------------------------
class MyROOTManager {
public:
  static MyROOTManager* GetPointer();

  void Initialize();
  void Finalize();

  // One fired pixel entry (after per-event aggregation and thresholding)
  void FillPixel(G4int eventID, G4int clusterID, std::uint16_t x, std::uint16_t y, G4double e_keV);

  // One cluster entry
  void FillCluster(G4int eventID, G4int clusterID, G4int nPix, G4double sumE_keV);

  // One registered event entry (only events with >=1 cluster)
  void FillEvent(G4int eventID, G4int nClusters, G4int nPixTotal, G4double sumE_keV);

  // Integrated images (optional but handy)
  void FillImageE(G4int ix, G4int iy, G4double e_keV);
  void FillImageN(G4int ix, G4int iy, G4double w = 1.0);

  // CLOG output (PIXet-like), per-thread file in MT
  void WriteCLOGFrame(G4int eventID,
                      const std::vector<std::vector<std::pair<int,double>>>& clustersPixels); 
  // clustersPixels: vector of clusters; each cluster is vector of (key, E_keV)
  // key = ix + 256*iy

  static void SetInversion(G4int inversion);
  static G4int GetInversion();

  static void MergeAll(const std::string& outFile = "SPECT.root",
                       bool deletePartials = false);
  static void MergeAllCLOG(const std::string& outFile = "SPECT.clog",
                           bool deletePartials = false);


// Экспорт интегральной карты срабатываний (TH2D imageN) в текстовый файл.
// Формат: 256 строк по 256 целых значений, разделённых пробелами.
// Значение = количество срабатываний в пикселе (sum over run).
//
// Использование:
//   MyROOTManager::ExportImageNText("SPECT.root", "imageN.txt");
//
// ВАЖНО для MT:
//   - вызывать ТОЛЬКО после MergeAll(), когда сформирован итоговый ROOT.
static void ExportTH2DText(const std::string& rootFile,
                          const std::string& outTxt,
                          const std::string& histName,
                          bool roundToInt,
                          int precision = 10);

  // Удобная обёртка для счётчика срабатываний (imageN).
  static void ExportImageNText(const std::string& rootFile,
                             const std::string& outTxt,
                             const std::string& histName = "imageN");

  // Удобная обёртка для карты энергии (imageE).
  // Пишет double без округления.
  static void ExportImageEText(const std::string& rootFile,
                               const std::string& outTxt,
                               const std::string& histName = "imageE");


private:
  MyROOTManager() = default;
  void Book();
  std::string MakeThreadFileName(const char* base, const char* ext) const;

private:
  G4int fThreadId{-1};
  int   fInversion{0};

  TFile* fFile{nullptr};
  TTree* fPixelTree{nullptr};
  TTree* fClusterTree{nullptr};
  TTree* fEventTree{nullptr};
  TH2D*  fImageE{nullptr}; // energy sum
  TH2D*  fImageN{nullptr}; // counts

  // PixelTree branches
  int fEventID_pix{-1};
  int fClusterID_pix{-1};
  std::uint16_t fX{0};
  std::uint16_t fY{0};
  double fE_keV{0.0};

  // ClusterTree branches
  int fEventID_cl{-1};
  int fClusterID_cl{-1};
  int fNPix_cl{0};
  double fSumE_cl{0.0};

  // EventTree branches
  int fEventID_ev{-1};
  int fNClusters_ev{0};
  int fNPixTot_ev{0};
  double fSumE_ev{0.0};

  // CLOG file
  std::FILE* fClog{nullptr};

  // MT helpers
  static G4Mutex fMergeMutex;
  static std::vector<std::string> fPartialRootFiles;
  static std::vector<std::string> fPartialClogFiles;

  static G4Mutex fRootIOMutex;
  static std::atomic<int> fInversionCfg;
};
