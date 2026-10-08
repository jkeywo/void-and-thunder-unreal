#include "VTPIESettings.h"
#include "VTGameplay.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
TArray<EVTPIEMode> UVTPIESettings::Modes(UWorld* World){if(World)if(auto* Data=Cast<UVTPIELevelModes>(World->GetWorldSettings()->GetAssetUserDataOfClass(UVTPIELevelModes::StaticClass())))return Data->SupportedModes;return {};}
FText UVTPIESettings::ModeLabel(EVTPIEMode Value){switch(Value){case EVTPIEMode::Skirmish:return NSLOCTEXT("VTEditor","Skirmish","Skirmish");case EVTPIEMode::TestRange:return NSLOCTEXT("VTEditor","TestRange","Test Range");default:return NSLOCTEXT("VTEditor","Sandbox","Sandbox");}}
FString UVTPIESettings::ModeId(EVTPIEMode Value){return Value==EVTPIEMode::Skirmish?TEXT("skirmish"):Value==EVTPIEMode::TestRange?TEXT("range"):TEXT("sandbox");}
bool UVTPIESettings::LoadoutCandidate(const UVTGameData* Data,FName Id,FVTLoadoutSelection& Candidate) const {
 const auto* Option=Data?Data->Loadouts.FindByPredicate([Id](const FVTLoadoutOption& O){return O.Id==Id;}):nullptr;if(!Option)return false;
 Candidate=Fit;
 if(Option->Slot==EVTLoadoutSlot::Broadside)Candidate.Broadside=Candidate.Broadside==Id?NAME_None:Id;
 else{Candidate.OverrideBatteries=Candidate.OverrideSpecials=true;Candidate.Battery=Candidate.Special=NAME_None;auto& Items=Option->Slot==EVTLoadoutSlot::Battery?Candidate.Batteries:Candidate.Specials;if(Items.Contains(Id))Items.Remove(Id);else Items.Add(Id);}
 FVTShipDefinition Resolved;return Data->ResolveFit(Hull,Candidate,Resolved);
}
bool UVTPIESettings::ToggleLoadout(const UVTGameData* Data,FName Id){FVTLoadoutSelection Candidate;if(!LoadoutCandidate(Data,Id,Candidate))return false;Fit=MoveTemp(Candidate);return true;}
void UVTPIESettings::SelectHull(const UVTGameData* Data,FName Id){
 const auto* Ship=Data?Data->FindShip(Id):nullptr;if(!Ship)return;Hull=Id;
 for(int Slot=1;Slot<3;++Slot){auto& Items=Slot==1?Fit.Batteries:Fit.Specials;TArray<FName> Kept;for(const auto& O:Data->Loadouts)if(int(O.Slot)==Slot&&Items.Contains(O.Id)&&Kept.Num()<Ship->Mounts)Kept.Add(O.Id);Items=MoveTemp(Kept);}
 FVTShipDefinition Resolved;if(!Data->ResolveFit(Hull,Fit,Resolved))Fit=FVTLoadoutSelection();
}
bool UVTPIESettings::Apply(UVTGameInstance* GI,UWorld* EditorWorld,const UVTGameData* Data) const {
 const auto Supported=Modes(EditorWorld);if(!GI||Supported.IsEmpty()||!Data)return false;
 FVTShipDefinition Resolved;if(!Data->ResolveFit(Hull,Fit,Resolved))return false;
 GI->PlayMode=ModeId(Supported.Contains(Mode)?Mode:Supported[0]);GI->SelectedHull=Hull;GI->SelectedFit=Fit;GI->ContinueWorld=false;GI->NewWorldPopulation=-1;return true;
}
