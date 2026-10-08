#include "VTProjectileSpawn.h"
#include "VTGameplay.h"
#include "VTCombat.h"
#include "VTUI.h"
#include "VTWorldAnchor.h"
#include "VTGate.h"
#include "InputActionValue.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "VTSaveSubsystem.h"
#include "Components/ComboBoxString.h"
#include "Kismet/GameplayStatics.h"
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
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FVTGatePassage,"VT.Native.PhysicalGatePassage",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FVTGatePassage::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const {for(const TCHAR* Id:{TEXT("corsair_frigate"),TEXT("corsair_cruiser"),TEXT("corsair_battleship")}){Names.Add(Id);Commands.Add(Id);}}
bool FVTGatePassage::RunTest(const FString& Hull) {
 FNativeFixture F;auto* S=F.Ship;S->InitializeShip(FName(Hull),0,FVTMotion());S->Combat->Shields=S->Definition.ShieldMax;S->IsNPC=false;S->Anchored=false;S->Disabled=false;const int Origin=S->SystemIndex;const FName Link=F.Sim->Data->Systems[Origin].Links[0];const auto Centre=F.Sim->JumpPosition(Origin,Link),Axis=Centre.GetSafeNormal();
 S->Movement->Motion=FVTMotion();S->Movement->Motion.Position=Centre;S->Movement->Previous=S->Movement->Motion;S->Intent.Buttons=VTButtons::Interact;
 bool Moved=false,Aligned=false,Entering=false;for(int I=0;I<64*30&&S->SystemIndex==Origin;++I){F.Sim->FixedStep();Moved|=(S->Movement->Motion.Position-Centre).Size()>20;Aligned|=FVector2D::DotProduct(FVector2D(FMath::Cos(S->Movement->Motion.Heading),FMath::Sin(S->Movement->Motion.Heading)),Axis)>0.98;Entering|=S->JumpEntering;}
 TestTrue(TEXT("Hold physically approaches ring"),Moved);TestTrue(TEXT("Hold aligns hull with aperture"),Aligned);TestTrue(TEXT("Charge unlocks physical entry"),Entering);TestEqual(TEXT("Crossing moves captain to linked system"),S->SystemIndex,F.Sim->Data->FindSystem(Link));
 FVTMotion A,B;A.Position=Centre-Axis*10;B=A;B.Position=Centre+Axis*10;B.Heading=FMath::Atan2(Axis.Y,Axis.X);TestTrue(TEXT("Outward aligned aperture crossing"),VTGate::Crossed(A,B,Centre,30,0.18));B.Position=A.Position;TestFalse(TEXT("Stationary charge cannot cross"),VTGate::Crossed(A,B,Centre,30,0.18));B.Position=Centre+Axis*10;B.Heading+=PI;TestFalse(TEXT("Backward hull rejected"),VTGate::Crossed(A,B,Centre,30,0.18));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTExclusiveAim,"VT.Native.ExclusiveSpecialAim",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTExclusiveAim::RunTest(const FString&) {
 FNativeFixture F;auto* S=F.Ship;F.Sim->SystemShips.SetNum(F.Sim->Data->Systems.Num());F.Sim->SystemShips[S->SystemIndex].Add(S);auto* PC=F.World->SpawnActor<AVTController>();PC->Possess(S);
 for(uint16 Bit:{VTButtons::Warp,VTButtons::Torpedo}){PC->LocalIntent.Buttons=Bit|VTButtons::AimPort|VTButtons::Port;TestFalse(TEXT("Special aim suppresses broadside cursor"),PC->UpdateBroadsideAim(10,F.Sim->Data->Feel.controls,0,S->Definition.Arc));TestEqual(TEXT("Pending broadside inputs cleared"),PC->LocalIntent.Buttons,Bit);PC->ReadFlight(FInputActionValue(true),3);PC->ReadFlight(FInputActionValue(false),3);TestEqual(TEXT("Broadside press/release ignored while special held"),PC->LocalIntent.Buttons,Bit);S->Intent.Buttons=Bit|VTButtons::Port|VTButtons::Starboard;S->Combat->WeaponsStep();TestEqual(TEXT("Authoritative bank remains available"),S->PortReload,0.f);TestEqual(TEXT("No broadside windup accepted"),S->Combat->PortCharge,0.f);}
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTLocalPresentation,"VT.Native.LocalPoseInterpolation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTLocalPresentation::RunTest(const FString&) {
 FNativeFixture F;auto* PC=F.World->SpawnActor<AVTController>();PC->Possess(F.Ship);auto* M=F.Ship->Movement.Get();M->Previous.Position=FVector2D(10,20);M->Motion.Position=FVector2D(30,20);M->Previous.Heading=PI-0.1;M->Motion.Heading=-PI+0.1;
 F.Sim->Accumulator=VT::Step*0.25;auto Pose=M->PresentationPose();TestTrue(TEXT("Local ship has fractional-step position"),Pose.Position.Equals(FVector2D(15,20),0.001));TestTrue(TEXT("Yaw takes short arc through wrap"),FMath::Abs(FMath::UnwindRadians(Pose.Heading-(PI-0.05)))<0.001);TestTrue(TEXT("Presentation leaves simulation untouched"),M->Motion.Position.Equals(FVector2D(30,20)));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTFitLifecycle,"VT.Native.FitLifecycleAndFrontend",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTFitLifecycle::RunTest(const FString&) {
 FNativeFixture F;FVTLoadoutSelection Fit;Fit.OverrideBatteries=Fit.OverrideSpecials=true;Fit.Batteries={FName("loadout.boost")};Fit.Specials={FName("loadout.microwarp")};F.Ship->Fit=Fit;F.Ship->OnRep_ClassId();TestFalse(TEXT("Class notification retains selected fit instead of EMP"),F.Ship->Definition.Equipment.EMP);TestTrue(TEXT("Selected warp survives class resolution"),F.Ship->Definition.Equipment.Warp);
 auto* Deferred=F.World->SpawnActorDeferred<AVTShip>(AVTShip::StaticClass(),FTransform::Identity);Deferred->InitializeShip(TEXT("corsair_cruiser"),0,FVTMotion());Deferred->Fit=Fit;Deferred->FinishSpawning(FTransform::Identity);TestFalse(TEXT("BeginPlay preserves received fit instead of EMP"),Deferred->Definition.Equipment.EMP);TestTrue(TEXT("BeginPlay preserves fitted warp"),Deferred->Definition.Equipment.Warp);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTGateReverse,"VT.Native.GateReverseApproach",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTGateReverse::RunTest(const FString&) {
 FVTShipStats Stats;Stats.Thrust=100;Stats.TurnRate=Stats.TurnRateSlow=1;Stats.ForwardDrag=1;FVTMotion Motion;Motion.Position=FVector2D(940,0);FVTPilotIntent Intent;Intent.Buttons=VTButtons::Interact;
 auto Guide=VTGate::Guide(Motion,Stats,Intent,FVector2D(1000,0),false,80,45,12,0.25);
 TestTrue(TEXT("Nearby staging behind bow uses reverse instead of a full turn"),Guide.Throttle<0);TestTrue(TEXT("Backing retains forward gate heading"),FMath::Abs(Guide.Turn)<0.001);
 Motion.Position=FVector2D(1920,0);Guide=VTGate::Guide(Motion,Stats,Intent,FVector2D(1000,0),false,80,45,12,0.25);TestTrue(TEXT("Long approach selects faster forward turn instead of slow reverse"),FMath::Abs(Guide.Turn)>0.5);
 Motion.Position=FVector2D(920,0);Guide=VTGate::Guide(Motion,Stats,Intent,FVector2D(1000,0),true,80,45,12,0.25);TestTrue(TEXT("Entry always accelerates forward through aperture"),Guide.Throttle>0);
 FNativeFixture F;auto* Shot=F.Ship->Combat->SpawnDeviceProjectile(EVTProjectileKind::Torpedo,FVector2D(10,20),FVector2D::ZeroVector,1,8,12);TestTrue(TEXT("Equipment projectile completes deferred lifecycle"),Shot->HasActorBegunPlay());TestTrue(TEXT("Equipment projectile begins with torpedo kind"),Shot->Kind==EVTProjectileKind::Torpedo);TestTrue(TEXT("Finished torpedo registered for simulation"),F.Sim->Projectiles.Contains(Shot));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTGateSurge,"VT.Native.GateSurgeArrivalAndSave",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTGateSurge::RunTest(const FString&) {
 FNativeFixture F;auto* S=F.Ship;S->IsNPC=false;S->Anchored=false;const auto Link=F.Sim->Data->Systems[0].Links[0];const auto Centre=F.Sim->JumpPosition(0,Link),Axis=Centre.GetSafeNormal();S->Movement->Motion=FVTMotion();S->Movement->Motion.Position=Centre-Axis*F.Sim->Data->GateApproachDistance;S->Movement->Motion.Heading=FMath::Atan2(Axis.Y,Axis.X);S->JumpDestination=Link;S->JumpEntering=true;S->JumpProgress=F.Sim->Data->Rules.JumpDwell;S->Intent.Buttons=VTButtons::Interact;
 double Peak=0;int Steps=0;while(S->SystemIndex==0&&Steps<64){F.Sim->FixedStep();if(S->SystemIndex==0){Peak=FMath::Max(Peak,S->Movement->Motion.Velocity.Size());TestTrue(TEXT("Source cannot teleport before reaching aperture"),FVector2D::DotProduct(S->Movement->Motion.Position-Centre,Axis)<=0);}++Steps;}
 TestEqual(TEXT("Surge crosses into linked system"),S->SystemIndex,F.Sim->Data->FindSystem(Link));TestTrue(TEXT("Departure crosses 80 units within half a second"),Steps<32);TestTrue(TEXT("Departure accelerates far above staging cruise"),Peak>F.Sim->Data->GateCruiseSpeed*4);TestTrue(TEXT("Arrival braking begins at destination ring"),S->JumpArriving);
 const auto Arrival=F.Sim->JumpPosition(S->SystemIndex,F.Sim->Data->Systems[0].Id),Target=Arrival*0.85;TestTrue(TEXT("Teleport position is destination aperture"),S->Movement->Motion.Position.Equals(Arrival,0.001));TestTrue(TEXT("Arrival endpoint is unchanged"),S->JumpArrivalTarget.Equals(Target,0.001));
 F.Sim->FixedStep();auto* Save=F.GI->GetSubsystem<UVTSaveSubsystem>();auto Record=Save->CaptureShip(S);TArray<uint8> Bytes;auto* Snapshot=NewObject<UVTWorldSave>();Snapshot->Ships.Add(Record);TestTrue(TEXT("Serialize committed arrival"),UGameplayStatics::SaveGameToMemory(Snapshot,Bytes));auto* Loaded=Cast<UVTWorldSave>(UGameplayStatics::LoadGameFromMemory(Bytes));if(TestNotNull(TEXT("Arrival save restored"),Loaded)){auto* Restored=Save->RestoreShip(Loaded->Ships[0]);TestTrue(TEXT("Arrival phase and timestamp restore without held input"),Restored->JumpArriving&&Restored->JumpArrivalStarted==S->JumpArrivalStarted&&Restored->Intent.Buttons==0);Restored->Movement->Step(FVTPilotIntent(),false);TestTrue(TEXT("Restored curve matches current arrival"),Restored->Movement->Motion.Position.Equals(S->Movement->Motion.Position,0.001));Restored->Destroy();}
 double PreviousSpeed=S->Movement->Motion.Velocity.Size();Steps=0;while(S->JumpArriving&&Steps<64){F.Sim->FixedStep();TestTrue(TEXT("Arrival decelerates each step"),S->Movement->Motion.Velocity.Size()<=PreviousSpeed+0.001);PreviousSpeed=S->Movement->Motion.Velocity.Size();++Steps;}
 TestFalse(TEXT("Braking completes"),S->JumpArriving);TestTrue(TEXT("Exact former arrival endpoint"),S->Movement->Motion.Position.Equals(Target,0.001));TestTrue(TEXT("Arrival ends at rest"),S->Movement->Motion.Velocity.IsNearlyZero());TestTrue(TEXT("Rapid arrival completes within half a second"),Steps<32);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTSpawnLifecycle,"VT.Modules.ProjectileCreation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTSpawnLifecycle::RunTest(const FString&){FNativeFixture F;for(auto Kind:{EVTProjectileKind::Cannon,EVTProjectileKind::EMP,EVTProjectileKind::Torpedo,EVTProjectileKind::Mine}){auto X=FVTProjectileSpawnSpec::Fired(F.Ship,Kind,FVector2D(700,700),FVector2D(12,3),5,8,4);X.TargetId=FGuid::NewGuid();X.Velocity3D=FVector(0,0,250);X.TurnRate=2;auto* P=VTProjectileSpawn::Create(F.World,X);TestTrue("Registered at BeginPlay",F.Sim->Projectiles.Contains(P));TestTrue("Full pose history",P->Position==X.Position&&P->Previous==X.Position);TestTrue("Kind and motion initialized",P->Kind==Kind&&P->TargetId==X.TargetId&&P->Velocity3D==X.Velocity3D);P->Destroy();}
 FVTSavedProjectile R;R.Id=FGuid::NewGuid();R.Source=F.Ship->PersistentId;R.AttackerProfile=FGuid::NewGuid();R.SourceFaction=FName("Corsairs");R.Kind=EVTProjectileKind::Torpedo;R.Remaining=4;R.Velocity3D=FVector(0,0,30);TMap<FGuid,AVTShip*> Entities;auto* P=VTProjectileSpawn::Create(F.World,FVTProjectileSpawnSpec::Restored(R,Entities));TestTrue("Absent source preserves durable attribution",!P->Source&&P->SourceId==R.Source&&P->AttackerProfile==R.AttackerProfile&&P->SourceFaction==R.SourceFaction);return true;}
#endif
