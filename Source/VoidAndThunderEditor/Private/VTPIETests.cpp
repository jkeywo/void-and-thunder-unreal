#include "VTFitEditor.h"
#include "VTPIESettings.h"
#include "VTGameplay.h"
#include "VTUI.h"
#include "Components/ComboBoxString.h"
#include "VTSaveSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "ToolMenus.h"
#include "Tests/AutomationEditorCommon.h"
#include "Settings/LevelEditorPlaySettings.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTPIESelection,"VT.Editor.PIESelections",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTPIESelection::RunTest(const FString&) {
 auto* Toolbar=UToolMenus::Get()->FindMenu(TEXT("LevelEditor.LevelEditorToolBar.PlayToolBar"));if(TestNotNull(TEXT("Native Play toolbar"),Toolbar)&&TestNotNull(TEXT("Game preview section"),Toolbar->FindSection(TEXT("VoidThunderPIE"))))for(int I=0;I<3;++I)TestNotNull(TEXT("PIE selector registered beside Play"),Toolbar->FindSection(TEXT("VoidThunderPIE"))->FindEntry(FName(*FString::Printf(TEXT("VTPIE%d"),I))));
 auto* Data=LoadObject<UVTGameData>(nullptr,TEXT("/Game/Data/DA_GameData.DA_GameData"));if(!TestNotNull(TEXT("Native catalogue"),Data))return false;Data->LoadCatalog();
 FVTFitEditor Shared;Shared.Initialize(*Data,FName("corsair_battleship"),{});auto* Adapter=NewObject<UVTPIESettings>();Adapter->Hull=FName("corsair_battleship");Adapter->Fit={};FText Feedback;
 for(const auto& O:Data->Loadouts){bool RuntimeAccepted=Shared.Toggle(*Data,O.Id,Feedback);bool EditorAccepted=Adapter->ToggleLoadout(Data,O.Id);TestEqual(TEXT("Runtime/PIE accept same checkbox edits"),EditorAccepted,RuntimeAccepted);TestTrue(TEXT("Runtime/PIE retain same fit"),Shared.Preview().Selection.Batteries==Adapter->Fit.Batteries&&Shared.Preview().Selection.Specials==Adapter->Fit.Specials&&Shared.Preview().Selection.Broadside==Adapter->Fit.Broadside);}
 Shared.SelectHull(*Data,FName("corsair_frigate"),Feedback);Adapter->SelectHull(Data,FName("corsair_frigate"));TestTrue(TEXT("Runtime/PIE prune same fit"),Shared.Preview().Selection.Batteries==Adapter->Fit.Batteries&&Shared.Preview().Selection.Specials==Adapter->Fit.Specials);
 auto* Sandbox=LoadObject<UWorld>(nullptr,TEXT("/Game/Maps/Sandbox.Sandbox"));auto* Menu=LoadObject<UWorld>(nullptr,TEXT("/Game/Maps/Menu.Menu"));
 TestEqual(TEXT("Sandbox supports all three rulesets"),UVTPIESettings::Modes(Sandbox).Num(),3);TestTrue(TEXT("Frontend does not offer gameplay modes"),UVTPIESettings::Modes(Menu).IsEmpty());
 auto* Settings=NewObject<UVTPIESettings>();Settings->Hull=TEXT("corsair_battleship");Settings->Fit=FVTLoadoutSelection();
 const auto* Guns=Data->Loadouts.FindByPredicate([](const FVTLoadoutOption& Option){return Option.Slot==EVTLoadoutSlot::Broadside;});if(TestNotNull(TEXT("Broadside variant"),Guns)){TestTrue(TEXT("Selecting a gun variant explicitly selects its checkbox fit"),Settings->ToggleLoadout(Data,Guns->Id));FVTShipDefinition GunOnly;TestTrue(TEXT("Gun-only fit resolves"),Data->ResolveFit(Settings->Hull,Settings->Fit,GunOnly));TestFalse(TEXT("Unselected EMP is not inherited behind unchecked boxes"),GunOnly.Equipment.EMP);TestFalse(TEXT("Unselected torpedoes are not inherited"),GunOnly.Equipment.Torpedoes);Settings->Fit=FVTLoadoutSelection();}
 for(FName Id:{FName("loadout.disruptor"),FName("loadout.boost"),FName("loadout.point_defense"),FName("loadout.torpedoes"),FName("loadout.microwarp"),FName("loadout.mines")})TestTrue(TEXT("Battleship accepts checked module"),Settings->ToggleLoadout(Data,Id));
 Settings->SelectHull(Data,TEXT("corsair_frigate"));const auto* Hull=Data->FindShip(Settings->Hull);TestEqual(TEXT("Smaller hull prunes batteries"),Settings->Fit.Batteries.Num(),Hull->Mounts);TestEqual(TEXT("Smaller hull prunes specials"),Settings->Fit.Specials.Num(),Hull->Mounts);
 FVTLoadoutSelection Candidate;TestFalse(TEXT("Mount limit disables extra battery"),Settings->LoadoutCandidate(Data,TEXT("loadout.point_defense"),Candidate));
 auto Batteries=Settings->Fit.Batteries,Specials=Settings->Fit.Specials;for(FName Id:Batteries)TestTrue(TEXT("Untick battery"),Settings->ToggleLoadout(Data,Id));for(FName Id:Specials)TestTrue(TEXT("Untick special"),Settings->ToggleLoadout(Data,Id));
 FVTShipDefinition Empty,Default;TestTrue(TEXT("Explicit empty fit valid"),Data->ResolveFit(Settings->Hull,Settings->Fit,Empty));TestTrue(TEXT("Legacy defaults valid"),Data->ResolveFit(Settings->Hull,FVTLoadoutSelection(),Default));
 TestFalse(TEXT("Empty fit removes EMP"),Empty.Equipment.EMP);TestFalse(TEXT("Empty fit removes boost"),Empty.Equipment.Boost);TestFalse(TEXT("Empty fit removes point defence"),Empty.Equipment.PointDefense);TestFalse(TEXT("Empty fit removes microwarp"),Empty.Equipment.Warp);TestFalse(TEXT("Empty fit removes torpedoes"),Empty.Equipment.Torpedoes);TestFalse(TEXT("Empty fit removes mines"),Empty.Equipment.Mines);
 auto* Personal=NewObject<UVTPersonalSave>();Personal->PreferredFit=Settings->Fit;TArray<uint8> Bytes;TestTrue(TEXT("Native serialize explicit empty fit"),UGameplayStatics::SaveGameToMemory(Personal,Bytes));auto* Restored=Cast<UVTPersonalSave>(UGameplayStatics::LoadGameFromMemory(Bytes));TestTrue(TEXT("Restore empty fit flags"),Restored&&Restored->PreferredFit.OverrideBatteries&&Restored->PreferredFit.OverrideSpecials&&Restored->PreferredFit.Batteries.IsEmpty()&&Restored->PreferredFit.Specials.IsEmpty());
 auto* GI=NewObject<UVTGameInstance>();Settings->Mode=EVTPIEMode::TestRange;TestTrue(TEXT("Apply supported level"),Settings->Apply(GI,Sandbox,Data));TestEqual(TEXT("Selected range rules"),GI->PlayMode,FString(TEXT("range")));Settings->SkipIntro=true;Settings->Apply(GI,Sandbox,Data);TestTrue(TEXT("PIE skip passed to game instance"),GI->SkipIntro);Settings->SkipIntro=false;Settings->Apply(GI,Sandbox,Data);TestFalse(TEXT("PIE intro enabled when unchecked"),GI->SkipIntro);TestEqual(TEXT("Selected hull"),GI->SelectedHull,Settings->Hull);TestFalse(TEXT("Do not override frontend"),Settings->Apply(GI,Menu,Data));
 return true;
}
class FVTCheckPIE : public IAutomationLatentCommand {
 FAutomationTestBase* Test;EVTPIEMode Mode;double Started=FPlatformTime::Seconds();
public:
 FVTCheckPIE(FAutomationTestBase* InTest,EVTPIEMode InMode):Test(InTest),Mode(InMode){}
 bool Update() override {
  auto* World=GEditor->PlayWorld.Get();if(!World){if(FPlatformTime::Seconds()-Started<30)return false;Test->AddError(TEXT("PIE failed to start"));return true;}
  auto* GI=Cast<UVTGameInstance>(World->GetGameInstance());auto* PC=Cast<AVTController>(World->GetFirstPlayerController());auto* Ship=PC?Cast<AVTShip>(PC->GetPawn()):nullptr;if(!Ship&&FPlatformTime::Seconds()-Started<30)return false;
  Test->TestNotNull(TEXT("PIE possesses selected hull"),Ship);if(Ship){Test->TestEqual(TEXT("Hull override before spawning"),Ship->ClassId,FName(TEXT("corsair_frigate")));Test->TestTrue(TEXT("Empty custom fit applied to pawn"),Ship->Fit.OverrideBatteries&&Ship->Fit.OverrideSpecials&&Ship->Fit.Batteries==GetDefault<UVTPIESettings>()->Fit.Batteries&&Ship->Fit.Specials==GetDefault<UVTPIESettings>()->Fit.Specials);}
  if(PC&&Test->TestNotNull(TEXT("Actual PIE frontend widget"),PC->UI.Get())){const auto Expected=GetDefault<UVTPIESettings>()->Fit;Test->TestTrue(TEXT("Frontend restores shared checkbox fit"),PC->UI->FitEditor.Preview().Selection.Batteries==Expected.Batteries&&!PC->UI->FitChecks.IsEmpty());Test->TestTrue(TEXT("Frontend stores explicit fit"),PC->UI->StoreFit());Test->TestTrue(TEXT("Frontend retains optional mask"),GI->SelectedFit.Batteries==Expected.Batteries&&GI->SelectedFit.Specials==Expected.Specials&&GI->SelectedFit.OverrideBatteries&&GI->SelectedFit.OverrideSpecials);}
  Test->TestEqual(TEXT("Mode applied before simulation"),GI->PlayMode,UVTPIESettings::ModeId(Mode));auto* Save=GI->GetSubsystem<UVTSaveSubsystem>();Test->TestTrue(TEXT("PIE personal preferences isolated"),Save->PersonalSlot.Contains(TEXT("-PIE-")));Test->TestTrue(TEXT("PIE campaign path isolated even after slot changes"),Save->CampaignPrefix.StartsWith(TEXT("PIE/")));
  Test->TestEqual(TEXT("Scenario selection matches mode"),World->GetSubsystem<UVTSimulation>()->ActiveScenario()!=nullptr,Mode!=EVTPIEMode::Sandbox);return true;
 }
};
class FVTRestorePIEPreferences : public IAutomationLatentCommand {
 EVTPIEMode Mode;FName Hull;FVTLoadoutSelection Fit;EPlayNetMode Net;int32 Players;
public:
 FVTRestorePIEPreferences():Mode(GetDefault<UVTPIESettings>()->Mode),Hull(GetDefault<UVTPIESettings>()->Hull),Fit(GetDefault<UVTPIESettings>()->Fit){GetDefault<ULevelEditorPlaySettings>()->GetPlayNetMode(Net);GetDefault<ULevelEditorPlaySettings>()->GetPlayNumberOfClients(Players);}
 bool Update() override {if(GEditor->PlayWorld)return false;auto* S=GetMutableDefault<UVTPIESettings>();S->Mode=Mode;S->Hull=Hull;S->Fit=Fit;auto* P=GetMutableDefault<ULevelEditorPlaySettings>();P->SetPlayNetMode(Net);P->SetPlayNumberOfClients(Players);return true;}
};
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FVTPIEPlay,"VT.Editor.PlaySelectedMode",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FVTPIEPlay::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const {for(auto Mode:{EVTPIEMode::Sandbox,EVTPIEMode::Skirmish,EVTPIEMode::TestRange}){Names.Add(UVTPIESettings::ModeLabel(Mode).ToString());Commands.Add(FString::FromInt(int(Mode)));}}
bool FVTPIEPlay::RunTest(const FString& Params){
 auto Restore=MakeShared<FVTRestorePIEPreferences>();
 FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/Sandbox"));auto* S=GetMutableDefault<UVTPIESettings>();S->Mode=EVTPIEMode(FCString::Atoi(*Params));S->Hull=TEXT("corsair_frigate");S->Fit=FVTLoadoutSelection();S->Fit.OverrideBatteries=S->Fit.OverrideSpecials=true;
 auto* P=GetMutableDefault<ULevelEditorPlaySettings>();P->SetPlayNetMode(EPlayNetMode::PIE_Standalone);P->SetPlayNumberOfClients(1);
 ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));ADD_LATENT_AUTOMATION_COMMAND(FVTCheckPIE(this,S->Mode));ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());FAutomationTestFramework::Get().EnqueueLatentCommand(Restore);return true;
}
#endif
