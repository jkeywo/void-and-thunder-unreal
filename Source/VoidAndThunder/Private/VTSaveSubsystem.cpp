#include "VTSaveSubsystem.h"
#include "VTGameplay.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
void UVTSaveSubsystem::Advance(float Dt) {
 SinceSave += Dt;
 auto* Sim = GetWorld()->GetSubsystem<UVTSimulation>();
 if (Sim->Data && SinceSave >= Sim->Data->Rules.AutosaveSeconds) Save();
}
bool UVTSaveSubsystem::Save() {
 UWorld* World = GetWorld(); if(!World || World->GetNetMode() == NM_Client) return false;
 auto* Sim=World->GetSubsystem<UVTSimulation>(); if(!Sim || !Sim->Bootstrapped) return false;
 auto* Snapshot=CastChecked<UVTWorldSave>(UGameplayStatics::CreateSaveGameObject(UVTWorldSave::StaticClass()));
 Snapshot->SimulationTime=Sim->SimulationTime;
 for(AVTShip* S:Sim->Ships) if(IsValid(S)) {
  FVTSavedShip R; R.Id=S->PersistentId; R.ClassId=S->ClassId; R.Faction=S->Faction; R.System=S->SystemIndex;
  R.Motion=S->Movement->Motion; R.Hull=S->Attributes->Hull.GetCurrentValue(); R.Battery=S->Attributes->Battery.GetCurrentValue(); R.NPC=S->IsNPC;
  Snapshot->Ships.Add(R);
 }
 if(UGameplayStatics::DoesSaveGameExist("Campaign",0)) {
  if(auto* Old=UGameplayStatics::LoadGameFromSlot("Campaign",0)) if(!UGameplayStatics::SaveGameToSlot(Old,"CampaignBackup",0)) return false;
 }
 bool OK=UGameplayStatics::SaveGameToSlot(Snapshot,"Campaign",0); if(OK) SinceSave=0;
 return OK;
}
bool UVTSaveSubsystem::Load() {
 UWorld* World=GetWorld(); if(!World || World->GetNetMode()==NM_Client) return false;
 auto* Snapshot=Cast<UVTWorldSave>(UGameplayStatics::LoadGameFromSlot("Campaign",0));
 if(!Snapshot || Snapshot->Version != 1) Snapshot=Cast<UVTWorldSave>(UGameplayStatics::LoadGameFromSlot("CampaignBackup",0));
 if(!Snapshot || Snapshot->Version != 1) return false;
 auto* Sim=World->GetSubsystem<UVTSimulation>();
 for(const auto& R:Snapshot->Ships) if(!Sim->Data || !Sim->Data->FindShip(R.ClassId) || !Sim->Data->Systems.IsValidIndex(R.System)) return false;
 auto Current=Sim->Ships;
 for(AVTShip* S:Current) if(IsValid(S) && S->IsNPC) { if(S->Controller) S->Controller->Destroy(); S->Destroy(); }
 for(const auto& R:Snapshot->Ships) if(R.NPC) {
  AVTShip* S=Sim->SpawnShip(R.ClassId,R.System,R.Motion,true,R.Faction); S->PersistentId=R.Id;
  S->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),R.Hull);
  S->Abilities->SetNumericAttributeBase(UVTAttributes::BatteryAttribute(),R.Battery);
 }
 Sim->SimulationTime=Snapshot->SimulationTime; Sim->Bootstrapped=true; SinceSave=0;
 return true;
}
