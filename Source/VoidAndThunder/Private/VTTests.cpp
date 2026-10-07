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
 FVTMotion A; FVTMotion B; B.Position=FVector2D(0,150); B.Heading=-PI/2;
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
 TestTrue("Prior Unreal schema migrates",Save->Migrate(Previous)); TestEqual("Migration advances schema",Previous->Version,3);
 TestEqual("Missing pose clock uses snapshot boundary",Previous->Ships[0].Motion.SimulationTime,12.);
 Previous->Version=4; TestFalse("Unknown future schema is rejected",Save->Migrate(Previous));
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
 World->BeginTearingDown(); GI->Shutdown();
 IFileManager::Get().Delete(*Path); IFileManager::Get().Delete(*(Path+TEXT(".backup"))); IFileManager::Get().Delete(*(Path+TEXT(".tmp")));
 World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return true;
}
#endif // WITH_EDITOR
#endif // WITH_DEV_AUTOMATION_TESTS
