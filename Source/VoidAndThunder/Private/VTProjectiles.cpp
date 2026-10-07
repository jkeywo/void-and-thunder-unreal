#include "VTCombat.h"
#include "VTGameplay.h"

void UVTSimulation::RebuildShipCells() {
 ShipCells.SetNum(SystemShips.Num()); LargestShipRadius=0;
 for(auto& Cells:ShipCells) for(auto& Cell:Cells) Cell.Value.Reset();
 for(AVTShip* Ship:Ships) if(IsValid(Ship)&&!Ship->Docked&&ShipCells.IsValidIndex(Ship->SystemIndex)) {
  auto P=Ship->Movement->Motion.Position;
  ShipCells[Ship->SystemIndex].FindOrAdd(FIntPoint(FMath::FloorToInt(P.X/128),FMath::FloorToInt(P.Y/128))).Add(Ship);
  LargestShipRadius=FMath::Max(LargestShipRadius,Ship->Definition.Radius);
 }
}
void UVTSimulation::QueryShips(int32 System,const FVector2D& Min,const FVector2D& Max,TArray<AVTShip*>& Result) const {
 Result.Reset(); if(!ShipCells.IsValidIndex(System)) return;
 FIntPoint A(FMath::FloorToInt(Min.X/128),FMath::FloorToInt(Min.Y/128)),B(FMath::FloorToInt(Max.X/128),FMath::FloorToInt(Max.Y/128));
 for(int Y=A.Y;Y<=B.Y;++Y) for(int X=A.X;X<=B.X;++X) if(const auto* Cell=ShipCells[System].Find(FIntPoint(X,Y))) Result.Append(*Cell);
}
void UVTSimulation::ProjectileStep() {
 RebuildShipCells();
 TArray<AVTShip*> Candidates; Candidates.Reserve(32);
 auto Shots=Projectiles;
 // Integrate planar shots before the point-defence sweep.
 for(AVTProjectile* Shot:Shots) if(IsValid(Shot)) {
  Shot->Previous=Shot->Position; Shot->Remaining-=VT::Step; Shot->ReportCountdown=FMath::Max(0.f,Shot->ReportCountdown-VT::Step);
  if(Shot->Kind!=EVTProjectileKind::Torpedo&&Shot->Kind!=EVTProjectileKind::Mine) Shot->Position+=Shot->Velocity*VT::Step;
 }
 for(AVTShip* S:Ships) if(IsValid(S)) S->Combat->PointDefenseStep();
 for(AVTProjectile* Shot:Shots) if(IsValid(Shot)) {
  if(!SystemShips.IsValidIndex(Shot->SystemIndex)) {Shot->Destroy(); continue;}
  if(Shot->Kind==EVTProjectileKind::Torpedo) {
   AVTShip* Target=nullptr;
   for(AVTShip* S:SystemShips[Shot->SystemIndex]) if(IsValid(S)&&S->PersistentId==Shot->TargetId&&!S->Docked) {Target=S; break;}
   if(!Target) {Shot->Destroy(); continue;}
   FVector Current(Shot->Position.X,Shot->Position.Y,Shot->Height), Goal(Target->Movement->Motion.Position.X,Target->Movement->Motion.Position.Y,0);
   FVector Direction=(Goal-Current).GetSafeNormal(), V=Shot->Velocity3D.GetSafeNormal(); float Speed=float(Shot->Velocity3D.Size());
   if(!Direction.IsNearlyZero()) {
    double Angle=FMath::Acos(FMath::Clamp(FVector::DotProduct(V,Direction),-1.,1.));
    if(Angle>1e-6) {FQuat Rotation=FQuat::FindBetweenNormals(V,Direction); V=FQuat::Slerp(FQuat::Identity,Rotation,FMath::Min(1.,Shot->TurnRate*VT::Step/Angle)).RotateVector(V);}
    Shot->Velocity3D=V*Speed;
   }
   Current+=Shot->Velocity3D*VT::Step; Shot->Position=FVector2D(Current.X,Current.Y); Shot->Height=float(Current.Z);
  }
  if(Occluded(Shot->SystemIndex,Shot->Previous,Shot->Position,Shot->Height)) {Shot->Destroy(); continue;}
  AVTShip* Hit=nullptr; double Best=DBL_MAX;
  double Margin=LargestShipRadius+Shot->Radius;
  FVector2D Min(FMath::Min(Shot->Previous.X,Shot->Position.X)-Margin,FMath::Min(Shot->Previous.Y,Shot->Position.Y)-Margin);
  FVector2D Max(FMath::Max(Shot->Previous.X,Shot->Position.X)+Margin,FMath::Max(Shot->Previous.Y,Shot->Position.Y)+Margin);
  QueryShips(Shot->SystemIndex,Min,Max,Candidates);
  for(AVTShip* S:Candidates) if(IsValid(S)&&S->PersistentId!=Shot->SourceId&&!S->Docked) {
   if(Shot->SourceNPC&&S->IsNPC&&S->Faction==Shot->SourceFaction) continue;
   float R=S->Definition.Radius+Shot->Radius;
   bool Contact=Shot->Kind==EVTProjectileKind::Torpedo ? (S->Movement->Motion.Position-Shot->Position).SizeSquared()+Shot->Height*Shot->Height<=R*R : VTCombat::SegmentDistanceSquared(Shot->Previous,Shot->Position,S->Movement->Motion.Position)<=R*R;
   if(!Contact) continue;
   if(Shot->Kind==EVTProjectileKind::Mine) {S->Combat->Damage(Shot->Damage*VT::Step,Shot->Position,Shot->Source,Shot->AttackerProfile,Shot->ReportCountdown<=0,Shot->Damage*0.25f,Shot->SourceFaction); continue;}
   double Along=(S->Movement->Motion.Position-Shot->Previous).SizeSquared(); if(Along<Best) {Hit=S; Best=Along;}
  }
  if(Shot->Kind==EVTProjectileKind::Mine&&Shot->ReportCountdown<=0) Shot->ReportCountdown=0.25f;
  if(Hit) {
   if(Shot->Kind==EVTProjectileKind::EMP) {if(!Hit->Invulnerable) Hit->Combat->ApplyDelta(UVTEMPStressEffect::StaticClass(),FMath::Min(Hit->Definition.EMPResist-Hit->Attributes->EMPStress.GetCurrentValue(),Shot->Damage*Hit->Definition.EMPResist));}
   else Hit->Combat->Damage(Shot->Damage,Shot->Previous,Shot->Source,Shot->AttackerProfile,true,-1,Shot->SourceFaction);
   Shot->Destroy();
  } else if(Shot->Remaining<=0) Shot->Destroy();
 }
}
