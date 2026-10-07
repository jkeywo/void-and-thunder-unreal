#pragma once
#include "CoreMinimal.h"
#include "VTTypes.h"
#include "Abilities/GameplayAbility.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "VTCombat.generated.h"
class AVTShip;
namespace VTCombat {
 VOIDANDTHUNDER_API int32 ShieldArc(float Heading,const FVector2D& Offset,int32 Arcs);
 VOIDANDTHUNDER_API FVector2D BroadsideDirection(float Heading,bool Port,const FVector2D& Aim,float Arc);
 VOIDANDTHUNDER_API float SegmentDistanceSquared(const FVector2D& A,const FVector2D& B,const FVector2D& P);
}
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
 bool Claimed=false;
 bool RestoringBank=false;
 void RestoreReload(bool Port,float Remaining);
 void Initialize();
 void SystemsStep();
 void WeaponsStep();
 void Damage(float Amount,const FVector2D& Impact,AVTShip* Attacker);
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
class VOIDANDTHUNDER_API AVTProjectile : public AActor {
 GENERATED_BODY()
public:
 AVTProjectile();
 UPROPERTY(Replicated) int32 SystemIndex=0;
 UPROPERTY(Replicated) FGuid PersistentId;
 UPROPERTY(Replicated) FGuid SourceId;
 UPROPERTY(Replicated) FVector2D Position=FVector2D::ZeroVector;
 UPROPERTY(Replicated) FVector2D Velocity=FVector2D::ZeroVector;
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
