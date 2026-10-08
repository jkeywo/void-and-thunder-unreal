#include "VTGameplay.h"
#include "VTCombat.h"
#include "VTUI.h"
#include "VTSaveSubsystem.h"
#include "Components/ComboBoxString.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace {
struct FNativeFixture {
 UWorld* World; UVTGameInstance* GI; UVTSimulation* Sim; AVTShip* Ship;
 FNativeFixture() {
  World=UWorld::CreateWorld(EWorldType::Game,false,FName("VTNativeIntegration"));
  auto& Context=GEngine->CreateNewWorldContext(EWorldType::Game); Context.SetCurrentWorld(World);
  GI=NewObject<UVTGameInstance>(GEngine); GI->InitializeHeadlessWorld(World); GI->PlayMode=TEXT("sandbox");
  World->SetGameMode(FURL()); World->InitializeActorsForPlay(FURL()); World->BeginPlay();
  Sim=World->GetSubsystem<UVTSimulation>(); Sim->Bootstrap(0); FVTMotion M;M.Position=FVector2D(500,-500);
  Ship=Sim->SpawnShip(TEXT("corsair_cruiser"),0,M,true,TEXT("Corsairs"));
  if(Ship->Controller) Ship->Controller->Destroy(); Ship->Anchored=true; Ship->Invulnerable=true;
 }
 ~FNativeFixture() {World->BeginTearingDown();GI->Shutdown();World->EndPlay(EEndPlayReason::Quit);World->DestroyWorld(false);GEngine->DestroyWorldContext(World);}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTNativeCooldown,"VT.Native.FixedStepGASLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTNativeCooldown::RunTest(const FString&) {
 FNativeFixture F; auto* S=F.Ship; S->Definition.Equipment.EMP=true; S->Definition.Equipment.EMPCooldown=0.5f;
 auto Tag=FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ship.EMP"));
 TestTrue(TEXT("Native activation commits"),S->Abilities->TryActivateAbilityByClass(UVTEMPAbility::StaticClass()));
 TestTrue(TEXT("GameplayEffect grants cooldown tag"),S->Abilities->HasMatchingGameplayTag(Tag));
 auto* Spec=S->Abilities->FindAbilitySpecFromClass(UVTEMPAbility::StaticClass()); auto* Ability=Cast<UVTDeviceAbility>(Spec->GetPrimaryInstance());
 for(int I=0;I<31;++I) Ability->FixedStep();
 TestTrue(TEXT("Cooldown survives 31 ordered steps"),Ability->IsActive());
 TestTrue(TEXT("Cooldown rejects a second activation"),!S->Abilities->TryActivateAbilityByClass(UVTEMPAbility::StaticClass()));
 Ability->FixedStep(); TestFalse(TEXT("32nd step expires cooldown effect"),S->Abilities->HasMatchingGameplayTag(Tag));
 TestFalse(TEXT("Task ends with ability"),Ability->IsActive());
 S->Combat->EquipmentState.EMPCooldown=0.25f; S->Combat->RestoreDevices();
 TestTrue(TEXT("Restore reinstates cooldown tag"),S->Abilities->HasMatchingGameplayTag(Tag));
 TestEqual(TEXT("Restore uses saved simulation duration"),Ability->CooldownTask->GetRemaining(),0.25f);
 for(int I=0;I<16;++I) Ability->FixedStep();
 S->Docked=true; S->Combat->EquipmentSystems();
 TestFalse(TEXT("Docking tag rejects equipment activation"),S->Abilities->TryActivateAbilityByClass(UVTEMPAbility::StaticClass()));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTNativeSave,"VT.Native.AsyncSaveRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTNativeSave::RunTest(const FString&) {
 FNativeFixture F; auto* Save=F.GI->GetSubsystem<UVTSaveSubsystem>(); Save->Slot=TEXT("NativeAsync-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
 TestTrue(TEXT("First snapshot commits"),Save->Save());
 F.Ship->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),25);
 TestTrue(TEXT("Second snapshot queues immutable bytes"),Save->Save(true));
 TestTrue(TEXT("Orderly flush commits pending write"),Save->FlushPendingSave());
 TestTrue(TEXT("Committed async snapshot loads"),Save->Load());
 TestEqual(TEXT("Async snapshot retains changed hull"),F.Sim->Ships[0]->Attributes->Hull.GetCurrentValue(),25.f);
 const FString Path=FPaths::ProjectSavedDir()/TEXT("SaveGames")/(Save->Slot+TEXT(".vts")); TArray<uint8> Broken={1,2,3};FFileHelper::SaveArrayToFile(Broken,*Path);
 TestTrue(TEXT("Corrupt committed save recovers last-good backup"),Save->Load());
 TestTrue(TEXT("Backup retains prior hull"),F.Sim->Ships[0]->Attributes->Hull.GetCurrentValue()>25);
 TestTrue(TEXT("Saving after fallback queues valid bytes"),Save->Save(true)); TestTrue(TEXT("Fallback writer flushes"),Save->FlushPendingSave());
 // Force replacement failure without touching an existing save or backup.
 const FString GoodSlot=Save->Slot; Save->Slot+=TEXT("-blocked"); const FString Blocked=FPaths::ProjectSavedDir()/TEXT("SaveGames")/(Save->Slot+TEXT(".vts"));IFileManager::Get().MakeDirectory(*Blocked,true);
 TestTrue(TEXT("Failed replacement is reported asynchronously"),Save->Save(true));
 AddExpectedError(TEXT("Background save failed"),EAutomationExpectedErrorFlags::Contains,1);
 TestFalse(TEXT("Write failure reaches caller"),Save->FlushPendingSave());
 IFileManager::Get().DeleteDirectory(*Blocked); Save->Slot=GoodSlot; TestTrue(TEXT("Previous committed slot remains loadable"),Save->Load());
 IFileManager::Get().Delete(*Path);IFileManager::Get().Delete(*(Path+TEXT(".backup")));IFileManager::Get().Delete(*(Blocked+TEXT(".tmp")));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTNativeChoices,"VT.Native.LocalizedSelectionIdentity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTNativeChoices::RunTest(const FString&) {
 auto* UI=NewObject<UVTUI>(); auto* Box=NewObject<UComboBoxString>(UI);
 const FText Label=NSLOCTEXT("VTTests","IdenticalName","Same translated name");
 UI->AddChoice(Box,TEXT("first"),Label);UI->AddChoice(Box,TEXT("second"),Label);
 Box->SetSelectedOption(TEXT("second"));TestEqual(TEXT("Identical labels retain distinct stable IDs"),UI->SelectedId(Box),FName("second"));
 TestTrue(TEXT("Option retains FText identity"),UI->ChoiceItems[FName("second")]->Label.IdenticalTo(Label));
 UI->AddChoice(Box,TEXT("__empty"),NSLOCTEXT("VTTests","Empty","Empty mount"));Box->SetSelectedOption(TEXT("__empty"));TestTrue(TEXT("Empty slot uses None identity"),UI->SelectedId(Box).IsNone());
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTNativeAbilityAuthority,"VT.Native.RejectDirectAbilityRPC",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTNativeAbilityAuthority::RunTest(const FString&) {
 FNativeFixture F; auto* S=F.Ship;S->Definition.Equipment.Mines=true;S->Combat->EquipmentState.MineMagazine=5;
 auto* Spec=S->Abilities->FindAbilitySpecFromClass(UVTMineAbility::StaticClass());
 S->Abilities->CallServerTryActivateAbility(Spec->Handle,false,FPredictionKey());
 TestEqual(TEXT("Raw client activation cannot consume ammunition"),S->Combat->EquipmentState.MineMagazine,5);
 TestEqual(TEXT("Raw client activation cannot spawn a mine"),F.Sim->Projectiles.Num(),0);
 TestTrue(TEXT("Authoritative intent path still activates"),S->Abilities->TryActivateAbilityByClass(UVTMineAbility::StaticClass()));
 TestEqual(TEXT("Authoritative activation consumes ammunition once"),S->Combat->EquipmentState.MineMagazine,4);
 auto Tag=FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Ship.Mine"));
 S->Abilities->CallServerEndAbility(Spec->Handle,Spec->ActivationInfo,FPredictionKey());
 TestTrue(TEXT("Raw client termination cannot erase cooldown"),S->Abilities->HasMatchingGameplayTag(Tag));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTFlightPresentation,"VT.Native.FlightPlayability",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTFlightPresentation::RunTest(const FString&) {
 FNativeFixture F;F.Ship->Anchored=false;F.Ship->Movement->Motion=FVTMotion();F.Ship->Combat->SpeedScale=1;
 FVTPilotIntent Input;Input.Throttle=1;FVTMotion Baseline;
 for(int I=0;I<64;++I){F.Ship->Movement->Step(Input,false);VT::HelmStep(Baseline,F.Ship->Definition.Stats,Input,F.Sim->Data->Rules.ReverseThrottle,VT::Step);}
 TestTrue(TEXT("Authored flight pace increases one-second travel"),F.Ship->Movement->Motion.Position.Size()>Baseline.Position.Size()*1.9);
 FVTMotion Right,Left;Input.Turn=VT::PlayerTurnInput(1);for(int I=0;I<32;++I)VT::HelmStep(Right,F.Ship->Definition.Stats,Input,0.25f,VT::Step);
 Input.Turn=VT::PlayerTurnInput(-1);for(int I=0;I<32;++I)VT::HelmStep(Left,F.Ship->Definition.Stats,Input,0.25f,VT::Step);
 TestTrue(TEXT("D / right stick turns toward Unreal right"),VT::ToWorld(Right.Position,0).Y>0);
 TestTrue(TEXT("A / left stick turns toward Unreal left"),VT::ToWorld(Left.Position,0).Y<0);
 TestEqual(TEXT("Projectile visual radius restores the original 7-unit sphere"),F.Sim->Data->ProjectileVisualRadius,7.f);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTRepeatedBankIntent,"VT.Native.PersistentBroadsideIntent",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTRepeatedBankIntent::RunTest(const FString&) {
 FNativeFixture F;auto* S=F.Ship;F.Sim->SystemShips.SetNum(F.Sim->Data->Systems.Num());F.Sim->SystemShips[S->SystemIndex].Add(S);S->Definition.ChargeTime=VT::Step*2;S->Definition.Reload=VT::Step*3;S->Definition.Guns=1;S->Intent.Buttons=VTButtons::Port;
 int32 Initial=F.Sim->Projectiles.Num();S->Combat->WeaponsStep();S->Combat->WeaponsStep();
 TestEqual(TEXT("Repeated intent retains windup without duplicate shots"),F.Sim->Projectiles.Num(),Initial);
 S->Combat->WeaponsStep();TestEqual(TEXT("Windup fires exactly once"),F.Sim->Projectiles.Num(),Initial+1);
 for(int I=0;I<3;++I)S->Combat->WeaponsStep();TestEqual(TEXT("Reload does not duplicate shots"),F.Sim->Projectiles.Num(),Initial+1);
 S->Combat->WeaponsStep();S->Combat->WeaponsStep();TestEqual(TEXT("Ready bank restarts immediately with its original windup"),F.Sim->Projectiles.Num(),Initial+2);
 return true;
}
#endif
