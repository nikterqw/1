/*
  ФАЙЛ: RootManager.cc

  Реализация записи ROOT/CLOG, порогов, кластеризации, merge файлов и экспорт imageN в
текст.
*/

#include "RootManager.hh"

#include "G4ios.hh"
#include "G4Threading.hh"

#include "TFile.h"
#include "TFileMerger.h"
#include "TTree.h"
#include "TH2D.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>

namespace {
  G4ThreadLocal MyROOTManager* gRM = nullptr;
}

G4Mutex MyROOTManager::fMergeMutex = G4MUTEX_INITIALIZER;
std::vector<std::string> MyROOTManager::fPartialRootFiles;
std::vector<std::string> MyROOTManager::fPartialClogFiles;

G4Mutex MyROOTManager::fRootIOMutex = G4MUTEX_INITIALIZER;
std::atomic<int> MyROOTManager::fInversionCfg{0};

MyROOTManager* MyROOTManager::GetPointer()
{
  if (!gRM) gRM = new MyROOTManager();
  return gRM;
}

void MyROOTManager::SetInversion(G4int inversion)
{
  fInversionCfg.store((inversion != 0) ? 1 : 0, std::memory_order_relaxed);
}

G4int MyROOTManager::GetInversion()
{
  return static_cast<G4int>(fInversionCfg.load(std::memory_order_relaxed));
}

std::string MyROOTManager::MakeThreadFileName(const char* base, const char* ext) const
{
#ifdef G4MULTITHREADED
  if (G4Threading::IsMultithreadedApplication()) {
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s_t%d.%s", base, fThreadId, ext);
    return std::string(buf);
  }
#endif
  return std::string(base) + "." + ext;
}

void MyROOTManager::Initialize()
{
  G4AutoLock lock(&fRootIOMutex);

  fThreadId  = G4Threading::G4GetThreadId();
  fInversion = GetInversion();

#ifdef G4MULTITHREADED
  if (G4Threading::IsMultithreadedApplication() && G4Threading::IsMasterThread()) {
    return;
  }
#endif

  const auto rootName = MakeThreadFileName("SPECT", "root");
  fFile = TFile::Open(rootName.c_str(), "RECREATE");
  if (!fFile || fFile->IsZombie()) {
    G4Exception("MyROOTManager::Initialize", "ROOT_FILE_CREATE_FAIL",
                FatalException, "Cannot create ROOT file");
  }

  fFile->cd();

  fPixelTree = new TTree("PixelTree", "Timepix-like fired pixels");
  fPixelTree->SetDirectory(nullptr);

  fClusterTree = new TTree("ClusterTree", "Clusters per event");
  fClusterTree->SetDirectory(nullptr);

  fEventTree = new TTree("EventTree", "Registered events (>=1 cluster)");
  fEventTree->SetDirectory(nullptr);

  fImageE = new TH2D("imageE", "Integrated energy map;pixel x;pixel y",
                     256, -0.5, 255.5, 256, -0.5, 255.5);
  fImageE->SetDirectory(fFile);

  fImageN = new TH2D("imageN", "Integrated hit count map;pixel x;pixel y",
                     256, -0.5, 255.5, 256, -0.5, 255.5);
  fImageN->SetDirectory(fFile);

  Book();

  // CLOG
  const auto clogName = MakeThreadFileName("SPECT", "clog");
  fClog = std::fopen(clogName.c_str(), "wb");
  if (!fClog) {
    G4Exception("MyROOTManager::Initialize", "CLOG_OPEN_FAIL",
                FatalException, "Cannot create CLOG file");
  }

#ifdef G4MULTITHREADED
  if (G4Threading::IsMultithreadedApplication()) {
    G4AutoLock lock2(&fMergeMutex);
    fPartialRootFiles.emplace_back(rootName);
    fPartialClogFiles.emplace_back(clogName);
  }
#endif
}

void MyROOTManager::Book()
{
  // PixelTree
  fPixelTree->Branch("EventID",   &fEventID_pix,   "EventID/I");
  fPixelTree->Branch("ClusterID", &fClusterID_pix, "ClusterID/I");
  fPixelTree->Branch("X", &fX, "X/s"); // unsigned short
  fPixelTree->Branch("Y", &fY, "Y/s");
  fPixelTree->Branch("E_keV", &fE_keV, "E_keV/D");

  // ClusterTree
  fClusterTree->Branch("EventID",   &fEventID_cl,   "EventID/I");
  fClusterTree->Branch("ClusterID", &fClusterID_cl, "ClusterID/I");
  fClusterTree->Branch("nPix",      &fNPix_cl,      "nPix/I");
  fClusterTree->Branch("SumE_keV",  &fSumE_cl,      "SumE_keV/D");

  // EventTree
  fEventTree->Branch("EventID",    &fEventID_ev,    "EventID/I");
  fEventTree->Branch("nClusters",  &fNClusters_ev,  "nClusters/I");
  fEventTree->Branch("nPixTotal",  &fNPixTot_ev,    "nPixTotal/I");
  fEventTree->Branch("SumE_keV",   &fSumE_ev,       "SumE_keV/D");
}

void MyROOTManager::FillPixel(G4int eventID, G4int clusterID, std::uint16_t x, std::uint16_t y, G4double e_keV)
{
  fEventID_pix = eventID;
  fClusterID_pix = clusterID;
  fX = x;
  fY = y;
  fE_keV = e_keV;
  if (fPixelTree) fPixelTree->Fill();
}

void MyROOTManager::FillCluster(G4int eventID, G4int clusterID, G4int nPix, G4double sumE_keV)
{
  fEventID_cl = eventID;
  fClusterID_cl = clusterID;
  fNPix_cl = nPix;
  fSumE_cl = sumE_keV;
  if (fClusterTree) fClusterTree->Fill();
}

void MyROOTManager::FillEvent(G4int eventID, G4int nClusters, G4int nPixTotal, G4double sumE_keV)
{
  fEventID_ev = eventID;
  fNClusters_ev = nClusters;
  fNPixTot_ev = nPixTotal;
  fSumE_ev = sumE_keV;
  if (fEventTree) fEventTree->Fill();
}

void MyROOTManager::FillImageE(G4int ix, G4int iy, G4double e_keV)
{
  if (fImageE) fImageE->Fill(ix, iy, e_keV);
}

void MyROOTManager::FillImageN(G4int ix, G4int iy, G4double w)
{
  if (fImageN) fImageN->Fill(ix, iy, w);
}

void MyROOTManager::WriteCLOGFrame(
    G4int eventID,
    const std::vector<std::vector<std::pair<int,double>>>& clustersPixels)
{
  if (!fClog) return;

  // PIXet: Frame <number> <timestamp_s> <exposure_s>  :contentReference[oaicite:1]{index=1}
  std::fprintf(fClog, "Frame %d 0 0\n", eventID);

  // Each following row: one cluster, list of pixels [X,Y,value] :contentReference[oaicite:2]{index=2}
  for (const auto& cl : clustersPixels) {
    for (const auto& kv : cl) {
      const int key = kv.first;
      const int ix = key % 256;
      const int iy = key / 256;
      const double val = kv.second;
      std::fprintf(fClog, "[%d,%d,%.6g] ", ix, iy, val);
    }
    std::fprintf(fClog, "\n");
  }
}

void MyROOTManager::Finalize()
{
  G4AutoLock lock(&fRootIOMutex);

#ifdef G4MULTITHREADED
  if (G4Threading::IsMultithreadedApplication() && G4Threading::IsMasterThread()) {
    return;
  }
#endif

  if (fClog) {
    std::fclose(fClog);
    fClog = nullptr;
  }

  if (!fFile) return;

  fFile->cd();

  if (fPixelTree) { fPixelTree->Write(); delete fPixelTree; fPixelTree = nullptr; }
  if (fClusterTree) { fClusterTree->Write(); delete fClusterTree; fClusterTree = nullptr; }
  if (fEventTree) { fEventTree->Write(); delete fEventTree; fEventTree = nullptr; }

  if (fImageE) { fImageE->Write(); fImageE = nullptr; }
  if (fImageN) { fImageN->Write(); fImageN = nullptr; }

  fFile->Write();
  fFile->Close();
  delete fFile;
  fFile = nullptr;
}

void MyROOTManager::MergeAll(const std::string& outFile, bool deletePartials)
{
  std::vector<std::string> files;
  {
    G4AutoLock lock(&fMergeMutex);
    files = fPartialRootFiles;
  }

  std::sort(files.begin(), files.end());
  files.erase(std::unique(files.begin(), files.end()), files.end());

  if (files.empty()) {
    G4cout << "MyROOTManager::MergeAll: no partial ROOT files to merge." << G4endl;
    return;
  }

  G4AutoLock lockIO(&fRootIOMutex);

  TFileMerger merger(true);
  merger.OutputFile(outFile.c_str(), "RECREATE");
  for (const auto& f : files) merger.AddFile(f.c_str());

  if (!merger.Merge()) {
    G4Exception("MyROOTManager::MergeAll", "ROOT_MERGE_FAIL",
                JustWarning, "TFileMerger failed");
    return;
  }

  if (deletePartials) {
    for (const auto& f : files) std::remove(f.c_str());
  }

  G4cout << "Merged " << files.size() << " files -> " << outFile << G4endl;
}

void MyROOTManager::MergeAllCLOG(const std::string& outFile, bool deletePartials)
{
  std::vector<std::string> files;
  {
    G4AutoLock lock(&fMergeMutex);
    files = fPartialClogFiles;
  }

  std::sort(files.begin(), files.end());
  files.erase(std::unique(files.begin(), files.end()), files.end());

  if (files.empty()) {
    G4cout << "MyROOTManager::MergeAllCLOG: no partial CLOG files to merge." << G4endl;
    return;
  }

  std::FILE* out = std::fopen(outFile.c_str(), "wb");
  if (!out) {
    G4Exception("MyROOTManager::MergeAllCLOG", "CLOG_MERGE_OPEN_FAIL",
                JustWarning, "Cannot create merged CLOG file");
    return;
  }

  // Simple concatenation (frames may be interleaved by thread; usually OK for offline processing)
  for (const auto& f : files) {
    std::FILE* in = std::fopen(f.c_str(), "rb");
    if (!in) continue;
    char buf[1 << 16];
    std::size_t n = 0;
    while ((n = std::fread(buf, 1, sizeof(buf), in)) > 0) {
      std::fwrite(buf, 1, n, out);
    }
    std::fclose(in);
  }
  std::fclose(out);

  if (deletePartials) {
    for (const auto& f : files) std::remove(f.c_str());
  }

  G4cout << "Merged " << files.size() << " CLOG files -> " << outFile << G4endl;
}


void MyROOTManager::ExportTH2DText(const std::string& rootFile,
                                   const std::string& outTxt,
                                   const std::string& histName,
                                   bool roundToInt,
                                   int precision)
{
  // Общий экспорт 2D-гистограммы (TH2D) в текстовый 2D массив:
  //  - строки = Y, столбцы = X
  //  - без заголовков (как ты просил)
  //  - для imageN используем округление (целочисленный счётчик)
  //  - для imageE пишем double (энергия)
  TFile* f = TFile::Open(rootFile.c_str(), "READ");
  if (!f || f->IsZombie()) {
    G4cout << "MyROOTManager::ExportTH2DText: cannot open ROOT file: "
           << rootFile << G4endl;
    return;
  }

  auto* h = dynamic_cast<TH2D*>(f->Get(histName.c_str()));
  if (!h) {
    G4cout << "MyROOTManager::ExportTH2DText: histogram '" << histName
           << "' not found in " << rootFile << G4endl;
    f->Close();
    return;
  }

  const int nx = h->GetNbinsX();
  const int ny = h->GetNbinsY();
  if (nx != 256 || ny != 256) {
    G4cout << "MyROOTManager::ExportTH2DText: unexpected histogram size "
           << nx << "x" << ny << " (expected 256x256). Exporting anyway." << G4endl;
  }

  std::ofstream out(outTxt, std::ios::out | std::ios::trunc);
  if (!out) {
    G4cout << "MyROOTManager::ExportTH2DText: cannot write: " << outTxt << G4endl;
    f->Close();
    return;
  }

  if (!roundToInt) {
    out.setf(std::ios::scientific);
    out.precision(precision);
  }

  // Пишем как матрицу [y][x] (строки = Y, столбцы = X), индексация ROOT: 1..N.
  for (int iy = 1; iy <= ny; ++iy) {
    for (int ix = 1; ix <= nx; ++ix) {
      const double v = h->GetBinContent(ix, iy);
      if (roundToInt) {
        const long long iv = static_cast<long long>(std::llround(v));
        out << iv;
      } else {
        out << v;
      }
      if (ix != nx) out << ' ';
    }
    out << '\n';
  }

  out.close();
  f->Close();

  G4cout << "MyROOTManager::ExportTH2DText: saved " << histName
         << " -> " << outTxt << G4endl;
}

void MyROOTManager::ExportImageNText(const std::string& rootFile,
                                     const std::string& outTxt,
                                     const std::string& histName)
{
  // imageN = счётчик срабатываний → пишем как целые.
  ExportTH2DText(rootFile, outTxt, histName, /*roundToInt=*/true);
}

void MyROOTManager::ExportImageEText(const std::string& rootFile,
                                     const std::string& outTxt,
                                     const std::string& histName)
{
  // imageE = энергия → пишем double без округления.
  ExportTH2DText(rootFile, outTxt, histName, /*roundToInt=*/false, /*precision=*/10);
}


