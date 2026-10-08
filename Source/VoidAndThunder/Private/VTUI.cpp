#include "VTUI.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Internationalization/StringTable.h"
#include "Blueprint/WidgetTree.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "VTGameplay.h"
#include "VTCombat.h"
#include "VTSessionSubsystem.h"
#include "VTSaveSubsystem.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/PanelWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

FVTInteractionHint VTInteractionHint(const AVTShip* Ship,const UVTSimulation* Sim) {
 FVTInteractionHint Hint;if(!Ship||!Sim||!Sim->Data||Ship->Disabled||Ship->Docked||Ship->Autopilot||!Sim->Data->Systems.IsValidIndex(Ship->SystemIndex))return Hint;
 const auto& Rules=Sim->Data->Rules;const auto Position=Ship->Movement->Motion.Position;
 if(Ship->Combat->BoardingTarget.IsValid())for(const AVTShip* Other:Sim->Queries.Ordered(Ship->SystemIndex))if(IsValid(Other)&&Other!=Ship&&Other->PersistentId==Ship->Combat->BoardingTarget&&Other->SystemIndex==Ship->SystemIndex&&Other->Disabled&&!Other->Invulnerable&&!Other->Combat->Claimed&&(Other->Movement->Motion.Position-Position).SizeSquared()<=FMath::Square(Rules.BoardRange)){
  Hint.Kind=EVTInteractionHint::Loot;Hint.Position=Other->Movement->Motion.Position;Hint.Label=NSLOCTEXT("VTUI","LootShip","Board and loot ship");Hint.Progress=FMath::Clamp(Ship->Combat->BoardingProgress/FMath::Max(0.001f,Rules.BoardDwell),0.f,1.f);return Hint;
 }
 const auto& System=Sim->Data->Systems[Ship->SystemIndex];
 if(System.Links.Contains(Ship->GatePassage->Status().Destination)){int32 Destination=Sim->Data->FindSystem(Ship->GatePassage->Status().Destination);const auto Jump=Sim->JumpPosition(Ship->SystemIndex,Ship->GatePassage->Status().Destination);if(Destination>=0&&(Position-Jump).SizeSquared()<FMath::Square(Sim->Data->GateInteractionRange())){Hint.Kind=EVTInteractionHint::Jump;Hint.Position=Jump;Hint.Label=FText::Format(NSLOCTEXT("VTUI","JumpTo","Align and fly through to {0}"),Sim->Data->Systems[Destination].DisplayName);Hint.Progress=FMath::Clamp(Ship->GatePassage->Status().Charge/FMath::Max(0.001f,Rules.JumpDwell),0.f,1.f);return Hint;}}
 if(System.HasStation&&(Position-Rules.StationPosition).SizeSquared()<=FMath::Square(Rules.StationRadius+Rules.BoardRange)){
  const auto* PS=Ship->GetPlayerState<AVTPlayerState>();const auto Standing=Sim->Standings.Read(PS?PS->Profile:FGuid(),System.Owner);const bool Allowed=!Standing.Found||Standing.Reputation>=Sim->Data->World.dock_refusal_threshold;
  if(Allowed){Hint.Kind=EVTInteractionHint::Dock;Hint.Position=Rules.StationPosition;Hint.Label=NSLOCTEXT("VTUI","HoldToDock","Hold position to dock");Hint.Progress=FMath::Clamp(Ship->DockProgress/FMath::Max(0.001f,Rules.BoardDwell),0.f,1.f);}
 }
 return Hint;
}

void UVTUI::NativeConstruct() {
 Super::NativeConstruct();
 WidgetCache.Reset(); ChoiceItems.Reset();
 TArray<UWidget*> Widgets; WidgetTree->GetAllWidgets(Widgets);
 for(auto* Widget:Widgets) WidgetCache.Add(Widget->GetFName(),Widget);
 GetGameInstance()->GetSubsystem<UVTSessionSubsystem>()->OnChanged.RemoveAll(this);
 GetGameInstance()->GetSubsystem<UVTSessionSubsystem>()->OnChanged.AddUObject(this,&UVTUI::RequestRefresh);
 StatusText=Cast<UTextBlock>(CachedWidget(TEXT("StatusText")) ? CachedWidget(TEXT("StatusText")) : CachedWidget(TEXT("Status"))); Flight=Cast<UTextBlock>(CachedWidget(TEXT("Flight"))); MenuPanel=Cast<UPanelWidget>(CachedWidget(TEXT("MenuPanel"))); HUDPanel=Cast<UPanelWidget>(CachedWidget(TEXT("HUDPanel")));
 BatterySecond=Cast<UComboBoxString>(CachedWidget(TEXT("BatterySecond"))); BatteryThird=Cast<UComboBoxString>(CachedWidget(TEXT("BatteryThird"))); SpecialSecond=Cast<UComboBoxString>(CachedWidget(TEXT("SpecialSecond"))); SpecialThird=Cast<UComboBoxString>(CachedWidget(TEXT("SpecialThird")));
 HullChoice=Cast<UComboBoxString>(CachedWidget(TEXT("HullChoice"))); BatteryChoice=Cast<UComboBoxString>(CachedWidget(TEXT("BatteryChoice"))); GunChoice=Cast<UComboBoxString>(CachedWidget(TEXT("GunChoice"))); SpecialChoice=Cast<UComboBoxString>(CachedWidget(TEXT("SpecialChoice"))); LANChoice=Cast<UComboBoxString>(CachedWidget(TEXT("LANChoice"))); Address=Cast<UEditableTextBox>(CachedWidget(TEXT("Address"))); WorldName=Cast<UEditableTextBox>(CachedWidget(TEXT("WorldName")));
#define BIND(Name,Method) if(auto* B=Cast<UButton>(CachedWidget(TEXT(Name)))) B->OnClicked.AddDynamic(this,&UVTUI::Method)
 BIND("WorldOptions",ToggleSessionOptions); BIND("Autopilot",ToggleAutopilot);BIND("GraphicsLow",PerformanceGraphics);BIND("GraphicsBalanced",BalancedGraphics);BIND("GraphicsHigh",HighGraphics);BIND("Create",CreateWorld); BIND("Continue",ContinueWorld); BIND("Join",JoinAddress); BIND("Discover",FindLAN); BIND("JoinLAN",JoinLAN); BIND("Skirmish",Skirmish); BIND("Range",TestRange); BIND("Resume",Resume); BIND("Leave",Leave); BIND("Quit",Quit); BIND("Save",Save); BIND("Repair",Repair); BIND("Undock",Undock); BIND("PayHeat",PayHeat); BIND("Refit",Refit); BIND("Recover",Recover);
#undef BIND
 for(auto* Box:{HullChoice.Get(),BatteryChoice.Get(),GunChoice.Get(),SpecialChoice.Get(),BatterySecond.Get(),BatteryThird.Get(),SpecialSecond.Get(),SpecialThird.Get()})if(Box)Box->ClearOptions();
 auto* GI=CastChecked<UVTGameInstance>(GetGameInstance()); auto* Data=GetWorld()->GetSubsystem<UVTSimulation>()->Data.Get();
 if(HullChoice) {for(const auto& D:Data->Ships) if(D.Id.ToString().StartsWith(TEXT("corsair"))) AddChoice(HullChoice,D.Id,FText::FromStringTable(TextTable?TextTable->GetStringTableId():FName(TEXT("/Game/UI/ST_UI.ST_UI")),FString(TEXT("class."))+D.Id.ToString()+TEXT(".name"))); HullChoice->SetSelectedOption(GI->SelectedHull.ToString());}
 if(GunChoice&&BatteryChoice&&SpecialChoice) {for(auto* Box:{GunChoice.Get(),BatteryChoice.Get(),SpecialChoice.Get()}) AddChoice(Box,NAME_None,NSLOCTEXT("VTUI","Default","Hull default"));
  for(const auto& O:Data->Loadouts) AddChoice(O.Slot==EVTLoadoutSlot::Broadside ? GunChoice.Get() : O.Slot==EVTLoadoutSlot::Battery ? BatteryChoice.Get() : SpecialChoice.Get(),O.Id,FText::FromStringTable(TextTable?TextTable->GetStringTableId():FName(TEXT("/Game/UI/ST_UI.ST_UI")),O.Id.ToString()+TEXT(".name")));
  for(auto* Box:{BatteryChoice.Get(),SpecialChoice.Get()})AddChoice(Box,FName("__empty"),NSLOCTEXT("VTUI","EmptyMount","Empty mount"));
  GunChoice->SetSelectedOption(GI->SelectedFit.Broadside.ToString());
  BatteryChoice->SetSelectedOption(!GI->SelectedFit.Batteries.IsEmpty()?GI->SelectedFit.Batteries[0].ToString():GI->SelectedFit.OverrideBatteries?TEXT("__empty"):GI->SelectedFit.Battery.ToString());
  SpecialChoice->SetSelectedOption(!GI->SelectedFit.Specials.IsEmpty()?GI->SelectedFit.Specials[0].ToString():GI->SelectedFit.OverrideSpecials?TEXT("__empty"):GI->SelectedFit.Special.ToString());}
 for(auto* Box:{BatterySecond.Get(),BatteryThird.Get(),SpecialSecond.Get(),SpecialThird.Get()}) if(Box) {AddChoice(Box,FName("__empty"),NSLOCTEXT("VTUI","EmptyMount","Empty mount")); Box->SetSelectedOption(TEXT("__empty"));}
 for(const auto& O:Data->Loadouts) {if(O.Slot==EVTLoadoutSlot::Battery) {if(BatterySecond) AddChoice(BatterySecond,O.Id,FText::FromStringTable(TextTable?TextTable->GetStringTableId():FName(TEXT("/Game/UI/ST_UI.ST_UI")),O.Id.ToString()+TEXT(".name"))); if(BatteryThird) AddChoice(BatteryThird,O.Id,FText::FromStringTable(TextTable?TextTable->GetStringTableId():FName(TEXT("/Game/UI/ST_UI.ST_UI")),O.Id.ToString()+TEXT(".name")));} if(O.Slot==EVTLoadoutSlot::Special) {if(SpecialSecond) AddChoice(SpecialSecond,O.Id,FText::FromStringTable(TextTable?TextTable->GetStringTableId():FName(TEXT("/Game/UI/ST_UI.ST_UI")),O.Id.ToString()+TEXT(".name"))); if(SpecialThird) AddChoice(SpecialThird,O.Id,FText::FromStringTable(TextTable?TextTable->GetStringTableId():FName(TEXT("/Game/UI/ST_UI.ST_UI")),O.Id.ToString()+TEXT(".name")));}}
 if(auto* Box=Cast<UComboBoxString>(CachedWidget(TEXT("PopulationChoice")))) {Box->AddOption(TEXT("Authored")); Box->AddOption(TEXT("Shared sandbox")); Box->SetSelectedOption(GI->PopulationProfile.ToString());}
 for(auto Pair:{TPair<UComboBoxString*,FName>(BatterySecond,GI->SelectedFit.Batteries.IsValidIndex(1) ? GI->SelectedFit.Batteries[1] : NAME_None),TPair<UComboBoxString*,FName>(BatteryThird,GI->SelectedFit.Batteries.IsValidIndex(2) ? GI->SelectedFit.Batteries[2] : NAME_None),TPair<UComboBoxString*,FName>(SpecialSecond,GI->SelectedFit.Specials.IsValidIndex(1) ? GI->SelectedFit.Specials[1] : NAME_None),TPair<UComboBoxString*,FName>(SpecialThird,GI->SelectedFit.Specials.IsValidIndex(2) ? GI->SelectedFit.Specials[2] : NAME_None)}) if(Pair.Key&&!Pair.Value.IsNone()) Pair.Key->SetSelectedOption(Pair.Value.ToString());
 FitEditor.Initialize(*Data,GI->SelectedHull,GI->SelectedFit);BuildFitEditor();
 SetMenu(GetWorld()->GetMapName().Contains(TEXT("Menu")));
 RefreshView();
}
void UVTUI::SetMenu(bool Open) {
 MenuOpen=Open;FocusPending=Open;if(auto* W=CachedWidget(TEXT("MenuBackdrop")))W->SetVisibility(Open?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed); if(auto* W=CachedWidget(TEXT("AbilityPanel")))W->SetVisibility(ESlateVisibility::Collapsed);
 if(auto* Captain=Cast<AVTController>(GetOwningPlayer())) Captain->UpdateInputContexts();
 RequestRefresh(); if(MenuPanel&&MenuPanel->GetParent()) MenuPanel->GetParent()->SetVisibility(Open ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); if(MenuPanel) MenuPanel->SetVisibility(Open ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); if(HUDPanel) HUDPanel->SetVisibility(ESlateVisibility::Collapsed);if(HUDPanel&&HUDPanel->GetParent())HUDPanel->GetParent()->SetVisibility(ESlateVisibility::Collapsed);
 if(auto* PC=GetOwningPlayer()) {if(Open) {FInputModeGameAndUI Mode; auto* Focus=CachedWidget(GetWorld()->GetMapName().Contains(TEXT("Menu"))?(SessionOptions?TEXT("Create"):TEXT("Skirmish")):TEXT("Resume"));Mode.SetWidgetToFocus(Focus ? Focus->TakeWidget() : TakeWidget()); Mode.SetHideCursorDuringCapture(false); PC->SetInputMode(Mode);} else {FInputModeGameOnly Mode;Mode.SetConsumeCaptureMouseDown(false);PC->SetInputMode(Mode);} PC->bShowMouseCursor=true;
  if(GetWorld()->GetNetMode()==NM_Standalone&&!GetWorld()->GetMapName().Contains(TEXT("Menu"))) PC->SetPause(Open);}
}
void UVTFitCheckBox::Changed(bool Checked){if(auto* UI=Editor.Get())if(!UI->FitUpdating)UI->ToggleFit(Equipment);}
void UVTUI::BuildFitEditor(){
 auto* Data=GetWorld()->GetSubsystem<UVTSimulation>()->Data.Get();
 auto* Row=Cast<UPanelWidget>(CachedWidget(TEXT("MountRow")));if(!Row)return;Row->ClearChildren();
 auto* Column=WidgetTree->ConstructWidget<UVerticalBox>();Row->AddChild(Column);
 if(auto* HullRow=Cast<UPanelWidget>(CachedWidget(TEXT("FitRow"))))while(HullRow->GetChildrenCount()>1)HullRow->RemoveChildAt(HullRow->GetChildrenCount()-1);
 if(auto* Heading=Cast<UTextBlock>(CachedWidget(TEXT("MountHeading"))))Heading->SetText(NSLOCTEXT("VTFit","Heading","Loadout"));
 auto* Defaults=WidgetTree->ConstructWidget<UVTFitCheckBox>();Defaults->Equipment=FName("__defaults");Defaults->Editor=this;auto* DefaultLabel=WidgetTree->ConstructWidget<UTextBlock>();if(HullChoice){DefaultLabel->SetFont(HullChoice->GetFont());DefaultLabel->SetColorAndOpacity(HullChoice->GetForegroundColor());}DefaultLabel->SetText(NSLOCTEXT("VTFit","DefaultFit","Use hull default equipment"));Defaults->AddChild(DefaultLabel);Defaults->OnCheckStateChanged.AddDynamic(Defaults,&UVTFitCheckBox::Changed);Column->AddChild(Defaults);FitChecks.Add(Defaults);
 FitSummary=WidgetTree->ConstructWidget<UTextBlock>();FitSummary->SetAutoWrapText(true);if(HullChoice){FitSummary->SetFont(HullChoice->GetFont());FitSummary->SetColorAndOpacity(HullChoice->GetForegroundColor());}Column->AddChild(FitSummary);
 for(int MountSlot=0;MountSlot<3;++MountSlot){auto* Line=WidgetTree->ConstructWidget<UHorizontalBox>();Column->AddChild(Line);
 for(const auto& O:Data->Loadouts)if(int(O.Slot)==MountSlot){auto* Check=WidgetTree->ConstructWidget<UVTFitCheckBox>();Check->Equipment=O.Id;Check->Editor=this;auto* Label=WidgetTree->ConstructWidget<UTextBlock>();if(HullChoice){Label->SetFont(HullChoice->GetFont());Label->SetColorAndOpacity(HullChoice->GetForegroundColor());}Label->SetMargin(FMargin(6,2));Label->SetText(FText::FromStringTable(TextTable?TextTable->GetStringTableId():FName(TEXT("/Game/UI/ST_UI.ST_UI")),O.Id.ToString()+TEXT(".name")));Check->AddChild(Label);Check->OnCheckStateChanged.AddDynamic(Check,&UVTFitCheckBox::Changed);Line->AddChild(Check);FitChecks.Add(Check);}}
 if(HullChoice)HullChoice->OnSelectionChanged.AddDynamic(this,&UVTUI::ChangeFitHull);RefreshFitEditor();
}
void UVTUI::RefreshFitEditor(const FText& Reason){FitUpdating=true;const auto& P=FitEditor.Preview();for(const auto& Check:FitChecks){const auto& F=P.Selection;Check->SetIsChecked(Check->Equipment==FName("__defaults")?F.Broadside.IsNone()&&!F.OverrideBatteries&&!F.OverrideSpecials&&F.Battery.IsNone()&&F.Special.IsNone():F.Broadside==Check->Equipment||F.Batteries.Contains(Check->Equipment)||F.Specials.Contains(Check->Equipment)||(F.Batteries.IsEmpty()&&!F.OverrideBatteries&&F.Battery==Check->Equipment)||(F.Specials.IsEmpty()&&!F.OverrideSpecials&&F.Special==Check->Equipment));}if(HullChoice)HullChoice->SetSelectedOption(P.Hull.ToString());if(FitSummary)FitSummary->SetText(Reason.IsEmpty()?P.Summary():FText::Format(NSLOCTEXT("VTFit","Feedback","{0}\n{1}"),P.Summary(),Reason));FitUpdating=false;}
void UVTUI::ToggleFit(FName Equipment){FText Reason;if(Equipment==FName("__defaults"))FitEditor.Initialize(*GetWorld()->GetSubsystem<UVTSimulation>()->Data,FitEditor.Preview().Hull,{});else FitEditor.Toggle(*GetWorld()->GetSubsystem<UVTSimulation>()->Data,Equipment,Reason);RefreshFitEditor(Reason);}
void UVTUI::ChangeFitHull(FString Item,ESelectInfo::Type Type){if(FitUpdating)return;FText Reason;FitEditor.SelectHull(*GetWorld()->GetSubsystem<UVTSimulation>()->Data,SelectedId(HullChoice),Reason);RefreshFitEditor(Reason);}
bool UVTUI::StoreFit() {
 auto* GI=CastChecked<UVTGameInstance>(GetGameInstance());
 if(!FitEditor.Preview().Valid)return false;
 GI->SelectedHull=FitEditor.Preview().Hull;GI->SelectedFit=FitEditor.Preview().Selection;
 if(auto* Box=Cast<UComboBoxString>(CachedWidget(TEXT("PopulationChoice"))))GI->PopulationProfile=FName(Box->GetSelectedOption());
 GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->RememberFit();return true;
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
void UVTUI::Save() {GetGameInstance()->GetSubsystem<UVTSessionSubsystem>()->SetStatus(GetWorld()->GetNetMode()==NM_Client ? TEXT("The host saves this world.") : GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->Save() ? TEXT("World saved.") : TEXT("World could not be saved; previous snapshot retained."));}
void UVTUI::Recover() {if(auto* PC=Cast<AVTController>(GetOwningPlayer())) PC->ServerRecover(); SetMenu(false);}
void UVTUI::Repair() {if(auto* PC=Cast<AVTController>(GetOwningPlayer())) PC->ServerStationAction(TEXT("repair"));}
void UVTUI::Undock() {if(auto* PC=Cast<AVTController>(GetOwningPlayer())) PC->ServerStationAction(TEXT("undock")); SetMenu(false);}
void UVTUI::PayHeat() {if(auto* PC=Cast<AVTController>(GetOwningPlayer())) PC->ServerStationAction(TEXT("pay_heat"));}
void UVTUI::Refit() {if(!StoreFit()) return; auto* GI=CastChecked<UVTGameInstance>(GetGameInstance()); if(auto* PC=Cast<AVTController>(GetOwningPlayer())) PC->ServerRefit(GI->SelectedHull,GI->SelectedFit);}
void UVTUI::RefreshView() {
 RefreshQueued=false;
 auto* CurrentShip=Cast<AVTShip>(GetOwningPlayerPawn()); auto* CurrentASC=CurrentShip ? CurrentShip->Abilities.Get() : nullptr;
 if(ObservedAbilities.Get()!=CurrentASC) {
  const FGameplayAttribute Attributes[]={UVTAttributes::HullAttribute(),UVTAttributes::BatteryAttribute(),UVTAttributes::GetEMPStressAttribute()};
  if(auto* Old=ObservedAbilities.Get()) for(int I=0;I<AttributeHandles.Num();++I) Old->GetGameplayAttributeValueChangeDelegate(Attributes[I]).Remove(AttributeHandles[I]);
  ObservedAbilities=CurrentASC; AttributeHandles.Reset();
  if(CurrentASC) for(const auto& Attribute:Attributes) AttributeHandles.Add(CurrentASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this,&UVTUI::AttributeChanged));
 }
 if(auto* Captain=Cast<AVTController>(GetOwningPlayer())) Captain->UpdateInputContexts();
 if(auto* Button=Cast<UButton>(CachedWidget(TEXT("Recover")))) {auto* Ship=Cast<AVTShip>(GetOwningPlayerPawn()); Button->SetIsEnabled(Ship&&Ship->Disabled&&!GetWorld()->GetSubsystem<UVTSimulation>()->ActiveScenario());}
 auto* Session=GetGameInstance()->GetSubsystem<UVTSessionSubsystem>();
 const bool Frontend=GetWorld()->GetMapName().Contains(TEXT("Menu"));auto* Vessel=Cast<AVTShip>(GetOwningPlayerPawn());
 auto Visible=[&](const TCHAR* Name,bool Show){if(auto* W=CachedWidget(Name))W->SetVisibility(Show?ESlateVisibility::Visible:ESlateVisibility::Collapsed);};
 for(const TCHAR* Name:{TEXT("WorldHeading"),TEXT("WorldName"),TEXT("PopulationChoice"),TEXT("Create"),TEXT("Continue"),TEXT("Address"),TEXT("Join"),TEXT("Discover"),TEXT("LANChoice"),TEXT("JoinLAN")})Visible(Name,Frontend&&SessionOptions);
 for(const TCHAR* Name:{TEXT("Skirmish"),TEXT("Range"),TEXT("WorldOptions")})Visible(Name,Frontend);
 Visible(TEXT("SoloHeading"),false);
 for(const TCHAR* Name:{TEXT("StationHeading"),TEXT("Repair"),TEXT("PayHeat"),TEXT("Refit"),TEXT("Undock")})Visible(Name,!Frontend&&Vessel&&Vessel->Docked);
 for(const TCHAR* Name:{TEXT("Recover"),TEXT("Resume"),TEXT("Leave")})Visible(Name,!Frontend);
 Visible(TEXT("Autopilot"),!Frontend&&Vessel&&!Vessel->Docked&&!Vessel->Disabled);
 if(auto* Label=Cast<UTextBlock>(CachedWidget(TEXT("AutopilotLabel"))))Label->SetText(FText::FromString(Vessel&&Vessel->Autopilot?TEXT("AI pilot: on"):TEXT("AI pilot: off")));
 Visible(TEXT("Save"),!Frontend&&GetWorld()->GetNetMode()!=NM_Client&&!GetWorld()->GetSubsystem<UVTSimulation>()->ActiveScenario());
 Visible(TEXT("FitRow"),Frontend||(Vessel&&Vessel->Docked));Visible(TEXT("MountRow"),(Frontend&&(!SessionOptions||CastChecked<UVTGameInstance>(GetGameInstance())->SkipIntro))||(Vessel&&Vessel->Docked));Visible(TEXT("FitHeading"),Frontend||(Vessel&&Vessel->Docked));Visible(TEXT("MountHeading"),(Frontend&&(!SessionOptions||CastChecked<UVTGameInstance>(GetGameInstance())->SkipIntro))||(Vessel&&Vessel->Docked));
 if(FocusPending&&MenuOpen){if(auto* Focus=CachedWidget(Frontend?(SessionOptions?TEXT("Create"):TEXT("Skirmish")):TEXT("Resume")))Focus->SetUserFocus(GetOwningPlayer());FocusPending=false;}
 for(const TCHAR* Name:{TEXT("HullChoice"),TEXT("GunChoice"),TEXT("BatteryChoice"),TEXT("SpecialChoice"),TEXT("BatterySecond"),TEXT("BatteryThird"),TEXT("SpecialSecond"),TEXT("SpecialThird")})Visible(Name,Frontend||(Vessel&&Vessel->Docked));
 for(auto* Box:{GunChoice.Get(),BatteryChoice.Get(),SpecialChoice.Get(),BatterySecond.Get(),BatteryThird.Get(),SpecialSecond.Get(),SpecialThird.Get()})if(Box)Box->SetVisibility(ESlateVisibility::Collapsed);
 if(auto* Graphics=Cast<UTextBlock>(CachedWidget(TEXT("GraphicsStatus"))))if(GEngine)if(auto* Settings=GEngine->GetGameUserSettings()){int Level=Settings->GetOverallScalabilityLevel();Graphics->SetText(FText::FromString(FString::Printf(TEXT("Graphics: %s"),Level==1?TEXT("Performance"):Level==2?TEXT("Balanced"):Level==3?TEXT("High"):TEXT("Custom"))));}
 if(StatusText) {FString Status=Session->Status; if(GetWorld()->GetMapName().Contains(TEXT("Menu"))) if(auto* Career=GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->Career.Get()) Status+=FString::Printf(TEXT("\nCareer: %d completed runs, %d victories, wave %d, %d prizes"),Career->Runs,Career->Victories,Career->DeepestWave,Career->ShipsBoarded); if(MenuOpen&&!GetWorld()->GetMapName().Contains(TEXT("Menu"))) if(auto* Player=GetOwningPlayerState<AVTPlayerState>()) {auto* Data=GetWorld()->GetSubsystem<UVTSimulation>()->Data.Get(); for(int I=0;I<Data->TrackedFactions.Num();++I) if(Player->Reputation.IsValidIndex(I)&&Player->Heat.IsValidIndex(I)) Status+=FString::Printf(TEXT("%s%s: standing %.0f / heat %.0f  "),I%2==0 ? TEXT("\n") : TEXT(" | "),*Data->TrackedFactions[I].ToString(),Player->Reputation[I],Player->Heat[I]);} StatusText->SetText(FText::FromString(Status));}
 if(LANChoice&&LastWorlds!=Session->Worlds.Num()) {LastWorlds=Session->Worlds.Num(); LANChoice->ClearOptions(); for(const auto& Name:Session->Worlds) LANChoice->AddOption(Name); if(LastWorlds>0) LANChoice->SetSelectedIndex(0);}
 auto* PC=Cast<AVTController>(GetOwningPlayer()); auto* Ship=PC ? Cast<AVTShip>(PC->GetPawn()) : nullptr; if(!Ship||!Flight) return;
 if(Ship->Docked&&!MenuOpen)SetMenu(true);
}
void AVTController::ToggleMenu() {if(UI) UI->SetMenu(!UI->MenuOpen); LocalIntent=FVTPilotIntent();}

int32 UVTUI::NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Culling,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool Enabled) const {
 int32 Top=Super::NativePaint(Args,Geometry,Culling,Elements,Layer+1,Style,Enabled)+1;
 auto* PC=GetOwningPlayer(); auto* Mine=PC ? Cast<AVTShip>(PC->GetPawn()) : nullptr; if(MenuOpen||!Mine) return Top;
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>(); auto* State=GetWorld()->GetGameState<AVTGameState>(); auto Font=HudFont;Font.Size=10;
 int32 ViewWidth=0,ViewHeight=0;PC->GetViewportSize(ViewWidth,ViewHeight);
 const FVector2D PixelToLocal=Geometry.GetLocalSize()/FVector2D(FMath::Max(1,ViewWidth),FMath::Max(1,ViewHeight));
 auto Line=[&](FVector2D A,FVector2D B,FLinearColor Colour) {TArray<FVector2D> Points={A,B}; FSlateDrawElement::MakeLines(Elements,Top,Geometry.ToPaintGeometry(),Points,ESlateDrawEffect::None,Colour,true,1);};
 if(auto* Captain=Cast<AVTController>(PC))for(bool Port:{true,false})if(Captain->LocalIntent.Buttons&(Port?VTButtons::AimPort:VTButtons::AimStarboard)) {
  auto Direction=VTCombat::BroadsideDirection(Mine->Movement->Motion.Heading,Port,Captain->LocalIntent.Aim,Mine->Definition.Arc);FVector2D A,B;
  for(int32 Gun=0;Gun<FMath::Max(1,Mine->Definition.Guns);++Gun) {
   const auto Shot=VTCombat::BroadsideShot(Mine->Movement->Motion.Position,Mine->Movement->Motion.Velocity,Direction,Mine->Definition,Sim->Data->Rules,Gun);
   if(PC->ProjectWorldLocationToScreen(VT::ToWorld(Shot.Key,Mine->SystemIndex),A,true)&&PC->ProjectWorldLocationToScreen(VT::ToWorld(Shot.Key+Shot.Value.GetSafeNormal()*Mine->Definition.MuzzleSpeed*Sim->Data->Rules.ProjectileTTL,Mine->SystemIndex),B,true))Line(A*PixelToLocal,B*PixelToLocal,(Port?Mine->PortReload:Mine->StarboardReload)<=0?FLinearColor(1,0.65f,0.15f):FLinearColor(0.45f,0.08f,0.05f));
  }
 }
 for(AVTShip* Other:Sim->Queries.Ordered(Mine->SystemIndex)) if(IsValid(Other)&&Other!=Mine) {
  FVector2D P; if(!PC->ProjectWorldLocationToScreen(Other->GetActorLocation(),P,true)) continue; P=P*PixelToLocal;
  if(P.X<10||P.Y<10||P.X>Geometry.GetLocalSize().X-10||P.Y>Geometry.GetLocalSize().Y-10) continue;
  FLinearColor Colour=Other->Disabled ? FLinearColor::Yellow : Other->Faction==Mine->Faction ? FLinearColor(0.3f,0.8f,1) : FLinearColor(1,0.4f,0.25f);
  if(Mine->Combat->EquipmentState.Locks.Contains(Other->PersistentId)) {Line(P+FVector2D(-20,-20),P+FVector2D(20,20),FLinearColor::Yellow); Line(P+FVector2D(-20,20),P+FVector2D(20,-20),FLinearColor::Yellow);}
 }
 if(auto* Captain=Cast<AVTController>(PC)) if((Captain->LocalIntent.Buttons&VTButtons::Warp)&&Mine->Definition.Equipment.Warp&&Mine->Combat->EquipmentState.WarpCooldown<=0) {FVector2D P; if(PC->ProjectWorldLocationToScreen(VT::ToWorld(Mine->Movement->Motion.Position+Captain->LocalIntent.CursorOffset.GetClampedToMaxSize(Mine->Definition.Equipment.WarpRange),Mine->SystemIndex),P,true)) {P=P*PixelToLocal; Line(P+FVector2D(-25,0),P+FVector2D(25,0),FLinearColor(0.2f,1,0.8f)); Line(P+FVector2D(0,-25),P+FVector2D(0,25),FLinearColor(0.2f,1,0.8f));}}
 Top=PaintFlightHUD(Geometry,Elements,Top+1);
 if(!ChartOpen||CastChecked<UVTGameInstance>(GetGameInstance())->PlayMode!=TEXT("sandbox")) return Top;
 FVector2D Offset(Geometry.GetLocalSize().X-430,35),Size(405,240);
 FSlateDrawElement::MakeBox(Elements,Top,Geometry.ToPaintGeometry(Size,FSlateLayoutTransform(Offset)),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,FLinearColor(0.025f,0.017f,0.006f,0.95f)); ++Top;
 FVector2D Min(DBL_MAX,DBL_MAX),Max(-DBL_MAX,-DBL_MAX);
 for(const auto& System:Sim->Data->Systems) {if(System.Id.ToString().StartsWith(TEXT("__intro_")))continue;Min.X=FMath::Min(Min.X,System.ChartPosition.X); Min.Y=FMath::Min(Min.Y,System.ChartPosition.Y); Max.X=FMath::Max(Max.X,System.ChartPosition.X); Max.Y=FMath::Max(Max.Y,System.ChartPosition.Y);}
 auto Point=[&](int I) {auto P=Sim->Data->Systems[I].ChartPosition; return Offset+FVector2D(25,25)+FVector2D((P.X-Min.X)/FMath::Max(1.,Max.X-Min.X)*240,(P.Y-Min.Y)/FMath::Max(1.,Max.Y-Min.Y)*165);};
 for(int I=0;I<Sim->Data->Systems.Num();++I) {if(UVTIntroComponent::IsArena(Sim,I))continue;for(FName LinkId:Sim->Data->Systems[I].Links) {int J=Sim->Data->FindSystem(LinkId); if(J>I) Line(Point(I),Point(J),FLinearColor(0.5f,0.34f,0.05f));}}
 for(int I=0;I<Sim->Data->Systems.Num();++I) {if(UVTIntroComponent::IsArena(Sim,I))continue;auto P=Point(I); auto Colour=I==Mine->SystemIndex ? FLinearColor(1,0.7f,0) : FLinearColor(0.7f,0.5f,0.15f); FString Label=Sim->Data->Systems[I].DisplayName.ToString()+FString::Printf(TEXT(" (%d)"),State&&State->Populations.IsValidIndex(I) ? State->Populations[I] : 0); FSlateDrawElement::MakeBox(Elements,Top,Geometry.ToPaintGeometry(FVector2D(5,5),FSlateLayoutTransform(P)),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,Colour); FSlateDrawElement::MakeText(Elements,Top,Geometry.ToPaintGeometry(FVector2D(110,20),FSlateLayoutTransform(P+FVector2D(7,0))),Label,Font,ESlateDrawEffect::None,Colour);}
 return Top;
}

void UVTUI::ApplyGraphics(int32 Preset){if(GEngine)if(auto* Settings=GEngine->GetGameUserSettings()){Settings->SetOverallScalabilityLevel(Preset);Settings->ScalabilityQuality.ResolutionQuality=100;Settings->ApplySettings(false);Settings->SaveSettings();}}
void UVTUI::PerformanceGraphics(){ApplyGraphics(1);}
void UVTUI::BalancedGraphics(){ApplyGraphics(2);}
void UVTUI::HighGraphics(){ApplyGraphics(3);}

void UVTUI::ToggleAutopilot(){if(auto* PC=Cast<AVTController>(GetOwningPlayer()))PC->ToggleAutopilot();}

UWidget* UVTUI::CachedWidget(FName Name) const {const auto* Widget=WidgetCache.Find(Name); return Widget ? Widget->Get() : nullptr;}
void UVTUI::AddChoice(UComboBoxString* Box,FName Id,const FText& Label) {
 if(!Box) return;
 auto* Item=NewObject<UVTUIOption>(this); Item->Id=Id==FName("__empty") ? NAME_None : Id; Item->Label=Label; Item->Font=Box->GetFont(); Item->Foreground=Box->GetForegroundColor(); ChoiceItems.Add(Id,Item);
 Box->OnGenerateWidgetEvent.BindDynamic(this,&UVTUI::GenerateChoice); Box->AddOption(Id.ToString());
}
FName UVTUI::SelectedId(UComboBoxString* Box) const {
 if(!Box) return NAME_None;
 auto* Item=ChoiceItems.Find(FName(Box->GetSelectedOption())); return Item ? (*Item)->Id : NAME_None;
}
UWidget* UVTUI::GenerateChoice(FString Key) {return GenerateChoiceWidget(FName(Key));}
UWidget* UVTUI::GenerateChoiceWidget_Implementation(FName Key) {
 auto* Text=WidgetTree->ConstructWidget<UTextBlock>(); auto* Item=ChoiceItems.Find(Key);
 Text->SetText(Item ? (*Item)->Label : FText::FromName(Key)); if(Item){Text->SetFont((*Item)->Font);Text->SetColorAndOpacity((*Item)->Foreground);} return Text;
}
void UVTUI::AttributeChanged(const FOnAttributeChangeData&) {RequestRefresh();}
void UVTUI::RequestRefresh() {
 if(RefreshQueued||!GetWorld()) return;
 RefreshQueued=true; GetWorld()->GetTimerManager().SetTimer(RefreshTimer,this,&UVTUI::RefreshView,0.1f,false);
}
void UVTUI::NativeTick(const FGeometry& Geometry,float Dt) {
 Super::NativeTick(Geometry,Dt);
 // Focus must be applied after the newly shown Slate subtree has laid out.
 if(FocusPending&&MenuOpen) {if(auto* Focus=CachedWidget(GetWorld()->GetMapName().Contains(TEXT("Menu")) ? (SessionOptions?TEXT("Create"):TEXT("Skirmish")) : TEXT("Resume"))) Focus->SetUserFocus(GetOwningPlayer()); FocusPending=false;}
}
void UVTUI::NativeDestruct() {
 if(GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RefreshTimer);
 if(auto* GI=GetGameInstance()) GI->GetSubsystem<UVTSessionSubsystem>()->OnChanged.RemoveAll(this);
 if(auto* ASC=ObservedAbilities.Get()) {const FGameplayAttribute Attributes[]={UVTAttributes::HullAttribute(),UVTAttributes::BatteryAttribute(),UVTAttributes::GetEMPStressAttribute()}; for(int I=0;I<AttributeHandles.Num();++I) ASC->GetGameplayAttributeValueChangeDelegate(Attributes[I]).Remove(AttributeHandles[I]);}
 RefreshQueued=false; AttributeHandles.Reset(); ObservedAbilities.Reset();
 Super::NativeDestruct();
}

void UVTUI::ToggleSessionOptions(){SessionOptions=!SessionOptions;FocusPending=true;RequestRefresh();}

