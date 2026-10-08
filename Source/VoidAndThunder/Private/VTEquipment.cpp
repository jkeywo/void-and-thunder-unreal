#include "VTProjectileSpawn.h"
#include "VTCombat.h"
#include "VTGameplay.h"
#include "GameplayEffect.h"

AVTShip* UVTCombatComponent::FindTarget(FGuid Id) const {
 auto* S=CastChecked<AVTShip>(GetOwner()); auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();
 auto* Target=Sim->Queries.Find(S->SystemIndex,Id);return Target&&!Target->Docked?Target:nullptr;
}
bool UVTCombatComponent::Hostile(const AVTShip* Other) const {
 auto* S=CastChecked<AVTShip>(GetOwner());
 return Other&&Other!=S&&!Other->Docked&&(!S->IsNPC||!Other->IsNPC||S->Faction!=Other->Faction);
}
AVTProjectile* UVTCombatComponent::SpawnDeviceProjectile(EVTProjectileKind Kind,const FVector2D& P,const FVector2D& V,float Damage,float TTL,float Radius) {
 return VTProjectileSpawn::Create(GetWorld(),FVTProjectileSpawnSpec::Fired(CastChecked<AVTShip>(GetOwner()),Kind,P,V,Damage,TTL,Radius));
}
void UVTCombatComponent::EquipmentSystems() {
 auto* S=CastChecked<AVTShip>(GetOwner()); const auto& D=S->Definition; const auto& E=D.Equipment; auto& R=EquipmentState;
 if(S->Attributes->EMPStress.GetCurrentValue()>0) ApplyDelta(UVTEMPStressEffect::StaticClass(),-FMath::Min(S->Attributes->EMPStress.GetCurrentValue(),D.EMPRecovery*VT::Step));
 static const FGameplayTag DockedTag=FGameplayTag::RequestGameplayTag(TEXT("State.Ship.Docked"));
 static const FGameplayTag DisabledTag=FGameplayTag::RequestGameplayTag(TEXT("State.Ship.Disabled"));
 if(TaggedDocked!=S->Docked) {TaggedDocked=S->Docked;S->Abilities->SetLooseGameplayTagCount(DockedTag,S->Docked ? 1 : 0,EGameplayTagReplicationState::TagAndCountToAll);}
 if(TaggedDisabled!=S->Disabled) {TaggedDisabled=S->Disabled;S->Abilities->SetLooseGameplayTagCount(DisabledTag,S->Disabled ? 1 : 0,EGameplayTagReplicationState::TagAndCountToAll);}
 bool Drawn=false; float Charge=S->Attributes->Battery.GetCurrentValue(); bool Operable=!S->Disabled&&!S->Docked;
 auto Draw=[&](bool Requested,float Rate) {if(!Operable||!Requested||Charge<=0) return false; Charge=FMath::Max(0.f,Charge-Rate*VT::Step); Drawn=true; return true;};
 BoostPowered=Draw(E.Boost&&(S->Intent.Buttons&VTButtons::Boost),E.BoostDrain);
 EMPPowered=Draw(E.EMP&&(S->Intent.Buttons&VTButtons::EMP),E.EMPDrain);
 PDPowered=Draw(E.PointDefense&&(S->Intent.Buttons&VTButtons::PointDefense),E.PDDrain);
 if(!Drawn) Charge=FMath::Min(D.BatteryMax,Charge+D.BatteryRecharge*VT::Step);
 ApplyDelta(UVTBatteryEffect::StaticClass(),Charge-S->Attributes->Battery.GetCurrentValue());
 if(BoostPowered&&!LastBoost) Cue(TEXT("GameplayCue.Ship.Boost")); LastBoost=BoostPowered;
 bool Brace=(S->Intent.Buttons&VTButtons::Brace)!=0; if(Brace&&!LastBrace) Cue(TEXT("GameplayCue.Ship.Brace")); LastBrace=Brace;
 bool Warning=S->Attributes->Hull.GetCurrentValue()<=D.Hull*0.35f; if(Warning&&!HullWarning) Cue(TEXT("GameplayCue.Ship.HullWarning")); HullWarning=Warning;
 SpeedScale=FMath::Clamp(1-S->Attributes->EMPStress.GetCurrentValue()/FMath::Max(0.001f,D.EMPResist),0.f,1.f)*(BoostPowered ? E.BoostMultiplier : 1);
 if(E.Torpedoes&&R.Loaded<E.Tubes&&R.TorpedoMagazine>0) {
  float Old=R.Loaded; R.Loaded=FMath::Min(float(E.Tubes),FMath::Min(FMath::FloorToFloat(Old)+R.TorpedoMagazine,Old+VT::Step/FMath::Max(0.001f,E.TubeReload)));
  R.TorpedoMagazine-=FMath::FloorToInt(R.Loaded)-FMath::FloorToInt(Old);
 }
 for(auto& Spec:S->Abilities->GetActivatableAbilities()) if(auto* A=Cast<UVTDeviceAbility>(Spec.GetPrimaryInstance())) if(A->IsActive()) A->FixedStep();
 bool Held=Operable&&(S->Intent.Buttons&VTButtons::Warp);
 if(E.Warp&&WarpHeld&&!Held&&Operable) S->Abilities->TryActivateAbilityByClass(UVTWarpAbility::StaticClass());
 WarpHeld=Held;
}
void UVTCombatComponent::EquipmentWeapons() {
 auto* S=CastChecked<AVTShip>(GetOwner()); const auto& E=S->Definition.Equipment; auto& R=EquipmentState;
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();
 if(S->Docked||S->Disabled) {R.Locks.Reset(); TorpedoHeld=false; return;}
 FVector2D Forward(FMath::Cos(S->Movement->Motion.Heading),FMath::Sin(S->Movement->Motion.Heading));
 if(E.EMP) {
  AVTShip* Target=nullptr; double Best=DBL_MAX;
  for(AVTShip* Other:Sim->Queries.Ordered(S->SystemIndex)) if(Hostile(Other)) {
   auto Offset=Other->Movement->Motion.Position-S->Movement->Motion.Position;
   if(Offset.SizeSquared()>E.EMPRange*E.EMPRange) continue;
   float Angle=FMath::UnwindRadians(FMath::Atan2(Offset.Y,Offset.X)-S->Movement->Motion.Heading);
   if(FMath::Abs(Angle)>E.EMPArc*0.5f) continue;
   double Distance=(Offset-S->Intent.CursorOffset).SizeSquared(); if(Distance<Best) {Target=Other; Best=Distance;}
  }
  float Desired=0;
  if(Target) {auto Offset=Target->Movement->Motion.Position-S->Movement->Motion.Position; auto Lead=Offset+Target->Movement->Motion.Velocity*(Offset.Size()/E.EMPSpeed); Desired=FMath::Clamp(FMath::UnwindRadians(float(FMath::Atan2(Lead.Y,Lead.X))-S->Movement->Motion.Heading),-E.EMPArc*0.5f,E.EMPArc*0.5f);}
  R.EMPAim+=FMath::Clamp(FMath::UnwindRadians(Desired-R.EMPAim),-E.EMPSwivel*VT::Step,E.EMPSwivel*VT::Step);
  if(EMPPowered&&R.EMPCooldown<=0) S->Abilities->TryActivateAbilityByClass(UVTEMPAbility::StaticClass());
 }
 if(E.Mines&&(S->Intent.Buttons&VTButtons::Mine)&&R.MineMagazine>0) S->Abilities->TryActivateAbilityByClass(UVTMineAbility::StaticClass());
 if(!E.Torpedoes) return;
 // Committed salvo launches from the current pose. A new hold does not reset its cadence.
 R.LaunchTimer=FMath::Max(0.f,R.LaunchTimer-VT::Step);
 if(!R.LaunchQueue.IsEmpty()&&R.LaunchTimer<=0) {
  FGuid Id=R.LaunchQueue[0]; R.LaunchQueue.RemoveAt(0); R.LaunchTimer=E.LockInterval;
  if(FindTarget(Id)) {auto Spec=FVTProjectileSpawnSpec::Fired(S,EVTProjectileKind::Torpedo,S->Movement->Motion.Position,FVector2D::ZeroVector,E.TorpedoDamage,8,12);Spec.TargetId=Id;Spec.Velocity3D=FVector(0,0,R.LaunchFlip ? -E.TorpedoSpeed : E.TorpedoSpeed);Spec.TurnRate=E.TorpedoTurn;VTProjectileSpawn::Create(GetWorld(),Spec);R.LaunchFlip=!R.LaunchFlip;}
 }
 bool Held=(S->Intent.Buttons&VTButtons::Torpedo)!=0;
 R.Locks.RemoveAll([&](FGuid Id){return !FindTarget(Id);});
 if(Held) {
  R.LockElapsed-=VT::Step;
  int32 Cap=FMath::Min(FMath::Min(E.Tubes,FMath::FloorToInt(R.Loaded)),6-R.LaunchQueue.Num());
  if(R.LockElapsed<=0&&R.Locks.Num()<Cap) {
   TArray<AVTShip*> InRange,Near;
   for(AVTShip* Other:Sim->Queries.Ordered(S->SystemIndex)) if(Hostile(Other)) {
    auto Offset=Other->Movement->Motion.Position-S->Movement->Motion.Position;
    if(Offset.SizeSquared()<=E.TorpedoRange*E.TorpedoRange) {InRange.Add(Other); if((Offset-S->Intent.CursorOffset).SizeSquared()<=E.LockRadius*E.LockRadius) Near.Add(Other);}
   }
   bool More=InRange.ContainsByPredicate([&](AVTShip* Other){return !R.Locks.Contains(Other->PersistentId);});
   Near.Sort([&](const AVTShip& A,const AVTShip& B){auto Cursor=S->Movement->Motion.Position+S->Intent.CursorOffset; return (A.Movement->Motion.Position-Cursor).SizeSquared()<(B.Movement->Motion.Position-Cursor).SizeSquared();});
   for(auto* Other:Near) if(!More||!R.Locks.Contains(Other->PersistentId)) {R.Locks.Add(Other->PersistentId); R.LockElapsed=E.LockInterval; break;}
  }
 } else if(TorpedoHeld) {
  bool Idle=R.LaunchQueue.IsEmpty(); int32 Count=FMath::Min(R.Locks.Num(),6-R.LaunchQueue.Num());
  for(int I=0;I<Count;++I) R.LaunchQueue.Add(R.Locks[I]);
  R.Loaded=FMath::Max(0.f,R.Loaded-Count); R.Locks.Reset(); R.LockElapsed=0; if(Idle&&Count) R.LaunchTimer=0;
 }
 TorpedoHeld=Held;
}
void UVTCombatComponent::PointDefenseStep() {
 auto* S=CastChecked<AVTShip>(GetOwner()); if(PDPowered&&!S->Docked&&!S->Disabled) S->Abilities->TryActivateAbilityByClass(UVTPDAbility::StaticClass());
}
void UVTCombatComponent::ExecuteDevice(EVTDevice Device) {
 auto* S=CastChecked<AVTShip>(GetOwner()); const auto& E=S->Definition.Equipment; auto& R=EquipmentState; const auto& M=S->Movement->Motion;
 FVector2D Forward(FMath::Cos(M.Heading),FMath::Sin(M.Heading));
 FGameplayCueParameters Cue; Cue.Location=VT::ToWorld(S->Movement->Motion.Position,S->SystemIndex);
 if(Device==EVTDevice::Microwarp&&!IsRunningCommandlet()) S->Abilities->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Ship.Warp")),Cue);
 if(Device==EVTDevice::EMP) {float Angle=M.Heading+R.EMPAim; FVector2D Direction(FMath::Cos(Angle),FMath::Sin(Angle)); SpawnDeviceProjectile(EVTProjectileKind::EMP,M.Position+Direction*26,Direction*E.EMPSpeed,E.EMPFraction,E.EMPRange/E.EMPSpeed+0.2f,6);}
 if(Device==EVTDevice::Mine) {--R.MineMagazine; SpawnDeviceProjectile(EVTProjectileKind::Mine,M.Position-Forward*34,FVector2D::ZeroVector,E.MineDamage,E.MineTTL,E.MineRadius);}
 if(Device==EVTDevice::Microwarp) {S->Movement->Motion.Position+=S->Intent.CursorOffset.GetClampedToMaxSize(E.WarpRange); S->Movement->Authority=S->Movement->Motion;S->Movement->Previous=S->Movement->Motion;S->Movement->RenderCorrection=FVector2D::ZeroVector; S->ForceNetUpdate();}
 if(Device==EVTDevice::PointDefense) {
  AVTProjectile* Target=nullptr; double Best=E.PDRadius*E.PDRadius;
  for(AVTProjectile* Shot:GetWorld()->GetSubsystem<UVTSimulation>()->Projectiles) if(IsValid(Shot)&&Shot->SystemIndex==S->SystemIndex&&Shot->SourceId!=S->PersistentId&&Shot->Kind!=EVTProjectileKind::Mine&&(!Shot->SourceNPC||!S->IsNPC||Shot->SourceFaction!=S->Faction)) {double Distance=(Shot->Position-M.Position).SizeSquared(); if(Distance<Best) {Target=Shot; Best=Distance;}}
  if(Target) Target->Destroy();
 }
}
UVTDeviceAbility::UVTDeviceAbility() {
 InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor; NetExecutionPolicy=EGameplayAbilityNetExecutionPolicy::ServerOnly; NetSecurityPolicy=EGameplayAbilityNetSecurityPolicy::ServerOnly;
 CooldownGameplayEffectClass=UVTFixedCooldownEffect::StaticClass();
 ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Ship.Docked")));
 ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Ship.Disabled")));
}
void UVTDeviceAbility::ConfigureDevice(EVTDevice Kind) {
 Device=Kind; const TCHAR* Names[]={TEXT("EMP"),TEXT("Mine"),TEXT("Warp"),TEXT("PointDefense")};
 int Index=Kind==EVTDevice::EMP ? 0 : Kind==EVTDevice::Mine ? 1 : Kind==EVTDevice::Microwarp ? 2 : 3;
 CooldownTags.Reset(); CooldownTags.AddTag(FGameplayTag::RequestGameplayTag(FName(FString(TEXT("Cooldown.Ship."))+Names[Index])));

}
float& UVTDeviceAbility::Snapshot() {
 auto* S=CastChecked<AVTShip>(GetAvatarActorFromActorInfo()); auto& R=S->Combat->EquipmentState;
 if(Device==EVTDevice::EMP) return R.EMPCooldown; if(Device==EVTDevice::Mine) return R.MineCooldown; if(Device==EVTDevice::Microwarp) return R.WarpCooldown; return R.PDCooldown;
}
void UVTDeviceAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* Event) {
 auto* S=Cast<AVTShip>(Info->AvatarActor.Get()); if(!S) {EndAbility(Handle,Info,ActivationInfo,true,true); return;}
 const auto& E=S->Definition.Equipment;
 bool Fitted=Device==EVTDevice::EMP ? E.EMP : Device==EVTDevice::Mine ? E.Mines : Device==EVTDevice::Microwarp ? E.Warp : E.PointDefense;
 if(!Fitted||((S->Docked||S->Disabled)&&!S->Combat->RestoringBank)) {EndAbility(Handle,Info,ActivationInfo,true,true); return;}
 if(!S->Combat->RestoringBank&&!CommitAbility(Handle,Info,ActivationInfo)) {EndAbility(Handle,Info,ActivationInfo,true,true);return;}
 const float Duration=Device==EVTDevice::EMP ? E.EMPCooldown : Device==EVTDevice::Mine ? E.MineCooldown : Device==EVTDevice::Microwarp ? E.WarpCooldown : 1/FMath::Max(0.001f,E.PDRate);
 if(S->Combat->RestoringBank) ApplyCooldown(Handle,Info,ActivationInfo);
 else S->Combat->ExecuteDevice(Device);
 CooldownTask=UVTFixedStepTask::Start(this,Duration); Snapshot()=Duration;
}
void UVTDeviceAbility::FixedStep() {
 if(!CooldownTask) return;
 bool Finished=CooldownTask->Advance(VT::Step); Snapshot()=CooldownTask->GetRemaining();
 if(Finished) EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,false);
}
void UVTDeviceAbility::RestoreCooldown(float Seconds) {if(CooldownTask) {CooldownTask->Restore(Seconds); Snapshot()=CooldownTask->GetRemaining();}}
void UVTCombatComponent::RestoreDevices() {
 auto* S=CastChecked<AVTShip>(GetOwner()); const auto Saved=EquipmentState;
 for(UClass* Class:{UVTEMPAbility::StaticClass(),UVTMineAbility::StaticClass(),UVTWarpAbility::StaticClass(),UVTPDAbility::StaticClass()}) {
  float Timer=Class==UVTEMPAbility::StaticClass() ? Saved.EMPCooldown : Class==UVTMineAbility::StaticClass() ? Saved.MineCooldown : Class==UVTWarpAbility::StaticClass() ? Saved.WarpCooldown : Saved.PDCooldown;
  if(Timer<=0) continue;
  RestoringBank=true; S->Abilities->TryActivateAbilityByClass(Class); RestoringBank=false;
  if(auto* Spec=S->Abilities->FindAbilitySpecFromClass(Class)) if(auto* A=Cast<UVTDeviceAbility>(Spec->GetPrimaryInstance())) if(A->IsActive()) {A->RestoreCooldown(Timer);}
 }
 EquipmentState.Locks.Reset(); EquipmentState.LockElapsed=0; WarpHeld=false; TorpedoHeld=false;
}

UVTFixedCooldownEffect::UVTFixedCooldownEffect() {DurationPolicy=EGameplayEffectDurationType::Infinite;}
void UVTDeviceAbility::ApplyCooldown(const FGameplayAbilitySpecHandle,const FGameplayAbilityActorInfo* Info,const FGameplayAbilityActivationInfo) const {
 auto* ASC=Info->AbilitySystemComponent.Get(); if(!ASC) return;
 auto Spec=ASC->MakeOutgoingSpec(UVTFixedCooldownEffect::StaticClass(),1,ASC->MakeEffectContext());
 Spec.Data->DynamicGrantedTags.AppendTags(CooldownTags);
 CooldownEffect=ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}
void UVTDeviceAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,const FGameplayAbilityActivationInfo ActivationInfo,bool Replicate,bool Cancelled) {
 if(Info&&Info->AbilitySystemComponent.IsValid()&&CooldownEffect.IsValid()) Info->AbilitySystemComponent->RemoveActiveGameplayEffect(CooldownEffect);
 CooldownEffect.Invalidate();
 Super::EndAbility(Handle,Info,ActivationInfo,Replicate,Cancelled);
}
