#include "VTGameplay.h"
#include "VTCombat.h"
#include "VTUI.h"
#include "VTWorldAnchor.h"
#include "InputActionValue.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTOriginalHUDAssets,"VT.Native.OriginalHUDAssets",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTOriginalHUDAssets::RunTest(const FString&) {
 auto* Class=LoadClass<UVTUI>(nullptr,TEXT("/Game/UI/WBP_UI.WBP_UI_C"));if(!TestNotNull(TEXT("Native authored UI class"),Class))return false;auto* UI=Class->GetDefaultObject<UVTUI>();
 TestEqual(TEXT("Original five panel frames are packaged references"),UI->HudPanels.Num(),5);
 auto* Font=Cast<UFont>(UI->HudFont.FontObject);if(!TestNotNull(TEXT("HUD uses a composite font, not a bare face"),Font))return false;
 const auto& Faces=Font->GetInternalCompositeFont().DefaultTypeface.Fonts;if(!TestEqual(TEXT("Monospace typeface exists"),Faces.Num(),1))return false;
 auto* Face=Cast<UFontFace>(Faces[0].Font.GetFontFaceAsset());if(TestNotNull(TEXT("Packaged inline font face"),Face)){TestTrue(TEXT("Font contains glyph bytes"),Face->FontFaceData->HasData());TestTrue(TEXT("No external font file dependency"),Face->LoadingPolicy==EFontLoadingPolicy::Inline);}
 if(TestNotNull(TEXT("String table is a cooked UI dependency"),UI->TextTable.Get()))TestEqual(TEXT("Cruiser name resolves through the actual registered table"),FText::FromStringTable(UI->TextTable->GetStringTableId(),TEXT("class.corsair_cruiser.name")).ToString(),FString(TEXT("Cruiser")));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTBroadsideControls,"VT.Native.BroadsideHoldRelease",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTBroadsideControls::RunTest(const FString&) {
 FNativeFixture F;auto* PC=F.World->SpawnActor<AVTController>();PC->Possess(F.Ship);
 const auto& Controls=F.Sim->Data->Feel.controls;const float Arc=0.4f,Heading=0.7f;
 PC->ReadFlight(FInputActionValue(true),3);
 TestTrue(TEXT("First press starts aiming"),(PC->LocalIntent.Buttons&VTButtons::AimPort)!=0);
 TestFalse(TEXT("Holding does not fire"),(PC->LocalIntent.Buttons&VTButtons::Port)!=0);
 PC->UpdateBroadsideAim(100,Controls,Heading,Arc);
 const float Expected=Heading+PI/2-100*Controls.mouse_aim_sens*Arc;
 TestTrue(TEXT("Mouse steers original authored arc"),PC->LocalIntent.Aim.Equals(FVector2D(FMath::Cos(Expected),FMath::Sin(Expected)),0.00001));
 const auto Held=PC->LocalIntent.Aim;
 PC->ReadFlight(FInputActionValue(false),3);PC->UpdateBroadsideAim(-1000,Controls,Heading,Arc);
 TestTrue(TEXT("Release requests one volley"),(PC->LocalIntent.Buttons&VTButtons::Port)!=0);
 TestTrue(TEXT("Release keeps held direction despite mouse motion"),PC->LocalIntent.Aim.Equals(Held,0.00001));
 PC->LocalIntent.Buttons=0;PC->UpdateBroadsideAim(0,Controls,Heading,Arc);
 PC->ReadFlight(FInputActionValue(true),4);PC->UpdateBroadsideAim(0,Controls,Heading,Arc);
 TestTrue(TEXT("New bank starts at its beam"),PC->LocalIntent.Aim.Equals(FVector2D(FMath::Cos(Heading-PI/2),FMath::Sin(Heading-PI/2)),0.00001));
 PC->UsingGamepadAim=true;PC->GamepadAim=FVector2D(1,0);PC->UpdateBroadsideAim(0,Controls,Heading,Arc);
 TestTrue(TEXT("Right stick selects full arc"),PC->LocalIntent.Aim.Equals(FVector2D(FMath::Cos(Heading-PI/2-Arc),FMath::Sin(Heading-PI/2-Arc)),0.00001));
 PC->CancelFlight(FInputActionValue(false),4);
 TestEqual(TEXT("Context cancellation never fires"),PC->LocalIntent.Buttons,uint16(0));
 F.Ship->Definition.Guns=3;F.Ship->Movement->Motion.Velocity=FVector2D(120,40);const int32 Start=F.Sim->Projectiles.Num();
 F.Ship->Combat->Volley(true,Held);
 TestEqual(TEXT("Volley emits one projectile per preview muzzle"),F.Sim->Projectiles.Num(),Start+3);
 for(int32 Gun=0;Gun<3;++Gun){const auto Preview=VTCombat::BroadsideShot(F.Ship->Movement->Motion.Position,F.Ship->Movement->Motion.Velocity,Held,F.Ship->Definition,F.Sim->Data->Rules,Gun);auto* Shot=F.Sim->Projectiles[Start+Gun].Get();TestTrue(TEXT("Preview origin matches emitted projectile"),Shot->Position.Equals(Preview.Key,0.00001));TestTrue(TEXT("Preview bearing matches momentum-inheriting projectile"),Shot->Velocity.GetSafeNormal().Equals(Preview.Value.GetSafeNormal(),0.00001));TestTrue(TEXT("Volley retains ship momentum"),Shot->Velocity.Equals(FVector2D(120,40)+Held*F.Ship->Definition.MuzzleSpeed,0.00001));}
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTInteractionHints,"VT.Native.ContextInteractionHints",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTInteractionHints::RunTest(const FString&) {
 FNativeFixture F;auto* S=F.Ship;S->IsNPC=false;FVTMotion M;M.Position=S->Movement->Motion.Position+FVector2D(30,0);auto* Prize=F.Sim->SpawnShip(TEXT("house_patrol"),S->SystemIndex,M,true,TEXT("Corsairs"));Prize->Disabled=true;Prize->Invulnerable=false;
 F.Sim->SystemShips.SetNum(F.Sim->Data->Systems.Num());F.Sim->SystemShips[S->SystemIndex].Reset();F.Sim->SystemShips[S->SystemIndex].Append({S,Prize});F.Sim->PiracyStep();
 TestTrue(TEXT("Authority-selected same-faction prize offers looting"),VTInteractionHint(S,F.Sim).Kind==EVTInteractionHint::Loot);
 S->Combat->BoardingProgress=F.Sim->Data->Rules.BoardDwell*0.5f;TestEqual(TEXT("Hint reflects actual boarding progress"),VTInteractionHint(S,F.Sim).Progress,0.5f);
 Prize->Combat->Claimed=true;TestTrue(TEXT("Claimed prize is not advertised"),VTInteractionHint(S,F.Sim).Kind==EVTInteractionHint::None);Prize->Combat->Claimed=false;
 Prize->SystemIndex=1;TestTrue(TEXT("Other-system prizes are not advertised"),VTInteractionHint(S,F.Sim).Kind==EVTInteractionHint::None);Prize->SystemIndex=S->SystemIndex;
 Prize->Movement->Motion.Position+=FVector2D(1000,0);TestTrue(TEXT("Stale out-of-range targets are hidden"),VTInteractionHint(S,F.Sim).Kind==EVTInteractionHint::None);S->Combat->BoardingTarget.Invalidate();
 const auto& System=F.Sim->Data->Systems[S->SystemIndex];if(TestTrue(TEXT("Fixture has a jump link"),!System.Links.IsEmpty())){S->JumpDestination=System.Links[0];S->Movement->Motion.Position=F.Sim->JumpPosition(S->SystemIndex,S->JumpDestination);S->JumpProgress=F.Sim->Data->Rules.JumpDwell*0.5f;auto Hint=VTInteractionHint(S,F.Sim);TestTrue(TEXT("Eligible jump includes destination"),Hint.Kind==EVTInteractionHint::Jump&&!Hint.Label.IsEmpty());TestEqual(TEXT("Jump progress is read-only"),Hint.Progress,0.5f);}
 S->Disabled=true;TestTrue(TEXT("Disabled captain has no misleading action"),VTInteractionHint(S,F.Sim).Kind==EVTInteractionHint::None);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTNearestStar,"VT.Native.RadialGridNearestStar",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTNearestStar::RunTest(const FString&) {
 TArray<FVTLandmarkDefinition> Bodies={FVTLandmarkDefinition(FVector2D(-500,20),120,0),FVTLandmarkDefinition(FVector2D(500,20),120,0),FVTLandmarkDefinition(FVector2D(450,20),120,1)};FVector2D Centre;
 TestTrue(TEXT("Nearest star found"),VTGrid::NearestStar(Bodies,FVector2D(450,20),Centre));TestTrue(TEXT("Nearby non-star cannot become radial origin"),Centre.Equals(FVector2D(500,20)));
 VTGrid::NearestStar(Bodies,FVector2D(-450,20),Centre);TestTrue(TEXT("Origin changes to nearer star"),Centre.Equals(FVector2D(-500,20)));
 TestTrue(TEXT("Travel applies the current system arena translation"),(VT::ToWorld(Centre,1)-VT::ToWorld(Centre,0)).Equals(VT::ArenaOrigin(1)-VT::ArenaOrigin(0)));
 Bodies.Reset();TestFalse(TEXT("No star means no fabricated radial centre"),VTGrid::NearestStar(Bodies,FVector2D::ZeroVector,Centre));
 return true;
}
#endif
