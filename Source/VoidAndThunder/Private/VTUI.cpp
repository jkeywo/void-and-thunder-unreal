#include "VTUI.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "VTGameplay.h"
#include "VTCombat.h"
#include "VTSessionSubsystem.h"
#include "VTSaveSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/PanelWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace {
FString DisplayId(FName Id) {
 FString Key=Id.ToString(); if(Key.StartsWith(TEXT("corsair"))||Key.StartsWith(TEXT("house"))) Key=TEXT("class.")+Key; Key+=TEXT(".name");
 FString Label=FText::FromStringTable(FName(TEXT("/Game/UI/ST_UI.ST_UI")),Key,EStringTableLoadingPolicy::FindOrFullyLoad).ToString(); return Label.Contains(TEXT("MISSING")) ? Id.ToString() : Label;
}
FName SelectedId(UComboBoxString* Box,UVTGameData* Data) {
 if(!Box) return NAME_None; FString Name=Box->GetSelectedOption();
 for(const auto& D:Data->Ships) if(DisplayId(D.Id)==Name) return D.Id;
 for(const auto& O:Data->Loadouts) if(DisplayId(O.Id)==Name) return O.Id;
 return NAME_None;
}
}
void UVTUI::NativeConstruct() {
 Super::NativeConstruct();
 StatusText=Cast<UTextBlock>(GetWidgetFromName(TEXT("Status"))); FlightText=Cast<UTextBlock>(GetWidgetFromName(TEXT("Flight"))); MenuPanel=Cast<UPanelWidget>(GetWidgetFromName(TEXT("MenuPanel"))); HUDPanel=Cast<UPanelWidget>(GetWidgetFromName(TEXT("HUDPanel")));
 BatterySecond=Cast<UComboBoxString>(GetWidgetFromName(TEXT("BatterySecond"))); BatteryThird=Cast<UComboBoxString>(GetWidgetFromName(TEXT("BatteryThird"))); SpecialSecond=Cast<UComboBoxString>(GetWidgetFromName(TEXT("SpecialSecond"))); SpecialThird=Cast<UComboBoxString>(GetWidgetFromName(TEXT("SpecialThird")));
 HullChoice=Cast<UComboBoxString>(GetWidgetFromName(TEXT("HullChoice"))); BatteryChoice=Cast<UComboBoxString>(GetWidgetFromName(TEXT("BatteryChoice"))); GunChoice=Cast<UComboBoxString>(GetWidgetFromName(TEXT("GunChoice"))); SpecialChoice=Cast<UComboBoxString>(GetWidgetFromName(TEXT("SpecialChoice"))); LANChoice=Cast<UComboBoxString>(GetWidgetFromName(TEXT("LANChoice"))); Address=Cast<UEditableTextBox>(GetWidgetFromName(TEXT("Address"))); WorldName=Cast<UEditableTextBox>(GetWidgetFromName(TEXT("WorldName")));
#define BIND(Name,Method) if(auto* B=Cast<UButton>(GetWidgetFromName(TEXT(Name)))) B->OnClicked.AddDynamic(this,&UVTUI::Method)
 BIND("GraphicsLow",PerformanceGraphics);BIND("GraphicsBalanced",BalancedGraphics);BIND("GraphicsHigh",HighGraphics);BIND("Create",CreateWorld); BIND("Continue",ContinueWorld); BIND("Join",JoinAddress); BIND("Discover",FindLAN); BIND("JoinLAN",JoinLAN); BIND("Skirmish",Skirmish); BIND("Range",TestRange); BIND("Resume",Resume); BIND("Leave",Leave); BIND("Quit",Quit); BIND("Save",Save); BIND("Repair",Repair); BIND("Undock",Undock); BIND("PayHeat",PayHeat); BIND("Refit",Refit); BIND("Recover",Recover);
#undef BIND
 auto* GI=CastChecked<UVTGameInstance>(GetGameInstance()); auto* Data=GetWorld()->GetSubsystem<UVTSimulation>()->Data.Get();
 if(HullChoice) {for(const auto& D:Data->Ships) if(D.Id.ToString().StartsWith(TEXT("corsair"))) HullChoice->AddOption(DisplayId(D.Id)); HullChoice->SetSelectedOption(DisplayId(GI->SelectedHull));}
 if(GunChoice&&BatteryChoice&&SpecialChoice) {GunChoice->AddOption(TEXT("Hull default")); BatteryChoice->AddOption(TEXT("Hull default")); SpecialChoice->AddOption(TEXT("Hull default"));
  for(const auto& O:Data->Loadouts) (O.Slot==EVTLoadoutSlot::Broadside ? GunChoice : O.Slot==EVTLoadoutSlot::Battery ? BatteryChoice : SpecialChoice)->AddOption(DisplayId(O.Id));
  GunChoice->SetSelectedOption(GI->SelectedFit.Broadside.IsNone() ? TEXT("Hull default") : DisplayId(GI->SelectedFit.Broadside)); BatteryChoice->SetSelectedOption(GI->SelectedFit.Battery.IsNone() ? TEXT("Hull default") : DisplayId(GI->SelectedFit.Battery)); SpecialChoice->SetSelectedOption(GI->SelectedFit.Special.IsNone() ? TEXT("Hull default") : DisplayId(GI->SelectedFit.Special));}
 for(auto* Box:{BatterySecond.Get(),BatteryThird.Get(),SpecialSecond.Get(),SpecialThird.Get()}) if(Box) {Box->AddOption(TEXT("Empty mount")); Box->SetSelectedOption(TEXT("Empty mount"));}
 for(const auto& O:Data->Loadouts) {if(O.Slot==EVTLoadoutSlot::Battery) {if(BatterySecond) BatterySecond->AddOption(DisplayId(O.Id)); if(BatteryThird) BatteryThird->AddOption(DisplayId(O.Id));} if(O.Slot==EVTLoadoutSlot::Special) {if(SpecialSecond) SpecialSecond->AddOption(DisplayId(O.Id)); if(SpecialThird) SpecialThird->AddOption(DisplayId(O.Id));}}
 if(auto* Box=Cast<UComboBoxString>(GetWidgetFromName(TEXT("PopulationChoice")))) {Box->AddOption(TEXT("Authored")); Box->AddOption(TEXT("Shared sandbox")); Box->SetSelectedOption(GI->PopulationProfile.ToString());}
 for(auto Pair:{TPair<UComboBoxString*,FName>(BatterySecond,GI->SelectedFit.Batteries.IsValidIndex(1) ? GI->SelectedFit.Batteries[1] : NAME_None),TPair<UComboBoxString*,FName>(BatteryThird,GI->SelectedFit.Batteries.IsValidIndex(2) ? GI->SelectedFit.Batteries[2] : NAME_None),TPair<UComboBoxString*,FName>(SpecialSecond,GI->SelectedFit.Specials.IsValidIndex(1) ? GI->SelectedFit.Specials[1] : NAME_None),TPair<UComboBoxString*,FName>(SpecialThird,GI->SelectedFit.Specials.IsValidIndex(2) ? GI->SelectedFit.Specials[2] : NAME_None)}) if(Pair.Key&&!Pair.Value.IsNone()) Pair.Key->SetSelectedOption(DisplayId(Pair.Value));
 SetMenu(GetWorld()->GetMapName().Contains(TEXT("Menu")));
}
void UVTUI::SetMenu(bool Open) {
 MenuOpen=Open;FocusPending=Open; if(MenuPanel&&MenuPanel->GetParent()) MenuPanel->GetParent()->SetVisibility(Open ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); if(MenuPanel) MenuPanel->SetVisibility(Open ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); if(HUDPanel) HUDPanel->SetVisibility(Open ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);if(HUDPanel&&HUDPanel->GetParent())HUDPanel->GetParent()->SetVisibility(Open ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
 if(auto* PC=GetOwningPlayer()) {if(Open) {FInputModeGameAndUI Mode; auto* Focus=GetWidgetFromName(GetWorld()->GetMapName().Contains(TEXT("Menu"))?TEXT("Create"):TEXT("Resume"));Mode.SetWidgetToFocus(Focus ? Focus->TakeWidget() : TakeWidget()); Mode.SetHideCursorDuringCapture(false); PC->SetInputMode(Mode);} else PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor=true;
  if(GetWorld()->GetNetMode()==NM_Standalone&&!GetWorld()->GetMapName().Contains(TEXT("Menu"))) PC->SetPause(Open);}
}
bool UVTUI::StoreFit() {
 auto* Data=GetWorld()->GetSubsystem<UVTSimulation>()->Data.Get();
 auto* GI=CastChecked<UVTGameInstance>(GetGameInstance()); if(auto* Box=Cast<UComboBoxString>(GetWidgetFromName(TEXT("PopulationChoice")))) GI->PopulationProfile=FName(Box->GetSelectedOption()); if(HullChoice) GI->SelectedHull=SelectedId(HullChoice,Data);
 auto Choice=[&](UComboBoxString* Box){return SelectedId(Box,Data);};
 GI->SelectedFit.Batteries.Reset(); GI->SelectedFit.Specials.Reset();
 auto Add=[&](TArray<FName>& Names,UComboBoxString* Box) {if(Box&&Box->GetSelectedOption()!=TEXT("Empty mount")&&Box->GetSelectedOption()!=TEXT("Hull default")) Names.AddUnique(SelectedId(Box,Data));};
 Add(GI->SelectedFit.Batteries,BatteryChoice); Add(GI->SelectedFit.Batteries,BatterySecond); Add(GI->SelectedFit.Batteries,BatteryThird); Add(GI->SelectedFit.Specials,SpecialChoice); Add(GI->SelectedFit.Specials,SpecialSecond); Add(GI->SelectedFit.Specials,SpecialThird);
 GI->SelectedFit.Broadside=Choice(GunChoice); GI->SelectedFit.Battery=Choice(BatteryChoice); GI->SelectedFit.Special=Choice(SpecialChoice);
 FVTShipDefinition Resolved; if(!Data->ResolveFit(GI->SelectedHull,GI->SelectedFit,Resolved)) {GetGameInstance()->GetSubsystem<UVTSessionSubsystem>()->Status=TEXT("This fit exceeds the selected hull mounts. Remove extra modules and try again."); return false;}
 GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->RememberFit(); return true;
}
void UVTUI::CreateWorld() {if(!StoreFit()) return; GetGameInstance()->GetSubsystem<UVTSessionSubsystem>()->CreateWorld(false,WorldName ? WorldName->GetText().ToString() : TEXT("Campaign"));}
void UVTUI::ContinueWorld() {if(!StoreFit()) return; GetGameInstance()->GetSubsystem<UVTSessionSubsystem>()->CreateWorld(true,WorldName ? WorldName->GetText().ToString() : TEXT("Campaign"));}
void UVTUI::JoinAddress() {if(!StoreFit()) return; if(Address) CastChecked<UVTGameInstance>(GetGameInstance())->Join(Address->GetText().ToString());}
void UVTUI::FindLAN() {GetGameInstance()->GetSubsystem<UVTSessionSubsystem>()->Discover();}
void UVTUI::JoinLAN() {if(!StoreFit()) return; if(LANChoice) GetGameInstance()->GetSubsystem<UVTSessionSubsystem>()->JoinWorld(LANChoice->GetSelectedIndex());}
void UVTUI::Skirmish() {if(!StoreFit()) return; CastChecked<UVTGameInstance>(GetGameInstance())->StartSolo(TEXT("skirmish"));}
void UVTUI::TestRange() {if(!StoreFit()) return; CastChecked<UVTGameInstance>(GetGameInstance())->StartSolo(TEXT("range"));}
void UVTUI::Resume() {SetMenu(false);}
void UVTUI::Leave() {CastChecked<UVTGameInstance>(GetGameInstance())->ReturnToMenu();}
void UVTUI::Quit() {GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->Save(); UKismetSystemLibrary::QuitGame(this,GetOwningPlayer(),EQuitPreference::Quit,false);}
void UVTUI::Save() {GetGameInstance()->GetSubsystem<UVTSessionSubsystem>()->Status=GetWorld()->GetNetMode()==NM_Client ? TEXT("The host saves this world.") : GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->Save() ? TEXT("World saved.") : TEXT("World could not be saved; previous snapshot retained.");}
void UVTUI::Recover() {if(auto* PC=Cast<AVTController>(GetOwningPlayer())) PC->ServerRecover(); SetMenu(false);}
void UVTUI::Repair() {if(auto* PC=Cast<AVTController>(GetOwningPlayer())) PC->ServerStationAction(TEXT("repair"));}
void UVTUI::Undock() {if(auto* PC=Cast<AVTController>(GetOwningPlayer())) PC->ServerStationAction(TEXT("undock")); SetMenu(false);}
void UVTUI::PayHeat() {if(auto* PC=Cast<AVTController>(GetOwningPlayer())) PC->ServerStationAction(TEXT("pay_heat"));}
void UVTUI::Refit() {if(!StoreFit()) return; auto* GI=CastChecked<UVTGameInstance>(GetGameInstance()); if(auto* PC=Cast<AVTController>(GetOwningPlayer())) PC->ServerRefit(GI->SelectedHull,GI->SelectedFit);}
void UVTUI::NativeTick(const FGeometry& Geometry,float Dt) {
 Super::NativeTick(Geometry,Dt); if(auto* Button=Cast<UButton>(GetWidgetFromName(TEXT("Recover")))) {auto* Ship=Cast<AVTShip>(GetOwningPlayerPawn()); Button->SetIsEnabled(Ship&&Ship->Disabled&&!GetWorld()->GetSubsystem<UVTSimulation>()->ActiveScenario());}
 auto* Session=GetGameInstance()->GetSubsystem<UVTSessionSubsystem>();
 const bool Frontend=GetWorld()->GetMapName().Contains(TEXT("Menu"));auto* Vessel=Cast<AVTShip>(GetOwningPlayerPawn());
 auto Visible=[&](const TCHAR* Name,bool Show){if(auto* W=GetWidgetFromName(Name))W->SetVisibility(Show?ESlateVisibility::Visible:ESlateVisibility::Collapsed);};
 for(const TCHAR* Name:{TEXT("WorldHeading"),TEXT("SoloHeading"),TEXT("WorldName"),TEXT("PopulationChoice"),TEXT("Create"),TEXT("Continue"),TEXT("Address"),TEXT("Join"),TEXT("Discover"),TEXT("LANChoice"),TEXT("JoinLAN"),TEXT("Skirmish"),TEXT("Range")})Visible(Name,Frontend);
 for(const TCHAR* Name:{TEXT("StationHeading"),TEXT("Repair"),TEXT("PayHeat"),TEXT("Refit"),TEXT("Undock")})Visible(Name,!Frontend&&Vessel&&Vessel->Docked);
 for(const TCHAR* Name:{TEXT("Recover"),TEXT("Resume"),TEXT("Leave")})Visible(Name,!Frontend);
 Visible(TEXT("Save"),!Frontend&&GetWorld()->GetNetMode()!=NM_Client&&!GetWorld()->GetSubsystem<UVTSimulation>()->ActiveScenario());
 Visible(TEXT("FitRow"),Frontend||(Vessel&&Vessel->Docked));Visible(TEXT("MountRow"),Frontend||(Vessel&&Vessel->Docked));Visible(TEXT("FitHeading"),Frontend||(Vessel&&Vessel->Docked));Visible(TEXT("MountHeading"),Frontend||(Vessel&&Vessel->Docked));
 if(FocusPending&&MenuOpen){if(auto* Focus=GetWidgetFromName(Frontend?TEXT("Create"):TEXT("Resume")))Focus->SetUserFocus(GetOwningPlayer());FocusPending=false;}
 for(const TCHAR* Name:{TEXT("HullChoice"),TEXT("GunChoice"),TEXT("BatteryChoice"),TEXT("SpecialChoice"),TEXT("BatterySecond"),TEXT("BatteryThird"),TEXT("SpecialSecond"),TEXT("SpecialThird")})Visible(Name,Frontend||(Vessel&&Vessel->Docked));
 if(auto* Graphics=Cast<UTextBlock>(GetWidgetFromName(TEXT("GraphicsStatus"))))if(GEngine)if(auto* Settings=GEngine->GetGameUserSettings()){int Level=Settings->GetOverallScalabilityLevel();Graphics->SetText(FText::FromString(FString::Printf(TEXT("Graphics: %s"),Level==1?TEXT("Performance"):Level==2?TEXT("Balanced"):Level==3?TEXT("High"):TEXT("Custom"))));}
 if(StatusText) {FString Status=Session->Status; if(GetWorld()->GetMapName().Contains(TEXT("Menu"))) if(auto* Career=GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->Career.Get()) Status+=FString::Printf(TEXT("\nCareer: %d completed runs, %d victories, wave %d, %d prizes"),Career->Runs,Career->Victories,Career->DeepestWave,Career->ShipsBoarded); if(MenuOpen&&!GetWorld()->GetMapName().Contains(TEXT("Menu"))) if(auto* Player=GetOwningPlayerState<AVTPlayerState>()) {auto* Data=GetWorld()->GetSubsystem<UVTSimulation>()->Data.Get(); for(int I=0;I<Data->TrackedFactions.Num();++I) if(Player->Reputation.IsValidIndex(I)&&Player->Heat.IsValidIndex(I)) Status+=FString::Printf(TEXT("%s%s: standing %.0f / heat %.0f  "),I%2==0 ? TEXT("\n") : TEXT(" | "),*Data->TrackedFactions[I].ToString(),Player->Reputation[I],Player->Heat[I]);} StatusText->SetText(FText::FromString(Status));}
 if(LANChoice&&LastWorlds!=Session->Worlds.Num()) {LastWorlds=Session->Worlds.Num(); LANChoice->ClearOptions(); for(const auto& Name:Session->Worlds) LANChoice->AddOption(Name); if(LastWorlds>0) LANChoice->SetSelectedIndex(0);}
 auto* PC=Cast<AVTController>(GetOwningPlayer()); auto* Ship=PC ? Cast<AVTShip>(PC->GetPawn()) : nullptr; if(!Ship||!FlightText) return;
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>(); auto* PS=PC->GetPlayerState<AVTPlayerState>(); auto* State=GetWorld()->GetGameState<AVTGameState>();
 FString Text=FString::Printf(TEXT("VOID & THUNDER  |  %s\nHull %.0f / %.0f   Battery %.1f / %.1f   EMP %.0f%%\nShields  Bow %.0f  Stern %.0f  Port %.0f  Starboard %.0f\nPort %.1fs  Starboard %.1fs   Torpedoes %.0f + %d   Locks %d   Mines %d\nCredits %d   Prizes %d   Captains %d\n%s %.1fs   Dock %.1fs   Boarding %.1fs\nW/S thrust   A/D turn   Mouse aim   LMB/RMB broadsides\nQ disruptor   Ctrl torpedoes (hold/release)   Shift warp (hold/release)\nSpace boost   C brace   B interact   M mines   X point defence   Esc menu"),*Sim->Data->Systems[Ship->SystemIndex].DisplayName.ToString(),Ship->Attributes->Hull.GetCurrentValue(),Ship->Definition.Hull,Ship->Attributes->Battery.GetCurrentValue(),Ship->Definition.BatteryMax,Ship->Attributes->EMPStress.GetCurrentValue()/Ship->Definition.EMPResist*100,Ship->Combat->Shields.X,Ship->Combat->Shields.Y,Ship->Combat->Shields.Z,Ship->Combat->Shields.W,Ship->PortReload,Ship->StarboardReload,Ship->Combat->EquipmentState.Loaded,Ship->Combat->EquipmentState.TorpedoMagazine,Ship->Combat->EquipmentState.Locks.Num(),Ship->Combat->EquipmentState.MineMagazine,PS ? PS->Credits : 0,PS ? PS->Boarded : 0,State ? State->PlayerArray.Num() : 0,*Ship->JumpDestination.ToString(),Ship->JumpProgress,Ship->DockProgress,Ship->Combat->BoardingProgress);
 if(State&&Sim->ActiveScenario()) {Text+=FString::Printf(TEXT("\nWave %d   Enemies %d   Focus %.1f / %.1f"),State->Wave,State->EnemiesRemaining,PC->AimBattery,Sim->Data->Feel.time.battery_max);}
 else if(PS) {int Owner=Sim->Data->FactionIndex(Sim->Data->Systems[Ship->SystemIndex].Owner); if(PS->Reputation.IsValidIndex(Owner)&&PS->Heat.IsValidIndex(Owner)) Text+=FString::Printf(TEXT("\nLocal standing %.0f   Heat %.0f   Clearance %d credits"),PS->Reputation[Owner],PS->Heat[Owner],FMath::CeilToInt(PS->Heat[Owner]*Sim->Data->Rules.CreditsPerHeat));}
 if(Ship->Disabled) Text+=TEXT("\nSHIP DISABLED — R / D-pad down to recover at a station (sandbox).");
 if(State&&!State->Outcome.IsEmpty()) Text+=TEXT("\n")+State->Outcome;
 FlightText->SetText(FText::FromString(Text)); if(Ship->Docked&&!MenuOpen) SetMenu(true);
}
void AVTController::ToggleMenu() {if(UI) UI->SetMenu(!UI->MenuOpen); LocalIntent=FVTPilotIntent();}

int32 UVTUI::NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Culling,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool Enabled) const {
 int32 Top=Super::NativePaint(Args,Geometry,Culling,Elements,Layer,Style,Enabled)+1;
 auto* PC=GetOwningPlayer(); auto* Mine=PC ? Cast<AVTShip>(PC->GetPawn()) : nullptr; if(MenuOpen||!Mine) return Top;
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>(); auto* State=GetWorld()->GetGameState<AVTGameState>(); auto Font=FCoreStyle::GetDefaultFontStyle("Regular",10);
 auto Line=[&](FVector2D A,FVector2D B,FLinearColor Colour) {TArray<FVector2D> Points={A,B}; FSlateDrawElement::MakeLines(Elements,Top,Geometry.ToPaintGeometry(),Points,ESlateDrawEffect::None,Colour,true,1);};
 for(AVTShip* Other:Sim->Ships) if(IsValid(Other)&&Other!=Mine&&Other->SystemIndex==Mine->SystemIndex) {
  FVector2D P; if(!PC->ProjectWorldLocationToScreen(Other->GetActorLocation(),P,true)) continue; P=Geometry.AbsoluteToLocal(P);
  if(P.X<10||P.Y<10||P.X>Geometry.GetLocalSize().X-10||P.Y>Geometry.GetLocalSize().Y-10) continue;
  FLinearColor Colour=Other->Disabled ? FLinearColor::Yellow : Other->Faction==Mine->Faction ? FLinearColor(0.3f,0.8f,1) : FLinearColor(1,0.4f,0.25f);
  Line(P+FVector2D(-18,20),P+FVector2D(18,20),FLinearColor(0.2f,0.2f,0.2f)); Line(P+FVector2D(-18,20),P+FVector2D(-18+36*Other->Attributes->Hull.GetCurrentValue()/Other->Definition.Hull,20),Colour);
  if(Mine->Combat->EquipmentState.Locks.Contains(Other->PersistentId)) {Line(P+FVector2D(-20,-20),P+FVector2D(20,20),FLinearColor::Yellow); Line(P+FVector2D(-20,20),P+FVector2D(20,-20),FLinearColor::Yellow);}
  if(Other->Disabled) FSlateDrawElement::MakeText(Elements,Top,Geometry.ToPaintGeometry(FVector2D(100,20),FSlateLayoutTransform(P+FVector2D(-20,25))),TEXT("BOARD"),Font,ESlateDrawEffect::None,Colour);
 }
 if(auto* Captain=Cast<AVTController>(PC)) if((Captain->LocalIntent.Buttons&VTButtons::Warp)&&Mine->Definition.Equipment.Warp&&Mine->Combat->EquipmentState.WarpCooldown<=0) {FVector2D P; if(PC->ProjectWorldLocationToScreen(VT::ToWorld(Mine->Movement->Motion.Position+Captain->LocalIntent.CursorOffset.GetClampedToMaxSize(Mine->Definition.Equipment.WarpRange),Mine->SystemIndex),P,true)) {P=Geometry.AbsoluteToLocal(P); Line(P+FVector2D(-25,0),P+FVector2D(25,0),FLinearColor(0.2f,1,0.8f)); Line(P+FVector2D(0,-25),P+FVector2D(0,25),FLinearColor(0.2f,1,0.8f));}}
 if(CastChecked<UVTGameInstance>(GetGameInstance())->PlayMode!=TEXT("sandbox")) return Top;
 FVector2D Offset(Geometry.GetLocalSize().X-430,35),Size(405,240);
 FSlateDrawElement::MakeBox(Elements,Top,Geometry.ToPaintGeometry(Size,FSlateLayoutTransform(Offset)),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,FLinearColor(0.015f,0.025f,0.04f,0.8f)); ++Top;
 FVector2D Min(DBL_MAX,DBL_MAX),Max(-DBL_MAX,-DBL_MAX);
 for(const auto& System:Sim->Data->Systems) {Min.X=FMath::Min(Min.X,System.ChartPosition.X); Min.Y=FMath::Min(Min.Y,System.ChartPosition.Y); Max.X=FMath::Max(Max.X,System.ChartPosition.X); Max.Y=FMath::Max(Max.Y,System.ChartPosition.Y);}
 auto Point=[&](int I) {auto P=Sim->Data->Systems[I].ChartPosition; return Offset+FVector2D(25,25)+FVector2D((P.X-Min.X)/FMath::Max(1.,Max.X-Min.X)*240,(P.Y-Min.Y)/FMath::Max(1.,Max.Y-Min.Y)*165);};
 for(int I=0;I<Sim->Data->Systems.Num();++I) {for(FName LinkId:Sim->Data->Systems[I].Links) {int J=Sim->Data->FindSystem(LinkId); if(J>I) Line(Point(I),Point(J),FLinearColor(0.25f,0.4f,0.5f));}}
 for(int I=0;I<Sim->Data->Systems.Num();++I) {auto P=Point(I); auto Colour=I==Mine->SystemIndex ? FLinearColor(0.3f,1,0.8f) : FLinearColor(0.65f,0.75f,0.85f); FString Label=Sim->Data->Systems[I].DisplayName.ToString()+FString::Printf(TEXT(" (%d)"),State&&State->Populations.IsValidIndex(I) ? State->Populations[I] : 0); FSlateDrawElement::MakeBox(Elements,Top,Geometry.ToPaintGeometry(FVector2D(5,5),FSlateLayoutTransform(P)),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,Colour); FSlateDrawElement::MakeText(Elements,Top,Geometry.ToPaintGeometry(FVector2D(110,20),FSlateLayoutTransform(P+FVector2D(7,0))),Label,Font,ESlateDrawEffect::None,Colour);}
 return Top;
}

void UVTUI::ApplyGraphics(int32 Preset){if(GEngine)if(auto* Settings=GEngine->GetGameUserSettings()){Settings->SetOverallScalabilityLevel(Preset);Settings->ScalabilityQuality.ResolutionQuality=100;Settings->ApplySettings(false);Settings->SaveSettings();}}
void UVTUI::PerformanceGraphics(){ApplyGraphics(1);}
void UVTUI::BalancedGraphics(){ApplyGraphics(2);}
void UVTUI::HighGraphics(){ApplyGraphics(3);}
