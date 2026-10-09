#include "VTProjectileSpawn.h"
#include "VTGameplay.h"
#include "VTNavigation.h"
#include "VTWorldAnchor.h"
#include "VTCombat.h"
#include "VTUI.h"
#include "VTWorldAnchor.h"
#include "VTGate.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputMappingContext.h"
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTThrottleInput,"VT.Input.ThrottleLadderAndLegacyMapping",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTThrottleInput::RunTest(const FString&) {
 FNativeFixture F;auto* PC=F.World->SpawnActor<AVTController>();PC->Possess(F.Ship);
 TestEqual(TEXT("Original default is half"),PC->ThrottleControl.Value(),0.5f);
 PC->StepThrottle(FInputActionValue(true),-1);TestEqual(TEXT("S selects halt"),PC->LocalIntent.Throttle,0.f);
 PC->StepThrottle(FInputActionValue(false),-1);TestEqual(TEXT("Release retains halt"),PC->LocalIntent.Throttle,0.f);
 PC->StepThrottle(FInputActionValue(true),-1);TestEqual(TEXT("Second S selects reverse"),PC->LocalIntent.Throttle,-1.f);
 PC->StepThrottle(FInputActionValue(true),-1);TestEqual(TEXT("Reverse saturates"),PC->LocalIntent.Throttle,-1.f);
 for(float Expected:{0.f,0.5f,1.f,1.f}){PC->StepThrottle(FInputActionValue(true),1);TestEqual(TEXT("W climbs and saturates"),PC->LocalIntent.Throttle,Expected);}
 PC->ReadFlight(FInputActionValue(0.62f),0);TestEqual(TEXT("Stick remains analogue"),PC->LocalIntent.Throttle,0.62f);
 PC->StepThrottle(FInputActionValue(true),-1);TestEqual(TEXT("Keyboard inherits nearest stick notch"),PC->LocalIntent.Throttle,0.f);
 PC->ReadFlight(FInputActionValue(0.f),0);TestEqual(TEXT("Centred stick halts"),PC->LocalIntent.Throttle,0.f);
 auto* Authored=LoadObject<UInputMappingContext>(nullptr,TEXT("/Game/Input/IMC_Flight.IMC_Flight"));
 auto* Adapted=PC->PrepareFlightMapping(Authored);TestNotNull(TEXT("Authored flight context"),Authored);TestNotNull(TEXT("Local flight context"),Adapted);
 if(!Authored||!Adapted)return false;
 TestTrue(TEXT("Authored package remains distinct"),Adapted!=Authored);
 bool Up=false,Down=false,Analog=false;
 for(const auto& M:Adapted->GetMappings()){if(M.Key==EKeys::W)Up=M.Action==PC->ThrottleUp;if(M.Key==EKeys::S)Down=M.Action==PC->ThrottleDown;if(M.Key==EKeys::Gamepad_LeftY)Analog=M.Action->GetFName()==TEXT("IA_Throttle");}
 TestTrue(TEXT("W/S use distinct edge actions and stick retains axis"),Up&&Down&&Analog);
 for(const auto& M:Authored->GetMappings())if(M.Key==EKeys::W||M.Key==EKeys::S)TestEqual(TEXT("Original asset is unchanged"),M.Action->GetFName(),FName(TEXT("IA_Throttle")));
 return true;
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
 FNativeFixture F;auto* S=F.Ship;F.Sim->Queries.BeginStep();S->Definition.ChargeTime=VT::Step*2;S->Definition.Reload=VT::Step*3;S->Definition.Guns=1;S->Intent.Buttons=VTButtons::Port;
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
 F.Sim->Queries.BeginStep();F.Sim->PiracyStep();
 TestTrue(TEXT("Authority-selected same-faction prize offers looting"),VTInteractionHint(S,F.Sim).Kind==EVTInteractionHint::Loot);
 S->Combat->BoardingProgress=F.Sim->Data->Rules.BoardDwell*0.5f;TestEqual(TEXT("Hint reflects actual boarding progress"),VTInteractionHint(S,F.Sim).Progress,0.5f);
 Prize->Combat->Claimed=true;TestTrue(TEXT("Claimed prize is not advertised"),VTInteractionHint(S,F.Sim).Kind==EVTInteractionHint::None);Prize->Combat->Claimed=false;
 Prize->SystemIndex=1;TestTrue(TEXT("Other-system prizes are not advertised"),VTInteractionHint(S,F.Sim).Kind==EVTInteractionHint::None);Prize->SystemIndex=S->SystemIndex;
 Prize->Movement->Motion.Position+=FVector2D(1000,0);TestTrue(TEXT("Stale out-of-range targets are hidden"),VTInteractionHint(S,F.Sim).Kind==EVTInteractionHint::None);S->Combat->BoardingTarget.Invalidate();
 const auto& System=F.Sim->Data->Systems[S->SystemIndex];if(TestTrue(TEXT("Fixture has a jump link"),!System.Links.IsEmpty())){FVTSavedShip GateRecord;GateRecord.JumpDestination=System.Links[0];GateRecord.JumpProgress=F.Sim->Data->Rules.JumpDwell*0.5f;S->GatePassage->Restore(GateRecord,F.Sim->SimulationTime);S->Movement->Motion.Position=F.Sim->JumpPosition(S->SystemIndex,S->GatePassage->Status().Destination);auto Hint=VTInteractionHint(S,F.Sim);TestTrue(TEXT("Eligible jump includes destination"),Hint.Kind==EVTInteractionHint::Jump&&!Hint.Label.IsEmpty());TestEqual(TEXT("Jump progress is read-only"),Hint.Progress,0.5f);}
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
 bool Moved=false,Aligned=false,Entering=false;for(int I=0;I<64*30&&S->SystemIndex==Origin;++I){F.Sim->FixedStep();Moved|=(S->Movement->Motion.Position-Centre).Size()>20;Aligned|=FVector2D::DotProduct(FVector2D(FMath::Cos(S->Movement->Motion.Heading),FMath::Sin(S->Movement->Motion.Heading)),Axis)>0.98;Entering|=S->GatePassage->Departing();}
 TestTrue(TEXT("Hold physically approaches ring"),Moved);TestTrue(TEXT("Hold aligns hull with aperture"),Aligned);TestTrue(TEXT("Charge unlocks physical entry"),Entering);TestEqual(TEXT("Crossing moves captain to linked system"),S->SystemIndex,F.Sim->Data->FindSystem(Link));
 FVTMotion A,B;A.Position=Centre-Axis*10;B=A;B.Position=Centre+Axis*10;B.Heading=FMath::Atan2(Axis.Y,Axis.X);TestTrue(TEXT("Outward aligned aperture crossing"),VTGate::Crossed(A,B,Centre,30,0.18));B.Position=A.Position;TestFalse(TEXT("Stationary charge cannot cross"),VTGate::Crossed(A,B,Centre,30,0.18));B.Position=Centre+Axis*10;B.Heading+=PI;TestFalse(TEXT("Backward hull rejected"),VTGate::Crossed(A,B,Centre,30,0.18));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTExclusiveAim,"VT.Native.ExclusiveSpecialAim",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTExclusiveAim::RunTest(const FString&) {
 FNativeFixture F;auto* S=F.Ship;F.Sim->Queries.BeginStep();auto* PC=F.World->SpawnActor<AVTController>();PC->Possess(S);
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTGateCamera,"VT.Native.GateCameraFocus",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTGateCamera::RunTest(const FString&) {
 FNativeFixture F;F.World->Tick(LEVELTICK_All,0.1f);auto* S=F.Ship;S->IsNPC=false;S->Anchored=false;auto* PC=F.World->SpawnActor<AVTController>();PC->Possess(S);auto* Camera=F.World->SpawnActor<AVTCameraManager>();Camera->InitializeFor(PC);FTViewTarget View;View.Target=S;auto Update=[&](){Camera->LastCameraReal=F.World->GetRealTimeSeconds()-0.05;Camera->UpdateViewTarget(View,VT::Step);};
 const auto Link=F.Sim->Data->Systems[0].Links[0];const auto Centre=F.Sim->JumpPosition(0,Link);S->Movement->Motion=FVTMotion();S->Movement->Motion.Position=Centre-Centre.GetSafeNormal()*F.Sim->Data->GateStartDistance();S->Movement->Motion.Heading=FMath::Atan2(Centre.Y,Centre.X);S->Movement->Previous=S->Movement->Authority=S->Movement->Motion;
 Update();FVTSavedShip R;R.JumpDestination=Link;R.JumpProgress=F.Sim->Data->Rules.JumpDwell;S->GatePassage->Restore(R,F.Sim->SimulationTime);S->LastInputTime=F.Sim->SimulationTime;S->Intent.Buttons=VTButtons::Interact;S->GatePassage->InteractionStep();Update();const auto Departure=Camera->Focus;const double Yaw=Camera->OrbitYaw,Pitch=Camera->OrbitPitch;const auto Eye=View.POV.Location;
 for(int I=0;I<12;++I){F.Sim->FixedStep();Update();}
 TestTrue(TEXT("Acceleration moves ship away from departure"),(VT::ToWorld(S->Movement->Motion.Position,0)-Departure).Size()>1000);
 TestTrue(TEXT("Departure camera retains focal position"),Camera->Focus.Equals(Departure,0.001));TestTrue(TEXT("Departure retains orbit and camera position"),Camera->OrbitYaw==Yaw&&Camera->OrbitPitch==Pitch&&View.POV.Location.Equals(Eye,0.001));
 F.Sim->TravelShip(S,F.Sim->Data->FindSystem(Link));Update();const auto Arrival=VT::ToWorld(S->GatePassage->Status().ArrivalTarget,S->SystemIndex);
 TestTrue(TEXT("Teleport immediately focuses final arrival position"),Camera->Focus.Equals(Arrival,0.001));TestTrue(TEXT("Camera ray points at arrival endpoint"),FVector::DotProduct(View.POV.Rotation.Vector(),(Arrival-View.POV.Location).GetSafeNormal())>0.9999);
 for(int I=0;I<12;++I){F.Sim->FixedStep();Update();TestTrue(TEXT("Braking retains final-position focus"),Camera->Focus.Equals(Arrival,0.001));}
 auto* RestoredCamera=F.World->SpawnActor<AVTCameraManager>();RestoredCamera->InitializeFor(PC);RestoredCamera->UpdateViewTarget(View,VT::Step);TestTrue(TEXT("New camera during restored arrival uses endpoint"),RestoredCamera->Focus.Equals(Arrival,0.001));
 for(int I=0;I<24;++I)F.Sim->FixedStep();Update();TestFalse(TEXT("Arrival completes"),S->GatePassage->Arriving());TestFalse(TEXT("Departure lock cleared"),Camera->GateDepartureFocus);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTGateMarker,"VT.Native.GateDistanceMarkerAndLegacyArrival",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTGateMarker::RunTest(const FString&) {
 FNativeFixture F;auto* D=F.Sim->Data.Get();TestEqual(TEXT("Double approach distance"),D->GateStartDistance(),160.f);TestEqual(TEXT("Staging remains within interaction range"),D->GateInteractionRange(),240.f);TestTrue(TEXT("Double arrival distance"),FMath::IsNearlyEqual(D->GateArrivalFraction(),0.3f));
 const FVector2D Centre(1000,0);FVTMotion P;const double Departure=FMath::Sqrt(2*D->GateStartDistance()/D->GatePassageAcceleration),Pause=D->GateMarkerPause,Cycle=2*Pause+Departure+D->GateFlashDuration+D->GateArrivalDuration;
 auto Sample=[&](double T){return VTGate::Preview(P,Centre,D->GateStartDistance(),D->GateArrivalFraction(),D->GatePassageAcceleration,D->GateArrivalDuration,D->GateFlashDuration,D->GateMarkerPause,T);};
 TestTrue(TEXT("Start arrow visible"),Sample(0));TestTrue(TEXT("Arrow begins at staging point facing gate"),P.Position.Equals(FVector2D(840,0),0.001)&&FMath::IsNearlyZero(P.Heading));
 Sample(Pause+Departure*0.5);const auto Half=P.Position;Sample(Pause+Departure*0.75);TestTrue(TEXT("Accelerating arrow approaches aperture"),Half.X>840&&P.Position.X>Half.X&&P.Position.X<1000);
 TestFalse(TEXT("Teleport hides moving arrow"),Sample(Pause+Departure+D->GateFlashDuration*0.5));
 TestTrue(TEXT("Arrow brakes inward after teleport"),Sample(Pause+Departure+D->GateFlashDuration+D->GateArrivalDuration*0.5));TestTrue(TEXT("Arrival arrow points inward"),P.Position.X>700&&P.Position.X<1000&&FMath::IsNearlyEqual(FMath::Abs(P.Heading),float(PI),0.001f));
 Sample(Cycle-0.1);TestTrue(TEXT("Preview ends at doubled arrival stop"),P.Position.Equals(FVector2D(700,0),0.001));Sample(Cycle);TestTrue(TEXT("Preview loops to marked start"),P.Position.Equals(FVector2D(840,0),0.001));
 FVTSavedShip Legacy;Legacy.JumpArriving=true;Legacy.JumpArrivalTarget=FVector2D(850,0);Legacy.JumpArrivalStarted=10;Legacy.Motion.SimulationTime=10.175;Legacy.Motion.Position=FVector2D(887.5,0);F.Ship->GatePassage->Restore(Legacy,40);TestTrue(TEXT("Legacy record retains original ring and stop"),F.Ship->GatePassage->Status().ArrivalOrigin.Equals(Centre,0.001)&&F.Ship->GatePassage->Status().ArrivalTarget==Legacy.JumpArrivalTarget);
 TestNotNull(TEXT("Authored flat gate arrow"),LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Environment/SM_GateArrow.SM_GateArrow")));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTGateSurge,"VT.Native.GateSurgeArrivalAndSave",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTGateSurge::RunTest(const FString&) {
 FNativeFixture F;auto* S=F.Ship;S->IsNPC=false;S->Anchored=false;const auto Link=F.Sim->Data->Systems[0].Links[0];const auto Centre=F.Sim->JumpPosition(0,Link),Axis=Centre.GetSafeNormal();S->Movement->Motion=FVTMotion();S->Movement->Motion.Position=Centre-Axis*F.Sim->Data->GateStartDistance();S->Movement->Motion.Heading=FMath::Atan2(Axis.Y,Axis.X);FVTSavedShip GateRecord;GateRecord.JumpDestination=Link;GateRecord.JumpProgress=F.Sim->Data->Rules.JumpDwell;S->GatePassage->Restore(GateRecord,F.Sim->SimulationTime);S->Intent.Buttons=VTButtons::Interact;S->GatePassage->InteractionStep();S->Intent.Buttons=VTButtons::Interact;
 double Peak=0;int Steps=0;while(S->SystemIndex==0&&Steps<64){F.Sim->FixedStep();if(S->SystemIndex==0){Peak=FMath::Max(Peak,S->Movement->Motion.Velocity.Size());TestTrue(TEXT("Source cannot teleport before reaching aperture"),FVector2D::DotProduct(S->Movement->Motion.Position-Centre,Axis)<=0);}++Steps;}
 TestEqual(TEXT("Surge crosses into linked system"),S->SystemIndex,F.Sim->Data->FindSystem(Link));TestTrue(TEXT("Departure crosses 160 units within 0.625 seconds"),Steps<40);TestTrue(TEXT("Departure accelerates far above staging cruise"),Peak>F.Sim->Data->GateCruiseSpeed*4);TestTrue(TEXT("Arrival braking begins at destination ring"),S->GatePassage->Arriving());
 const auto Arrival=F.Sim->JumpPosition(S->SystemIndex,F.Sim->Data->Systems[0].Id),Target=Arrival*(1-F.Sim->Data->GateArrivalFraction());TestTrue(TEXT("Teleport position is destination aperture"),S->Movement->Motion.Position.Equals(Arrival,0.001));TestTrue(TEXT("Arrival distance is doubled"),S->GatePassage->Status().ArrivalTarget.Equals(Target,0.001));
 F.Sim->FixedStep();auto* Save=F.GI->GetSubsystem<UVTSaveSubsystem>();auto Record=Save->CaptureShip(S);TArray<uint8> Bytes;auto* Snapshot=NewObject<UVTWorldSave>();Snapshot->Ships.Add(Record);TestTrue(TEXT("Serialize committed arrival"),UGameplayStatics::SaveGameToMemory(Snapshot,Bytes));auto* Loaded=Cast<UVTWorldSave>(UGameplayStatics::LoadGameFromMemory(Bytes));if(TestNotNull(TEXT("Arrival save restored"),Loaded)){auto* Restored=Save->RestoreShip(Loaded->Ships[0]);TestTrue(TEXT("Arrival phase and timestamp restore without held input"),Restored->GatePassage->Status().ArrivalOrigin.Equals(Arrival,0.001)&&Restored->GatePassage->Arriving()&&Restored->GatePassage->Status().ArrivalStarted==S->GatePassage->Status().ArrivalStarted&&Restored->Intent.Buttons==0);Restored->Movement->Step(FVTPilotIntent(),false);TestTrue(TEXT("Restored curve matches current arrival"),Restored->Movement->Motion.Position.Equals(S->Movement->Motion.Position,0.001));Restored->Destroy();}
 double PreviousSpeed=S->Movement->Motion.Velocity.Size();Steps=0;while(S->GatePassage->Arriving()&&Steps<64){F.Sim->FixedStep();TestTrue(TEXT("Arrival decelerates each step"),S->Movement->Motion.Velocity.Size()<=PreviousSpeed+0.001);PreviousSpeed=S->Movement->Motion.Velocity.Size();++Steps;}
 TestFalse(TEXT("Braking completes"),S->GatePassage->Arriving());TestTrue(TEXT("Exact doubled arrival endpoint"),S->Movement->Motion.Position.Equals(Target,0.001));TestTrue(TEXT("Arrival ends at rest"),S->Movement->Motion.Velocity.IsNearlyZero());TestTrue(TEXT("Rapid arrival completes within half a second"),Steps<32);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTSpawnLifecycle,"VT.Modules.ProjectileCreation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTSpawnLifecycle::RunTest(const FString&){FNativeFixture F;for(auto Kind:{EVTProjectileKind::Cannon,EVTProjectileKind::EMP,EVTProjectileKind::Torpedo,EVTProjectileKind::Mine}){auto X=FVTProjectileSpawnSpec::Fired(F.Ship,Kind,FVector2D(700,700),FVector2D(12,3),5,8,4);X.TargetId=FGuid::NewGuid();X.Velocity3D=FVector(0,0,250);X.TurnRate=2;auto* P=VTProjectileSpawn::Create(F.World,X);TestTrue("Registered at BeginPlay",F.Sim->Projectiles.Contains(P));TestTrue("Full pose history",P->Position==X.Position&&P->Previous==X.Position);TestTrue("Kind and motion initialized",P->Kind==Kind&&P->TargetId==X.TargetId&&P->Velocity3D==X.Velocity3D);P->Destroy();}
 FVTSavedProjectile R;R.Id=FGuid::NewGuid();R.Source=F.Ship->PersistentId;R.AttackerProfile=FGuid::NewGuid();R.SourceFaction=FName("Corsairs");R.Kind=EVTProjectileKind::Torpedo;R.Remaining=4;R.Velocity3D=FVector(0,0,30);TMap<FGuid,AVTShip*> Entities;auto* P=VTProjectileSpawn::Create(F.World,FVTProjectileSpawnSpec::Restored(R,Entities));TestTrue("Absent source preserves durable attribution",!P->Source&&P->SourceId==R.Source&&P->AttackerProfile==R.AttackerProfile&&P->SourceFaction==R.SourceFaction);return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTQueriesTest,"VT.Modules.PhaseScopedShipQueries",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTQueriesTest::RunTest(const FString&){FNativeFixture F;FVTMotion M;M.Position=FVector2D(-128,-128);auto* A=F.Sim->SpawnShip("corsair_frigate",0,M,true,"Corsairs");A->Definition.Radius=180;A->Disabled=true;F.Sim->Queries.BeginStep();TestTrue("Durable lookup",F.Sim->Queries.Find(0,A->PersistentId)==A);F.Sim->Queries.BuildSpatial();TArray<AVTShip*> Out;F.Sim->Queries.SweptCandidates(0,FVector2D(-400,-128),FVector2D(100,-128),5,Out);TestTrue("Large hull at negative cell edge included",Out.Contains(A));F.Sim->Queries.BuildBoarding();TestTrue("Disabled candidate",F.Sim->Queries.Boarding(0).Contains(A));A->Docked=true;F.Sim->Queries.BuildSpatial();F.Sim->Queries.SweptCandidates(0,M.Position,M.Position,5,Out);TestFalse("Docked hull omitted",Out.Contains(A));const auto Id=A->PersistentId;A->Destroy();TestNull("Destroyed ID removed",F.Sim->Queries.Find(0,Id));return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTStandingsTest,"VT.Modules.CaptainStandingsOwners",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTStandingsTest::RunTest(const FString&){FNativeFixture F;auto* PC=F.World->SpawnActor<AVTController>();PC->InitPlayerState();auto* PS=PC->GetPlayerState<AVTPlayerState>();if(!PS)return false;PS->Profile=FGuid::NewGuid();PS->Reputation=F.Sim->Data->InitialReputation;PS->Heat.Init(10,PS->Reputation.Num());auto* Save=F.GI->GetSubsystem<UVTSaveSubsystem>();FVTSavedPlayer R;R.Profile=PS->Profile;R.Reputation=PS->Reputation;R.Heat=PS->Heat;Save->PlayerRecords.Add(R);const auto Faction=F.Sim->Data->TrackedFactions[0];F.Sim->Standings.Decay(VT::Step);TestEqual("Connected snapshot does not decay twice",Save->PlayerRecords.Last().Heat[0],10.f);const auto Live=F.Sim->Standings.Read(PS->Profile,Faction);auto Profile=PS->Profile;Save->PlayerRecords.Last().Heat=PS->Heat;Save->PlayerRecords.Last().Reputation=PS->Reputation;PC->Destroy();F.Sim->Standings.Decay(VT::Step);auto Offline=F.Sim->Standings.Read(Profile,Faction);TestTrue("Offline trajectory continues",FMath::IsNearlyEqual(Offline.Heat,Live.Heat-F.Sim->Data->World.heat_decay_per_sec*VT::Step));int32 Credits=10000;TestTrue("Heat payment through same owner",F.Sim->Standings.PayHeat(Profile,Faction,Credits));TestEqual("Paid heat cleared",F.Sim->Standings.Read(Profile,Faction).Heat,0.f);const auto Count=Save->PlayerRecords.Num();F.Sim->Standings.Rescue(FGuid::NewGuid(),Faction);TestEqual("Unknown captain not created",Save->PlayerRecords.Num(),Count);return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTGateResumeTest,"VT.Modules.GateAdvancedClockRestore",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTGateResumeTest::RunTest(const FString&){FNativeFixture F;auto* S=F.Ship;S->IsNPC=false;S->Anchored=false;F.Sim->SimulationTime=10;S->Movement->Motion.SimulationTime=10;S->GatePassage->BeginArrival(FVector2D(1800,0),10);F.Sim->SimulationTime+=VT::Step;S->Movement->Step({},false);auto* Save=F.GI->GetSubsystem<UVTSaveSubsystem>();const auto Record=Save->CaptureShip(S);const double Elapsed=Record.Motion.SimulationTime-Record.JumpArrivalStarted;F.Sim->SimulationTime+=30;auto* R=Save->RestoreShip(Record);TestTrue("Committed arrival resumes without held input",R->GatePassage->Arriving()&&R->Intent.Buttons==0&&R->InputQueue.IsEmpty());TestTrue("Elapsed arrival rebased after 30 seconds",FMath::IsNearlyEqual(R->Movement->Motion.SimulationTime-R->GatePassage->Status().ArrivalStarted,Elapsed,0.000001));R->Movement->Step({},false);TestTrue("Host clock advance does not skip braking",R->Movement->Motion.Position.Equals(Record.Motion.Position,0.001));FVTMotion Predicted=R->Movement->Motion;FVTPilotIntent I;R->GatePassage->Integrate(Predicted,R->Definition.Stats,I,true);F.Sim->SimulationTime+=VT::Step;R->Movement->Step({},false);TestTrue("Prediction samples same arrival curve",Predicted.Position.Equals(R->Movement->Motion.Position,0.001));auto DockRecord=Record;DockRecord.JumpArriving=false;DockRecord.DockProgress=1;auto* DockedRestore=Save->RestoreShip(DockRecord);TestEqual("Gate restore retains unrelated docking progress",DockedRestore->DockProgress,1.f);return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTQueryCompleteness,"VT.Modules.SweptQueryCompleteness",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTQueryCompleteness::RunTest(const FString&){FNativeFixture F;for(int I=0;I<24;++I){FVTMotion M;M.Position=FVector2D(-450+(I%6)*128,-256+(I/6)*128);auto* S=F.Sim->SpawnShip("corsair_frigate",I%2,M,true,"Corsairs");S->Definition.Radius=I%4==0?180:25;S->Docked=I%7==0;}F.Sim->Queries.BeginStep();F.Sim->Queries.BuildSpatial();TArray<AVTShip*> Out;for(int System=0;System<2;++System)for(int Lane=-3;Lane<4;++Lane){FVector2D A(-700,Lane*100),B(700,Lane*100);F.Sim->Queries.SweptCandidates(System,A,B,5,Out);for(auto* S:F.Sim->Queries.Ordered(System))if(!S->Docked&&VTCombat::SegmentDistanceSquared(A,B,S->Movement->Motion.Position)<=FMath::Square(S->Definition.Radius+5))TestTrue("Broad phase includes every brute-force contact",Out.Contains(S));for(auto* S:Out)TestEqual("Candidates remain scoped to system",S->SystemIndex,System);}return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTNavigationRules,"VT.Navigation.ScaleBoundaryRouteAndEquipment",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTNavigationRules::RunTest(const FString&){
 FNativeFixture F;auto* D=F.Sim->Data.Get();TestEqual(TEXT("Celestial radius enlarged fivefold"),D->Landmarks[0].Radius,600.f);TestEqual(TEXT("Station moved fivefold"),D->Rules.StationPosition,FVector2D(1500,1500));TestEqual(TEXT("Arenas ten times farther apart"),(VT::ArenaOrigin(1)-VT::ArenaOrigin(0)).X,10000000.);
 const float R=D->Systems[0].Radius;TestEqual(TEXT("Full speed at boundary"),D->BoundarySpeed(0,{R,0}),1.f);TestTrue(TEXT("Halfway through 100 m linear ramp"),FMath::IsNearlyEqual(D->BoundarySpeed(0,{R+50,0}),0.505f));TestTrue(TEXT("One percent after 100 m"),FMath::IsNearlyEqual(D->BoundarySpeed(0,{R+100,0}),0.01f));TestTrue(TEXT("Minimum speed beyond band"),FMath::IsNearlyEqual(D->BoundarySpeed(0,{R+1000,0}),0.01f));
 auto* PC=F.World->SpawnActor<AVTController>();PC->Possess(F.Ship);F.Ship->Anchored=false;F.Ship->Movement->Motion.Position={R+100,0};F.Ship->Movement->Motion.Velocity={100,0};FVTPilotIntent I;I.Throttle=1;auto Initial=F.Ship->Movement->Motion;F.Ship->Movement->Step(I,false);auto Authority=F.Ship->Movement->Motion;F.Ship->Movement->Motion=Initial;F.Ship->Movement->Step(I,true);TestTrue(TEXT("Prediction shares boundary motion"),F.Ship->Movement->Motion.Position.Equals(Authority.Position,0.001));TestTrue(TEXT("Existing velocity capped to reduced maximum"),Authority.Velocity.Size()<=F.Ship->Definition.Stats.MaxSpeed*D->FlightSpeedMultiplier*0.01001);
 F.Ship->Definition.Equipment.Warp=false;F.Ship->Definition.Equipment.Torpedoes=true;PC->ReadFlight(FInputActionValue(true),7);TestFalse(TEXT("Unfitted shift does not enter aim mode"),bool(PC->LocalIntent.Buttons&VTButtons::Warp));PC->ReadFlight(FInputActionValue(true),6);TestTrue(TEXT("Equipped ctrl still aims"),bool(PC->LocalIntent.Buttons&VTButtons::Torpedo));F.Ship->Definition.Equipment.Warp=true;F.Ship->Definition.Equipment.Torpedoes=false;VT::FilterEquipmentIntent(PC->LocalIntent,F.Ship->Definition.Equipment);TestFalse(TEXT("Removed device cancels stale aim"),bool(PC->LocalIntent.Buttons&VTButtons::Torpedo));PC->ReadFlight(FInputActionValue(true),7);TestTrue(TEXT("Warp-only fit can aim warp"),bool(PC->LocalIntent.Buttons&VTButtons::Warp));
 for(int A=0;A<10;++A)for(int B=0;B<10;++B){auto Path=VTNavigation::Route(*D,A,B);if(Path.IsEmpty())continue;TestEqual(TEXT("Route starts at ship"),Path[0],A);TestEqual(TEXT("Route ends at selected system"),Path.Last(),B);for(int N=1;N<Path.Num();++N)TestTrue(TEXT("Every waypoint follows a real gate"),D->Systems[Path[N-1]].Links.Contains(D->Systems[Path[N]].Id));if(Path.Num()>1){auto AfterJump=VTNavigation::Route(*D,Path[1],B);TestEqual(TEXT("Route advances after independent jump"),AfterJump.Num(),Path.Num()-1);}}
 auto Private=VTNavigation::Route(*D,0,10);TestTrue(TEXT("Cannot plot an entry into private intro"),Private.IsEmpty());TestTrue(TEXT("Invalid destination is safe"),VTNavigation::Route(*D,0,999).IsEmpty());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTNavigationStandings,"VT.Navigation.RelationshipsVisibilityAndSaveScale",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTNavigationStandings::RunTest(const FString&){
 FNativeFixture F;auto* PC=F.World->SpawnActor<AVTController>();PC->Possess(F.Ship);auto* PS=PC->GetPlayerState<AVTPlayerState>();PS->Profile=FGuid::NewGuid();PS->Reputation=F.Sim->Data->InitialReputation;PS->Heat.Init(0,PS->Reputation.Num());FVTMotion M;M.Position={1500,-1500};auto* Other=F.Sim->SpawnShip(TEXT("house_patrol"),0,M,true,TEXT("Guild"));int Guild=F.Sim->Data->FactionIndex(TEXT("Guild"));
 PS->Reputation[Guild]=50;auto Friendly=VTNavigation::RelationshipColour(F.Ship,Other,F.Sim);TestTrue(TEXT("Friendly green"),Friendly.G>Friendly.R);PS->Reputation[Guild]=0;auto Neutral=VTNavigation::RelationshipColour(F.Ship,Other,F.Sim);TestTrue(TEXT("Neutral amber"),Neutral.R>Neutral.G&&Neutral.G>Neutral.B);PS->Reputation[Guild]=-50;auto Hostile=VTNavigation::RelationshipColour(F.Ship,Other,F.Sim);TestTrue(TEXT("Hostile red"),Hostile.R>Hostile.G);PS->Reputation[Guild]=50;PS->Heat[Guild]=100;TestEqual(TEXT("Hostile heat is visible to this captain"),VTNavigation::RelationshipColour(F.Ship,Other,F.Sim),Hostile);
 auto* Anchor=F.World->SpawnActor<AVTWorldAnchor>();Anchor->System=1;Anchor->Kind=0;TestTrue(TEXT("Distant star relevant"),Anchor->IsNetRelevantFor(PC,F.Ship,FVector::ZeroVector));Anchor->Kind=3;TestFalse(TEXT("Distant planet not relevant"),Anchor->IsNetRelevantFor(PC,F.Ship,FVector::ZeroVector));Anchor->System=0;TestTrue(TEXT("Local planet relevant"),Anchor->IsNetRelevantFor(PC,F.Ship,FVector::ZeroVector));
 auto* Save=F.GI->GetSubsystem<UVTSaveSubsystem>();auto* Snapshot=NewObject<UVTWorldSave>();FVTSavedShip Old;Old.Motion.Position={500,-500};Old.JumpArrivalOrigin={1000,0};Old.JumpArrivalTarget={700,0};Snapshot->Ships.Add(Old);FVTSavedProjectile Shot;Shot.Position={510,-500};Snapshot->Projectiles.Add(Shot);TestTrue(TEXT("Schema six layout migration accepted"),Save->Migrate(Snapshot));TestEqual(TEXT("Ship placement scaled"),Snapshot->Ships[0].Motion.Position,FVector2D(2500,-2500));TestEqual(TEXT("Projectile placement scaled"),Snapshot->Projectiles[0].Position,FVector2D(2550,-2500));TestEqual(TEXT("Committed arrival endpoint scaled"),Snapshot->Ships[0].JumpArrivalTarget,FVector2D(3500,0));Save->Migrate(Snapshot);TestEqual(TEXT("Repeated load does not scale twice"),Snapshot->Ships[0].Motion.Position,FVector2D(2500,-2500));return true;
}
#endif

