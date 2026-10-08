#include "VTFitEditor.h"
#include "VTPIESettings.h"
#include "VTGameplay.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
TArray<EVTPIEMode> UVTPIESettings::Modes(UWorld* World){if(World)if(auto* Data=Cast<UVTPIELevelModes>(World->GetWorldSettings()->GetAssetUserDataOfClass(UVTPIELevelModes::StaticClass())))return Data->SupportedModes;return {};}
FText UVTPIESettings::ModeLabel(EVTPIEMode Value){switch(Value){case EVTPIEMode::Skirmish:return NSLOCTEXT("VTEditor","Skirmish","Skirmish");case EVTPIEMode::TestRange:return NSLOCTEXT("VTEditor","TestRange","Test Range");default:return NSLOCTEXT("VTEditor","Sandbox","Sandbox");}}
FString UVTPIESettings::ModeId(EVTPIEMode Value){return Value==EVTPIEMode::Skirmish?TEXT("skirmish"):Value==EVTPIEMode::TestRange?TEXT("range"):TEXT("sandbox");}
bool UVTPIESettings::LoadoutCandidate(const UVTGameData* Data,FName Id,FVTLoadoutSelection& Candidate) const {FVTFitEditor Editor;FText Reason;if(!Data||!Editor.Initialize(*Data,Hull,Fit)||!Editor.Toggle(*Data,Id,Reason))return false;Candidate=Editor.Preview().Selection;return true;}
bool UVTPIESettings::ToggleLoadout(const UVTGameData* Data,FName Id){FVTFitEditor Editor;if(!Data||!Editor.Initialize(*Data,Hull,Fit)||!Editor.Toggle(*Data,Id,FitFeedback))return false;Fit=Editor.Preview().Selection;return true;}
void UVTPIESettings::SelectHull(const UVTGameData* Data,FName Id){FVTFitEditor Editor;if(Data&&Editor.Initialize(*Data,Hull,Fit)&&Editor.SelectHull(*Data,Id,FitFeedback)){Hull=Id;Fit=Editor.Preview().Selection;}}
bool UVTPIESettings::Apply(UVTGameInstance* GI,UWorld* EditorWorld,const UVTGameData* Data) const {
 const auto Supported=Modes(EditorWorld);if(!GI||Supported.IsEmpty()||!Data)return false;
 FVTShipDefinition Resolved;if(!Data->ResolveFit(Hull,Fit,Resolved))return false;
 GI->PlayMode=ModeId(Supported.Contains(Mode)?Mode:Supported[0]);GI->SelectedHull=Hull;GI->SelectedFit=Fit;GI->ContinueWorld=false;GI->NewWorldPopulation=-1;return true;
}
