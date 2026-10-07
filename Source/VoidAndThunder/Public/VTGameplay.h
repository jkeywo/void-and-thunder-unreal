#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "AIController.h"
#include "Subsystems/WorldSubsystem.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayAbilitySpec.h"
#include "VTGameData.h"
#include "InputActionValue.h"
#include "VTGameplay.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class AVTShip;

UCLASS()
class VOIDANDTHUNDER_API UVTAttributes : public UAttributeSet {
 GENERATED_BODY()
public:
 UPROPERTY(ReplicatedUsing=OnRep_Hull) FGameplayAttributeData Hull;
 UPROPERTY(ReplicatedUsing=OnRep_Battery) FGameplayAttributeData Battery;
 UFUNCTION() void OnRep_Hull(const FGameplayAttributeData& Old);
 UFUNCTION() void OnRep_Battery(const FGameplayAttributeData& Old);
 GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UVTAttributes, Hull)
 GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UVTAttributes, Battery)
 static FGameplayAttribute HullAttribute();
 static FGameplayAttribute BatteryAttribute();
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
};

UCLASS()
class VOIDANDTHUNDER_API UVTShipMovement : public UActorComponent {
 GENERATED_BODY()
public:
 UVTShipMovement();
 UPROPERTY(ReplicatedUsing=OnRep_Authority) FVTMotion Authority;
 FVTMotion Motion;
 FVTMotion Previous;
 float Accumulator = 0;
 struct FPending { FVTPilotIntent Intent; };
 TArray<FPending> Pending;
 void Step(const FVTPilotIntent& Intent, bool Predict);
 void ApplyPose();
 UFUNCTION() void OnRep_Authority();
 virtual void TickComponent(float Dt, ELevelTick Tick, FActorComponentTickFunction* Fn) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
};

UCLASS()
class VOIDANDTHUNDER_API AVTShip : public APawn, public IAbilitySystemInterface {
 GENERATED_BODY()
public:
 AVTShip();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UVTShipMovement> Movement;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UAbilitySystemComponent> Abilities;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UVTAttributes> Attributes;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraBoom;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
 UPROPERTY(Replicated, BlueprintReadOnly) int32 SystemIndex = 0;
 UPROPERTY(Replicated, BlueprintReadOnly) FName ClassId = "corsair_cruiser";
 UPROPERTY(Replicated, BlueprintReadOnly) FName Faction = "Corsairs";
 UPROPERTY(Replicated, BlueprintReadOnly) FGuid PersistentId;
 UPROPERTY(Replicated, BlueprintReadOnly) bool Docked = false;
 UPROPERTY(Replicated, BlueprintReadOnly) bool Disabled = false;
 UPROPERTY(Replicated, BlueprintReadOnly) float PortReload = 0;
 UPROPERTY(Replicated, BlueprintReadOnly) float StarboardReload = 0;
 FVTShipDefinition Definition;
 FVTPilotIntent Intent;
 uint32 LastReceived = 0;
 double LastInputTime = 0;
 UPROPERTY(Replicated, BlueprintReadOnly) bool IsNPC = false;
 bool Anchored = false;
 virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return Abilities; }
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
 virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
 UFUNCTION(Server, Unreliable) void ServerIntent(FVTPilotIntent Value);
 void InitializeShip(FName ShipId, int32 System, const FVTMotion& Initial);
};

UCLASS()
class VOIDANDTHUNDER_API AVTShipAI : public AAIController {
 GENERATED_BODY()
public:
 AVTShipAI();
 void Decide(float Dt);
};

UCLASS()
class VOIDANDTHUNDER_API AVTPlayerState : public APlayerState {
 GENERATED_BODY()
public:
 UPROPERTY(Replicated, BlueprintReadOnly) int32 Credits = 0;
 UPROPERTY(Replicated, BlueprintReadOnly) int32 Boarded = 0;
 UPROPERTY(Replicated, BlueprintReadOnly) FGuid Profile;
 UPROPERTY(Replicated, BlueprintReadOnly) TArray<float> Reputation;
 UPROPERTY(Replicated, BlueprintReadOnly) TArray<float> Heat;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
};

UCLASS()
class VOIDANDTHUNDER_API AVTGameState : public AGameStateBase {
 GENERATED_BODY()
public:
 UPROPERTY(Replicated, BlueprintReadOnly) double SimulationTime = 0;
 UPROPERTY(Replicated, BlueprintReadOnly) TArray<int32> Populations;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
};

UCLASS()
class VOIDANDTHUNDER_API AVTController : public APlayerController {
 GENERATED_BODY()
public:
 AVTController();
 virtual void BeginPlay() override;
 virtual void SetupInputComponent() override;
 void ReadFlight(const FInputActionValue& Value, int32 Index);
 virtual void PlayerTick(float Dt) override;
 UPROPERTY() TObjectPtr<UInputMappingContext> FlightMapping;
 UPROPERTY() TArray<TObjectPtr<UInputAction>> Actions;
 FVTPilotIntent LocalIntent;
 uint32 NextSequence = 0;
 bool ProbeJumped=false;
 void ValidationInput(float Dt);
 float SendAccumulator = 0;
 UFUNCTION(Exec) void VTJump(FString Destination);
 UFUNCTION(Server, Reliable) void ServerJump(FName Destination);
 UFUNCTION(Exec) void VTHost();
 UFUNCTION(Exec) void VTJoin(FString Address);
 UFUNCTION(Exec) void VTScale(int32 Count);
 UFUNCTION(Exec) void VTSave();
 UFUNCTION(Exec) void VTLoad();
};

UCLASS()
class VOIDANDTHUNDER_API UVTSimulation : public UTickableWorldSubsystem {
 GENERATED_BODY()
public:
 UPROPERTY() TObjectPtr<UVTGameData> Data;
 UPROPERTY() TArray<TObjectPtr<AVTShip>> Ships;
 TArray<double> StepMilliseconds;
 double Accumulator = 0;
 double SimulationTime = 0;
 bool Bootstrapped = false;
 bool ProbeWrote=false, ProbeScaled=false, ProbeMoved=false, ProbeOriginSet=false;
 int32 MaxPlayersObserved=0;
 FVector2D ProbeOrigin;
 void ValidationTick();
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Tick(float Dt) override;
 virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UVTSimulation, STATGROUP_Tickables); }
 virtual bool DoesSupportWorldType(EWorldType::Type Type) const override {return Type == EWorldType::Game || Type == EWorldType::PIE;}
 void Bootstrap(int32 Population = -1);
 void FixedStep();
 AVTShip* SpawnShip(FName Id, int32 System, const FVTMotion& Motion, bool NPC, FName Faction);
};

UCLASS()
class VOIDANDTHUNDER_API AVTGameMode : public AGameModeBase {
 GENERATED_BODY()
public:
 AVTGameMode();
 virtual void BeginPlay() override;
 virtual void PostLogin(APlayerController* NewPlayer) override;
 virtual void Logout(AController* Exiting) override;
 virtual void RestartPlayer(AController* Player) override;
};

UCLASS()
class VOIDANDTHUNDER_API UVTGameInstance : public UGameInstance {
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable) void Host();
 UFUNCTION(BlueprintCallable) void Join(const FString& Address);
};
