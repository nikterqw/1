/*
  ФАЙЛ: B1EventAction.cc

  Рисует первичные вершины и траектории вторичных частиц
  для первых SPECT_SOURCE_SAMPLE событий.
*/

#include "B1EventAction.hh"

#include "B1RunAction.hh"

#include "G4Event.hh"
#include "G4ios.hh"

#include "G4VVisManager.hh"
#include "G4Circle.hh"
#include "G4Polyline.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4Point3D.hh"

#include "G4TrajectoryContainer.hh"
#include "G4Trajectory.hh"
#include "G4VTrajectory.hh"
#include "G4VTrajectoryPoint.hh"

B1EventAction::B1EventAction(B1RunAction* runAction, G4int sourceSampleN)
  : G4UserEventAction()
  , fRunAction(runAction)
  , fSourceSampleN(sourceSampleN)
{
}

void B1EventAction::BeginOfEventAction(const G4Event*)
{
}

void B1EventAction::EndOfEventAction(const G4Event* event)
{
  const G4int id = event->GetEventID() + 1;
  if (id % 1000000 == 0) {
    G4cout << "Timepix detector: processed event " << id << G4endl;
  }

  if (fSourceSampleN <= 0) return;
  if (event->GetEventID() >= fSourceSampleN) return;

  auto* vis = G4VVisManager::GetConcreteInstance();
  if (!vis) return;

  // Точки первичных вершин
  const auto* vtx = event->GetPrimaryVertex();
  if (vtx) {
    const auto pos = vtx->GetPosition();
    G4Circle circle(G4Point3D(pos.x(), pos.y(), pos.z()));
    circle.SetScreenSize(3.0);
    circle.SetFillStyle(G4Circle::filled);
    G4VisAttributes att(G4Colour(1.0, 1.0, 0.0));
    circle.SetVisAttributes(att);
    vis->Draw(circle); // Передача по значению/ссылке
  }

  // Траектории
  auto* trajectories = event->GetTrajectoryContainer();
  if (!trajectories) return;

  for (size_t i = 0; i < trajectories->entries(); ++i) {
    auto* traj = (*trajectories)[i];
    if (!traj) continue;

    // В Geant4 11+ GetPoint() требует индекс
    const G4int nPoints = traj->GetPointEntries();
    if (nPoints < 2) continue;

    G4Polyline polyline;
    for (G4int j = 0; j < nPoints; ++j) {
      const auto* p = traj->GetPoint(j);
      if (p) {
        polyline.push_back(G4Point3D(p->GetPosition()));
      }
    }

    G4VisAttributes trajVis(G4Colour(0.0, 1.0, 0.0));
    trajVis.SetForceSolid(true);
    polyline.SetVisAttributes(trajVis);
    vis->Draw(polyline); // Передача по значению/ссылке
  }
}
