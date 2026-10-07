#include "VTSaveSubsystem.h"
#include "VTGameplay.h"
#include "VTCombat.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Windows/WindowsHWrapper.h"
namespace {
constexpr uint32 Magic=0x56545332;
FString SavePath(const FString& Slot) {return FPaths::ProjectSavedDir()/TEXT("SaveGames")/(Slot+TEXT(".vts"));}
bool Encode(UVTWorldSave* Save,TArray<uint8>& Bytes) {
 TArray<uint8> Payload; if(!UGameplayStatics::SaveGameToMemory(Save,Payload)) return false;
 FMemoryWriter W(Bytes); uint32 M=Magic, CRC=FCrc::MemCrc32(Payload.GetData(),Payload.Num()); int32 Size=Payload.Num();
 W<<M<<Size<<CRC; W.Serialize(Payload.GetData(),Payload.Num()); return !W.IsError();
}
UVTWorldSave* Decode(const TArray<uint8>& Bytes) {
 if(Bytes.Num()<12||Bytes.Num()>128*1024*1024) return nullptr;
 FMemoryReader R(Bytes); uint32 M=0,CRC=0; int32 Size=0; R<<M<<Size<<CRC;
 if(M!=Magic||Size!=Bytes.Num()-12||Size<=0||FCrc::MemCrc32(Bytes.GetData()+12,Size)!=CRC) return nullptr;
 TArray<uint8> Payload; Payload.Append(Bytes.GetData()+12,Size);
 return Cast<UVTWorldSave>(UGameplayStatics::LoadGameFromMemory(Payload));
}
bool AtomicWrite(const FString& Path,const TArray<uint8>& Bytes) {
 IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
 const FString Temp=Path+TEXT(".tmp");
 if(!FFileHelper::SaveArrayToFile(Bytes,*Temp)) return false;
 // Windows rename replaces the destination atomically; a failed replacement retains the old file.
 return MoveFileExW(*Temp,*Path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
}
}
FVTSavedShip UVTSaveSubsystem::CaptureShip(AVTShip* S) const {
 FVTSavedShip R; R.Id=S->PersistentId; R.ClassId=S->ClassId; R.Faction=S->Faction; R.System=S->SystemIndex;
 R.Motion=S->Movement->Motion; R.Hull=S->Attributes->Hull.GetCurrentValue(); R.Battery=S->Attributes->Battery.GetCurrentValue(); R.NPC=S->IsNPC;
 R.Invulnerable=S->Invulnerable; R.Anchored=S->Anchored; R.BoardingTarget=S->Combat->BoardingTarget; R.BoardingProgress=S->Combat->BoardingProgress;
 R.Disabled=S->Disabled; R.Docked=S->Docked; R.Shields=S->Combat->Shields; R.Suppression=S->Combat->Suppression;
 R.PortReload=S->PortReload; R.StarboardReload=S->StarboardReload;
 // A held bank wind-up is cancelled on restoration; retain the spent bank's reload.
 if(S->Combat->PortCharge>0) R.PortReload=S->Definition.Reload;
 if(S->Combat->StarboardCharge>0) R.StarboardReload=S->Definition.Reload;
 return R;
}
AVTShip* UVTSaveSubsystem::RestoreShip(const FVTSavedShip& R) {
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>(); FVTMotion M=R.Motion; M.Ack=0;
 AVTShip* S=Sim->SpawnShip(R.ClassId,R.System,M,R.NPC,R.Faction); S->PersistentId=R.Id; S->Disabled=R.Disabled; S->Docked=R.Docked; S->Invulnerable=R.Invulnerable; S->Anchored=R.Anchored;
 S->Combat->BoardingTarget=R.BoardingTarget; S->Combat->BoardingProgress=R.BoardingProgress;
 S->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),R.Hull); S->Abilities->SetNumericAttributeBase(UVTAttributes::BatteryAttribute(),R.Battery);
 S->Combat->Shields=R.Shields; S->Combat->Suppression=R.Suppression;
 S->Combat->RestoreReload(true,R.PortReload); S->Combat->RestoreReload(false,R.StarboardReload);
 S->Intent=FVTPilotIntent(); return S;
}
void UVTSaveSubsystem::CapturePlayer(AVTController* PC) {
 if(!PC) return; auto* PS=PC->GetPlayerState<AVTPlayerState>(); auto* Ship=Cast<AVTShip>(PC->GetPawn()); if(!PS||!Ship||!PS->Profile.IsValid()) return;
 FVTSavedPlayer* R=PlayerRecords.FindByPredicate([PS](const FVTSavedPlayer& Item){return Item.Profile==PS->Profile;});
 if(!R) {FVTSavedPlayer New; New.Profile=PS->Profile; New.Token=FGuid::NewGuid(); R=&PlayerRecords.Add_GetRef(New);}
 R->Ship=CaptureShip(Ship); R->Credits=PS->Credits; R->Boarded=PS->Boarded; R->Reputation=PS->Reputation; R->Heat=PS->Heat;
}
bool UVTSaveSubsystem::Validate(const UVTWorldSave* S) const {
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>(); if(!S||S->Version!=3||!S->WorldId.IsValid()||!FMath::IsFinite(S->SimulationTime)||S->SimulationTime<0||S->Ships.Num()>10000||S->Projectiles.Num()>100000) return false;
 TSet<FGuid> IDs;
 auto ValidShip=[&](const FVTSavedShip& R) {
  const auto* Def=Sim->Data->FindShip(R.ClassId);
  if(!R.Id.IsValid()||IDs.Contains(R.Id)||!Def||!Sim->Data->Systems.IsValidIndex(R.System)||!FMath::IsFinite(R.Motion.SimulationTime)||R.Motion.SimulationTime<0||R.Motion.Position.ContainsNaN()||R.Motion.Velocity.ContainsNaN()||!FMath::IsFinite(R.Motion.Heading)||!FMath::IsFinite(R.Motion.Omega)||!FMath::IsFinite(R.Hull)||R.Hull<0||R.Hull>Def->Hull||!FMath::IsFinite(R.Battery)||R.Battery<0||R.Battery>Def->BatteryMax) return false;
  for(int I=0;I<4;++I) if(!FMath::IsFinite(R.Shields[I])||R.Shields[I]<0||R.Shields[I]>Def->ShieldMax[I]||!FMath::IsFinite(R.Suppression[I])||R.Suppression[I]<0) return false;
  if(!FMath::IsFinite(R.BoardingProgress)||R.BoardingProgress<0||R.BoardingProgress>Sim->Data->Rules.BoardDwell) return false;
  if(!FMath::IsFinite(R.PortReload)||R.PortReload<0||!FMath::IsFinite(R.StarboardReload)||R.StarboardReload<0) return false;
  IDs.Add(R.Id); return true;
 };
 for(const auto& R:S->Ships) if(!R.NPC||!ValidShip(R)) return false;
 TSet<FGuid> Profiles;
 for(const auto& P:S->Players) {
  if(!P.Profile.IsValid()||!P.Token.IsValid()||Profiles.Contains(P.Profile)||!ValidShip(P.Ship)||P.Ship.NPC) return false;
  Profiles.Add(P.Profile);
  for(float V:P.Reputation) if(!FMath::IsFinite(V)) return false;
  for(float V:P.Heat) if(!FMath::IsFinite(V)||V<0) return false;
 }
 for(const auto& P:S->Projectiles) {
  if(!P.Id.IsValid()||IDs.Contains(P.Id)||!Sim->Data->Systems.IsValidIndex(P.System)||P.Position.ContainsNaN()||P.Velocity.ContainsNaN()||!FMath::IsFinite(P.Damage)||P.Damage<0||!FMath::IsFinite(P.Remaining)||P.Remaining<=0||!FMath::IsFinite(P.Radius)||P.Radius<0) return false;
  IDs.Add(P.Id);
 }
 return true;
}
void UVTSaveSubsystem::Advance(float Dt) {SinceSave+=Dt; auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>(); if(Sim->Data&&SinceSave>=Sim->Data->Rules.AutosaveSeconds) Save();}
bool UVTSaveSubsystem::Save() {
 UWorld* World=GetWorld(); if(!World||World->GetNetMode()==NM_Client) return false;
 auto* Sim=World->GetSubsystem<UVTSimulation>(); if(!Sim||!Sim->Bootstrapped||!Sim->Data) return false;
 for(auto It=World->GetPlayerControllerIterator();It;++It) CapturePlayer(Cast<AVTController>(It->Get()));
 auto* Snapshot=CastChecked<UVTWorldSave>(UGameplayStatics::CreateSaveGameObject(UVTWorldSave::StaticClass()));
 Snapshot->WorldId=WorldId; Snapshot->Seed=Sim->WorldSeed; Snapshot->SimulationTime=Sim->SimulationTime; Snapshot->Players=PlayerRecords;
 for(AVTShip* Ship:Sim->Ships) if(IsValid(Ship)&&Ship->IsNPC) Snapshot->Ships.Add(CaptureShip(Ship));
 for(AVTProjectile* P:Sim->Projectiles) if(IsValid(P)&&P->Remaining>0) {
  FVTSavedProjectile R; R.Id=P->PersistentId; R.Source=P->SourceId; R.System=P->SystemIndex; R.Position=P->Position; R.Velocity=P->Velocity; R.Damage=P->Damage; R.Remaining=P->Remaining; R.Radius=P->Radius; Snapshot->Projectiles.Add(R);
 }
 if(!Validate(Snapshot)) return false;
 TArray<uint8> Bytes; if(!Encode(Snapshot,Bytes)) return false;
 const FString Path=SavePath(Slot); TArray<uint8> Old;
 if(FFileHelper::LoadFileToArray(Old,*Path)) if(auto* Good=Decode(Old)) if(Migrate(Good)&&Validate(Good)) if(!AtomicWrite(Path+TEXT(".backup"),Old)) return false;
 if(!AtomicWrite(Path,Bytes)) return false; SinceSave=0; return true;
}
bool UVTSaveSubsystem::Load() {
 UWorld* World=GetWorld(); if(!World||World->GetNetMode()==NM_Client) return false;
 const FString Path=SavePath(Slot); TArray<uint8> Bytes; UVTWorldSave* Snapshot=nullptr;
 if(FFileHelper::LoadFileToArray(Bytes,*Path)) Snapshot=Decode(Bytes);
 if(Snapshot&&!Migrate(Snapshot)) Snapshot=nullptr;
 if(!Validate(Snapshot)) {Bytes.Reset(); if(FFileHelper::LoadFileToArray(Bytes,*(Path+TEXT(".backup")))) Snapshot=Decode(Bytes); if(Snapshot&&!Migrate(Snapshot)) Snapshot=nullptr;}
 if(!Validate(Snapshot)) return false;
 auto* Sim=World->GetSubsystem<UVTSimulation>();
 auto Current=Sim->Ships; auto Shots=Sim->Projectiles;
 for(AVTProjectile* P:Shots) if(IsValid(P)) P->Destroy();
 for(AVTShip* Ship:Current) if(IsValid(Ship)) {if(Ship->IsNPC&&Ship->Controller) Ship->Controller->Destroy(); Ship->Destroy();}
 PlayerRecords=Snapshot->Players; WorldId=Snapshot->WorldId; Sim->WorldSeed=Snapshot->Seed;
 TMap<FGuid,AVTShip*> Entities;
 for(const auto& R:Snapshot->Ships) Entities.Add(R.Id,RestoreShip(R));
 for(auto It=World->GetPlayerControllerIterator();It;++It) if(auto* PC=Cast<AVTController>(It->Get())) if(auto* PS=PC->GetPlayerState<AVTPlayerState>()) {
  if(const auto* R=PlayerRecords.FindByPredicate([PS](const FVTSavedPlayer& P){return P.Profile==PS->Profile;})) {
   auto* Ship=RestoreShip(R->Ship); Entities.Add(R->Ship.Id,Ship); PC->Possess(Ship); PC->LocalIntent=FVTPilotIntent(); PC->NextSequence=0; PC->SendAccumulator=0;
   PS->Credits=R->Credits; PS->Boarded=R->Boarded; PS->Reputation=R->Reputation; PS->Heat=R->Heat;
   PC->ClientAcceptIdentity(WorldId,R->Token);
  } else if(auto* Mode=World->GetAuthGameMode<AVTGameMode>()) Mode->RestartPlayer(PC);
 }
 for(const auto& R:Snapshot->Projectiles) {
  auto* P=World->SpawnActor<AVTProjectile>(); P->PersistentId=R.Id; P->SourceId=R.Source; P->Source=Entities.FindRef(R.Source); P->SystemIndex=R.System;
  P->Position=R.Position; P->Previous=R.Position; P->Velocity=R.Velocity; P->Damage=R.Damage; P->Remaining=R.Remaining; P->Radius=R.Radius;
 }
 Sim->SimulationTime=Snapshot->SimulationTime; Sim->Accumulator=0; Sim->Bootstrapped=true; SinceSave=0; return true;
}

void UVTSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
 Super::Initialize(Collection);
 TearDownHandle=FWorldDelegates::OnWorldBeginTearDown.AddUObject(this,&UVTSaveSubsystem::WorldTearDown);
#if !UE_BUILD_SHIPPING
 FString Probe; if(FParse::Value(FCommandLine::Get(),TEXT("VTProbe="),Probe)) {PersonalSlot+=TEXT("-")+Probe; Slot=TEXT("Validation-")+Probe;}
#endif
 Personal=Cast<UVTPersonalSave>(UGameplayStatics::LoadGameFromSlot(PersonalSlot,0));
 if(!Personal) Personal=CastChecked<UVTPersonalSave>(UGameplayStatics::CreateSaveGameObject(UVTPersonalSave::StaticClass()));
 if(!Personal->Profile.IsValid()) {Personal->Profile=FGuid::NewGuid(); UGameplayStatics::SaveGameToSlot(Personal,PersonalSlot,0);}
}
void UVTSaveSubsystem::StoreToken(FGuid World,FGuid Token) {
 auto* Existing=Personal->Tokens.FindByPredicate([World](const FVTReconnectToken& R){return R.World==World;});
 if(Existing) Existing->Token=Token; else {FVTReconnectToken R; R.World=World; R.Token=Token; Personal->Tokens.Add(R);}
 UGameplayStatics::SaveGameToSlot(Personal,PersonalSlot,0);
}

void UVTSaveSubsystem::WorldTearDown(UWorld* World) {if(World&&World==GetWorld()&&World->GetGameInstance()==GetGameInstance()) Save();}
void UVTSaveSubsystem::Deinitialize() {FWorldDelegates::OnWorldBeginTearDown.Remove(TearDownHandle); Super::Deinitialize();}

bool UVTSaveSubsystem::Migrate(UVTWorldSave* Snapshot) const {
 if(!Snapshot) return false;
 if(Snapshot->Version==2) {
  auto Upgrade=[Snapshot](FVTSavedShip& Ship) {if(Ship.Motion.SimulationTime==0) Ship.Motion.SimulationTime=Snapshot->SimulationTime;};
  for(auto& Ship:Snapshot->Ships) Upgrade(Ship);
  for(auto& Captain:Snapshot->Players) Upgrade(Captain.Ship);
  Snapshot->Version=3;
 }
 return Snapshot->Version==3;
}
