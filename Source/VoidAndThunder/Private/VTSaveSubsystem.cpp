#include "VTSaveSubsystem.h"
#include "Async/Async.h"
#include "VTSessionSubsystem.h"
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
 FVTSavedShip R; R.Id=S->PersistentId; R.ClassId=S->ClassId; R.Fit=S->Fit; R.Faction=S->Faction; R.System=S->SystemIndex;
 R.Motion=S->Movement->Motion; R.Hull=S->Attributes->Hull.GetCurrentValue(); R.Battery=S->Attributes->Battery.GetCurrentValue(); R.NPC=S->IsNPC;
 R.Invulnerable=S->Invulnerable; R.Anchored=S->Anchored; R.ShipRole=S->ShipRole; R.Brain=S->Brain; R.DockProgress=S->DockProgress; R.JumpProgress=S->JumpProgress; R.JumpDestination=S->JumpDestination; R.BoardingTarget=S->Combat->BoardingTarget; R.BoardingProgress=S->Combat->BoardingProgress;
 R.Disabled=S->Disabled; R.Docked=S->Docked; R.Shields=S->Combat->Shields; R.Suppression=S->Combat->Suppression;
 R.EMPStress=S->Attributes->EMPStress.GetCurrentValue(); R.Equipment=S->Combat->EquipmentState; R.Equipment.Locks.Reset(); R.Equipment.LockElapsed=0;
 R.PortReload=S->PortReload; R.StarboardReload=S->StarboardReload;
 // A held bank wind-up is cancelled on restoration; retain the spent bank's reload.
 if(S->Combat->PortCharge>0) R.PortReload=S->Definition.Reload;
 if(S->Combat->StarboardCharge>0) R.StarboardReload=S->Definition.Reload;
 return R;
}
AVTShip* UVTSaveSubsystem::RestoreShip(const FVTSavedShip& R) {
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>(); FVTMotion M=R.Motion; M.Ack=0;
 AVTShip* S=Sim->SpawnShip(R.ClassId,R.System,M,R.NPC,R.Faction); S->PersistentId=R.Id; S->Disabled=R.Disabled; S->Docked=R.Docked; S->Invulnerable=R.Invulnerable; S->Anchored=R.Anchored; S->ShipRole=R.ShipRole; S->Brain=R.Brain; S->DockProgress=R.DockProgress; S->JumpProgress=R.JumpProgress; S->JumpDestination=R.JumpDestination; S->Brain.Shoulder=-1; S->Brain.Thumb=-1; S->Brain.AimLock=0; S->Brain.WarpPrime=0;
 S->ApplyFit(R.Fit,false); S->Combat->BoardingTarget=R.BoardingTarget; S->Combat->BoardingProgress=R.BoardingProgress;
 S->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),R.Hull); S->Abilities->SetNumericAttributeBase(UVTAttributes::BatteryAttribute(),R.Battery);
 S->Abilities->SetNumericAttributeBase(UVTAttributes::GetEMPStressAttribute(),R.EMPStress); S->Combat->EquipmentState=R.Equipment; S->Combat->RestoreDevices();
 S->Combat->Shields=R.Shields; S->Combat->Suppression=R.Suppression;
 S->Combat->RestoreReload(true,R.PortReload); S->Combat->RestoreReload(false,R.StarboardReload);
 S->Intent=FVTPilotIntent(); S->InputQueue.Reset(); return S;
}
void UVTSaveSubsystem::CapturePlayer(AVTController* PC) {
 if(!PC||!PC->GetWorld()->GetSubsystem<UVTSimulation>()->Bootstrapped) return; auto* PS=PC->GetPlayerState<AVTPlayerState>(); auto* Ship=Cast<AVTShip>(PC->GetPawn()); if(!PS||!Ship||!PS->Profile.IsValid()) return;
 FVTSavedPlayer* R=PlayerRecords.FindByPredicate([PS](const FVTSavedPlayer& Item){return Item.Profile==PS->Profile;});
 if(!R) {FVTSavedPlayer New; New.Profile=PS->Profile; New.Token=FGuid::NewGuid(); R=&PlayerRecords.Add_GetRef(New);}
 R->Ship=CaptureShip(Ship); R->Credits=PS->Credits; R->Boarded=PS->Boarded; R->Reputation=PS->Reputation; R->Heat=PS->Heat;
}
bool UVTSaveSubsystem::Validate(const UVTWorldSave* S) const {
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>(); if(!S||S->Version!=5||!S->WorldId.IsValid()||!FMath::IsFinite(S->SimulationTime)||S->SimulationTime<0||S->Ships.Num()>10000||S->Projectiles.Num()>100000) return false;
 TSet<FGuid> IDs;
 auto ValidShip=[&](const FVTSavedShip& R) {
  FVTShipDefinition Resolved; if(!Sim->Data->ResolveFit(R.ClassId,R.Fit,Resolved)) return false; const auto* Def=&Resolved;
  if(!R.Id.IsValid()||IDs.Contains(R.Id)||!Def||!Sim->Data->Systems.IsValidIndex(R.System)||!FMath::IsFinite(R.Motion.SimulationTime)||R.Motion.SimulationTime<0||R.Motion.Position.ContainsNaN()||R.Motion.Velocity.ContainsNaN()||!FMath::IsFinite(R.Motion.Heading)||!FMath::IsFinite(R.Motion.Omega)||!FMath::IsFinite(R.Hull)||R.Hull<0||R.Hull>Def->Hull||!FMath::IsFinite(R.Battery)||R.Battery<0||R.Battery>Def->BatteryMax) return false;
  for(int I=0;I<4;++I) if(!FMath::IsFinite(R.Shields[I])||R.Shields[I]<0||R.Shields[I]>Def->ShieldMax[I]||!FMath::IsFinite(R.Suppression[I])||R.Suppression[I]<0) return false;
  if(!FMath::IsFinite(R.BoardingProgress)||R.BoardingProgress<0||R.BoardingProgress>Sim->Data->Rules.BoardDwell) return false;
  if(!FMath::IsFinite(R.PortReload)||R.PortReload<0||!FMath::IsFinite(R.StarboardReload)||R.StarboardReload<0) return false;
  if(R.ShipRole<0||R.ShipRole>2||!FMath::IsFinite(R.DockProgress)||R.DockProgress<0||R.DockProgress>Sim->Data->Rules.BoardDwell+VT::Step+0.001f||!FMath::IsFinite(R.JumpProgress)||R.JumpProgress<0||R.JumpProgress>Sim->Data->Rules.JumpDwell+VT::Step+0.001f||(!R.JumpDestination.IsNone()&&!Sim->Data->Systems[R.System].Links.Contains(R.JumpDestination))) return false;
  const auto& Brain=R.Brain; if(Brain.Alert.ContainsNaN()||!FMath::IsFinite(Brain.ScanProgress)||Brain.ScanProgress<0||Brain.ScanProgress>1||!FMath::IsFinite(Brain.AttackTime)||!FMath::IsFinite(Brain.DistressTimer)) return false;
  for(float Clock:{Brain.AimLock,Brain.ThumbTravel,Brain.WarpPrime,Brain.AlertTTL}) if(!FMath::IsFinite(Clock)||Clock<0) return false;
  const auto& E=R.Equipment; const auto& D=Def->Equipment;
  if(!FMath::IsFinite(R.EMPStress)||R.EMPStress<0||R.EMPStress>Def->EMPResist||!FMath::IsFinite(E.Loaded)||E.Loaded<0||E.Loaded>D.Tubes||E.TorpedoMagazine<0||E.TorpedoMagazine>D.TorpedoMagazine||E.MineMagazine<0||E.MineMagazine>D.MineMagazine||E.LaunchQueue.Num()>6||E.Locks.Num()>6||!FMath::IsFinite(E.EMPAim)) return false;
  for(float Timer:{E.LockElapsed,E.LaunchTimer,E.EMPCooldown,E.MineCooldown,E.WarpCooldown,E.PDCooldown}) if(!FMath::IsFinite(Timer)||Timer<0) return false;
  IDs.Add(R.Id); return true;
 };
 for(const auto& R:S->Ships) if(!R.NPC||!ValidShip(R)) return false;
 TSet<FGuid> Profiles;
 for(const auto& P:S->Players) {
  if(!P.Profile.IsValid()||!P.Token.IsValid()||Profiles.Contains(P.Profile)||!ValidShip(P.Ship)||P.Ship.NPC) return false;
  if(P.Reputation.Num()!=Sim->Data->TrackedFactions.Num()||P.Heat.Num()!=P.Reputation.Num()||P.Credits<0||P.Boarded<0) return false;
  Profiles.Add(P.Profile);
  for(float V:P.Reputation) if(!FMath::IsFinite(V)||V<-100||V>100) return false;
  for(float V:P.Heat) if(!FMath::IsFinite(V)||V<0||V>100) return false;
 }
 for(const auto& P:S->Projectiles) {
  if(!P.Id.IsValid()||IDs.Contains(P.Id)||!Sim->Data->Systems.IsValidIndex(P.System)||P.Position.ContainsNaN()||P.Velocity.ContainsNaN()||!FMath::IsFinite(P.Damage)||P.Damage<0||!FMath::IsFinite(P.Remaining)||P.Remaining<=0||!FMath::IsFinite(P.Radius)||P.Radius<0) return false;
  if(!FMath::IsFinite(P.ReportCountdown)||P.ReportCountdown<0||P.ReportCountdown>0.25f||P.Velocity3D.ContainsNaN()||!FMath::IsFinite(P.Height)||!FMath::IsFinite(P.TurnRate)||P.TurnRate<0||uint8(P.Kind)>uint8(EVTProjectileKind::Mine)) return false;
  IDs.Add(P.Id);
 }
 return true;
}
bool UVTSaveSubsystem::FlushPendingSave(bool Wait) {
 if(!PendingWrite.IsValid()) return LastWriteSucceeded;
 if(!Wait&&!PendingWrite.IsReady()) return false;
 LastWriteSucceeded=PendingWrite.Get(); PendingWrite=TFuture<bool>();
 if(LastWriteSucceeded) LastGoodBytes.Add(PendingPath,MoveTemp(PendingBytes));
 else {PendingBytes.Reset(); UE_LOG(LogTemp,Error,TEXT("Background save failed; previous snapshot retained: %s"),*PendingPath); if(auto* Session=GetGameInstance()->GetSubsystem<UVTSessionSubsystem>()) Session->SetStatus(TEXT("Autosave failed; previous snapshot retained."));}
 PendingPath.Reset(); return LastWriteSucceeded;
}
void UVTSaveSubsystem::Advance(float Dt) {
 FlushPendingSave(false); SinceSave+=Dt;
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();
 if(Sim->Data&&SinceSave>=Sim->Data->Rules.AutosaveSeconds&&!PendingWrite.IsValid()) if(Save(true)) SinceSave=0;
}
bool UVTSaveSubsystem::Save(bool Background) {
 if(Background&&PendingWrite.IsValid()&&!PendingWrite.IsReady()) return false;
 FlushPendingSave();
 UWorld* World=GetWorld(); if(!World||World->GetNetMode()==NM_Client) return false;
 auto* Sim=World->GetSubsystem<UVTSimulation>(); if(!Sim||!Sim->Bootstrapped||!Sim->Data||World->GetMapName().Contains(TEXT("Menu"))||CastChecked<UVTGameInstance>(GetGameInstance())->PlayMode!=TEXT("sandbox")) return false;
 for(auto It=World->GetPlayerControllerIterator();It;++It) CapturePlayer(Cast<AVTController>(It->Get()));
 auto* Snapshot=CastChecked<UVTWorldSave>(UGameplayStatics::CreateSaveGameObject(UVTWorldSave::StaticClass()));
 Snapshot->WorldId=WorldId; Snapshot->Seed=Sim->WorldSeed; Snapshot->SimulationTime=Sim->SimulationTime; Snapshot->Players=PlayerRecords;
 for(AVTShip* Ship:Sim->Ships) if(IsValid(Ship)&&Ship->IsNPC) Snapshot->Ships.Add(CaptureShip(Ship));
 for(AVTProjectile* P:Sim->Projectiles) if(IsValid(P)&&P->Remaining>0) {
  FVTSavedProjectile R; R.Id=P->PersistentId; R.Source=P->SourceId; R.System=P->SystemIndex; R.Position=P->Position; R.Velocity=P->Velocity; R.Damage=P->Damage; R.Remaining=P->Remaining; R.Radius=P->Radius; R.Kind=P->Kind; R.Target=P->TargetId; R.AttackerProfile=P->AttackerProfile; R.SourceFaction=P->SourceFaction; R.SourceNPC=P->SourceNPC; R.Height=P->Height; R.Velocity3D=P->Velocity3D; R.TurnRate=P->TurnRate; R.ReportCountdown=P->ReportCountdown; Snapshot->Projectiles.Add(R);
 }
 if(!Validate(Snapshot)) return false;
 TArray<uint8> Bytes; if(!Encode(Snapshot,Bytes)) return false;
 const FString Path=SavePath(CampaignPrefix+Slot); // Validate the previous snapshot once per slot on the game thread; workers only own bytes.
 if(!LastGoodBytes.Contains(Path)) {
  TArray<uint8> Old;
  if(FFileHelper::LoadFileToArray(Old,*Path)) if(auto* Good=Decode(Old)) if(Migrate(Good)&&Validate(Good)) LastGoodBytes.Add(Path,MoveTemp(Old));
 }
 TArray<uint8> Previous=LastGoodBytes.FindRef(Path);
 auto Write=[Path,Previous=MoveTemp(Previous),Bytes]() {
  if(!Previous.IsEmpty()&&!AtomicWrite(Path+TEXT(".backup"),Previous)) return false;
  return AtomicWrite(Path,Bytes);
 };
 if(Background) {PendingPath=Path; PendingBytes=Bytes; PendingWrite=Async(EAsyncExecution::ThreadPool,MoveTemp(Write)); SinceSave=0; return true;}
 LastWriteSucceeded=Write(); if(LastWriteSucceeded) {LastGoodBytes.Add(Path,MoveTemp(Bytes)); SinceSave=0;} return LastWriteSucceeded;
}
bool UVTSaveSubsystem::Load() {
 FlushPendingSave();
 UWorld* World=GetWorld(); if(!World||World->GetNetMode()==NM_Client) return false;
 const FString Path=SavePath(CampaignPrefix+Slot); TArray<uint8> Bytes; UVTWorldSave* Snapshot=nullptr;
 if(FFileHelper::LoadFileToArray(Bytes,*Path)) Snapshot=Decode(Bytes);
 if(Snapshot&&!Migrate(Snapshot)) Snapshot=nullptr;
 if(!Validate(Snapshot)) {Bytes.Reset(); if(FFileHelper::LoadFileToArray(Bytes,*(Path+TEXT(".backup")))) Snapshot=Decode(Bytes); if(Snapshot&&!Migrate(Snapshot)) Snapshot=nullptr;}
 if(!Validate(Snapshot)) return false;
 LastGoodBytes.Add(SavePath(CampaignPrefix+Slot),Bytes);
 auto* Sim=World->GetSubsystem<UVTSimulation>();
 auto Current=Sim->Ships; auto Shots=Sim->Projectiles;
 for(AVTProjectile* P:Shots) if(IsValid(P)) P->Destroy();
 for(AVTShip* Ship:Current) if(IsValid(Ship)) {if(Ship->IsNPC&&Ship->Controller) Ship->Controller->Destroy(); Ship->Destroy();}
 PlayerRecords=Snapshot->Players; WorldId=Snapshot->WorldId; Sim->WorldSeed=Snapshot->Seed;
 TMap<FGuid,AVTShip*> Entities;
 for(const auto& R:Snapshot->Ships) Entities.Add(R.Id,RestoreShip(R));
 for(auto It=World->GetPlayerControllerIterator();It;++It) if(auto* PC=Cast<AVTController>(It->Get())) if(auto* PS=PC->GetPlayerState<AVTPlayerState>()) {
  if(!PS->Profile.IsValid()) continue;
  if(const auto* R=PlayerRecords.FindByPredicate([PS](const FVTSavedPlayer& P){return P.Profile==PS->Profile;})) {
   auto* Ship=RestoreShip(R->Ship); Entities.Add(R->Ship.Id,Ship); PC->Possess(Ship); PC->LocalIntent=FVTPilotIntent(); PC->NextSequence=0; PC->SendAccumulator=0;
   PS->Credits=R->Credits; PS->Boarded=R->Boarded; PS->Reputation=R->Reputation; PS->Heat=R->Heat;
   PC->ClientAcceptIdentity(WorldId,R->Token);
  } else if(auto* Mode=World->GetAuthGameMode<AVTGameMode>()) Mode->RestartPlayer(PC);
 }
 for(const auto& R:Snapshot->Projectiles) {
  auto* P=World->SpawnActor<AVTProjectile>(); P->PersistentId=R.Id; P->SourceId=R.Source; P->Source=Entities.FindRef(R.Source); P->SystemIndex=R.System;
  P->Kind=R.Kind; P->TargetId=R.Target; P->AttackerProfile=R.AttackerProfile; P->SourceFaction=R.SourceFaction; P->SourceNPC=R.SourceNPC; P->Height=R.Height; P->Velocity3D=R.Velocity3D; P->TurnRate=R.TurnRate; P->ReportCountdown=R.ReportCountdown;
  P->Position=R.Position; P->Previous=R.Position; P->Velocity=R.Velocity; P->Damage=R.Damage; P->Remaining=R.Remaining; P->Radius=R.Radius;
 }
 Sim->SimulationTime=Snapshot->SimulationTime; Sim->Accumulator=0; Sim->Bootstrapped=true; SinceSave=0; return true;
}

void UVTSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
 Super::Initialize(Collection);
 TearDownHandle=FWorldDelegates::OnWorldBeginTearDown.AddUObject(this,&UVTSaveSubsystem::WorldTearDown);
#if !UE_BUILD_SHIPPING
 FString Probe; if(FParse::Value(FCommandLine::Get(),TEXT("VTProbe="),Probe)) {PersonalSlot+=TEXT("-")+Probe; Slot=TEXT("Validation-")+Probe; CareerSlot+=TEXT("-")+Probe;}
#endif
#if WITH_EDITOR
 if(GetWorld()&&GetWorld()->WorldType==EWorldType::PIE){const auto* Context=GEngine->GetWorldContextFromWorld(GetWorld());const FString Suffix=FString::Printf(TEXT("-PIE-%d"),Context?Context->PIEInstance:0);PersonalSlot+=Suffix;CareerSlot+=Suffix;CampaignPrefix=FString::Printf(TEXT("PIE/%d/"),Context?Context->PIEInstance:0);}
#endif
 Personal=Cast<UVTPersonalSave>(UGameplayStatics::LoadGameFromSlot(PersonalSlot,0));
 if(!Personal) Personal=CastChecked<UVTPersonalSave>(UGameplayStatics::CreateSaveGameObject(UVTPersonalSave::StaticClass()));
 Career=Cast<UVTCareerSave>(UGameplayStatics::LoadGameFromSlot(CareerSlot,0)); if(!Career) Career=CastChecked<UVTCareerSave>(UGameplayStatics::CreateSaveGameObject(UVTCareerSave::StaticClass()));
 if(auto* GI=Cast<UVTGameInstance>(GetGameInstance())) {GI->SelectedHull=Personal->PreferredHull; GI->SelectedFit=Personal->PreferredFit;}
 if(!Personal->Profile.IsValid()) {Personal->Profile=FGuid::NewGuid(); UGameplayStatics::SaveGameToSlot(Personal,PersonalSlot,0);}
}
void UVTSaveSubsystem::ResetWorldIdentity() {FlushPendingSave(); WorldId=FGuid::NewGuid(); PlayerRecords.Reset();}
void UVTSaveSubsystem::StoreToken(FGuid World,FGuid Token) {
 auto* Existing=Personal->Tokens.FindByPredicate([World](const FVTReconnectToken& R){return R.World==World;});
 if(Existing) Existing->Token=Token; else {FVTReconnectToken R; R.World=World; R.Token=Token; Personal->Tokens.Add(R);}
 UGameplayStatics::SaveGameToSlot(Personal,PersonalSlot,0);
}

void UVTSaveSubsystem::WorldTearDown(UWorld* World) {
 if(World&&World==GetWorld()&&World->GetGameInstance()==GetGameInstance()) {
  Save();
  // The complete boundary snapshot is final. Later Logout/Shutdown callbacks see
  // a partially destroyed world and must not replace it with an empty population.
  if(auto* Sim=World->GetSubsystem<UVTSimulation>()) Sim->Bootstrapped=false;
 }
}
void UVTSaveSubsystem::Deinitialize() {FlushPendingSave(); FWorldDelegates::OnWorldBeginTearDown.Remove(TearDownHandle); Super::Deinitialize();}

bool UVTSaveSubsystem::Migrate(UVTWorldSave* Snapshot) const {
 if(!Snapshot) return false;
 if(Snapshot->Version==2) {
  auto Upgrade=[Snapshot](FVTSavedShip& Ship) {if(Ship.Motion.SimulationTime==0) Ship.Motion.SimulationTime=Snapshot->SimulationTime;};
  for(auto& Ship:Snapshot->Ships) Upgrade(Ship);
  for(auto& Captain:Snapshot->Players) Upgrade(Captain.Ship);
  Snapshot->Version=3;
 }
 if(Snapshot->Version==3) {
  auto Upgrade=[this](FVTSavedShip& Ship) {if(const auto* D=GetWorld()->GetSubsystem<UVTSimulation>()->Data->FindShip(Ship.ClassId)) {Ship.Equipment.Loaded=D->Equipment.Tubes; Ship.Equipment.TorpedoMagazine=D->Equipment.TorpedoMagazine; Ship.Equipment.MineMagazine=D->Equipment.MineMagazine;}};
  for(auto& Ship:Snapshot->Ships) Upgrade(Ship); for(auto& Captain:Snapshot->Players) Upgrade(Captain.Ship); Snapshot->Version=4;
 }
 if(Snapshot->Version==4) Snapshot->Version=5;
 return Snapshot->Version==5;
}

void UVTSaveSubsystem::RememberFit() {
 auto* GI=CastChecked<UVTGameInstance>(GetGameInstance()); Personal->PreferredHull=GI->SelectedHull; Personal->PreferredFit=GI->SelectedFit; UGameplayStatics::SaveGameToSlot(Personal,PersonalSlot,0);
}
void UVTSaveSubsystem::RecordSolo(bool Victory,int32 Wave,int32 Boarded) {
 if(SoloRecorded||!Career) return; SoloRecorded=true; ++Career->Runs; Career->Victories+=Victory ? 1 : 0; Career->DeepestWave=FMath::Max(Career->DeepestWave,Wave); Career->ShipsBoarded+=Boarded; UGameplayStatics::SaveGameToSlot(Career,CareerSlot,0);
}
