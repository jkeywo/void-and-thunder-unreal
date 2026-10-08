#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "AIController.h"
#include "Camera/PlayerCameraManager.h"
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
class AVTProjectile;
class UVTCombatComponent;

VOIDANDTHUNDER_API void VTNotifyHUD(UWorld* World);
UCLASS()
class VOIDANDTHUNDER_API UVTAttributes : public UAttributeSet {
 GENERATED_BODY()
public:
 UPROPERTY(ReplicatedUsing=OnRep_Hull) FGameplayAttributeData Hull;
 UPROPERTY(ReplicatedUsing=OnRep_Battery) FGameplayAttributeData Battery;
 UPROPERTY(ReplicatedUsing=OnRep_EMPStress) FGameplayAttributeData EMPStress;
 UFUNCTION() void OnRep_EMPStress(const FGameplayAttributeData& Old);
 GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UVTAttributes, EMPStress)
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
 TArray<FVTMotion> ReplicaFrames;
 double LastAuthorityReceived = 0;
 int32 ReplicaSystem = INDEX_NONE;
 struct FPending { FVTPilotIntent Intent; };
 TArray<FPending> Pending;
 FVector2D RenderCorrection=FVector2D::ZeroVector;
 float RenderHeadingCorrection=0;
 bool MeasureCorrections=false;
 TArray<double> CorrectionDistances;
 uint32 MaxPendingObserved=0;
 void Step(const FVTPilotIntent& Intent, bool Predict);
 FVTMotion PresentationPose() const;
 void ApplyPose();
 UFUNCTION() void OnRep_Authority();
 virtual void TickComponent(float Dt, ELevelTick Tick, FActorComponentTickFunction* Fn) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
};

UCLASS()
class VOIDANDTHUNDER_API AVTShip : public APawn, public IAbilitySystemInterface {
 GENERATED_BODY()
public:
 UFUNCTION() void OnRep_UIState();
 AVTShip();
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UVTShipMovement> Movement;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UVTCombatComponent> Combat;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UAbilitySystemComponent> Abilities;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UVTAttributes> Attributes;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraBoom;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
 UPROPERTY() TArray<TObjectPtr<class UNiagaraComponent>> EngineTrails;
 void PresentEngines();
 UPROPERTY(ReplicatedUsing=OnRep_UIState, BlueprintReadOnly) int32 SystemIndex = 0;
 UPROPERTY(ReplicatedUsing=OnRep_ClassId, BlueprintReadOnly) FName ClassId = "corsair_cruiser";
 UPROPERTY(ReplicatedUsing=OnRep_UIState, BlueprintReadOnly) FName Faction = "Corsairs";
 UPROPERTY(ReplicatedUsing=OnRep_UIState, BlueprintReadOnly) FGuid PersistentId;
 UPROPERTY(ReplicatedUsing=OnRep_UIState, BlueprintReadOnly) bool Docked = false;
 UPROPERTY(ReplicatedUsing=OnRep_UIState, BlueprintReadOnly) bool Disabled = false;
 UPROPERTY(ReplicatedUsing=OnRep_UIState, BlueprintReadOnly) float PortReload = 0;
 UPROPERTY(ReplicatedUsing=OnRep_UIState, BlueprintReadOnly) float StarboardReload = 0;
 UPROPERTY(ReplicatedUsing=OnRep_Fit,BlueprintReadOnly) FVTLoadoutSelection Fit;
 UFUNCTION() void OnRep_Fit();
 UFUNCTION() void OnRep_ClassId();
 UPROPERTY(ReplicatedUsing=OnRep_Autopilot,BlueprintReadOnly) bool Autopilot=false;
 UFUNCTION() void OnRep_Autopilot();
 bool ApplyFit(const FVTLoadoutSelection& Selected,bool Refill);
 FVTShipDefinition Definition;
 FVTPilotIntent Intent;
 TArray<FVTPilotIntent> InputQueue;
 uint32 LastReceived = 0;
 double LastInputTime = 0;
 UPROPERTY(ReplicatedUsing=OnRep_UIState, BlueprintReadOnly) bool IsNPC = false;
 bool Invulnerable = false;
 bool Anchored = false;
 UPROPERTY(ReplicatedUsing=OnRep_UIState,BlueprintReadOnly) int32 ShipRole=0;
 FVTBrainState Brain;
 UPROPERTY(ReplicatedUsing=OnRep_UIState,BlueprintReadOnly) float DockProgress=0;
 UPROPERTY(ReplicatedUsing=OnRep_UIState,BlueprintReadOnly) float JumpProgress=0;
 UPROPERTY(ReplicatedUsing=OnRep_UIState,BlueprintReadOnly) bool JumpEntering=false;
 UPROPERTY(ReplicatedUsing=OnRep_UIState,BlueprintReadOnly) FName JumpDestination;
 virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return Abilities; }
 virtual void PossessedBy(AController* NewController) override;
 virtual void OnRep_Controller() override;
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
 virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
 UFUNCTION(Server, Unreliable) void ServerIntent(FVTPilotIntent Value);
 UFUNCTION(Server,Unreliable) void ServerIntentBatch(const TArray<FVTPilotIntent>& Values);
 void InitializeShip(FName ShipId, int32 System, const FVTMotion& Initial);
};

UCLASS()
class VOIDANDTHUNDER_API AVTShipAI : public AAIController {
 GENERATED_BODY()
public:
 AVTShipAI();
 static void CrewStep(AVTShip* Ship);
 void Decide(float Dt);
 static void DecideShip(AVTShip* Ship,float Dt);
};

UCLASS()
class VOIDANDTHUNDER_API AVTPlayerState : public APlayerState {
 GENERATED_BODY()
public:
 UFUNCTION() void OnRep_UIState();
 UPROPERTY(ReplicatedUsing=OnRep_UIState, BlueprintReadOnly) int32 Credits = 0;
 UPROPERTY(ReplicatedUsing=OnRep_UIState, BlueprintReadOnly) int32 Boarded = 0;
 UPROPERTY(ReplicatedUsing=OnRep_UIState, BlueprintReadOnly) FGuid Profile;
 UPROPERTY(ReplicatedUsing=OnRep_UIState, BlueprintReadOnly) TArray<float> Reputation;
 UPROPERTY(ReplicatedUsing=OnRep_UIState, BlueprintReadOnly) TArray<float> Heat;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
};

UCLASS()
class VOIDANDTHUNDER_API AVTGameState : public AGameStateBase {
 GENERATED_BODY()
public:
 UFUNCTION() void OnRep_UIState();
 UPROPERTY(ReplicatedUsing=OnRep_UIState, BlueprintReadOnly) double SimulationTime = 0;
 UPROPERTY(ReplicatedUsing=OnRep_UIState, BlueprintReadOnly) TArray<int32> Populations;
 UPROPERTY(ReplicatedUsing=OnRep_UIState,BlueprintReadOnly) int32 Wave=0;
 UPROPERTY(ReplicatedUsing=OnRep_UIState,BlueprintReadOnly) int32 EnemiesRemaining=0;
 UPROPERTY(ReplicatedUsing=OnRep_UIState,BlueprintReadOnly) FString Outcome;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
};

UCLASS()
class VOIDANDTHUNDER_API AVTCameraManager : public APlayerCameraManager {
 GENERATED_BODY()
public:
 virtual void UpdateViewTarget(FTViewTarget& OutVT,float DeltaTime) override;
 bool RigReady=false;
 FGuid RigShip; int32 RigSystem=-1;
 double LastCameraReal=0,OrbitYaw=0,OrbitPitch=0,OrbitDistance=0,OrbitFov=0,FreeYaw=0,FreePitch=0,LookIdle=0,MenuOrbit=0;
 FVector Focus=FVector::ZeroVector,ImpactKick=FVector::ZeroVector;
 FVector2D LastCursor=FVector2D::ZeroVector;
 void AddImpactKick(const FVector& Direction,float Magnitude);

};

UCLASS()
class VOIDANDTHUNDER_API AVTController : public APlayerController {
 GENERATED_BODY()
public:
 AVTController();
 UPROPERTY() TObjectPtr<class UVTUI> UI;
 void ToggleMenu();
 void ToggleAutopilot();
 UFUNCTION(Server,Reliable,BlueprintCallable) void ServerSetAutopilot(bool Enabled);
 virtual void PawnLeavingGame() override;
 virtual void BeginPlay() override;
 virtual void SetupInputComponent() override;
 void ReadFlight(const FInputActionValue& Value, int32 Index);
 void CancelFlight(const FInputActionValue& Value,int32 Index);
 virtual void PlayerTick(float Dt) override;
 UPROPERTY() TObjectPtr<UInputMappingContext> FlightMapping;
 UPROPERTY() TObjectPtr<UInputMappingContext> CommonMapping;
 UPROPERTY() TObjectPtr<UInputMappingContext> MenuMapping;
 UPROPERTY() TObjectPtr<UInputMappingContext> DockedMapping;
 int32 InputContextState=-1;
 void UpdateInputContexts();
 TWeakObjectPtr<class USceneComponent> AudioListenerRoot;
 UFUNCTION(BlueprintCallable,Category="Input") bool RemapControl(FName MappingName,FKey NewKey);
 UPROPERTY() TArray<TObjectPtr<UInputAction>> Actions;
 FVTPilotIntent LocalIntent;
 float BroadsideOffset=0;
 bool UpdateBroadsideAim(float MouseDelta,const FVTFeelControls& Controls,float Heading,float Arc);
 uint32 NextSequence = 0;
 float AimBattery=5, AimDilation=1, HitStop=0, CameraTrauma=0;
 double LastRealTick=0;
 bool UsingGamepadAim=false;
 FVector2D GamepadAim=FVector2D::ZeroVector;
 int32 InteractionProbeStage=0,InteractionProbeBoarded=0;
 bool InteractionProbePrompt=false,InteractionProbeLooted=false;
 FKey InteractionProbeKey;
 int32 FlightProbeStage=0;bool FlightProbeHeld=false,FlightProbePassed=false;FVector2D FlightProbeOrigin;
 int32 BroadsideProbeStage=0;
 bool BroadsideProbeHeld=false,BroadsideProbePassed=false;
 bool ProbeJumped=false;
 void ValidationInput(float Dt);
 float SendAccumulator = 0;
 UFUNCTION(Client,Reliable) void ClientIdentify(FGuid World);
 UFUNCTION(Server,Reliable) void ServerIdentify(FGuid Profile,FGuid Token,FName Hull,FVTLoadoutSelection Selection);
 FName InitialHull;
 FVTLoadoutSelection InitialFit;
 UFUNCTION(Client,Reliable) void ClientAcceptIdentity(FGuid World,FGuid Token);
 UFUNCTION(Server,Reliable,BlueprintCallable) void ServerStationAction(FName Action);
 UFUNCTION(Server,Reliable,BlueprintCallable) void ServerRefit(FName Hull,FVTLoadoutSelection Selection);
 void ToggleHUD(bool Chart);
 UFUNCTION(Exec) void VTRecover();
 UFUNCTION(Server,Reliable) void ServerRecover();
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
 UPROPERTY() TArray<TObjectPtr<AVTProjectile>> Projectiles;
 TArray<TArray<AVTShip*>> SystemShips;
 // Broad phase only: Actors and their components remain the gameplay owners.
 TArray<TMap<FIntPoint,TArray<AVTShip*>>> ShipCells;
 float LargestShipRadius=0;
 void RebuildShipCells();
 void QueryShips(int32 System,const FVector2D& Min,const FVector2D& Max,TArray<AVTShip*>& Result) const;
 FVector2D JumpPosition(int32 System,FName Destination) const;
 void TravelShip(AVTShip* Ship,int32 Destination);
 void RecordHit(AVTShip* Victim,AVTShip* Attacker,float Amount,FGuid Profile=FGuid(),FName AttackerFaction=NAME_None);
 void AwardAvenging(AVTShip* Destroyed,const TArray<TObjectPtr<AVTShip>>& Survivors);
 void WorldStep();
 void CreateAnchors();
 void ScenarioStep();
 bool SoloSpawned=false;
 uint32 DirectorSeed=12345;
 const FVTScenarioDefinition* ActiveScenario() const;
 bool BehaviorHostile(AVTShip* Mine,AVTShip* Other) const;
 void ProjectileStep();
 void ContactStep();
 bool Occluded(int32 System,const FVector2D& A,const FVector2D& B,float Height=0) const;
 void LandmarkStep();
 void PiracyStep();
 TArray<double> StepMilliseconds;
 double PhaseTotals[8]={};
 TArray<double> RenderFrameMilliseconds;
 double LastRenderFrame=0;
 bool ScreenshotRequested=false;
 bool UIProbeStarted=false,UIProbeFinished=false,UIInitialFocus=false,UINavigationPassed=false;
 double Accumulator = 0;
 double SimulationTime = 0;
 int32 WorldSeed=12345;
 bool Bootstrapped = false;
 bool ProbeLoaded=false;
 bool ProbeWrote=false, ProbeScaled=false, ProbeMoved=false, ProbeOriginSet=false;
 int32 MaxPlayersObserved=0;
 FVector2D ProbeOrigin;
 void ValidationTick();
 void SoakTick(const FString& ProbeRole);
 double SoakNextSave=300,SoakNextSample=60,SoakFirstTime=0;
 int32 SoakSnapshots=0,SoakInitialNPCs=0;
 bool SoakFailed=false;
 TArray<FString> SoakSamples;
 TSet<int32> SoakSystemsAdvanced;
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Tick(float Dt) override;
 virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UVTSimulation, STATGROUP_Tickables); }
 virtual bool DoesSupportWorldType(EWorldType::Type Type) const override {return Type == EWorldType::Game || Type == EWorldType::PIE;}
 void Bootstrap(int32 Population = -1,bool Synthetic=true);
 void FixedStep();
 void ConfigurePopulationFixture(int32 Count,bool Busy,bool Armed);
 AVTShip* SpawnShip(FName Id, int32 System, const FVTMotion& Motion, bool NPC, FName Faction);
};

UCLASS()
class VOIDANDTHUNDER_API AVTGameMode : public AGameModeBase {
 GENERATED_BODY()
public:
 AVTGameMode();
 virtual bool SetPause(APlayerController* PC,FCanUnpause CanUnpauseDelegate=FCanUnpause()) override;
 virtual void BeginPlay() override;
 virtual void PostLogin(APlayerController* NewPlayer) override;
 virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
 virtual void PreLogin(const FString& Options,const FString& Address,const FUniqueNetIdRepl& UniqueId,FString& ErrorMessage) override;
 virtual void Logout(AController* Exiting) override;
 virtual void RestartPlayer(AController* Player) override;
 void RecoverShip(AVTShip* Ship);
};

UCLASS()
class VOIDANDTHUNDER_API UVTGameInstance : public UGameInstance {
 GENERATED_BODY()
public:
 #if WITH_EDITOR
 void InitializeHeadlessWorld(UWorld* World);
#endif
 UPROPERTY(BlueprintReadOnly) FString PlayMode=TEXT("sandbox");
 UPROPERTY(BlueprintReadOnly) bool ContinueWorld=false;
 UPROPERTY(BlueprintReadWrite) FName PopulationProfile=TEXT("Shared sandbox");
 int32 NewWorldPopulation=-1;
 bool ValidationSessionStarted=false,ValidationDiscoveryStarted=false;
 UPROPERTY(BlueprintReadWrite) FName SelectedHull=TEXT("corsair_cruiser");
 UPROPERTY(BlueprintReadWrite) FVTLoadoutSelection SelectedFit;
 UFUNCTION(BlueprintCallable) void StartSolo(const FString& Mode);
 UFUNCTION(BlueprintCallable) void ReturnToMenu();
 virtual void Init() override;
 virtual void OnStart() override;
 FDelegateHandle NetworkFailureHandle;
 void NetworkFailed(UWorld* World,UNetDriver* Driver,ENetworkFailure::Type Type,const FString& Message);
 virtual void Shutdown() override;
 UFUNCTION(BlueprintCallable) void Host();
 UFUNCTION(BlueprintCallable) void Join(const FString& Address);
};
