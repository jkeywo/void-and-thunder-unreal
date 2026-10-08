#pragma once
#include "Components/ActorComponent.h"
#include "VTTypes.h"
#include "VTGatePassage.generated.h"
struct FVTSavedShip;
UENUM(BlueprintType) enum class EVTGatePhase:uint8 {Idle,Staging,Departing,Arriving};
USTRUCT(BlueprintType) struct FVTGateState {
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) EVTGatePhase Phase=EVTGatePhase::Idle;
 UPROPERTY(BlueprintReadOnly) FName Destination;
 UPROPERTY(BlueprintReadOnly) float Charge=0;
 UPROPERTY(BlueprintReadOnly) double ArrivalStarted=0;
 UPROPERTY(BlueprintReadOnly) FVector2D ArrivalTarget=FVector2D::ZeroVector;
};
UCLASS(ClassGroup=(VoidThunder),meta=(BlueprintSpawnableComponent))
class VOIDANDTHUNDER_API UVTGatePassage:public UActorComponent {
 GENERATED_BODY()
 UPROPERTY(ReplicatedUsing=OnRep_State) FVTGateState State;
 UFUNCTION() void OnRep_State();
 void ResetTransient();
public:
 UVTGatePassage();
 const FVTGateState& Status() const{return State;}
 bool Arriving() const{return State.Phase==EVTGatePhase::Arriving;}
 bool Departing() const{return State.Phase==EVTGatePhase::Departing;}
 void InteractionStep();
 bool Integrate(FVTMotion& Motion,const FVTShipStats& Stats,FVTPilotIntent& Intent,bool Predict);
 void BeginArrival(const FVector2D& Origin,double Time);
 void Capture(FVTSavedShip& Record) const;
 void Restore(const FVTSavedShip& Record,double ResumeTime);
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Props) const override;
};
