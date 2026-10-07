#include "VTGameplay.h"
#include "VTCombat.h"
void UVTSimulation::ContactStep() {
 const auto& R=Data->Rules;
 for(auto& Bucket:SystemShips) for(int I=0;I<Bucket.Num();++I) for(int J=I+1;J<Bucket.Num();++J) {
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
  A->Combat->Damage(Damage,AM.Position+Normal*A->Definition.Radius,B);
  B->Combat->Damage(Damage,BM.Position-Normal*B->Definition.Radius,A);
 }
}
void UVTSimulation::PiracyStep() {
 auto Current=Ships;
 for(AVTShip* S:Current) if(IsValid(S)&&!S->IsNPC&&!S->Docked&&!S->Disabled) {
  auto* Combat=S->Combat.Get();
  AVTShip* Target=nullptr; double Best=Data->Rules.BoardRange*Data->Rules.BoardRange;
  for(AVTShip* Other:SystemShips[S->SystemIndex]) if(IsValid(Other)&&Other->PersistentId==Combat->BoardingTarget&&Other->Disabled&&!Other->Invulnerable&&!Other->Combat->Claimed&&(Other->Movement->Motion.Position-S->Movement->Motion.Position).SizeSquared()<=Best) {Target=Other; break;}
  if(!Target) for(AVTShip* Other:SystemShips[S->SystemIndex]) if(IsValid(Other)&&Other!=S&&Other->Disabled&&!Other->Invulnerable&&!Other->Combat->Claimed) {
   double Distance=(Other->Movement->Motion.Position-S->Movement->Motion.Position).SizeSquared();
   if(Distance<=Best) {Best=Distance; Target=Other;}
  }
  FGuid Id=Target ? Target->PersistentId : FGuid();
  if(Combat->BoardingTarget!=Id) {Combat->BoardingTarget=Id; Combat->BoardingProgress=0;}
  if(!Target||!(S->Intent.Buttons&VTButtons::Interact)) continue;
  Combat->BoardingProgress+=VT::Step;
  if(Combat->BoardingProgress<Data->Rules.BoardDwell) continue;
  // Authority serializes claims: the next captain can never receive the same prize.
  Target->Combat->Claimed=true;
  if(auto* PS=S->GetPlayerState<AVTPlayerState>()) ++PS->Boarded;
  S->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),FMath::Min(S->Definition.Hull,S->Attributes->Hull.GetCurrentValue()+S->Definition.Hull*Data->Rules.BoardRepairFraction));
  if(Target->IsNPC) {if(Target->Controller) Target->Controller->Destroy(); Target->Destroy();}
  else if(auto* Mode=GetWorld()->GetAuthGameMode<AVTGameMode>()) Mode->RecoverShip(Target);
  Combat->BoardingTarget.Invalidate(); Combat->BoardingProgress=0;
 }
}
