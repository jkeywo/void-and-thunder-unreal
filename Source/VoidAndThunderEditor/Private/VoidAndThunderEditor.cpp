#include "Modules/ModuleManager.h"
#include "Internationalization/StringTable.h"
#include "VTPIESettings.h"
#include "VTGameplay.h"
#include "Editor.h"
#include "ToolMenus.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"
class FVoidAndThunderEditorModule : public IModuleInterface {
 FDelegateHandle PIEHandle;TStrongObjectPtr<UVTGameData> Catalog;TStrongObjectPtr<UStringTable> TextTable;
 UWorld* EditorWorld() const{return GEditor?GEditor->GetEditorWorldContext().World():nullptr;}
 UVTGameData* Data(){if(!Catalog.IsValid()){Catalog.Reset(LoadObject<UVTGameData>(nullptr,TEXT("/Game/Data/DA_GameData.DA_GameData")));if(Catalog.IsValid())Catalog->LoadCatalog();}return Catalog.Get();}
 bool Editable() const{return GEditor&&!GEditor->IsPlaySessionInProgress()&&!UVTPIESettings::Modes(EditorWorld()).IsEmpty();}
 FText Name(FName Id,bool Ship){if(!TextTable.IsValid())TextTable.Reset(LoadObject<UStringTable>(nullptr,TEXT("/Game/UI/ST_UI.ST_UI")));return FText::FromStringTable(TEXT("/Game/UI/ST_UI.ST_UI"),Ship?TEXT("class.")+Id.ToString()+TEXT(".name"):Id.ToString()+TEXT(".name"));}
 FText Label(int32 Index){auto* S=GetDefault<UVTPIESettings>();if(Index==0){auto Modes=UVTPIESettings::Modes(EditorWorld());return Modes.IsEmpty()?NSLOCTEXT("VTEditor","Unsupported","No gameplay modes"):UVTPIESettings::ModeLabel(Modes.Contains(S->Mode)?S->Mode:Modes[0]);}if(Index==1){Data();return Name(S->Hull,true);}return FText::FromString(!S->Fit.OverrideBatteries&&!S->Fit.OverrideSpecials&&S->Fit.Broadside.IsNone()?TEXT("Loadout: hull defaults"):FString::Printf(TEXT("Loadout: %d selected"),S->Fit.Batteries.Num()+S->Fit.Specials.Num()+(S->Fit.Broadside.IsNone()?0:1)));}
 TSharedRef<SWidget> Menu(int32 Index){FMenuBuilder M(false,nullptr);auto* S=GetMutableDefault<UVTPIESettings>();auto* D=Data();if(!D)return M.MakeWidget();
  if(Index==0){for(auto Mode:UVTPIESettings::Modes(EditorWorld()))M.AddMenuEntry(UVTPIESettings::ModeLabel(Mode),NSLOCTEXT("VTEditor","ModeTip","Select the gameplay rules used when Play starts. Solo modes use one standalone player."),FSlateIcon(),FUIAction(FExecuteAction::CreateLambda([S,Mode](){S->Mode=Mode;S->SaveConfig();if(Mode!=EVTPIEMode::Sandbox){auto* P=GetMutableDefault<ULevelEditorPlaySettings>();P->SetPlayNetMode(EPlayNetMode::PIE_Standalone);P->SetPlayNumberOfClients(1);P->SaveConfig();}}),FCanExecuteAction(),FIsActionChecked::CreateLambda([S,Mode](){return S->Mode==Mode;})),NAME_None,EUserInterfaceActionType::RadioButton);}
  else if(Index==1){for(const auto& Ship:D->Ships)if(Ship.Id.ToString().StartsWith(TEXT("corsair"))){auto Id=Ship.Id;M.AddMenuEntry(Name(Id,true),NSLOCTEXT("VTEditor","ShipTip","Select the captain's hull. Excess modules are removed in catalogue order to fit its mounts."),FSlateIcon(),FUIAction(FExecuteAction::CreateLambda([S,D,Id](){S->SelectHull(D,Id);S->SaveConfig();}),FCanExecuteAction(),FIsActionChecked::CreateLambda([S,Id](){return S->Hull==Id;})),NAME_None,EUserInterfaceActionType::RadioButton);}}
  else{
   M.AddMenuEntry(NSLOCTEXT("VTEditor","HullDefaults","Use hull default equipment"),NSLOCTEXT("VTEditor","CustomTip","Tick modules below to replace the hull's optional equipment. Unticking all modules leaves those mounts empty."),FSlateIcon(),FUIAction(FExecuteAction::CreateLambda([S](){S->Fit=FVTLoadoutSelection();S->SaveConfig();}),FCanExecuteAction(),FIsActionChecked::CreateLambda([S](){return !S->Fit.OverrideBatteries&&!S->Fit.OverrideSpecials&&S->Fit.Broadside.IsNone();})),NAME_None,EUserInterfaceActionType::Check);
   for(int Slot=0;Slot<3;++Slot){M.BeginSection(NAME_None,Slot==0?NSLOCTEXT("VTEditor","Guns","Broadside (one; unchecked uses hull guns)"):Slot==1?NSLOCTEXT("VTEditor","Batteries","Battery modules"):NSLOCTEXT("VTEditor","Specials","Special modules"));for(const auto& O:D->Loadouts)if(int(O.Slot)==Slot){auto Id=O.Id;M.AddMenuEntry(Name(Id,false),NSLOCTEXT("VTEditor","MountTip","Tick or untick this module. Unavailable choices exceed this hull's mount limit."),FSlateIcon(),FUIAction(FExecuteAction::CreateLambda([S,D,Id](){S->ToggleLoadout(D,Id);S->SaveConfig();}),FCanExecuteAction::CreateLambda([S,D,Id](){FVTLoadoutSelection Candidate;return S->LoadoutCandidate(D,Id,Candidate);}),FIsActionChecked::CreateLambda([S,Id,Slot](){return Slot==0?S->Fit.Broadside==Id:Slot==1?S->Fit.Batteries.Contains(Id):S->Fit.Specials.Contains(Id);})),NAME_None,EUserInterfaceActionType::Check);}M.EndSection();}
  }return M.MakeWidget();
 }
 void RegisterMenus(){FToolMenuOwnerScoped Owner(this);auto* MenuBar=UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.PlayToolBar"));auto& Section=MenuBar->AddSection(TEXT("VoidThunderPIE"),FText::GetEmpty(),FToolMenuInsert(TEXT("Play"),EToolMenuInsertType::After));for(int32 I=0;I<3;++I){auto Widget=SNew(SBox).MinDesiredWidth(I==1?150:140)[SNew(SComboButton).IsEnabled_Lambda([this](){return Editable();}).ToolTipText(NSLOCTEXT("VTEditor","ToolbarTip","Void & Thunder PIE setup. Supported modes come from the loaded level's World Settings asset user data. Open Sandbox to preview gameplay." )).OnGetMenuContent_Lambda([this,I](){return Menu(I);}).ButtonContent()[SNew(STextBlock).Text_Lambda([this,I](){return Label(I);})]];Section.AddEntry(FToolMenuEntry::InitWidget(FName(*FString::Printf(TEXT("VTPIE%d"),I)),Widget,FText::GetEmpty()));}}
 void ConfigurePIE(UGameInstance* Instance){if(auto* GI=Cast<UVTGameInstance>(Instance))GetDefault<UVTPIESettings>()->Apply(GI,EditorWorld(),Data());}
public:
 virtual void StartupModule() override{PIEHandle=FWorldDelegates::OnPIEMapReady.AddRaw(this,&FVoidAndThunderEditorModule::ConfigurePIE);if(!IsRunningCommandlet())UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this,&FVoidAndThunderEditorModule::RegisterMenus));}
 virtual void ShutdownModule() override{FWorldDelegates::OnPIEMapReady.Remove(PIEHandle);if(UToolMenus::IsToolMenuUIEnabled()){UToolMenus::UnRegisterStartupCallback(this);UToolMenus::UnregisterOwner(this);}Catalog.Reset();TextTable.Reset();}
};
IMPLEMENT_MODULE(FVoidAndThunderEditorModule,VoidAndThunderEditor);
