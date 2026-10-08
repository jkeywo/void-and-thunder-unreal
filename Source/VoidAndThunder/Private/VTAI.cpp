#include "VTGameplay.h"
#include "VTCombat.h"

namespace {
const FName Freebooters(TEXT("Freebooters"));
const FName Houses(TEXT("Houses"));
float Ramp(float X,float Full,float Zero) {return FMath::Abs(Zero-Full)<1e-6 ? (X<=Full ? 1.f : 0.f) : FMath::Clamp((Zero-X)/(Zero-Full),0.f,1.f);}
void Face(AVTShip* Ship,FVector2D Aim,float Throttle,const FVTAITuning& T) {Ship->Intent.Turn=FMath::Clamp(FMath::UnwindRadians(float(FMath::Atan2(Aim.Y,Aim.X))-Ship->Movement->Motion.Heading)*T.turn_gain,-1.f,1.f); Ship->Intent.Throttle=Throttle;}
void Beam(AVTShip* Ship,AVTShip* Target,const FVTAITuning& T,bool Flee) {
 auto Offset=Target->Movement->Motion.Position-Ship->Movement->Motion.Position;
 float Bearing=FMath::Atan2(Offset.Y,Offset.X), Heading=Ship->Movement->Motion.Heading;
 auto Lead=Offset+Target->Movement->Motion.Velocity*T.station_lead_secs; float Desired=FMath::Atan2(Lead.Y,Lead.X), Cruise=1;
 if(Flee) Desired=Bearing+PI;
 else if(Offset.Size()<=Ship->Definition.AIEngageRange) {float Port=Desired-PI/2,Starboard=Desired+PI/2; Desired=FMath::Abs(FMath::UnwindRadians(Port-Heading))<=FMath::Abs(FMath::UnwindRadians(Starboard-Heading)) ? Port : Starboard; Cruise=T.station_throttle;}
 float Error=FMath::UnwindRadians(Desired-Heading); Ship->Intent.Turn=FMath::Clamp(Error*T.turn_gain,-1.f,1.f); Ship->Intent.Throttle=Cruise*(1-T.turn_ease*FMath::Abs(Error)/PI);
}
struct Solution {bool Valid=false; float Distance=FLT_MAX; FVector2D Direction=FVector2D::ZeroVector;};
template<typename Allocator>
Solution Gun(AVTShip* Ship,const TArray<AVTShip*,Allocator>& Targets,bool Port) {
 if((Port ? Ship->PortReload : Ship->StarboardReload)>0) return {};
 for(auto* Target:Targets) {
  auto P=Target->Movement->Motion.Position-Ship->Movement->Motion.Position, V=Target->Movement->Motion.Velocity-Ship->Movement->Motion.Velocity; float Speed=Ship->Definition.MuzzleSpeed;
  double A=V.SizeSquared()-Speed*Speed, B=2*FVector2D::DotProduct(P,V), C=P.SizeSquared(), Time=-1;
  if(FMath::Abs(A)<1e-6) {if(FMath::Abs(B)>1e-6) Time=-C/B;}
  else {double D=B*B-4*A*C; if(D>=0) {double X=(-B-FMath::Sqrt(D))/(2*A),Y=(-B+FMath::Sqrt(D))/(2*A); if(X>0) Time=X; if(Y>0&&(Time<0||Y<Time)) Time=Y;}}
  if(Time<0) continue; auto Lead=P+V*Time;
  if(Lead.Size()>Speed*2.5||FMath::Abs(FMath::UnwindRadians(float(FMath::Atan2(Lead.Y,Lead.X))-Ship->Movement->Motion.Heading-(Port ? PI/2 : -PI/2)))>Ship->Definition.Arc) continue;
  return {true,float(Lead.Size()),Lead.GetSafeNormal()};
 }
 return {};
}
}
bool UVTSimulation::BehaviorHostile(AVTShip* Mine,AVTShip* Other) const {
 if(Mine==Other||Other->Docked||Mine->Faction==Other->Faction) return false;
 if(Mine->Faction==Freebooters||Other->Faction==Freebooters) return true;
 if(Mine->Brain.LastAttacker==Other->PersistentId&&SimulationTime-Mine->Brain.AttackTime<Data->World.recent_attack_memory) return true;
 if(!Other->IsNPC) {
  auto* PS=Other->GetPlayerState<AVTPlayerState>();const auto Standing=Standings.Read(PS?PS->Profile:FGuid(),Mine->Faction);
  return Standing.Found&&(Standing.Reputation<Data->World.hostile_threshold||Standing.Heat>=Data->World.heat_engage_threshold);
 }
 if(Mine->ShipRole==0&&Mine->Faction==Houses) return true;
 return Data->StandingBetween(Mine->Faction,Other->Faction)<Data->World.hostile_threshold;
}
void AVTShipAI::Decide(float Dt) {DecideShip(Cast<AVTShip>(GetPawn()),Dt);}
void AVTShipAI::DecideShip(AVTShip* Ship,float Dt) {
  if(!Ship||Ship->Docked||Ship->Disabled||Ship->Anchored) return;
 auto* Sim=Ship->GetWorld()->GetSubsystem<UVTSimulation>(); const auto& T=Sim->Data->AI; const auto& P=T.pilot; const auto& D=Ship->Definition; auto E=D.Equipment;
 // The captain sees only their stations; crew operates the full fit separately.
 for(EVTDevice Device:Ship->Fit.CrewedDevices) {
  if(Device==EVTDevice::EMP)E.EMP=false;
  if(Device==EVTDevice::Torpedo)E.Torpedoes=false;
  if(Device==EVTDevice::PointDefense)E.PointDefense=false;
  if(Device==EVTDevice::Mine)E.Mines=false;
 }
 auto& R=Ship->Combat->EquipmentState; auto& Brain=Ship->Brain; const auto& M=Ship->Movement->Motion;
 Ship->Intent=FVTPilotIntent(); TArray<AVTShip*,TInlineAllocator<64>> Targets,Prizes;
 const bool Utility=(D.AIAbilities||Ship->Autopilot)&&Ship->ShipRole!=1;
 struct Contact {AVTShip* Ship; double DistanceSquared;};
 TArray<Contact,TInlineAllocator<64>> OrderedTargets;
 AVTShip* NearestTarget=nullptr; double NearestDistance=DBL_MAX; bool NearestTied=false;
 float Threat=0,Danger=0;
 for(auto* Other:Sim->Queries.Ordered(Ship->SystemIndex)) if(IsValid(Other)&&Other!=Ship&&!Other->Docked) {
  if(Other->Disabled) {if(!Other->Invulnerable&&Other->Faction!=Ship->Faction) Prizes.Add(Other); continue;}
  bool Hostile=Ship->ShipRole==1 ? Ship->Faction!=Other->Faction : Sim->BehaviorHostile(Ship,Other);
  // Ordinary skirmish enemies fight player ships without the campaign reputation gate.
  if(Ship->ShipRole==0&&Ship->Faction==Houses&&!Other->IsNPC) Hostile=true;
  if(Hostile&&!(Ship->ShipRole==2&&!Other->IsNPC&&(Brain.ScanTarget!=Other->PersistentId||Brain.ScanProgress<1))) {
   double Squared=(Other->Movement->Motion.Position-M.Position).SizeSquared();
   if(Squared<NearestDistance) {NearestTarget=Other;NearestDistance=Squared;NearestTied=false;}
   else if(Squared==NearestDistance) NearestTied=true;
   if(Utility) {OrderedTargets.Add({Other,Squared});float Distance=float(FMath::Sqrt(Squared));Threat+=FMath::Clamp(1-Distance/T.surround_radius,0.f,1.f);Danger+=FMath::Clamp(1-Distance/(D.MuzzleSpeed*2.5f),0.f,1.f);}
   else Targets.Add(Other);
  }
 }
 auto Near=[&](const AVTShip& A,const AVTShip& B){return (A.Movement->Motion.Position-M.Position).SizeSquared()<(B.Movement->Motion.Position-M.Position).SizeSquared();};
 if(Utility) {OrderedTargets.Sort([](const Contact& A,const Contact& B){return A.DistanceSquared<B.DistanceSquared;});for(const Contact& Contact:OrderedTargets)Targets.Add(Contact.Ship);}
 else if(NearestTied) {Targets.Sort(Near);NearestTarget=Targets[0];}
 Prizes.Sort(Near);
 auto* Target=Utility ? (Targets.IsEmpty() ? nullptr : Targets[0]) : NearestTarget; auto* Prize=Prizes.IsEmpty() ? nullptr : Prizes[0];
 if(Ship->ShipRole==1) {if(Target&&(Target->Movement->Motion.Position-M.Position).SizeSquared()<=400*400) Face(Ship,M.Position-Target->Movement->Motion.Position,1,T); return;}
 float Hull=Ship->Attributes->Hull.GetCurrentValue()/D.Hull;
 if(Ship->ShipRole==2) {
  AVTShip* Contact=nullptr; double Best=DBL_MAX;
  for(auto* Other:Sim->Queries.Ordered(Ship->SystemIndex)) if(IsValid(Other)&&!Other->IsNPC&&!Other->Docked&&!Other->Disabled) {double Distance=(Other->Movement->Motion.Position-M.Position).SizeSquared(); if(Distance<Best) {Contact=Other; Best=Distance;}}
  if(Contact) {
   if(Brain.ScanTarget!=Contact->PersistentId) {Brain.ScanTarget=Contact->PersistentId; Brain.ScanProgress=0;}
   bool InRange=Best<=T.surround_radius*T.surround_radius&&!Sim->Occluded(Ship->SystemIndex,M.Position,Contact->Movement->Motion.Position);
   Brain.ScanProgress=FMath::Clamp(Brain.ScanProgress+(InRange ? Sim->Data->World.scan_rate : -Sim->Data->World.scan_decay_rate)*Dt,0.f,1.f);
   bool Hostile=Sim->BehaviorHostile(Ship,Contact);
   if(Hostile&&Brain.ScanProgress>=1) {Target=Contact; if(InRange) for(auto* Patrol:Sim->Queries.Ordered(Ship->SystemIndex)) if(Patrol->ShipRole==2&&Patrol->Faction==Ship->Faction) {Patrol->Brain.Alert=Contact->Movement->Motion.Position; Patrol->Brain.AlertTTL=Sim->Data->World.alert_ttl;}}
   else if(!Target) {if(Brain.ScanProgress<1&&InRange) Face(Ship,Contact->Movement->Motion.Position-M.Position,Best>FMath::Square(T.surround_radius*0.7f) ? 0.4f : 0.f,T); else if(Brain.AlertTTL>0) Face(Ship,Brain.Alert-M.Position,1,T); return;}
  } else if(!Target) {if(Brain.AlertTTL>0) Face(Ship,Brain.Alert-M.Position,1,T); return;}
 }
 if(!D.AIAbilities&&!Ship->Autopilot) {if(Target) {Beam(Ship,Target,T,Hull<D.AIFleeFraction); float Angle=FMath::UnwindRadians(float(FMath::Atan2(Target->Movement->Motion.Position.Y-M.Position.Y,Target->Movement->Motion.Position.X-M.Position.X))-M.Heading); if(FMath::Abs(Angle-PI/2)<=D.AIFireArc) Ship->Intent.Buttons|=VTButtons::Port; if(FMath::Abs(Angle+PI/2)<=D.AIFireArc) Ship->Intent.Buttons|=VTButtons::Starboard; Ship->Intent.Aim=D.AIAim ? (Target->Movement->Motion.Position-M.Position).GetSafeNormal() : FVector2D::ZeroVector;} return;}
 float Shields=0,MaxShields=0; for(int I=0;I<4;++I) {Shields+=Ship->Combat->Shields[I]; MaxShields+=D.ShieldMax[I];}
 float Integrity=(Ship->Attributes->Hull.GetCurrentValue()+P.shield_worth*Shields)/(D.Hull+P.shield_worth*MaxShields);
 float Stock=float(R.TorpedoMagazine)/FMath::Max(1,E.TorpedoMagazine), Battery=Ship->Attributes->Battery.GetCurrentValue()/FMath::Max(0.001f,D.BatteryMax);
 float Bow=D.ShieldMax.X>0 ? Ship->Combat->Shields.X/D.ShieldMax.X : 0, Stern=D.ShieldMax.Y>0 ? Ship->Combat->Shields.Y/D.ShieldMax.Y : 0;
 auto Presentation=[&](bool ShowingBow) {float Mine=ShowingBow ? Bow : Stern,Theirs=ShowingBow ? Stern : Bow; bool MH=(ShowingBow ? Ship->Combat->Suppression.X : Ship->Combat->Suppression.Y)>0,TH=(ShowingBow ? Ship->Combat->Suppression.Y : Ship->Combat->Suppression.X)>0; float Edge=((Mine-Theirs)-((MH&&Mine<=0 ? 0.5f : 0)-(TH&&Theirs<=0 ? 0.5f : 0)))*P.shield_bias; return FMath::Max(P.presentation_floor,1+Edge*FMath::Clamp(Danger,0.f,1.f));};
 float Scores[7]={}; float Distance=Target ? float((Target->Movement->Motion.Position-M.Position).Size()) : 0; bool Armed=((Ship->PortReload<=0||Ship->StarboardReload<=0)&&Target&&Distance<=D.MuzzleSpeed*2.5f)||(E.Torpedoes&&R.Loaded>=1);
 if(Target) {
  Scores[0]=P.w_broadside*(Ship->PortReload<=0||Ship->StarboardReload<=0 ? 1 : P.reloading_interest)*(0.3f+0.7f*Ramp(Distance,D.AIEngageRange,D.AIEngageRange*4));
  if(E.Torpedoes&&R.Loaded>=1&&Distance<=E.TorpedoRange) Scores[1]=P.w_torpedo*(0.5f+0.5f*FMath::Clamp(Distance/E.TorpedoRange,0.f,1.f))*(P.scarcity_floor+(1-P.scarcity_floor)*Stock);
  float Alignment=FMath::Max(0.f,float(FVector2D::DotProduct(FVector2D(FMath::Cos(M.Heading),FMath::Sin(M.Heading)),(Target->Movement->Motion.Position-M.Position).GetSafeNormal())));
  if(E.Boost) Scores[3]=P.w_ram*Ramp(Distance,D.AIEngageRange*0.5f,D.AIEngageRange*1.5f)*(0.3f+0.7f*Alignment)*Battery*Ramp(Integrity,1,P.ram_hull_floor)*(Armed ? 0.3f : 1.f)*Presentation(true);
  if(E.EMP&&Distance<=E.EMPRange) Scores[5]=P.w_emp*FMath::Clamp(1-Target->Attributes->EMPStress.GetCurrentValue()/Target->Definition.EMPResist,0.f,1.f)*Ramp(Distance,E.EMPRange*0.5f,E.EMPRange)*(Armed ? P.emp_busy_interest : 1)*Presentation(true);
  if(Integrity<D.AIFleeFraction) Scores[6]=P.w_disengage*(0.6f+0.4f*Ramp(Integrity,0,D.AIFleeFraction))*FMath::Clamp(Danger,0.f,1.f)*Presentation(false);
 }
 if(Prize) Scores[2]=P.w_board*FMath::Min(1.25f,P.board_base+P.board_repair_pull*(1-Hull)+P.board_resupply_pull*(1-Stock))*(0.3f+0.7f*Ramp(float((Prize->Movement->Motion.Position-M.Position).Size()),Sim->Data->Rules.BoardRange,E.TorpedoRange))*Ramp(Threat,0,P.board_safe_threat);
 if(E.Warp&&R.WarpCooldown<=0) {float Escape=FMath::Clamp(Threat/FMath::Max(0.01f,P.warp_escape_threat),0.f,1.5f)*(0.6f+0.4f*(1-Integrity)); float Objective=Target ? Distance : Prize ? float((Prize->Movement->Motion.Position-M.Position).Size()) : 0; float Reposition=Ramp(Objective,E.WarpRange*1.5f,D.AIEngageRange*2)*(1-FMath::Clamp(Threat,0.f,1.f))*P.warp_reposition_interest; Scores[4]=P.w_microwarp*FMath::Max(Escape,Reposition);}
 int Action=-1; float BestScore=0; for(int I=0;I<7;++I) {float Score=Scores[I]+(Scores[I]>0&&Brain.Action==I ? P.commit_bonus : 0); if(Score>BestScore) {Action=I; BestScore=Score;}}
 Brain.Action=Action; auto Aim=Target ? Target->Movement->Motion.Position : Prize ? Prize->Movement->Motion.Position : M.Position+FVector2D(FMath::Cos(M.Heading),FMath::Sin(M.Heading))*D.AIEngageRange;
 bool Boost=false,Brace=false,Board=false,Emp=false,Screen=false,Torpedo=false,Warp=false;
 if(Action==0&&Target) Beam(Ship,Target,T,false);
 if(Action==1&&Target) {Face(Ship,Aim-M.Position,Ramp(Distance,E.TorpedoRange*P.torpedo_standoff*1.6f,E.TorpedoRange*P.torpedo_standoff*0.7f),T); Torpedo=R.Locks.Num()<FMath::Max(1,FMath::Min(int(T.torpedo_min_volley),FMath::FloorToInt(R.Loaded)));}
 if(Action==2&&Prize) {Aim=Prize->Movement->Motion.Position; float Dist=float((Aim-M.Position).Size()); Face(Ship,Aim-M.Position,Dist<=Sim->Data->Rules.BoardRange ? 0 : FMath::Max(0.15f,Ramp(Dist,Sim->Data->Rules.BoardRange*4,Sim->Data->Rules.BoardRange)),T); Board=Dist<=Sim->Data->Rules.BoardRange; Brace=Board;}
 if(Action==3&&Target) {Face(Ship,Aim-M.Position,1,T); Boost=true; Brace=true;}
 if(Action==4) {FVector2D Center=FVector2D::ZeroVector; int Count=0; for(auto* Other:Targets) if((Other->Movement->Motion.Position-M.Position).Size()<E.WarpRange*0.5f) {Center+=Other->Movement->Motion.Position; ++Count;}
  if(Count&&(!(Target&&Distance>D.AIEngageRange*2)||Hull<D.AIFleeFraction)) Aim=M.Position+(M.Position-Center/Count).GetSafeNormal()*E.WarpRange;
  else Aim=M.Position+(Aim-M.Position).GetClampedToMaxSize(E.WarpRange+D.AIEngageRange)-(Aim-M.Position).GetSafeNormal()*FMath::Min(D.AIEngageRange,float((Aim-M.Position).Size()));
  Face(Ship,Aim-M.Position,1,T); Brain.WarpPrime+=Dt; Warp=Brain.WarpPrime<T.warp_prime;}
 if(Action==5&&Target) {Face(Ship,Aim-M.Position,P.emp_throttle,T); Emp=true;}
 if(Action==6&&Target) {Aim=M.Position+(M.Position-Target->Movement->Motion.Position).GetSafeNormal()*E.WarpRange; Face(Ship,Aim-M.Position,1,T); Boost=true;}
 Solution Port=Gun(Ship,Targets,true),Star=Gun(Ship,Targets,false);
 for(auto* Other:Targets) {auto Offset=Other->Movement->Motion.Position-M.Position; float Rel=FMath::UnwindRadians(float(FMath::Atan2(Offset.Y,Offset.X))-M.Heading); if(E.EMP&&Offset.Size()<=E.EMPRange&&FMath::Abs(Rel)<=E.EMPArc*0.5f&&Other->Attributes->EMPStress.GetCurrentValue()<Other->Definition.EMPResist) Emp=true;
  if(E.Mines&&R.MineMagazine>0&&R.MineCooldown<=0&&Offset.Size()<=E.MineRadius*FMath::Max(1.f,P.mine_lead)&&FMath::Abs(Rel)>PI/2&&(float(R.MineMagazine)/E.MineMagazine>P.mine_reserve||Integrity<1)) Ship->Intent.Buttons|=VTButtons::Mine;}
 if(!Torpedo&&Action!=4&&E.Torpedoes&&R.Loaded>=1) for(auto* Other:Targets) if((Other->Movement->Motion.Position-M.Position).Size()<=E.TorpedoRange&&(Other->Movement->Motion.Position-Aim).Size()<=E.LockRadius) {Torpedo=R.Locks.Num()<FMath::Max(1,FMath::Min(int(T.torpedo_min_volley),FMath::FloorToInt(R.Loaded))); break;}
 if(E.PointDefense) for(AVTProjectile* Shot:Sim->Projectiles) if(IsValid(Shot)&&Shot->SystemIndex==Ship->SystemIndex&&Shot->SourceFaction!=Ship->Faction&&Shot->Kind!=EVTProjectileKind::Mine&&(Shot->Position-M.Position).Size()<=E.PDRadius) {Screen=FMath::Clamp(Danger/FMath::Max(0.01f,P.screen_danger),0.f,1.f)*(P.screen_base+(1-P.screen_base)*(1-Integrity))>=P.screen_raise; break;}
 Board|=Prize&&(Prize->Movement->Motion.Position-M.Position).Size()<=Sim->Data->Rules.BoardRange;
 Brace|=!Port.Valid&&!Star.Valid&&Target&&Distance<=D.MuzzleSpeed*2.5f*P.brace_range_frac;
 int Wanted=Warp ? 3 : Torpedo&&(Action==1||R.Locks.Num()>0) ? 2 : Port.Valid&&(!Star.Valid||Port.Distance<=Star.Distance) ? 0 : Star.Valid ? 1 : Torpedo ? 2 : -1;
 bool Live=Brain.Shoulder==0 ? Port.Valid : Brain.Shoulder==1 ? Star.Valid : Brain.Shoulder==2 ? Torpedo : Brain.Shoulder==3 ? Warp : false;
 Brain.AimLock=FMath::Max(0.f,Brain.AimLock-Dt);
 if(!(Brain.AimLock>0&&Wanted!=Brain.Shoulder&&Live)) {if(Wanted!=Brain.Shoulder) Brain.AimLock=Wanted==2||Brain.Shoulder==2 ? P.torpedo_aim_lock : P.aim_lock; Brain.Shoulder=Wanted;}
 if(Brain.Shoulder==0) {Ship->Intent.Buttons|=VTButtons::Port; Ship->Intent.Aim=Port.Direction;}
 if(Brain.Shoulder==1) {Ship->Intent.Buttons|=VTButtons::Starboard; Ship->Intent.Aim=Star.Direction;}
 if(Brain.Shoulder==2) Ship->Intent.Buttons|=VTButtons::Torpedo;
 if(Brain.Shoulder==3) Ship->Intent.Buttons|=VTButtons::Warp; else Brain.WarpPrime=0;
 int Thumb=Boost ? 0 : Board&&Action==2 ? 4 : Emp ? 1 : Screen ? 2 : Brace ? 3 : Board ? 4 : -1;
 Brain.ThumbTravel=FMath::Max(0.f,Brain.ThumbTravel-Dt); if(Thumb>=0&&Thumb!=Brain.Thumb) Brain.ThumbTravel=P.thumb_travel; Brain.Thumb=Thumb;
 if(Brain.ThumbTravel<=0&&Thumb>=0) {const uint16 Bits[]={VTButtons::Boost,VTButtons::EMP,VTButtons::PointDefense,VTButtons::Brace,VTButtons::Interact}; Ship->Intent.Buttons|=Bits[Thumb];}
 Ship->Intent.CursorOffset=(Aim-M.Position).GetClampedToMaxSize(1300); if(Brain.Shoulder<0||Brain.Shoulder>=2) Ship->Intent.Aim=Ship->Intent.CursorOffset.GetSafeNormal();
 CrewStep(Ship);
}
void AVTShipAI::CrewStep(AVTShip* Ship) {
 if(Ship->Fit.CrewedDevices.IsEmpty()||Ship->Disabled||Ship->Docked||Ship->ShipRole==1) return;
 auto* Sim=Ship->GetWorld()->GetSubsystem<UVTSimulation>(); TArray<AVTShip*> Targets;
 for(auto* Other:Sim->Queries.Ordered(Ship->SystemIndex)) if(IsValid(Other)&&!Other->Disabled&&Other!=Ship&&!Other->Docked&&(Ship->IsNPC ? Sim->BehaviorHostile(Ship,Other) : Other->Faction!=Ship->Faction||(Ship->Brain.LastAttacker==Other->PersistentId&&Sim->SimulationTime-Ship->Brain.AttackTime<Sim->Data->World.recent_attack_memory))) Targets.Add(Other);
 for(auto Device:Ship->Fit.CrewedDevices) {
  if(Device==EVTDevice::Port||Device==EVTDevice::Starboard) {auto Solution=Gun(Ship,Targets,Device==EVTDevice::Port); if(Solution.Valid) Ship->Intent.Buttons|=Device==EVTDevice::Port ? VTButtons::Port : VTButtons::Starboard;}
  if(Device==EVTDevice::EMP) for(auto* Target:Targets) {auto Offset=Target->Movement->Motion.Position-Ship->Movement->Motion.Position; if(Offset.Size()<=Ship->Definition.Equipment.EMPRange&&FMath::Abs(FMath::UnwindRadians(float(FMath::Atan2(Offset.Y,Offset.X))-Ship->Movement->Motion.Heading))<=Ship->Definition.Equipment.EMPArc*0.5f) {Ship->Intent.Buttons|=VTButtons::EMP; break;}}
  if(Device==EVTDevice::Torpedo&&!Targets.IsEmpty()&&(Targets[0]->Movement->Motion.Position-Ship->Movement->Motion.Position-Ship->Intent.CursorOffset).Size()<=Ship->Definition.Equipment.LockRadius&&Ship->Combat->EquipmentState.Locks.Num()<3) Ship->Intent.Buttons|=VTButtons::Torpedo;
  if(Device==EVTDevice::PointDefense) for(AVTProjectile* Shot:Sim->Projectiles) if(IsValid(Shot)&&Shot->SystemIndex==Ship->SystemIndex&&Shot->SourceFaction!=Ship->Faction&&(Shot->Position-Ship->Movement->Motion.Position).Size()<=Ship->Definition.Equipment.PDRadius) {Ship->Intent.Buttons|=VTButtons::PointDefense; break;}
  if(Device==EVTDevice::Mine) for(auto* Target:Targets) {auto Offset=Target->Movement->Motion.Position-Ship->Movement->Motion.Position; if(Offset.Size()<=Ship->Definition.Equipment.MineRadius*Sim->Data->AI.pilot.mine_lead&&FVector2D::DotProduct(Offset,FVector2D(FMath::Cos(Ship->Movement->Motion.Heading),FMath::Sin(Ship->Movement->Motion.Heading)))<0) {Ship->Intent.Buttons|=VTButtons::Mine; break;}}
 }
}
