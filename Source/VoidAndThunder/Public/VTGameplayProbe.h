#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "VTGameplayProbe.generated.h"
class AVTController;
class AVTShip;
UCLASS()
class VOIDANDTHUNDER_API AVTGameplayProbe : public AInfo {
 GENERATED_BODY()
public:
 AVTGameplayProbe();
 UPROPERTY(Replicated) int32 Phase=0;
 UPROPERTY(Replicated) TArray<FGuid> Profiles;
 UPROPERTY(Replicated) int32 StationSystem=-1;
 UPROPERTY(Replicated) int32 DestinationSystem=-1;
 UPROPERTY(Replicated) FString Failure;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 void ServerStep();
 void DriveLocal(AVTController* Controller);
 double PhaseStarted=0,Started=0,FinishAt=0;
 int32 SeenPhases=0,LastLocalPhase=-1;
 bool SentAction=false,BurstStarted=false,BurstEnded=false,Reported=false;
 TMap<FString,bool> Checks;
 TArray<TWeakObjectPtr<AVTController>> Captains;
 FGuid OldVictim,RefitVictim;
 TArray<int32> OtherSystems;
 TArray<FVector2D> MovementOrigins;
 FVector2D LocalMovementOrigin=FVector2D::ZeroVector;
 uint32 BurstEndAck=0,NetworkPilotAck=0;
 double InitialHull=0;
 int32 InitialPrizes=0,InitialCredits=0;
 void Enter(int32 Next);
 void Place(AVTShip* Ship,int32 System,const FVector2D& Position,float Heading=0);
 void ReportLocal(AVTController* Controller);
};
