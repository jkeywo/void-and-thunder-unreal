#include "VTGameplay.h"
#include "VTGate.h"
#include "VTCombat.h"
#include "VTSaveSubsystem.h"

FVector2D UVTSimulation::JumpPosition(int32 System,FName Destination) const {
 const auto& Origin=Data->Systems[System]; int Index=Data->FindSystem(Destination); if(Index<0) return FVector2D::ZeroVector;
 auto Delta=Data->Systems[Index].ChartPosition-Origin.ChartPosition; Delta.Y=-Delta.Y;
 return Delta.GetSafeNormal()*Origin.Radius*Data->Rules.JumpEdgeFraction;
}
void UVTSimulation::TravelShip(AVTShip* Ship,int32 Destination) {
 if(!Data->Systems.IsValidIndex(Destination)) return;
 FName Previous=Data->Systems[Ship->SystemIndex].Id; Ship->SystemIndex=Destination;
 auto& M=Ship->Movement->Motion; M.Position=JumpPosition(Destination,Previous)*0.85; M.Velocity=FVector2D::ZeroVector;
 Ship->Movement->Previous=M; Ship->Movement->Authority=M; Ship->Movement->Pending.Reset(); Ship->Combat->EquipmentState.Locks.Reset(); Ship->Combat->EquipmentState.LockElapsed=0; Ship->Combat->BoardingTarget.Invalidate(); Ship->Combat->BoardingProgress=0;
 Ship->JumpDestination=NAME_None; Ship->JumpProgress=0; Ship->JumpEntering=false; Ship->DockProgress=0; Ship->Intent=FVTPilotIntent(); Ship->ForceNetUpdate();
}
void UVTSimulation::RecordHit(AVTShip* Victim,AVTShip* Attacker,float Amount,FGuid Profile,FName AttackerFaction) {
 if(!AttackerFaction.IsNone()) Victim->Brain.LastAttackerFaction=AttackerFaction;
 if(IsValid(Attacker)) {Victim->Brain.LastAttacker=Attacker->PersistentId; Victim->Brain.LastAttackerFaction=Attacker->Faction; if(auto* PS=Attacker->GetPlayerState<AVTPlayerState>()) Profile=PS->Profile;}
 Victim->Brain.LastAttackerProfile=Profile; Victim->Brain.AttackTime=SimulationTime;
 if(Victim->ShipRole==1&&!Victim->Brain.DistressSent&&Victim->Brain.DistressTimer<0) Victim->Brain.DistressTimer=3;
 if(!Profile.IsValid()||Amount<=0) return;
 AVTPlayerState* PS=nullptr;
 for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It) if(auto* State=It->Get()->GetPlayerState<AVTPlayerState>()) if(State->Profile==Profile) {PS=State; break;}
 auto* Save=GetWorld()->GetGameInstance()->GetSubsystem<UVTSaveSubsystem>();
 auto* Record=Save ? Save->PlayerRecords.FindByPredicate([Profile](const FVTSavedPlayer& R){return R.Profile==Profile;}) : nullptr;
 auto* Heat=PS ? &PS->Heat : Record ? &Record->Heat : nullptr; auto* Reputation=PS ? &PS->Reputation : Record ? &Record->Reputation : nullptr;
 if(!Heat||!Reputation) return;
 auto Crime=[&](FName Faction) {int I=Data->FactionIndex(Faction); if(Heat->IsValidIndex(I)&&Reputation->IsValidIndex(I)&&(*Reputation)[I]>=Data->World.hostile_threshold) (*Heat)[I]=FMath::Min(100.f,(*Heat)[I]+Amount*Data->World.heat_per_damage);};
 Crime(Victim->Faction); FName Owner=Data->Systems[Victim->SystemIndex].Owner;
 if(Owner!=Victim->Faction&&Data->StandingBetween(Owner,Victim->Faction)>=Data->World.hostile_threshold) Crime(Owner);
}
void UVTSimulation::AwardAvenging(AVTShip* Destroyed,const TArray<TObjectPtr<AVTShip>>& Survivors) {
 const FGuid Profile=Destroyed->Brain.LastAttackerProfile; if(!Profile.IsValid()) return;
 TArray<float>* Reputation=nullptr;
 for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It) if(auto* PS=It->Get()->GetPlayerState<AVTPlayerState>()) if(PS->Profile==Profile) {Reputation=&PS->Reputation; break;}
 if(!Reputation) if(auto* Save=GetWorld()->GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()) if(auto* Record=Save->PlayerRecords.FindByPredicate([Profile](const FVTSavedPlayer& R){return R.Profile==Profile;})) Reputation=&Record->Reputation;
 if(!Reputation) return;
 for(AVTShip* Rescued:Survivors) if(IsValid(Rescued)&&Rescued!=Destroyed&&Rescued->Attributes->Hull.GetCurrentValue()>0&&Rescued->Brain.LastAttackerFaction==Destroyed->Faction&&SimulationTime-Rescued->Brain.AttackTime<Data->World.recent_attack_memory) {int Faction=Data->FactionIndex(Rescued->Faction); if(Reputation->IsValidIndex(Faction)) (*Reputation)[Faction]=FMath::Min(100.f,(*Reputation)[Faction]+Data->World.avenge_reputation_bonus);}
}
void UVTSimulation::WorldStep() {
 for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It) if(auto* PS=It->Get()->GetPlayerState<AVTPlayerState>()) {
  for(int I=0;I<PS->Heat.Num();++I) if(PS->Reputation.IsValidIndex(I)) {float Decay=FMath::Min(PS->Heat[I],Data->World.heat_decay_per_sec*VT::Step); PS->Heat[I]-=Decay; PS->Reputation[I]=FMath::Clamp(PS->Reputation[I]-Decay*Data->World.heat_to_reputation_rate,-100.f,100.f);}
 }
 // Disconnected captains retain individual standing while the hosted world runs.
 auto* Save=GetWorld()->GetGameInstance()->GetSubsystem<UVTSaveSubsystem>();
 if(Save) for(auto& Record:Save->PlayerRecords) {
  bool Connected=false; for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It) if(auto* PS=It->Get()->GetPlayerState<AVTPlayerState>()) if(PS->Profile==Record.Profile) {Connected=true; break;}
  if(Connected) continue;
  for(int I=0;I<Record.Heat.Num();++I) if(Record.Reputation.IsValidIndex(I)) {float Decay=FMath::Min(Record.Heat[I],Data->World.heat_decay_per_sec*VT::Step); Record.Heat[I]-=Decay; Record.Reputation[I]=FMath::Clamp(Record.Reputation[I]-Decay*Data->World.heat_to_reputation_rate,-100.f,100.f);}
 }
 for(AVTShip* Ship:Ships) if(IsValid(Ship)) {
  Ship->Brain.AlertTTL=FMath::Max(0.f,Ship->Brain.AlertTTL-VT::Step);
  if(Ship->Brain.DistressTimer>=0&&!Ship->Disabled) {Ship->Brain.DistressTimer-=VT::Step; if(Ship->Brain.DistressTimer<=0) {Ship->Brain.DistressSent=true; Ship->Brain.DistressTimer=-1; for(auto* Patrol:SystemShips[Ship->SystemIndex]) if(Patrol->ShipRole==2&&(Patrol->Faction==Ship->Faction||Data->StandingBetween(Patrol->Faction,Ship->Faction)>=0)) {Patrol->Brain.Alert=Ship->Movement->Motion.Position; Patrol->Brain.AlertTTL=Data->World.alert_ttl;}}}
  if(Ship->IsNPC||Ship->Docked||Ship->Disabled) continue;
  const auto& System=Data->Systems[Ship->SystemIndex]; auto* PS=Ship->GetPlayerState<AVTPlayerState>(); int Owner=Data->FactionIndex(System.Owner);
  bool Allowed=!PS||!PS->Reputation.IsValidIndex(Owner)||PS->Reputation[Owner]>=Data->World.dock_refusal_threshold;
  bool NearStation=System.HasStation&&Allowed&&(Ship->Movement->Motion.Position-Data->Rules.StationPosition).SizeSquared()<=FMath::Square(Data->Rules.StationRadius+Data->Rules.BoardRange);
  // Legacy station docking accrues by holding position; jump/boarding use the interaction key.
  Ship->DockProgress=NearStation ? Ship->DockProgress+VT::Step : 0;
  if(Ship->DockProgress>=Data->Rules.BoardDwell) {Ship->Docked=true; Ship->Intent=FVTPilotIntent(); Ship->Movement->Motion.Velocity=FVector2D::ZeroVector; continue;}
  FName Destination; double Best=Data->Rules.JumpRange*Data->Rules.JumpRange;
  for(FName Link:System.Links) {double Distance=(Ship->Movement->Motion.Position-JumpPosition(Ship->SystemIndex,Link)).SizeSquared(); if(Distance<Best) {Destination=Link; Best=Distance;}}
  if(Destination!=Ship->JumpDestination) {Ship->JumpDestination=Destination;Ship->JumpProgress=0;Ship->JumpEntering=false;}
  const bool Held=!Destination.IsNone()&&(Ship->Intent.Buttons&VTButtons::Interact)&&!Ship->Combat->BoardingTarget.IsValid();
  if(!Held){Ship->JumpEntering=false;continue;}
  Ship->JumpProgress=FMath::Min(Data->Rules.JumpDwell,Ship->JumpProgress+VT::Step);
  const auto Centre=JumpPosition(Ship->SystemIndex,Destination),Normal=Centre.GetSafeNormal();const auto& Motion=Ship->Movement->Motion;
  const float Alignment=FVector2D::DotProduct(FVector2D(FMath::Cos(Motion.Heading),FMath::Sin(Motion.Heading)),Normal);
  if(!Ship->JumpEntering&&Ship->JumpProgress>=Data->Rules.JumpDwell&&(Motion.Position-(Centre-Normal*Data->GateApproachDistance)).SizeSquared()<=FMath::Square(Data->GateArrivalTolerance*1.5f)&&Motion.Velocity.SizeSquared()<FMath::Square(Data->GateCruiseSpeed*0.3f)&&Alignment>=FMath::Cos(Data->GateAlignmentTolerance))Ship->JumpEntering=true;
  if(Ship->JumpEntering&&VTGate::Crossed(Ship->Movement->Previous,Motion,Centre,Data->GateOpeningRadius-Ship->Definition.Radius,Data->GateAlignmentTolerance))TravelShip(Ship,Data->FindSystem(Destination));
 }
}
void AVTController::ServerStationAction_Implementation(FName Action) {
 auto* Ship=Cast<AVTShip>(GetPawn()); auto* PS=GetPlayerState<AVTPlayerState>(); if(!Ship||!PS||!Ship->Docked) return;
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>(); const auto& Rules=Sim->Data->Rules;
 if((Ship->Movement->Motion.Position-Rules.StationPosition).SizeSquared()>FMath::Square(Rules.StationRadius+Rules.BoardRange)||!Sim->Data->Systems[Ship->SystemIndex].HasStation) return;
 if(Action==FName("undock")) {Ship->Docked=false; Ship->DockProgress=0; Ship->Movement->Motion.Position=Rules.StationPosition+FVector2D(0,-Rules.StationRadius-Rules.BoardRange); Ship->Intent=FVTPilotIntent();}
 if(Action==FName("repair")) {Ship->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),Ship->Definition.Hull); Ship->Abilities->SetNumericAttributeBase(UVTAttributes::GetEMPStressAttribute(),0); Ship->Disabled=false;}
 if(Action==FName("pay_heat")) {int I=Sim->Data->FactionIndex(Sim->Data->Systems[Ship->SystemIndex].Owner); if(PS->Heat.IsValidIndex(I)) {int Cost=FMath::CeilToInt(PS->Heat[I]*Rules.CreditsPerHeat); if(PS->Credits>=Cost) {PS->Credits-=Cost; PS->Heat[I]=0;}}}
 GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->CapturePlayer(this);
}
void AVTController::ServerRefit_Implementation(FName Hull,FVTLoadoutSelection Selection) {
 auto* Ship=Cast<AVTShip>(GetPawn()); auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>(); if(!Ship||!Ship->Docked||Ship->Disabled||!Sim->Data->Systems[Ship->SystemIndex].HasStation||(Ship->Movement->Motion.Position-Sim->Data->Rules.StationPosition).Size()>Sim->Data->Rules.StationRadius+Sim->Data->Rules.BoardRange) return;
 FVTShipDefinition Resolved; if(!Sim->Data->ResolveFit(Hull,Selection,Resolved)||Hull.ToString().StartsWith(TEXT("house"))) return;
 auto* Replacement=Sim->SpawnShip(Hull,Ship->SystemIndex,Ship->Movement->Motion,false,Ship->Faction); Replacement->ApplyFit(Selection,true); Replacement->Docked=true; Possess(Replacement); Ship->Destroy();
 if(const auto* Record=GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->PlayerRecords.FindByPredicate([&](const FVTSavedPlayer& R){return R.Profile==GetPlayerState<AVTPlayerState>()->Profile;})) ClientAcceptIdentity(GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->WorldId,Record->Token);
}
