#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "VTTypes.h"
#include "VTIntro.generated.h"
class AVTShip; class UVTSimulation;
UENUM(BlueprintType)
enum class EVTIntroStage:uint8 { Complete, Wake, Helm, Debris, Repairs, Challenge, Duel, Salvage, Gate, Handoff, BatteryChoice, BatteryTrial, SpecialChoice, SpecialTrial };
USTRUCT(BlueprintType)
struct FVTIntroProgress {
 GENERATED_BODY()
 // Missing property in an existing schema-6 campaign means already introduced.
 UPROPERTY(BlueprintReadOnly) EVTIntroStage Stage=EVTIntroStage::Complete;
 UPROPERTY() float Elapsed=0;
 UPROPERTY() float Distance=0;
 UPROPERTY() float Turn=0;
 UPROPERTY(BlueprintReadOnly) FName BatteryChoice;
 UPROPERTY(BlueprintReadOnly) FName SpecialChoice;
 UPROPERTY() float DeviceSeconds=0;
};
USTRUCT(BlueprintType)
struct FVTIntroBeat {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) EVTIntroStage Stage=EVTIntroStage::Wake;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool Enemy=false;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FText Speech;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FText Objective;
};
USTRUCT(BlueprintType)
struct FVTIntroRepair {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Equipment;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FText Label;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FText Trial;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName InputAction;
};
UCLASS(BlueprintType)
class VOIDANDTHUNDER_API UVTIntroData:public UPrimaryDataAsset {
 GENERATED_BODY()
public:
 UVTIntroData();
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FVTIntroBeat> Beats;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FVTIntroRepair> BatteryRepairs;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FVTIntroRepair> SpecialRepairs;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TObjectPtr<class UTexture2D> EngineerPortrait;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TObjectPtr<class UTexture2D> CaptainPortrait;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVector2D Start=FVector2D(-620,-500);
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVector2D ClearPoint=FVector2D(-250,-500);
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVector2D DebrisPoint=FVector2D(-220,-260);
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVector2D EnemyPoint=FVector2D(350,-500);
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float HelmDistance=250;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float HelmTurn=0.35f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float WaypointRadius=110;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float RepairSeconds=8;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float InitialHullFraction=0.45f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float EnemyHullFraction=0.45f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float EnemyDamageScale=0.35f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float DebrisHull=20;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float DebrisRadius=45;
 const FVTIntroBeat* Beat(EVTIntroStage Stage) const;
};
UCLASS(ClassGroup=(Gameplay),meta=(BlueprintSpawnableComponent))
class VOIDANDTHUNDER_API UVTIntroComponent:public UActorComponent {
 GENERATED_BODY()
public:
 UVTIntroComponent();
 UPROPERTY(ReplicatedUsing=OnRep_Progress,BlueprintReadOnly) FVTIntroProgress Progress;
 UPROPERTY(Replicated,BlueprintReadOnly) FVector2D ObjectivePosition=FVector2D::ZeroVector;
 UPROPERTY(Replicated,BlueprintReadOnly) bool HasWaypoint=false;
 UPROPERTY() TObjectPtr<UVTIntroData> Data;
 TWeakObjectPtr<AVTShip> Target;
 int32 Arena=INDEX_NONE;
 bool Active() const {return Progress.Stage!=EVTIntroStage::Complete;}
 bool InArena() const {return Active()&&Progress.Stage!=EVTIntroStage::Handoff;}
 bool WeaponsOnline() const {return !Active()||Progress.Stage>=EVTIntroStage::Debris;}
 bool SystemsOnline() const {return !Active()||Progress.Stage>=EVTIntroStage::Repairs;}
 bool JumpOnline() const {return !Active()||Progress.Stage==EVTIntroStage::Gate||Progress.Stage==EVTIntroStage::Handoff;}
 bool CanAdvance() const;
 const TArray<FVTIntroRepair>* RepairOptions() const;
 const FVTIntroRepair* CurrentRepair() const;
 UFUNCTION(Server,Reliable) void ServerChooseRepair(int32 Option);
 void Start();
 void Restore(FVTIntroProgress Saved);
 void FixedStep();
 void Filter(FVTPilotIntent& Intent) const;
 void Retry();
 void Cleanup();
 void Change(EVTIntroStage Stage);
 UFUNCTION() void OnRep_Progress();
 UFUNCTION(Server,Reliable) void ServerAdvance();
 UFUNCTION(Server,Reliable) void ServerSkip();
 UFUNCTION(Client,Reliable) void ClientResetHelm();
 UFUNCTION(Client,Reliable) void ClientKeepFit(FName Hull,const FVTLoadoutSelection& Fit);
 virtual void BeginPlay() override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
 static UVTIntroComponent* For(const AVTShip* Ship);
 static bool IsArena(const UVTSimulation* Sim,int32 System);
 static void PrepareArenas(UVTSimulation* Sim);
private:
 void Rebuild();
 AVTShip* SpawnFixture(const FVector2D& Position,bool Debris,bool Decorative=false);
};
