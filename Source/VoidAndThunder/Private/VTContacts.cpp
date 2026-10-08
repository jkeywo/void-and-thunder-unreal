#include "VTGameplay.h"
#include "VTCombat.h"
void UVTSimulation::ContactStep() {
 const auto& R=Data->Rules;
 for(int System=0;System<Queries.SystemCount();++System) if(const auto& Bucket=Queries.Ordered(System);true) for(int I=0;I<Bucket.Num();++I) for(int J=I+1;J<Bucket.Num();++J) {
  AVTShip* A=Bucket[I]; AVTShip* B=Bucket[J]; if(A->Docked||B->Docked) continue;
  auto& AM=A->Movement->Motion; auto& BM=B->Movement->Motion;
  FVector2D Delta=BM.Position-AM.Position; float Reach=A->Definition.Radius+B->Definition.Radius;
  if(Delta.SizeSquared()>=Reach*Reach) continue;
  float Distance=float(Delta.Size()); FVector2D Normal=Distance>0.001 ? Delta/Distance : FVector2D(1,0);
  float Closing=float(FVector2D::DotProduct(AM.Velocity-BM.Velocity,Normal));
  float AS=A->Anchored ? 0 : B->Anchored ? 1 : 0.5f; float BS=B->Anchored ? 0 : A->Anchored ? 1 : 0.5f;
  float Overlap=(Reach-Distance)*R.RamSeparation;
  AM.Position-=Normal*Overlap*AS; BM.Position+=Normal*Overlap*BS;
  if(Closing<=0) continue;
  float Impulse=Closing*(1+R.RamRestitution); AM.Velocity-=Normal*Impulse*AS; BM.Velocity+=Normal*Impulse*BS;
  float Damage=FMath::Max(0.f,Closing-R.RamThreshold)*R.RamDamagePerSpeed;
  if(Damage<=0 || (A->IsNPC&&B->IsNPC&&A->Faction==B->Faction)) continue;
  A->Combat->Damage(Damage*(1+(B->Combat->BoostPowered ? 2.5f*FMath::Clamp(float(FVector2D::DotProduct(FVector2D(FMath::Cos(BM.Heading),FMath::Sin(BM.Heading)),-Normal)),0.f,1.f) : 0)),AM.Position+Normal*A->Definition.Radius,B);
  B->Combat->Damage(Damage*(1+(A->Combat->BoostPowered ? 2.5f*FMath::Clamp(float(FVector2D::DotProduct(FVector2D(FMath::Cos(AM.Heading),FMath::Sin(AM.Heading)),Normal)),0.f,1.f) : 0)),BM.Position-Normal*B->Definition.Radius,A);
 }
}
void UVTSimulation::PiracyStep() {
 // Build after crippling/death. Recheck live claims/validity after each award.
 Queries.BuildBoarding();
 auto Current=Ships;
 for(AVTShip* S:Current) if(IsValid(S)&&!S->Docked&&!S->Disabled&&S->ShipRole!=1) {
  auto* Combat=S->Combat.Get();
  AVTShip* Target=nullptr; double Best=Data->Rules.BoardRange*Data->Rules.BoardRange;
  for(AVTShip* Other:Queries.Boarding(S->SystemIndex)) if(IsValid(Other)&&Other->PersistentId==Combat->BoardingTarget&&Other->Disabled&&!Other->Invulnerable&&!Other->Combat->Claimed&&(Other->Movement->Motion.Position-S->Movement->Motion.Position).SizeSquared()<=Best) {Target=Other; break;}
  if(!Target) for(AVTShip* Other:Queries.Boarding(S->SystemIndex)) if(IsValid(Other)&&Other!=S&&(!S->IsNPC||Other->Faction!=S->Faction)&&Other->Disabled&&!Other->Invulnerable&&!Other->Combat->Claimed) {
   double Distance=(Other->Movement->Motion.Position-S->Movement->Motion.Position).SizeSquared();
   if(Distance<=Best) {Best=Distance; Target=Other;}
  }
  FGuid Id=Target ? Target->PersistentId : FGuid();
  if(Combat->BoardingTarget!=Id) {Combat->BoardingTarget=Id; Combat->BoardingProgress=0;}
  if(!Target||!(S->Intent.Buttons&VTButtons::Interact)) continue;
  Combat->BoardingProgress+=VT::Step;
  if(Combat->BoardingProgress<Data->Rules.BoardDwell) continue;
  // Authority serializes claims: the next captain can never receive the same prize.
  Target->Combat->Claimed=true; S->Combat->Cue(TEXT("GameplayCue.Ship.Board"));
  if(auto* PS=S->GetPlayerState<AVTPlayerState>()) {++PS->Boarded; PS->Credits+=Data->Rules.BoardingBounty;}
  uint32 Random=uint32(WorldSeed)^GetTypeHash(Target->PersistentId); const auto& E=S->Definition.Equipment;
  auto Supply=[&](int Min,int Max){return Min+FMath::Min(Max-Min,int(VT::LcgNext(Random)*(Max-Min+1)));};
  S->Combat->EquipmentState.TorpedoMagazine=FMath::Min(E.TorpedoMagazine,S->Combat->EquipmentState.TorpedoMagazine+Supply(E.TorpedoResupplyMin,E.TorpedoResupplyMax));
  S->Combat->EquipmentState.MineMagazine=FMath::Min(E.MineMagazine,S->Combat->EquipmentState.MineMagazine+Supply(E.MineResupplyMin,E.MineResupplyMax));
  S->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),FMath::Min(S->Definition.Hull,S->Attributes->Hull.GetCurrentValue()+S->Definition.Hull*Data->Rules.BoardRepairFraction));
  if(Target->IsNPC) {if(Target->Controller) Target->Controller->Destroy(); Target->Destroy();}
  else if(auto* Mode=GetWorld()->GetAuthGameMode<AVTGameMode>()) Mode->RecoverShip(Target);
  Combat->BoardingTarget.Invalidate(); Combat->BoardingProgress=0;
 }
}

bool UVTSimulation::Occluded(int32 System,const FVector2D& A,const FVector2D& B,float Height) const {

 for(const auto& Body:Data->Landmarks) if(VTCombat::SegmentDistanceSquared(A,B,Body.Position)+Height*Height<Body.Radius*Body.Radius) return true;
 return Data->Systems.IsValidIndex(System)&&Data->Systems[System].HasStation&&VTCombat::SegmentDistanceSquared(A,B,Data->Rules.StationPosition)+Height*Height<Data->Rules.StationRadius*Data->Rules.StationRadius;
}
void UVTSimulation::LandmarkStep() {

 for(AVTShip* Ship:Ships) if(IsValid(Ship)&&!Ship->Docked&&!Ship->Anchored) {
  auto& M=Ship->Movement->Motion;
  auto Resolve=[&](FVector2D Position,float Radius) {auto Delta=M.Position-Position; float Distance=float(Delta.Size()),Reach=Radius+Ship->Definition.Radius; if(Distance>=Reach) return; auto Normal=Distance>0.001f ? Delta/Distance : FVector2D(1,0); M.Position=Position+Normal*Reach; double Inward=FVector2D::DotProduct(M.Velocity,Normal); if(Inward<0) M.Velocity-=Normal*Inward;};
  for(const auto& Body:Data->Landmarks) Resolve(Body.Position,Body.Radius);
  if(Data->Systems[Ship->SystemIndex].HasStation) Resolve(Data->Rules.StationPosition,Data->Rules.StationRadius);
 }
}
