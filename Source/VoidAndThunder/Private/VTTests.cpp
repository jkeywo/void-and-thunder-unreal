#include "VTTypes.h"
#include "VTCombat.h"
#include "VTGameplay.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "VTSaveSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTThrottleTest,"VT.Flight.ThrustAndReverse",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTThrottleTest::RunTest(const FString& Params) {
 FVTShipStats S; S.Thrust=100; S.MaxSpeed=200; S.ForwardDrag=0; S.LateralDrag=0;
 FVTPilotIntent I; I.Throttle=1; FVTMotion Forward; VT::HelmStep(Forward,S,I,0.25f,1);
 TestTrue("Forward thrust matches Rust fixture",FMath::Abs(Forward.Velocity.X-100)<0.0001);
 I.Throttle=-1; FVTMotion Reverse; VT::HelmStep(Reverse,S,I,0.25f,1);
 TestTrue("Reverse matches Rust fixture",FMath::Abs(Reverse.Velocity.X+25)<0.0001); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTDragTest,"VT.Flight.AnisotropicDragAndClamp",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTDragTest::RunTest(const FString& Params) {
 FVTShipStats S; S.Thrust=0; S.MaxSpeed=1000; S.ForwardDrag=0.5f; S.LateralDrag=4;
 FVTMotion M; M.Velocity=FVector2D(100,100); VT::HelmStep(M,S,FVTPilotIntent(),0.25f,1);
 TestTrue("Along drag is exponential",FMath::Abs(M.Velocity.X-100*FMath::Exp(-0.5f))<0.001);
 TestTrue("Lateral drag bites harder",FMath::Abs(M.Velocity.Y-100*FMath::Exp(-4.f))<0.001);
 S.Thrust=10000; S.MaxSpeed=200; FVTPilotIntent I; I.Throttle=1;
 for(int N=0;N<1000;++N) VT::HelmStep(M,S,I,0.25f,VT::Step);
 TestTrue("Top speed is bounded",M.Velocity.Size()<=200.001); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTRandomTest,"VT.Migration.LegacySpawnJitter",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTRandomTest::RunTest(const FString& Params) {
 uint32 Seed=1; float First=VT::LcgNext(Seed); TestEqual("Wrapping LCG matches legacy state",Seed,1015568748u);
 TestTrue("Legacy jitter remains in unit range",First>=0&&First<1); TestTrue("Successive values vary",First!=VT::LcgNext(Seed)); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTInputTest,"VT.Network.RejectInvalidIntent",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTInputTest::RunTest(const FString& Params) {
 FVTPilotIntent I; TestTrue("Neutral intent valid",VT::ValidIntent(I));
 I.Throttle=2; TestFalse("Out of range throttle rejected",VT::ValidIntent(I));
 I.Throttle=0; I.Aim.X=std::numeric_limits<double>::quiet_NaN(); TestFalse("NaN aim rejected",VT::ValidIntent(I));
 TestFalse("Sequence jump without an outage is rejected",VT::SequenceAdvanceAllowed(400,64,0.1));
 TestTrue("Five-second outage can resume the 64 Hz stream",VT::SequenceAdvanceAllowed(400,64,5));
 TestFalse("Outage does not authorize an arbitrary sequence",VT::SequenceAdvanceAllowed(100000,64,5));
 TestFalse("Duplicate/outdated input remains rejected",VT::SequenceAdvanceAllowed(64,64,5));
 TestTrue("Sequence wrap advances safely",VT::SequenceAdvanceAllowed(8,0xfffffff0u,0.1));
 TestFalse("Invalid elapsed clock is rejected",VT::SequenceAdvanceAllowed(8,1,std::numeric_limits<double>::quiet_NaN()));
 I.Aim=FVector2D(0,1); I.Buttons=65535; TestFalse("Unknown device bits rejected",VT::ValidIntent(I)); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTArcTest,"VT.Combat.DirectionalShields",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTArcTest::RunTest(const FString& Params) {
 TestEqual("Dome answers everywhere",VTCombat::ShieldArc(0,FVector2D(-1,0),1),0);
 TestEqual("Two bank bow",VTCombat::ShieldArc(0,FVector2D(1,0),2),0);
 TestEqual("Two bank stern",VTCombat::ShieldArc(0,FVector2D(-1,0),2),1);
 TestEqual("Quadrant port",VTCombat::ShieldArc(0,FVector2D(0,1),4),2);
 TestEqual("Quadrant starboard",VTCombat::ShieldArc(0,FVector2D(0,-1),4),3);
 TestEqual("45 degree boundary resolves bow",VTCombat::ShieldArc(0,FVector2D(1,1),4),0);
 TestEqual("Rotated stern",VTCombat::ShieldArc(PI/2,FVector2D(0,-1),4),1); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTBallisticsTest,"VT.Combat.ArcAndSweptContact",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTBallisticsTest::RunTest(const FString& Params) {
 auto Port=VTCombat::BroadsideDirection(0,true,FVector2D(1,0),PI/4);
 TestTrue("Aim clamped to beam arc",Port.Equals(FVector2D(FMath::Cos(PI/4),FMath::Sin(PI/4)),0.0001));
 auto Star=VTCombat::BroadsideDirection(0,false,FVector2D::ZeroVector,PI/4);
 TestTrue("Neutral aim fires starboard beam",Star.Equals(FVector2D(0,-1),0.0001));
 TestEqual("Swept shot sees hull between endpoints",VTCombat::SegmentDistanceSquared(FVector2D(-100,0),FVector2D(100,0),FVector2D(0,2)),4.f);
 TestEqual("Contact before segment clamps to start",VTCombat::SegmentDistanceSquared(FVector2D(0,0),FVector2D(10,0),FVector2D(-2,0)),4.f); return true;
}
#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTCombatIntegrationTest,"VT.Combat.AuthoritativeVolley",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTCombatIntegrationTest::RunTest(const FString& Params) {
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,FName("VTCombatTest"));
 auto& Context=GEngine->CreateNewWorldContext(EWorldType::Game); Context.SetCurrentWorld(World);
 World->SetGameInstance(NewObject<UVTGameInstance>(GEngine)); World->SetGameMode(FURL()); World->InitializeActorsForPlay(FURL()); World->BeginPlay();
 auto* Sim=World->GetSubsystem<UVTSimulation>(); Sim->Bootstrap(0);
 TestNotNull("Native cruiser definition imported",Sim->Data->FindShip("corsair_cruiser"));
 AddInfo(FString::Printf(TEXT("Native asset shield %.3f count %d"),Sim->Data->Ships[0].ShieldMax.X,Sim->Data->Ships.Num()));
 FVTMotion A; A.Position=FVector2D(500,-500); FVTMotion B; B.Position=FVector2D(500,-350); B.Heading=-PI/2;
 auto* Shooter=Sim->SpawnShip("corsair_cruiser",0,A,false,"Corsairs");
 auto* Target=Sim->SpawnShip("corsair_cruiser",0,B,false,"Corsairs");
 Shooter->Intent.Aim=FVector2D(0,1); Shooter->Intent.Buttons=VTButtons::Port;
 for(int I=0;I<64;++I) Sim->FixedStep();
 AddInfo(FString::Printf(TEXT("Target hull %.3f shields %.3f %.3f max %.3f heading %.3f damage %.3f"),Target->Attributes->Hull.GetCurrentValue(),Target->Combat->Shields.X,Target->Combat->Shields.Y,Target->Definition.ShieldMax.X,Target->Movement->Motion.Heading,Shooter->Definition.Damage));
 TestTrue("Same faction players can deal damage",Target->Attributes->Hull.GetCurrentValue()<Target->Definition.Hull);
 TestTrue("Fore bank absorbs exactly 22.5 of 36 damage",FMath::Abs(Target->Attributes->Hull.GetCurrentValue()-36.5f)<0.001f);
 TestTrue("Stern bank remains full",FMath::Abs(Target->Combat->Shields[1]-22.5f)<0.001f);
 TestTrue("GAS activation owns independent cooldown",Shooter->PortReload>8.9f&&Shooter->StarboardReload==0);
 TestEqual("One volley resolves once",Sim->Projectiles.Num(),0);
 World->BeginTearingDown(); World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTSaveTest,"VT.Persistence.AtomicSnapshotAndFallback",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTSaveTest::RunTest(const FString& Params) {
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,FName("VTPersistenceTest"));
 auto& Context=GEngine->CreateNewWorldContext(EWorldType::Game); Context.SetCurrentWorld(World);
 auto* GI=NewObject<UVTGameInstance>(GEngine); GI->InitializeHeadlessWorld(World);
 World->SetGameMode(FURL()); World->InitializeActorsForPlay(FURL()); World->BeginPlay();
 auto* Sim=World->GetSubsystem<UVTSimulation>(); Sim->Bootstrap(0);
 auto* Save=GI->GetSubsystem<UVTSaveSubsystem>();
 auto* Previous=NewObject<UVTWorldSave>(); Previous->Version=2; Previous->SimulationTime=12; FVTSavedShip OldShip; Previous->Ships.Add(OldShip);
 TestTrue("Prior Unreal schema migrates",Save->Migrate(Previous)); TestEqual("Migration advances schema",Previous->Version,5);
 TestEqual("Missing pose clock uses snapshot boundary",Previous->Ships[0].Motion.SimulationTime,12.);
 Previous->Version=6; TestFalse("Unknown future schema is rejected",Save->Migrate(Previous));
 Save->Slot=TEXT("Automation-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
 FVTMotion M; auto* Shooter=Sim->SpawnShip("corsair_cruiser",0,M,false,"Corsairs"); Shooter->IsNPC=true;
 M.Position=FVector2D(700,700); auto* Target=Sim->SpawnShip("house_patrol",0,M,false,"Guild"); Target->IsNPC=true;
 Target->Combat->Damage(4,M.Position,Shooter);
 Shooter->Intent.Buttons=VTButtons::Port; Shooter->Intent.Aim=FVector2D(0,1); Sim->FixedStep();
 FGuid SourceID=Shooter->PersistentId,TargetID=Target->PersistentId;
 TestTrue("Snapshot written",Save->Save());
 Target->Combat->Damage(3,M.Position,Shooter); TestTrue("Second snapshot creates backup",Save->Save());
 TestTrue("Snapshot restored",Save->Load());
 AVTShip* Restored=Sim->Ships.FindByPredicate([TargetID](const TObjectPtr<AVTShip>& S){return S->PersistentId==TargetID;})->Get();
 TestTrue("Hull round trip",FMath::Abs(Restored->Attributes->Hull.GetCurrentValue()-93)<0.001);
 TestEqual("Active projectiles round trip",Sim->Projectiles.Num(),3);
 for(AVTProjectile* P:Sim->Projectiles) {TestEqual("Durable source ID retained",P->SourceId,SourceID); TestTrue("Source reference resolves",IsValid(P->Source)&&P->Source->PersistentId==SourceID);}
 auto* RestoredShooter=Sim->Ships.FindByPredicate([SourceID](const TObjectPtr<AVTShip>& S){return S->PersistentId==SourceID;})->Get();
 TestTrue("Held input cancelled",RestoredShooter->Intent.Buttons==0);
 TestTrue("Cooldown restored without firing",RestoredShooter->PortReload>9.9f);
 const FString Path=FPaths::ProjectSavedDir()/TEXT("SaveGames")/(Save->Slot+TEXT(".vts"));
 FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Path,true);
 Restored->Combat->Damage(1,Restored->Movement->Motion.Position,RestoredShooter);
 TestFalse("Failed replacement reports failure",Save->Save()); FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Path,false);
 TestTrue("Prior save survives failed replacement",Save->Load());
 TArray<uint8> Bytes; FFileHelper::LoadFileToArray(Bytes,*Path); Bytes.Last()^=0xff; FFileHelper::SaveArrayToFile(Bytes,*Path);
 TestTrue("Corrupt primary uses last good backup",Save->Load());
 World->BeginTearingDown(); TestFalse("Cleanup cannot replace final world snapshot",Save->Save()); GI->Shutdown();
 IFileManager::Get().Delete(*Path); IFileManager::Get().Delete(*(Path+TEXT(".backup"))); IFileManager::Get().Delete(*(Path+TEXT(".tmp")));
 World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTEquipmentTest,"VT.Combat.EquipmentLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTEquipmentTest::RunTest(const FString& Params) {
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,FName("VTEquipmentTest"));
 auto& Context=GEngine->CreateNewWorldContext(EWorldType::Game); Context.SetCurrentWorld(World);
 World->SetGameInstance(NewObject<UVTGameInstance>(GEngine)); World->SetGameMode(FURL()); World->InitializeActorsForPlay(FURL()); World->BeginPlay();
 auto* Sim=World->GetSubsystem<UVTSimulation>(); Sim->Bootstrap(0);
 FVTMotion M; M.Position=FVector2D(500,-500); auto* Ship=Sim->SpawnShip("corsair_cruiser",0,M,false,"Corsairs"); M.Position=FVector2D(600,-500); auto* Target=Sim->SpawnShip("house_patrol",0,M,false,"Houses");
 TestTrue("Imported cruiser fits disruptor and torpedoes",Ship->Definition.Equipment.EMP&&Ship->Definition.Equipment.Torpedoes);
 TestTrue("Imported default excludes alternative battery devices",!Ship->Definition.Equipment.Boost&&!Ship->Definition.Equipment.PointDefense);
 Ship->Definition.Equipment.Boost=true; Ship->Definition.Equipment.PointDefense=true; Ship->Definition.Equipment.Mines=true; Ship->Definition.Equipment.Warp=true;
 Ship->Abilities->SetNumericAttributeBase(UVTAttributes::BatteryAttribute(),0.01f); Ship->Intent.Buttons=VTButtons::Boost|VTButtons::EMP|VTButtons::PointDefense; Ship->Combat->SystemsStep();
 TestTrue("Partial final battery step powers boost first",Ship->Combat->BoostPowered);
 TestFalse("Later device cannot spend exhausted charge",Ship->Combat->EMPPowered); TestFalse("Point defence shares the same exhausted pool",Ship->Combat->PDPowered);
 TestEqual("Drawing does not also recharge",Ship->Attributes->Battery.GetCurrentValue(),0.f);
 Ship->Intent.Buttons=0; Ship->Combat->SystemsStep(); TestTrue("Idle battery recharges",Ship->Attributes->Battery.GetCurrentValue()>0);
 Ship->Abilities->SetNumericAttributeBase(UVTAttributes::GetEMPStressAttribute(),50); Ship->Combat->SystemsStep(); TestTrue("EMP scales drive and recovers",Ship->Combat->SpeedScale>0.5f&&Ship->Combat->SpeedScale<0.51f);
 Ship->Abilities->SetNumericAttributeBase(UVTAttributes::GetEMPStressAttribute(),0); Ship->Abilities->SetNumericAttributeBase(UVTAttributes::BatteryAttribute(),3);
 Ship->Intent.Buttons=VTButtons::EMP; Ship->Intent.CursorOffset=FVector2D(100,0); Sim->FixedStep(); Ship->Intent.Buttons=0;
 for(int I=0;I<24;++I) Sim->FixedStep(); TestTrue("EMP projectile changes GAS stress",Target->Attributes->EMPStress.GetCurrentValue()>20); TestEqual("EMP bypasses hull damage",Target->Attributes->Hull.GetCurrentValue(),Target->Definition.Hull);
 Ship->Intent.Buttons=VTButtons::Torpedo; Sim->FixedStep(); TestEqual("First torpedo lock is immediate",Ship->Combat->EquipmentState.Locks.Num(),1);
 Ship->Intent.Buttons=0; Sim->FixedStep(); TestEqual("Release commits one launch",Ship->Combat->EquipmentState.LaunchQueue.Num(),1);
 Sim->FixedStep(); TestEqual("Queue drains on following step",Ship->Combat->EquipmentState.LaunchQueue.Num(),0);
 AVTProjectile* Torp=nullptr; for(AVTProjectile* Shot:Sim->Projectiles) if(Shot->Kind==EVTProjectileKind::Torpedo) Torp=Shot;
 TestNotNull("Committed launch creates torpedo",Torp); if(Torp) {TestEqual("Torpedo retains target ID",Torp->TargetId,Target->PersistentId); TestTrue("Torpedo arcs off the plane",FMath::Abs(Torp->Height)>0);}
 int Ammo=Ship->Combat->EquipmentState.MineMagazine; Ship->Intent.Buttons=VTButtons::Mine; Sim->FixedStep(); TestEqual("Mine expends one round",Ship->Combat->EquipmentState.MineMagazine,Ammo-1); Sim->FixedStep(); TestEqual("GAS cooldown prevents repeated mine in same cadence",Ship->Combat->EquipmentState.MineMagazine,Ammo-1);
 Ship->Intent.Buttons=VTButtons::Warp; Ship->Intent.CursorOffset=FVector2D(900,0); Sim->FixedStep(); FVector2D Before=Ship->Movement->Motion.Position; Ship->Intent.Buttons=0; Sim->FixedStep(); TestTrue("Warp release clamps range",FMath::Abs((Ship->Movement->Motion.Position-Before).Size()-506)<0.1); TestTrue("Warp GAS cooldown is active",Ship->Combat->EquipmentState.WarpCooldown>19.9f);
 FVTPilotIntent Invalid; Invalid.CursorOffset=FVector2D(1301,0); TestFalse("Unbounded cursor request rejected",VT::ValidIntent(Invalid));
 World->BeginTearingDown(); World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTWorldTest,"VT.World.SandboxOwnershipAndTravel",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTWorldTest::RunTest(const FString& Params) {
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,FName("VTWorldTest")); auto& Context=GEngine->CreateNewWorldContext(EWorldType::Game); Context.SetCurrentWorld(World);
 auto* GI=NewObject<UVTGameInstance>(GEngine); GI->InitializeHeadlessWorld(World); World->SetGameMode(FURL()); World->InitializeActorsForPlay(FURL()); World->BeginPlay(); auto* Sim=World->GetSubsystem<UVTSimulation>(); Sim->Bootstrap(0);
 auto* A=World->SpawnActor<AVTController>(); auto* B=World->SpawnActor<AVTController>(); auto* AP=A->GetPlayerState<AVTPlayerState>(); auto* BP=B->GetPlayerState<AVTPlayerState>();
 TestNotNull("First captain has PlayerState",AP); TestNotNull("Second captain has PlayerState",BP);
 if(!AP||!BP) {GI->Shutdown(); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return false;}
 AP->Profile=FGuid::NewGuid(); BP->Profile=FGuid::NewGuid(); AP->Reputation=Sim->Data->InitialReputation; BP->Reputation=Sim->Data->InitialReputation; AP->Heat.Init(0,8); BP->Heat.Init(0,8);
 FVTMotion M; M.Position=FVector2D(500,0); auto* AS=Sim->SpawnShip("corsair_cruiser",0,M,false,"Corsairs"); A->Possess(AS); M.Position=FVector2D(500,110); auto* BS=Sim->SpawnShip("corsair_cruiser",0,M,false,"Corsairs"); B->Possess(BS);
 M.Position=FVector2D(500,55); auto* Prize=Sim->SpawnShip("house_patrol",0,M,false,"Guild"); Prize->IsNPC=true; Prize->Disabled=true;
 AS->Intent.Buttons=VTButtons::Interact; BS->Intent.Buttons=VTButtons::Interact;
 for(int I=0;I<200;++I) Sim->FixedStep(); TestEqual("Contested prize awarded exactly once",AP->Boarded+BP->Boarded,1); TestEqual("One bounty credited",AP->Credits+BP->Credits,Sim->Data->Rules.BoardingBounty);
 AS->Intent=FVTPilotIntent(); BS->Intent=FVTPilotIntent(); M.Position=FVector2D(800,0); auto* Civilian=Sim->SpawnShip("house_patrol",1,M,false,"Guild"); Civilian->IsNPC=true; Civilian->ShipRole=1;
 M.Position=FVector2D(700,0); auto* Patrol=Sim->SpawnShip("house_patrol",1,M,false,"Guild"); Patrol->IsNPC=true; Patrol->ShipRole=2;
 Civilian->Combat->Damage(1,M.Position,AS); int Guild=Sim->Data->FactionIndex("Guild"); TestTrue("Crime attributed to attacking captain",AP->Heat[Guild]>0); TestEqual("Other captain heat unchanged",BP->Heat[Guild],0.f);
 for(int I=0;I<200;++I) Sim->FixedStep(); TestTrue("Distress broadcasts in an unoccupied system",Civilian->Brain.DistressSent); TestTrue("Background patrol receives distress",Patrol->Brain.AlertTTL>0);
 float Rep=AP->Reputation[Guild]; float Heat=AP->Heat[Guild]; Sim->RecordHit(Civilian,AS,30); Sim->FixedStep(); TestTrue("Crime heat is personal and decays",AP->Heat[Guild]>Heat); TestTrue("Decay converts to persistent standing loss",AP->Reputation[Guild]<Rep);
 int Origin=Sim->Data->FindSystem(Sim->Data->StartSystem); AS->SystemIndex=Origin; auto Destination=Sim->Data->Systems[Origin].Links[0]; AS->Movement->Motion.Position=Sim->JumpPosition(Origin,Destination); AS->Movement->Motion.Velocity=FVector2D::ZeroVector;
 AS->Intent.Buttons=VTButtons::Interact; for(int I=0;I<64;++I) Sim->FixedStep(); float Progress=AS->JumpProgress; AS->Intent.Buttons=0; for(int I=0;I<64;++I) Sim->FixedStep(); TestEqual("Released jump input freezes dwell",AS->JumpProgress,Progress);
 AS->Intent.Buttons=VTButtons::Interact; for(int I=0;I<64*30&&AS->SystemIndex==Origin;++I) Sim->FixedStep(); TestEqual("Only initiating ship transfers",AS->SystemIndex,Sim->Data->FindSystem(Destination)); TestEqual("Other player remains in own system",BS->SystemIndex,0);
 FVTLoadoutSelection Bad; Bad.Battery="invented"; TestFalse("Server rejects unknown equipment",AS->ApplyFit(Bad,true));
 FVTLoadoutSelection Multi; Multi.Batteries={FName("loadout.disruptor"),FName("loadout.boost"),FName("loadout.point_defense")}; Multi.Specials={FName("loadout.torpedoes"),FName("loadout.microwarp"),FName("loadout.mines")}; FVTShipDefinition Fit;
 TestTrue("Battleship accepts three authored mounts",Sim->Data->ResolveFit("corsair_battleship",Multi,Fit)); TestFalse("Frigate rejects oversized fit",Sim->Data->ResolveFit("corsair_frigate",Multi,Fit)); TestTrue("Surplus mount gives crew the screen",Sim->Data->CrewForFit("corsair_battleship",Multi).Contains(EVTDevice::PointDefense));
 GI->GetSubsystem<UVTSaveSubsystem>()->Slot=TEXT("AutomationWorld-")+FGuid::NewGuid().ToString(); World->BeginTearingDown(); GI->Shutdown(); World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTScenarioTest,"VT.World.SoloScenarioParity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTScenarioTest::RunTest(const FString& Params) {
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,FName("VTScenarioTest")); auto& Context=GEngine->CreateNewWorldContext(EWorldType::Game); Context.SetCurrentWorld(World); auto* GI=NewObject<UVTGameInstance>(GEngine); World->SetGameInstance(GI); GI->PlayMode=TEXT("skirmish"); World->SetGameMode(FURL()); World->InitializeActorsForPlay(FURL()); World->BeginPlay(); auto* Sim=World->GetSubsystem<UVTSimulation>(); Sim->Bootstrap(0);
 FVTMotion M; M.Position=FVector2D(0,-520); auto* SoloShip=Sim->SpawnShip("corsair_cruiser",0,M,false,"Corsairs"); SoloShip->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),1); auto* State=World->GetGameState<AVTGameState>(); Sim->FixedStep(); TestEqual("Skirmish starts with authored wave one",State->Wave,1); TestEqual("First wave has two ships",State->EnemiesRemaining,2); TestFalse("Solo captain can fight at low hull as in the source",SoloShip->Disabled);
 for(int Wave=1;Wave<=3;++Wave) {auto Current=Sim->Ships; for(AVTShip* S:Current) if(S->IsNPC) {if(S->Controller) S->Controller->Destroy(); S->Destroy();} Sim->FixedStep(); if(Wave==1) TestEqual("Second wave adds one ship",State->EnemiesRemaining,3); if(Wave==2) {TestEqual("Finale replaces regular wave",State->EnemiesRemaining,1); for(AVTShip* S:Sim->Ships) if(S->IsNPC) TestEqual("Finale is authored bastion",S->ClassId,FName("house_bastion"));}}
 TestEqual("Clearing all waves reports victory",State->Outcome,FString(TEXT("Skirmish cleared")));
 auto Current=Sim->Ships; for(AVTShip* S:Current) {if(S->Controller) S->Controller->Destroy(); S->Destroy();} GI->PlayMode=TEXT("range"); Sim->SoloSpawned=false; State->Outcome.Reset(); State->Wave=0; Sim->SpawnShip("corsair_cruiser",0,M,false,"Corsairs"); Sim->FixedStep();
 TestEqual("Test Range has no waves",State->Wave,0); TestEqual("Test Range places one target",State->EnemiesRemaining,1); for(AVTShip* S:Sim->Ships) if(S->IsNPC) TestTrue("Range target is anchored and invulnerable",S->Anchored&&S->Invulnerable&&S->Controller==nullptr);
 World->BeginTearingDown(); World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTBoundaryTest,"VT.World.LandmarksAndIntentAuthority",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTBoundaryTest::RunTest(const FString& Params) {
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,FName("VTBoundaryTest")); auto& Context=GEngine->CreateNewWorldContext(EWorldType::Game); Context.SetCurrentWorld(World); World->SetGameInstance(NewObject<UVTGameInstance>(GEngine)); World->SetGameMode(FURL()); World->InitializeActorsForPlay(FURL()); World->BeginPlay(); auto* Sim=World->GetSubsystem<UVTSimulation>(); Sim->Bootstrap(0);
 FVTMotion M; M.Position=FVector2D(50,0); M.Velocity=FVector2D(-20,5); auto* Ship=Sim->SpawnShip("corsair_cruiser",0,M,false,"Corsairs"); Sim->LandmarkStep();
 TestTrue("Star separates hull from solid surface",FMath::Abs(Ship->Movement->Motion.Position.Size()-(120+Ship->Definition.Radius))<0.001); TestTrue("Landmark removes inward velocity",FMath::Abs(Ship->Movement->Motion.Velocity.X)<0.001); TestEqual("Tangential velocity retained",Ship->Movement->Motion.Velocity.Y,5.);
 TestTrue("Star blocks scanner line of sight",Sim->Occluded(0,FVector2D(-200,0),FVector2D(200,0))); TestFalse("Clear scanner ray is unblocked",Sim->Occluded(0,FVector2D(-200,-400),FVector2D(200,-400)));
 auto* Shot=Ship->Combat->SpawnDeviceProjectile(EVTProjectileKind::Cannon,FVector2D(-200,0),FVector2D(30000,0),1,3,6); Sim->ProjectileStep(); TestFalse("Swept projectile cannot pass through star",IsValid(Shot));
 FVTPilotIntent Intent; Intent.Sequence=1; Intent.Throttle=0.5f; Ship->ServerIntent_Implementation(Intent); TestEqual("Valid intent is queued",Ship->InputQueue.Num(),1); Ship->ServerIntent_Implementation(Intent); TestEqual("Replay cannot enqueue twice",Ship->InputQueue.Num(),1);
 Intent.Sequence=2; Intent.Throttle=2; Ship->ServerIntent_Implementation(Intent); TestEqual("Invalid throttle rejected at authority",Ship->LastReceived,uint32(1)); Intent.Throttle=0.5f; Intent.Sequence=300; Ship->ServerIntent_Implementation(Intent); TestEqual("Sequence jump rejected",Ship->LastReceived,uint32(1));
 TArray<FVTPilotIntent> Oversized; Oversized.Init(Intent,13); Ship->ServerIntentBatch_Implementation(Oversized); TestEqual("Oversized batch rejected",Ship->InputQueue.Num(),1);
 Sim->FixedStep(); TestEqual("Acknowledgement follows consumed intent",Ship->Movement->Authority.Ack,uint32(1)); TestEqual("Consumed command removed",Ship->InputQueue.Num(),0);
 Sim->FixedStep(); auto* State=World->GetGameState<AVTGameState>(); TestEqual("Population summary does not accumulate",State->Populations[0],1);
 TestEqual("String table resolves before menu choices are built",FText::FromStringTable(FName(TEXT("/Game/UI/ST_UI.ST_UI")),TEXT("class.corsair_cruiser.name"),EStringTableLoadingPolicy::FindOrFullyLoad).ToString(),FString(TEXT("Cruiser")));
 Ship->Docked=true; auto Position=Ship->Movement->Motion.Position; Intent.Sequence=2; Intent.Throttle=1; Ship->ServerIntent_Implementation(Intent); Sim->FixedStep(); TestEqual("Docked ship acknowledges intent",Ship->Movement->Authority.Ack,uint32(2)); TestTrue("Docking keeps pose frozen",Ship->Movement->Motion.Position.Equals(Position,0.001));
 World->BeginTearingDown(); World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTIdentityTest,"VT.Network.IdentityReconnectAndRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTIdentityTest::RunTest(const FString& Params) {
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,FName("VTIdentityTest")); auto& Context=GEngine->CreateNewWorldContext(EWorldType::Game); Context.SetCurrentWorld(World); auto* GI=NewObject<UVTGameInstance>(GEngine); GI->InitializeHeadlessWorld(World); World->SetGameMode(FURL()); World->InitializeActorsForPlay(FURL()); World->BeginPlay(); auto* Sim=World->GetSubsystem<UVTSimulation>(); Sim->Bootstrap(0); auto* Save=GI->GetSubsystem<UVTSaveSubsystem>(); Save->Slot=TEXT("AutomationIdentity-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
 auto* Captain=World->SpawnActor<AVTController>(); FGuid Profile=FGuid::NewGuid(); Captain->ServerIdentify_Implementation(Profile,FGuid(),FName("corsair_frigate"),FVTLoadoutSelection()); auto* PS=Captain->GetPlayerState<AVTPlayerState>(); auto* Ship=Cast<AVTShip>(Captain->GetPawn()); TestTrue("New profile receives a ship and identity",PS&&PS->Profile==Profile&&Ship); if(!Ship) return false;
 TestEqual("New guest owns its selected hull",Ship->ClassId,FName("corsair_frigate"));
 auto* Bad=World->SpawnActor<AVTController>(); Bad->ServerIdentify_Implementation(FGuid::NewGuid(),FGuid(),FName("house_patrol"),FVTLoadoutSelection()); TestFalse("New guest cannot request an NPC hull",Bad->GetPlayerState<AVTPlayerState>()->Profile.IsValid()); Bad->Destroy();
 PS->Credits=321; PS->Boarded=9; Ship->Movement->Motion.Position=FVector2D(500,-500); Save->CapturePlayer(Captain); FGuid Id=Ship->PersistentId; FGuid Token=Save->PlayerRecords[0].Token;
 auto* Duplicate=World->SpawnActor<AVTController>(); Duplicate->ServerIdentify_Implementation(Profile,Token,FName("corsair_cruiser"),FVTLoadoutSelection()); TestFalse("Simultaneous duplicate profile rejected",Duplicate->GetPlayerState<AVTPlayerState>()->Profile.IsValid()); TestNull("Rejected identity gets no ship",Duplicate->GetPawn()); Duplicate->Destroy();
 Captain->Destroy(); TestFalse("Disconnect removes original ship",IsValid(Ship));
 FVTMotion M; M.Position=FVector2D(600,-500); auto* Victim=Sim->SpawnShip("house_patrol",0,M,true,"Guild");
 int Guild=Sim->Data->FactionIndex("Guild"); Sim->RecordHit(Victim,nullptr,10,Profile,FName("Corsairs"));
 TestEqual("In-flight damage remains attributed after disconnect",Save->PlayerRecords[0].Heat[Guild],4.f); TestEqual("In-flight attacker faction survives pawn destruction",Victim->Brain.LastAttackerFaction,FName("Corsairs"));
 auto* Aggressor=Sim->SpawnShip("house_patrol",0,M,true,"Janissariat"); Sim->RecordHit(Victim,Aggressor,1); Aggressor->Brain.LastAttackerProfile=Profile;
 float Reputation=Save->PlayerRecords[0].Reputation[Guild]; Sim->AwardAvenging(Aggressor,Sim->Ships); TestEqual("Offline captain receives rescue standing",Save->PlayerRecords[0].Reputation[Guild],Reputation+Sim->Data->World.avenge_reputation_bonus);

 auto* Returning=World->SpawnActor<AVTController>(); Returning->ServerIdentify_Implementation(Profile,FGuid::NewGuid(),FName("corsair_cruiser"),FVTLoadoutSelection()); TestFalse("Wrong reconnect token rejected",Returning->GetPlayerState<AVTPlayerState>()->Profile.IsValid()); Returning->ServerIdentify_Implementation(Profile,Token,FName("corsair_cruiser"),FVTLoadoutSelection()); auto* Restored=Cast<AVTShip>(Returning->GetPawn()); auto* Returned=Returning->GetPlayerState<AVTPlayerState>(); TestNotNull("Valid token restores captain",Restored);
 if(Restored) {TestEqual("Reconnect retains durable ship ID",Restored->PersistentId,Id); TestEqual("Saved hull overrides a new initial selection",Restored->ClassId,FName("corsair_frigate")); TestEqual("Fallback spawn retains authoritative saved hull",Returning->InitialHull,FName("corsair_frigate")); TestEqual("Reconnect retains personal credits",Returned->Credits,321); TestEqual("Reconnect retains prize tally",Returned->Boarded,9); TestTrue("Reconnect cancels held actions",Restored->Intent.Buttons==0&&Restored->InputQueue.IsEmpty());
  Restored->Disabled=true; Returning->ServerRecover_Implementation(); auto* Recovered=Cast<AVTShip>(Returning->GetPawn()); TestTrue("Defeat recovery chooses reachable station",Recovered&&Sim->Data->Systems[Recovered->SystemIndex].HasStation); if(Recovered) {TestTrue("Recovery places hull at station",(Recovered->Movement->Motion.Position-Sim->Data->Rules.StationPosition).Size()<Sim->Data->Rules.StationRadius+100); TestEqual("Recovery restores selected hull",Recovered->ClassId,FName("corsair_frigate")); TestEqual("Recovery applies no credit penalty",Returned->Credits,321); TestEqual("Recovery restores hull integrity",Recovered->Attributes->Hull.GetCurrentValue(),Recovered->Definition.Hull);}}
 FGuid Campaign=Save->WorldId; Save->StoreToken(Campaign,Token); Save->ResetWorldIdentity();
 TestTrue("Solo identity is independent of campaign",Save->WorldId!=Campaign); TestTrue("New world clears prior captain records",Save->PlayerRecords.IsEmpty());
 const auto* Prior=Save->Personal->Tokens.FindByPredicate([Campaign](const FVTReconnectToken& R){return R.World==Campaign;}); TestTrue("Campaign reconnect token survives solo reset",Prior&&Prior->Token==Token);
 World->BeginTearingDown(); TestFalse("Finalized world stays closed to saves",Save->Save()); GI->Shutdown(); World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return true;
}
#endif // WITH_EDITOR
#endif // WITH_DEV_AUTOMATION_TESTS
