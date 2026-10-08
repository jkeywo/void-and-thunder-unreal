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
 FName Previous=Data->Systems[Ship->SystemIndex].Id; Ship->SystemIndex=Destination;Queries.Invalidate();
 auto& M=Ship->Movement->Motion; M.Position=JumpPosition(Destination,Previous);Ship->JumpArrivalTarget=M.Position*0.85;Ship->JumpArrivalStarted=SimulationTime;Ship->JumpArriving=true;VTGate::Arrive(M,M.Position,Ship->JumpArrivalTarget,0,Data->GateArrivalDuration);
 Ship->Movement->Previous=M; Ship->Movement->Authority=M; Ship->Movement->Pending.Reset(); Ship->Combat->EquipmentState.Locks.Reset(); Ship->Combat->EquipmentState.LockElapsed=0; Ship->Combat->BoardingTarget.Invalidate(); Ship->Combat->BoardingProgress=0;
 Ship->JumpDestination=NAME_None; Ship->JumpProgress=0; Ship->JumpEntering=false; Ship->DockProgress=0; Ship->Intent=FVTPilotIntent(); Ship->ForceNetUpdate();
}
void UVTSimulation::RecordHit(AVTShip* Victim,AVTShip* Attacker,float Amount,FGuid Profile,FName AttackerFaction) {
 if(!AttackerFaction.IsNone()) Victim->Brain.LastAttackerFaction=AttackerFaction;
 if(IsValid(Attacker)) {Victim->Brain.LastAttacker=Attacker->PersistentId; Victim->Brain.LastAttackerFaction=Attacker->Faction; if(auto* PS=Attacker->GetPlayerState<AVTPlayerState>()) Profile=PS->Profile;}
 Victim->Brain.LastAttackerProfile=Profile; Victim->Brain.AttackTime=SimulationTime;
 if(Victim->ShipRole==1&&!Victim->Brain.DistressSent&&Victim->Brain.DistressTimer<0) Victim->Brain.DistressTimer=3;
 Standings.Crime(Profile,Victim,Amount);
}
void UVTSimulation::AwardAvenging(AVTShip* Destroyed,const TArray<TObjectPtr<AVTShip>>& Survivors) {
 const FGuid Profile=Destroyed->Brain.LastAttackerProfile; if(!Profile.IsValid()) return;
 for(AVTShip* Rescued:Survivors) if(IsValid(Rescued)&&Rescued!=Destroyed&&Rescued->Attributes->Hull.GetCurrentValue()>0&&Rescued->Brain.LastAttackerFaction==Destroyed->Faction&&SimulationTime-Rescued->Brain.AttackTime<Data->World.recent_attack_memory)Standings.Rescue(Profile,Rescued->Faction);
}
void UVTSimulation::WorldStep() {
 Standings.Decay(VT::Step);
 for(AVTShip* Ship:Ships) if(IsValid(Ship)) {
  Ship->Brain.AlertTTL=FMath::Max(0.f,Ship->Brain.AlertTTL-VT::Step);
  if(Ship->Brain.DistressTimer>=0&&!Ship->Disabled) {Ship->Brain.DistressTimer-=VT::Step; if(Ship->Brain.DistressTimer<=0) {Ship->Brain.DistressSent=true; Ship->Brain.DistressTimer=-1; for(auto* Patrol:Queries.Ordered(Ship->SystemIndex)) if(Patrol->ShipRole==2&&(Patrol->Faction==Ship->Faction||Data->StandingBetween(Patrol->Faction,Ship->Faction)>=0)) {Patrol->Brain.Alert=Ship->Movement->Motion.Position; Patrol->Brain.AlertTTL=Data->World.alert_ttl;}}}
  if(Ship->IsNPC||Ship->Docked||Ship->Disabled||Ship->JumpArriving) continue;
  const auto& System=Data->Systems[Ship->SystemIndex]; auto* PS=Ship->GetPlayerState<AVTPlayerState>(); auto Standing=Standings.Read(PS?PS->Profile:FGuid(),System.Owner);
  bool Allowed=!Standing.Found||Standing.Reputation>=Data->World.dock_refusal_threshold;
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
 if(Action==FName("pay_heat")) Sim->Standings.PayHeat(PS->Profile,Sim->Data->Systems[Ship->SystemIndex].Owner,PS->Credits);
 GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->CapturePlayer(this);
}
void AVTController::ServerRefit_Implementation(FName Hull,FVTLoadoutSelection Selection) {
 auto* Ship=Cast<AVTShip>(GetPawn()); auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>(); if(!Ship||!Ship->Docked||Ship->Disabled||!Sim->Data->Systems[Ship->SystemIndex].HasStation||(Ship->Movement->Motion.Position-Sim->Data->Rules.StationPosition).Size()>Sim->Data->Rules.StationRadius+Sim->Data->Rules.BoardRange) return;
 FVTShipDefinition Resolved; if(!Sim->Data->ResolveFit(Hull,Selection,Resolved)||Hull.ToString().StartsWith(TEXT("house"))) return;
 auto* Replacement=Sim->SpawnShip(Hull,Ship->SystemIndex,Ship->Movement->Motion,false,Ship->Faction); Replacement->ApplyFit(Selection,true); Replacement->Docked=true; Possess(Replacement); Ship->Destroy();
 if(const auto* Record=GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->PlayerRecords.FindByPredicate([&](const FVTSavedPlayer& R){return R.Profile==GetPlayerState<AVTPlayerState>()->Profile;})) ClientAcceptIdentity(GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->WorldId,Record->Token);
}
