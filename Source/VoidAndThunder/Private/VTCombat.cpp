#include "VTCombat.h"
#include "Materials/MaterialInterface.h"
#include "VTGameplay.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
int32 VTCombat::ShieldArc(float Heading,const FVector2D& Offset,int32 Arcs) {
 if(Arcs<=1) return 0;
 if(Arcs<4) return FVector2D::DotProduct(Offset,FVector2D(FMath::Cos(Heading),FMath::Sin(Heading)))>=0 ? 0 : 1;
 float Bearing=FMath::UnwindRadians(FMath::Atan2(Offset.Y,Offset.X)-Heading);
 if(FMath::Abs(Bearing)<=PI/4+1e-4f) return 0;
 if(FMath::Abs(Bearing)>=3*PI/4) return 1;
 return Bearing>0 ? 2 : 3;
}
FVector2D VTCombat::BroadsideDirection(float Heading,bool Port,const FVector2D& Aim,float Arc) {
 float Beam=Heading+(Port ? PI/2 : -PI/2);
 float Angle=Beam;
 if(Aim.SizeSquared()>1e-6) Angle+=FMath::Clamp(FMath::UnwindRadians(float(FMath::Atan2(Aim.Y,Aim.X))-Beam),-Arc,Arc);
 return FVector2D(FMath::Cos(Angle),FMath::Sin(Angle));
}
TPair<FVector2D,FVector2D> VTCombat::BroadsideShot(const FVector2D& Position,const FVector2D& Velocity,const FVector2D& Direction,const FVTShipDefinition& Ship,const FVTRules& Rules,int32 Gun) {
 const int32 Guns=FMath::Max(1,Ship.Guns);
 const float Fraction=Guns<=1?0:float(Gun)/(Guns-1)-0.5f;
 return {Position+Direction*Rules.MuzzleStandoff+FVector2D(-Direction.Y,Direction.X)*(Fraction*Rules.HullLength),Velocity+Direction*Ship.MuzzleSpeed};
}
float VTCombat::SegmentDistanceSquared(const FVector2D& A,const FVector2D& B,const FVector2D& P) {
 auto D=B-A; double T=D.SizeSquared()>1e-12 ? FMath::Clamp(FVector2D::DotProduct(P-A,D)/D.SizeSquared(),0.,1.) : 0;
 return float((P-A-D*T).SizeSquared());
}
UVTCombatComponent::UVTCombatComponent() {SetIsReplicatedByDefault(true);}
void UVTCombatComponent::Initialize() {
 auto* S=CastChecked<AVTShip>(GetOwner()); Shields=S->Definition.ShieldMax; Suppression=FVTShieldBanks(0,0,0,0);
 int Count=S->Definition.ShieldArcs<2 ? 1 : S->Definition.ShieldArcs<4 ? 2 : 4;
 for(int I=Count;I<4;++I) Shields[I]=0;
 EquipmentState.Loaded=S->Definition.Equipment.Tubes; EquipmentState.TorpedoMagazine=S->Definition.Equipment.TorpedoMagazine; EquipmentState.MineMagazine=S->Definition.Equipment.MineMagazine;
 if(S->HasAuthority()) {
 for(UClass* Class:{UVTEMPAbility::StaticClass(),UVTMineAbility::StaticClass(),UVTWarpAbility::StaticClass(),UVTPDAbility::StaticClass()}) S->Abilities->GiveAbility(FGameplayAbilitySpec(Class,1));
 S->Abilities->GiveAbility(FGameplayAbilitySpec(UVTBroadsideAbility::StaticClass(),1,0)); S->Abilities->GiveAbility(FGameplayAbilitySpec(UVTStarboardAbility::StaticClass(),1,1));}
}
void UVTCombatComponent::SystemsStep() {
 auto* S=CastChecked<AVTShip>(GetOwner());
 int Count=S->Definition.ShieldArcs<2 ? 1 : S->Definition.ShieldArcs<4 ? 2 : 4;
 for(int I=0;I<Count;++I) {
  if(Suppression[I]>0) Suppression[I]=FMath::Max(0.,Suppression[I]-VT::Step);
  else Shields[I]=FMath::Min(S->Definition.ShieldMax[I],Shields[I]+S->Definition.ShieldRegen*VT::Step);
 }
 EquipmentSystems();
}
void UVTCombatComponent::WeaponsStep() {
 auto* S=CastChecked<AVTShip>(GetOwner());
 // Tick existing activations before consuming this step's requests.
 bool PortActive=false,StarboardActive=false;
 for(auto& Spec:S->Abilities->GetActivatableAbilities()) if(auto* A=Cast<UVTBroadsideAbility>(Spec.GetPrimaryInstance())) {if(A->IsActive())A->FixedStep();(A->Port ? PortActive : StarboardActive)=A->IsActive();}
 EquipmentWeapons();
 if(S->Docked||S->Disabled||(S->Intent.Buttons&(VTButtons::Warp|VTButtons::Torpedo))) return;
 if((S->Intent.Buttons&VTButtons::Port)&&!PortActive) S->Abilities->TryActivateAbilityByClass(UVTBroadsideAbility::StaticClass());
 if((S->Intent.Buttons&VTButtons::Starboard)&&!StarboardActive) S->Abilities->TryActivateAbilityByClass(UVTStarboardAbility::StaticClass());
}
void UVTCombatComponent::Damage(float Amount,const FVector2D& Impact,AVTShip* Attacker,FGuid AttackerProfile,bool Announce,float ReportMagnitude,FName AttackerFaction) {
 auto* S=CastChecked<AVTShip>(GetOwner()); if(!S->HasAuthority()||S->Docked||Amount<=0||!FMath::IsFinite(Amount)) return;
 FGameplayCueParameters Cue; Cue.Location=VT::ToWorld(Impact,S->SystemIndex); Cue.RawMagnitude=ReportMagnitude>=0 ? ReportMagnitude : Amount; if(Announce&&!IsRunningCommandlet()) S->Abilities->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Ship.Hit")),Cue);
 if(S->Invulnerable) return;
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();
 Sim->RecordHit(S,Attacker,Amount,AttackerProfile,AttackerFaction);
 if(S->Intent.Buttons&VTButtons::Brace) Amount*=Sim->Data->Rules.BraceDamageFactor;
 int Arc=VTCombat::ShieldArc(S->Movement->Motion.Heading,Impact-S->Movement->Motion.Position,S->Definition.ShieldArcs);
 if(S->Definition.ShieldMax[Arc]>0) {Suppression[Arc]=S->Definition.ShieldDelay; float Absorbed=FMath::Min(float(Shields[Arc]),Amount); Shields[Arc]-=Absorbed; Amount-=Absorbed;}
 // Hull has exactly one owner: the GAS attribute. Every weapon and contact uses this path.
 ApplyDelta(UVTHullEffect::StaticClass(),-Amount);
}
void UVTCombatComponent::Volley(bool Port,const FVector2D& Direction) {
 auto* S=CastChecked<AVTShip>(GetOwner()); auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();
 FGameplayCueParameters Cue; Cue.Location=VT::ToWorld(S->Movement->Motion.Position,S->SystemIndex); if(!IsRunningCommandlet()) S->Abilities->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Ship.Fire")),Cue);
 const auto& D=S->Definition; const auto& Rules=Sim->Data->Rules;
 int Guns=FMath::Max(1,D.Guns);
 for(int I=0;I<Guns;++I) {
  const auto Geometry=VTCombat::BroadsideShot(S->Movement->Motion.Position,S->Movement->Motion.Velocity,Direction,D,Rules,I);
  const auto P=Geometry.Key;
  auto* Shot=GetWorld()->SpawnActor<AVTProjectile>();
  Shot->SystemIndex=S->SystemIndex; Shot->PersistentId=FGuid::NewGuid(); Shot->Source=S; Shot->SourceId=S->PersistentId; Shot->SourceFaction=S->Faction; Shot->SourceNPC=S->IsNPC; if(auto* PS=S->GetPlayerState<AVTPlayerState>()) Shot->AttackerProfile=PS->Profile;
  Shot->Position=P; Shot->Previous=P; Shot->Velocity=Geometry.Value;
  Shot->Damage=D.Damage; Shot->Remaining=Rules.ProjectileTTL; Shot->Radius=Rules.ProjectileRadius;
 }
}
void UVTCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
 Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(UVTCombatComponent,EquipmentState); DOREPLIFETIME(UVTCombatComponent,SpeedScale); DOREPLIFETIME(UVTCombatComponent,BoostPowered); DOREPLIFETIME(UVTCombatComponent,BoardingTarget); DOREPLIFETIME(UVTCombatComponent,BoardingProgress); DOREPLIFETIME(UVTCombatComponent,Shields); DOREPLIFETIME(UVTCombatComponent,Suppression); DOREPLIFETIME(UVTCombatComponent,PortCharge); DOREPLIFETIME(UVTCombatComponent,StarboardCharge);
}
UVTBroadsideAbility::UVTBroadsideAbility() {
 InstancingPolicy=EGameplayAbilityInstancingPolicy::InstancedPerActor; NetExecutionPolicy=EGameplayAbilityNetExecutionPolicy::ServerOnly; NetSecurityPolicy=EGameplayAbilityNetSecurityPolicy::ServerOnly;
 ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Ship.Docked"))); ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Ship.Disabled")));
}
void UVTBroadsideAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* Event) {
 auto* S=Cast<AVTShip>(Info->AvatarActor.Get());
 if(!S||((S->Docked||S->Disabled||(S->Intent.Buttons&(VTButtons::Warp|VTButtons::Torpedo)))&&!S->Combat->RestoringBank)) {EndAbility(Handle,Info,ActivationInfo,true,true); return;}
 if(S->Combat->RestoringBank) {Fired=true; ChargeRemaining=0; ReloadRemaining=0; BankTask=UVTFixedStepTask::Start(this,0); return;}
 if(!CommitAbility(Handle,Info,ActivationInfo)) {EndAbility(Handle,Info,ActivationInfo,true,true);return;}
 Fired=false; ChargeRemaining=S->Definition.ChargeTime; ReloadRemaining=0;
 ChargeDirection=VTCombat::BroadsideDirection(S->Movement->Motion.Heading,Port,S->Intent.Aim,S->Definition.Arc);
 if(ChargeRemaining<=0) {S->Combat->Volley(Port,ChargeDirection); Fired=true; ReloadRemaining=S->Definition.Reload;}
 BankTask=UVTFixedStepTask::Start(this,Fired ? ReloadRemaining : ChargeRemaining);
 (Port ? S->PortReload : S->StarboardReload)=ReloadRemaining;
 (Port ? S->Combat->PortCharge : S->Combat->StarboardCharge)=ChargeRemaining;
}
void UVTBroadsideAbility::FixedStep() {
 auto* S=CastChecked<AVTShip>(GetAvatarActorFromActorInfo());
 if(!Fired) {
  if(S->Intent.Buttons&(VTButtons::Warp|VTButtons::Torpedo)){ChargeRemaining=0;(Port?S->Combat->PortCharge:S->Combat->StarboardCharge)=0;EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,true);return;}
  BankTask->Advance(VT::Step); ChargeRemaining=BankTask->GetRemaining();
  if(ChargeRemaining<=0) {if(!S->Disabled&&!S->Docked) S->Combat->Volley(Port,ChargeDirection); Fired=true; ReloadRemaining=S->Definition.Reload; BankTask->Restore(ReloadRemaining);}
 } else {BankTask->Advance(VT::Step); ReloadRemaining=BankTask->GetRemaining();}
 (Port ? S->PortReload : S->StarboardReload)=ReloadRemaining;
 (Port ? S->Combat->PortCharge : S->Combat->StarboardCharge)=ChargeRemaining;
 if(Fired&&ReloadRemaining<=0) EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,false);
}
AVTProjectile::AVTProjectile() {
 bReplicates=true; SetReplicateMovement(false); PrimaryActorTick.bCanEverTick=true;
 auto* Mesh=CreateDefaultSubobject<UStaticMeshComponent>("ProjectileMesh"); RootComponent=Mesh; Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Mesh->SetCanEverAffectNavigation(false); Mesh->SetCastShadow(false); Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"))); Mesh->SetRelativeScale3D(FVector(2));
}
void AVTProjectile::BeginPlay() {Super::BeginPlay(); auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>(); Sim->Projectiles.AddUnique(this);
 if(IsRunningCommandlet()||!FApp::CanEverRender())return;
 auto* Mesh=CastChecked<UStaticMeshComponent>(RootComponent); Mesh->SetRelativeScale3D(FVector(2*(Sim->Data ? (Kind==EVTProjectileKind::Torpedo?Sim->Data->TorpedoVisualRadius:Sim->Data->ProjectileVisualRadius) : 7.f)));
 if(Sim->Data)Mesh->SetMaterial(0,Kind==EVTProjectileKind::Torpedo?Sim->Data->TorpedoMaterial.LoadSynchronous():Sim->Data->ProjectileMaterial.Get());}
void AVTProjectile::EndPlay(const EEndPlayReason::Type Reason) {if(auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>()) Sim->Projectiles.Remove(this); Super::EndPlay(Reason);}
void AVTProjectile::Tick(float Dt) {Super::Tick(Dt); if(!FApp::CanEverRender()) return; auto* Mesh=CastChecked<UStaticMeshComponent>(RootComponent);
 auto* PC=GetWorld()->GetFirstPlayerController(); auto* Viewer=PC ? Cast<AVTShip>(PC->GetPawn()) : nullptr; bool Visible=Viewer&&Viewer->SystemIndex==SystemIndex; if(Mesh->IsVisible()!=Visible) Mesh->SetVisibility(Visible); if(Visible) SetActorLocation(VT::ToWorld(Position,SystemIndex)+FVector(0,0,Height*100));}
bool AVTProjectile::IsNetRelevantFor(const AActor* RealViewer,const AActor* ViewTarget,const FVector& SrcLocation) const {
 auto* PC=Cast<APlayerController>(RealViewer); auto* S=PC ? Cast<AVTShip>(PC->GetPawn()) : Cast<AVTShip>(ViewTarget); return S&&S->SystemIndex==SystemIndex;
}
void AVTProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
 Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AVTProjectile,Kind); DOREPLIFETIME(AVTProjectile,TargetId); DOREPLIFETIME(AVTProjectile,AttackerProfile); DOREPLIFETIME(AVTProjectile,SourceFaction); DOREPLIFETIME(AVTProjectile,SourceNPC); DOREPLIFETIME(AVTProjectile,Height); DOREPLIFETIME(AVTProjectile,Velocity3D); DOREPLIFETIME(AVTProjectile,TurnRate); DOREPLIFETIME(AVTProjectile,SystemIndex); DOREPLIFETIME(AVTProjectile,PersistentId); DOREPLIFETIME(AVTProjectile,SourceId); DOREPLIFETIME(AVTProjectile,Position); DOREPLIFETIME(AVTProjectile,Velocity); DOREPLIFETIME(AVTProjectile,Damage); DOREPLIFETIME(AVTProjectile,Remaining); DOREPLIFETIME(AVTProjectile,Radius);
}

void UVTCombatComponent::RestoreReload(bool Port,float Remaining) {
 if(Remaining<=0) return;
 auto* S=CastChecked<AVTShip>(GetOwner());
 UClass* Class=Port ? UVTBroadsideAbility::StaticClass() : UVTStarboardAbility::StaticClass();
 RestoringBank=true; S->Abilities->TryActivateAbilityByClass(Class); RestoringBank=false;
 if(auto* Spec=S->Abilities->FindAbilitySpecFromClass(Class)) if(auto* A=Cast<UVTBroadsideAbility>(Spec->GetPrimaryInstance())) {A->Fired=true; A->ChargeRemaining=0; A->ReloadRemaining=Remaining; if(A->BankTask) A->BankTask->Restore(Remaining);}
 (Port ? S->PortReload : S->StarboardReload)=Remaining;
}

namespace {
void DeltaEffect(UGameplayEffect* Effect,FGameplayAttribute Attribute) {
 Effect->DurationPolicy=EGameplayEffectDurationType::Instant;
 FGameplayModifierInfo Modifier; Modifier.Attribute=Attribute; Modifier.ModifierOp=EGameplayModOp::Additive; FSetByCallerFloat Caller; Caller.DataName=TEXT("Delta"); Modifier.ModifierMagnitude=FGameplayEffectModifierMagnitude(Caller); Effect->Modifiers.Add(Modifier);
}
}
UVTHullEffect::UVTHullEffect() {DeltaEffect(this,UVTAttributes::HullAttribute());}
UVTBatteryEffect::UVTBatteryEffect() {DeltaEffect(this,UVTAttributes::BatteryAttribute());}
UVTEMPStressEffect::UVTEMPStressEffect() {DeltaEffect(this,UVTAttributes::GetEMPStressAttribute());}
void UVTCombatComponent::ApplyDelta(TSubclassOf<UGameplayEffect> Effect,float Delta) {
 auto* Ship=CastChecked<AVTShip>(GetOwner()); if(!Ship->HasAuthority()||Delta==0) return;
 auto Spec=Ship->Abilities->MakeOutgoingSpec(Effect,1,Ship->Abilities->MakeEffectContext()); if(Spec.IsValid()) {Spec.Data->SetSetByCallerMagnitude(FName(TEXT("Delta")),Delta); Ship->Abilities->ApplyGameplayEffectSpecToSelf(*Spec.Data);}
}
void UVTCombatComponent::Cue(FName Name,float Magnitude) {
 if(IsRunningCommandlet()) return; auto* Ship=CastChecked<AVTShip>(GetOwner()); FGameplayCueParameters Parameters; Parameters.Location=VT::ToWorld(Ship->Movement->Motion.Position,Ship->SystemIndex); Parameters.RawMagnitude=Magnitude; Ship->Abilities->ExecuteGameplayCue(FGameplayTag::RequestGameplayTag(Name),Parameters);
}

void UVTCombatComponent::OnRep_UIState() {VTNotifyHUD(GetWorld());}
