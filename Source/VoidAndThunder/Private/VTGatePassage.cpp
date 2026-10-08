#include "VTGatePassage.h"
#include "VTGameplay.h"
#include "VTGate.h"
#include "VTCombat.h"
#include "VTSaveSubsystem.h"
#include "Net/UnrealNetwork.h"
UVTGatePassage::UVTGatePassage(){SetIsReplicatedByDefault(true);PrimaryComponentTick.bCanEverTick=false;}
void UVTGatePassage::OnRep_State(){VTNotifyHUD(GetWorld());}
void UVTGatePassage::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(UVTGatePassage,State);}
void UVTGatePassage::ResetTransient(){auto* S=CastChecked<AVTShip>(GetOwner());auto* M=S->Movement.Get();M->Previous=M->Authority=M->Motion;M->Pending.Reset();M->ReplicaFrames.Reset();M->RenderCorrection=FVector2D::ZeroVector;M->RenderHeadingCorrection=0;S->Intent=FVTPilotIntent();S->InputQueue.Reset();S->Combat->EquipmentState.Locks.Reset();S->Combat->EquipmentState.LockElapsed=0;S->Combat->BoardingTarget.Invalidate();S->Combat->BoardingProgress=0;S->Combat->TorpedoHeld=S->Combat->WarpHeld=false;S->DockProgress=0;}
void UVTGatePassage::BeginArrival(const FVector2D& Origin,double Time){auto* S=CastChecked<AVTShip>(GetOwner());if(!S->HasAuthority())return;auto* D=GetWorld()->GetSubsystem<UVTSimulation>()->Data.Get();State={};State.Phase=EVTGatePhase::Arriving;State.ArrivalStarted=Time;State.ArrivalTarget=Origin*0.85;VTGate::Arrive(S->Movement->Motion,Origin,State.ArrivalTarget,0,D->GateArrivalDuration);ResetTransient();S->ForceNetUpdate();}
void UVTGatePassage::Capture(FVTSavedShip& R) const{R.JumpProgress=State.Charge;R.JumpDestination=State.Destination;R.JumpArriving=Arriving();R.JumpArrivalStarted=State.ArrivalStarted;R.JumpArrivalTarget=State.ArrivalTarget;}
void UVTGatePassage::Restore(const FVTSavedShip& R,double Time){auto* S=CastChecked<AVTShip>(GetOwner());State={};State.Charge=R.JumpProgress;State.Destination=R.JumpDestination;State.ArrivalTarget=R.JumpArrivalTarget;State.ArrivalStarted=R.JumpArrivalStarted;if(R.JumpArriving){State.Phase=EVTGatePhase::Arriving;auto* D=GetWorld()->GetSubsystem<UVTSimulation>()->Data.Get();State.ArrivalStarted=Time-FMath::Clamp(R.Motion.SimulationTime-R.JumpArrivalStarted,0.,double(D->GateArrivalDuration));}ResetTransient();S->ForceNetUpdate();}
bool UVTGatePassage::Integrate(FVTMotion& Motion,const FVTShipStats& Stats,FVTPilotIntent& Intent,bool Predict){auto* S=CastChecked<AVTShip>(GetOwner());auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();auto* D=Sim->Data.Get();if(!D||S->Docked||S->Anchored)return false;
 if(Arriving()){const double Time=Predict?Motion.SimulationTime+VT::Step:Sim->SimulationTime;const bool Done=VTGate::Arrive(Motion,State.ArrivalTarget/0.85,State.ArrivalTarget,Time-State.ArrivalStarted,D->GateArrivalDuration);if(Done&&!Predict)State.Phase=EVTGatePhase::Idle;return true;}
 if(S->IsNPC||S->Autopilot||!(Intent.Buttons&VTButtons::Interact)||S->Combat->BoardingTarget.IsValid()||!D->Systems.IsValidIndex(S->SystemIndex))return false;
 FName Link=Departing()?State.Destination:NAME_None;double Best=FMath::Square(D->Rules.JumpRange);if(Link.IsNone())for(FName Candidate:D->Systems[S->SystemIndex].Links){double Distance=(Motion.Position-Sim->JumpPosition(S->SystemIndex,Candidate)).SizeSquared();if(Distance<Best){Best=Distance;Link=Candidate;}}
 if(Link.IsNone())return false;if(Departing()){VTGate::Depart(Motion,Sim->JumpPosition(S->SystemIndex,Link),D->GatePassageAcceleration,VT::Step);return true;}
 Intent=VTGate::Guide(Motion,Stats,Intent,Sim->JumpPosition(S->SystemIndex,Link),false,D->GateApproachDistance,D->GateCruiseSpeed,D->GateArrivalTolerance,D->Rules.ReverseThrottle);return false;
}
void UVTGatePassage::InteractionStep(){auto* S=CastChecked<AVTShip>(GetOwner());if(!S->HasAuthority()||S->IsNPC||S->Docked||S->Disabled||Arriving())return;auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();auto* D=Sim->Data.Get();const auto& System=D->Systems[S->SystemIndex];FName Destination;double Best=FMath::Square(D->Rules.JumpRange);for(FName Link:System.Links){double Distance=(S->Movement->Motion.Position-Sim->JumpPosition(S->SystemIndex,Link)).SizeSquared();if(Distance<Best){Destination=Link;Best=Distance;}}
 if(Destination!=State.Destination){State.Destination=Destination;State.Charge=0;State.Phase=EVTGatePhase::Idle;}
 const bool Held=!Destination.IsNone()&&(S->Intent.Buttons&VTButtons::Interact)&&!S->Combat->BoardingTarget.IsValid();if(!Held){State.Phase=EVTGatePhase::Idle;return;}
 State.Charge=FMath::Min(D->Rules.JumpDwell,State.Charge+VT::Step);if(!Departing())State.Phase=EVTGatePhase::Staging;
 const auto Centre=Sim->JumpPosition(S->SystemIndex,Destination),Normal=Centre.GetSafeNormal();const auto& M=S->Movement->Motion;const float Alignment=FVector2D::DotProduct(FVector2D(FMath::Cos(M.Heading),FMath::Sin(M.Heading)),Normal);
 if(!Departing()&&State.Charge>=D->Rules.JumpDwell&&(M.Position-(Centre-Normal*D->GateApproachDistance)).SizeSquared()<=FMath::Square(D->GateArrivalTolerance*1.5f)&&M.Velocity.SizeSquared()<FMath::Square(D->GateCruiseSpeed*0.3f)&&Alignment>=FMath::Cos(D->GateAlignmentTolerance))State.Phase=EVTGatePhase::Departing;
 if(Departing()&&VTGate::Crossed(S->Movement->Previous,M,Centre,D->GateOpeningRadius-S->Definition.Radius,D->GateAlignmentTolerance))Sim->TravelShip(S,D->FindSystem(Destination));
}

