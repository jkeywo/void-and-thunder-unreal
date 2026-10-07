#include "VTGameplay.h"
#include "VTCombat.h"
#include "Engine/Engine.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace {
struct FAIFixture {
 UWorld* World;
 UVTSimulation* Sim;
 AVTShip* Pilot;
 AVTShip* Target;
 AVTShipAI* Controller;
 FAIFixture() {
  World=UWorld::CreateWorld(EWorldType::Game,false,FName("VTAIExpectation"));
  GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
  World->SetGameInstance(NewObject<UVTGameInstance>(GEngine));World->SetGameMode(FURL());World->InitializeActorsForPlay(FURL());World->BeginPlay();
  Sim=World->GetSubsystem<UVTSimulation>();Sim->Bootstrap(0);
  FVTMotion M;M.Position=FVector2D(500,-500);
  Pilot=Sim->SpawnShip("corsair_cruiser",0,M,true,"Freebooters");M.Position+=FVector2D(0,250);
  Target=Sim->SpawnShip("house_patrol",0,M,true,"Guild");
  Sim->SystemShips.SetNum(Sim->Data->Systems.Num());Sim->SystemShips[0]={Pilot,Target};
  Controller=Cast<AVTShipAI>(Pilot->GetController());
  if(!Controller){Controller=World->SpawnActor<AVTShipAI>();Controller->Possess(Pilot);}
  Pilot->Fit.CrewedDevices.Reset();Pilot->Definition.AIAbilities=true;Pilot->Definition.AIEngageRange=400;
 }
 ~FAIFixture(){World->BeginTearingDown();World->EndPlay(EEndPlayReason::Quit);World->DestroyWorld(false);GEngine->DestroyWorldContext(World);}
 void Settle(){for(int I=0;I<64;++I)Controller->Decide(VT::Step);}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTAIExpectations,"VT.AI.LegacyBehaviorExpectations",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTAIExpectations::RunTest(const FString& Params) {
 // Independent behavioral expectations from the pinned pilot.rs tests. These are
 // behavior assertions, not a claim that all legacy score formulas are identical.
 {
  FAIFixture F;F.Controller->Decide(VT::Step);
  TestEqual("Healthy beam contact selects broadside",F.Pilot->Brain.Action,0);
  TestTrue("Beam contact fires port bank",bool(F.Pilot->Intent.Buttons&VTButtons::Port));
  F.Target->Movement->Motion.Position=F.Pilot->Movement->Motion.Position+FVector2D(250,0);F.Pilot->Brain=FVTBrainState();F.Settle();
  TestFalse("EMP does not take the helm from ready broadside",F.Pilot->Brain.Action==5);
 }
 {
  FAIFixture F;F.Target->Docked=true;F.Settle();
  TestEqual("Empty field holds station",F.Pilot->Brain.Action,-1);
  TestEqual("Empty field has no throttle",F.Pilot->Intent.Throttle,0.f);
  TestEqual("Empty field has no fire orders",F.Pilot->Intent.Buttons,uint16(0));
 }
 {
  FAIFixture F;F.Target->Disabled=true;F.Target->Movement->Motion.Position=F.Pilot->Movement->Motion.Position+FVector2D(50,0);F.Settle();
  TestEqual("Quiet nearby hulk selects boarding",F.Pilot->Brain.Action,2);
  TestTrue("Boarding thumb reaches interact",bool(F.Pilot->Intent.Buttons&VTButtons::Interact));
 }
 {
  FAIFixture F;F.Pilot->ShipRole=1;F.Target->Movement->Motion.Position=F.Pilot->Movement->Motion.Position+FVector2D(100,0);F.Settle();
  TestTrue("Civilian turns away from close foreign contact",FMath::Abs(F.Pilot->Intent.Turn)>0.9f);
  TestEqual("Civilian runs at full throttle",F.Pilot->Intent.Throttle,1.f);
  TestEqual("Civilian does not operate combat kit",F.Pilot->Intent.Buttons,uint16(0));
 }
 {
  FAIFixture F;F.Pilot->Fit.CrewedDevices={EVTDevice::Port,EVTDevice::Microwarp};F.Pilot->Intent.Throttle=0.35f;F.Pilot->Intent.Turn=-0.4f;F.Pilot->Intent.CursorOffset=FVector2D(23,7);
  AVTShipAI::CrewStep(F.Pilot);
  TestTrue("Independent crew fires its assigned beam mount",bool(F.Pilot->Intent.Buttons&VTButtons::Port));
  TestEqual("Crew preserves captain thrust",F.Pilot->Intent.Throttle,0.35f);TestEqual("Crew preserves captain turn",F.Pilot->Intent.Turn,-0.4f);
  TestTrue("Crew preserves captain aim cursor",F.Pilot->Intent.CursorOffset.Equals(FVector2D(23,7),0.001));
  TestFalse("Crew cannot engage microwarp",bool(F.Pilot->Intent.Buttons&VTButtons::Warp));
  F.Pilot->Disabled=true;F.Pilot->Intent.Buttons=0;AVTShipAI::CrewStep(F.Pilot);TestEqual("Disabled crew is inactive",F.Pilot->Intent.Buttons,uint16(0));
 }
 return true;
}
#endif
