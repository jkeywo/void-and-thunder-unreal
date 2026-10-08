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
 Ship->Movement->Motion.Position=JumpPosition(Destination,Previous);Ship->GatePassage->BeginArrival(Ship->Movement->Motion.Position,SimulationTime);
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
  if(Ship->IsNPC||Ship->Docked||Ship->Disabled||Ship->GatePassage->Arriving()) continue;
  const auto& System=Data->Systems[Ship->SystemIndex]; auto* PS=Ship->GetPlayerState<AVTPlayerState>(); auto Standing=Standings.Read(PS?PS->Profile:FGuid(),System.Owner);
  bool Allowed=!Standing.Found||Standing.Reputation>=Data->World.dock_refusal_threshold;
  bool NearStation=System.HasStation&&Allowed&&(Ship->Movement->Motion.Position-Data->Rules.StationPosition).SizeSquared()<=FMath::Square(Data->Rules.StationRadius+Data->Rules.BoardRange);
  // Legacy station docking accrues by holding position; jump/boarding use the interaction key.
  Ship->DockProgress=NearStation ? Ship->DockProgress+VT::Step : 0;
  if(Ship->DockProgress>=Data->Rules.BoardDwell) {Ship->Docked=true; Ship->Intent=FVTPilotIntent(); Ship->Movement->Motion.Velocity=FVector2D::ZeroVector; continue;}
  Ship->GatePassage->InteractionStep();
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
