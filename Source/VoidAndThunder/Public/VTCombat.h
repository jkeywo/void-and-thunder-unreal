#pragma once
#include "CoreMinimal.h"
#include "VTTypes.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "VTCombat.generated.h"
class AVTShip;
class AVTProjectile;
namespace VTCombat {
 VOIDANDTHUNDER_API int32 ShieldArc(float Heading,const FVector2D& Offset,int32 Arcs);
 VOIDANDTHUNDER_API FVector2D BroadsideDirection(float Heading,bool Port,const FVector2D& Aim,float Arc);
 VOIDANDTHUNDER_API float SegmentDistanceSquared(const FVector2D& A,const FVector2D& B,const FVector2D& P);
}
UCLASS()
class VOIDANDTHUNDER_API UVTHullEffect : public UGameplayEffect {
 GENERATED_BODY()
public: UVTHullEffect();
};
UCLASS()
class VOIDANDTHUNDER_API UVTBatteryEffect : public UGameplayEffect {
 GENERATED_BODY()
public: UVTBatteryEffect();
};
UCLASS()
class VOIDANDTHUNDER_API UVTEMPStressEffect : public UGameplayEffect {
 GENERATED_BODY()
public: UVTEMPStressEffect();
};
UCLASS()
class VOIDANDTHUNDER_API UVTCombatComponent : public UActorComponent {
 GENERATED_BODY()
public:
 UVTCombatComponent();
 UPROPERTY(Replicated,BlueprintReadOnly) FVTShieldBanks Shields=FVTShieldBanks(0,0,0,0);
 UPROPERTY(Replicated,BlueprintReadOnly) FVTShieldBanks Suppression=FVTShieldBanks(0,0,0,0);
 UPROPERTY(Replicated,BlueprintReadOnly) float PortCharge=0;
 UPROPERTY(Replicated,BlueprintReadOnly) float StarboardCharge=0;
 UPROPERTY(Replicated,BlueprintReadOnly) FGuid BoardingTarget;
 UPROPERTY(Replicated,BlueprintReadOnly) float BoardingProgress=0;
 UPROPERTY(Replicated,BlueprintReadOnly) FVTEquipmentState EquipmentState;
 UPROPERTY(Replicated,BlueprintReadOnly) float SpeedScale=1;
 UPROPERTY(Replicated,BlueprintReadOnly) bool BoostPowered=false;
 bool EMPPowered=false, PDPowered=false, WarpHeld=false, TorpedoHeld=false;
 void EquipmentSystems();
 void EquipmentWeapons();
 void PointDefenseStep();
 void ExecuteDevice(EVTDevice Device);
 void RestoreDevices();
 AVTShip* FindTarget(FGuid Id) const;
 bool Hostile(const AVTShip* Other) const;
 AVTProjectile* SpawnDeviceProjectile(EVTProjectileKind Kind,const FVector2D& Position,const FVector2D& Velocity,float Damage,float TTL,float Radius);
 bool Claimed=false;
 bool LastBoost=false,LastBrace=false,HullWarning=false;
 void ApplyDelta(TSubclassOf<UGameplayEffect> Effect,float Delta);
 void Cue(FName Name,float Magnitude=0);
 bool RestoringBank=false;
 void RestoreReload(bool Port,float Remaining);
 void Initialize();
 void SystemsStep();
 void WeaponsStep();
 void Damage(float Amount,const FVector2D& Impact,AVTShip* Attacker,FGuid AttackerProfile=FGuid(),bool Announce=true,float ReportMagnitude=-1,FName AttackerFaction=NAME_None);
 void Volley(bool Port,const FVector2D& Direction);
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
// The active GAS instance owns each bank's wind-up and cooldown; simulation drives its clock.
UCLASS()
class VOIDANDTHUNDER_API UVTBroadsideAbility : public UGameplayAbility {
 GENERATED_BODY()
public:
 UVTBroadsideAbility();
 bool Port=true;
 float ChargeRemaining=0;
 float ReloadRemaining=0;
 FVector2D ChargeDirection=FVector2D::ZeroVector;
 bool Fired=false;
 virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* Event) override;
 void FixedStep();
};
UCLASS()
class VOIDANDTHUNDER_API UVTStarboardAbility : public UVTBroadsideAbility {
 GENERATED_BODY()
public:
 UVTStarboardAbility() {Port=false;}
};
UCLASS()
class VOIDANDTHUNDER_API UVTDeviceAbility : public UGameplayAbility {
 GENERATED_BODY()
public:
 UVTDeviceAbility();
 EVTDevice Device=EVTDevice::EMP;
 float Remaining=0;
 virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,const FGameplayAbilityActivationInfo ActivationInfo,const FGameplayEventData* Event) override;
 void FixedStep();
 float& Snapshot();
};
UCLASS() class VOIDANDTHUNDER_API UVTEMPAbility : public UVTDeviceAbility { GENERATED_BODY() public: UVTEMPAbility(){Device=EVTDevice::EMP;} };
UCLASS() class VOIDANDTHUNDER_API UVTMineAbility : public UVTDeviceAbility { GENERATED_BODY() public: UVTMineAbility(){Device=EVTDevice::Mine;} };
UCLASS() class VOIDANDTHUNDER_API UVTWarpAbility : public UVTDeviceAbility { GENERATED_BODY() public: UVTWarpAbility(){Device=EVTDevice::Microwarp;} };
UCLASS() class VOIDANDTHUNDER_API UVTPDAbility : public UVTDeviceAbility { GENERATED_BODY() public: UVTPDAbility(){Device=EVTDevice::PointDefense;} };
UCLASS()
class VOIDANDTHUNDER_API AVTProjectile : public AActor {
 GENERATED_BODY()
public:
 AVTProjectile();
 UPROPERTY(Replicated) int32 SystemIndex=0;
 UPROPERTY(Replicated) FGuid PersistentId;
 UPROPERTY(Replicated) FGuid SourceId;
 UPROPERTY(Replicated) FVector2D Position=FVector2D::ZeroVector;
 UPROPERTY(Replicated) FVector2D Velocity=FVector2D::ZeroVector;
 UPROPERTY(Replicated) EVTProjectileKind Kind=EVTProjectileKind::Cannon;
 UPROPERTY(Replicated) FGuid TargetId;
 UPROPERTY(Replicated) FGuid AttackerProfile;
 UPROPERTY(Replicated) FName SourceFaction;
 UPROPERTY(Replicated) bool SourceNPC=false;
 UPROPERTY(Replicated) float Height=0;
 float ReportCountdown=0;
 UPROPERTY(Replicated) FVector Velocity3D=FVector::ZeroVector;
 UPROPERTY(Replicated) float TurnRate=0;
 UPROPERTY(Replicated) float Damage=0;
 UPROPERTY(Replicated) float Remaining=0;
 UPROPERTY(Replicated) float Radius=5;
 UPROPERTY() TObjectPtr<AVTShip> Source;
 FVector2D Previous=FVector2D::ZeroVector;
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 virtual void Tick(float Dt) override;
 virtual bool IsNetRelevantFor(const AActor* RealViewer,const AActor* ViewTarget,const FVector& SrcLocation) const override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
