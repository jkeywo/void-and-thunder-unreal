#include "VTWorldAnchor.h"
#include "VTGameplay.h"
#include "VTGate.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
AVTWorldAnchor::AVTWorldAnchor() {PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.bStartWithTickEnabled=false;bReplicates=true; SetReplicateMovement(true); Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Landmark")); RootComponent=Mesh;GateStartArrow=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateStartArrow"));GatePreviewArrow=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GatePreviewArrow"));for(auto* Arrow:{GateStartArrow.Get(),GatePreviewArrow.Get()}){Arrow->SetupAttachment(Mesh);Arrow->SetCollisionEnabled(ECollisionEnabled::NoCollision);Arrow->SetCanEverAffectNavigation(false);Arrow->SetCastShadow(false);Arrow->SetVisibility(false);} Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetCanEverAffectNavigation(false); Mesh->SetCastShadow(false); Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));}
void AVTWorldAnchor::BeginPlay() {Super::BeginPlay();SetActorTickEnabled(!IsRunningCommandlet()&&FApp::CanEverRender());
 if(Kind==2&&!IsRunningCommandlet()&&FApp::CanEverRender()) {
  auto* ArrowMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Environment/SM_GateArrow.SM_GateArrow"));auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Environment/M_Jump.M_Jump"));
  for(auto* Arrow:{GateStartArrow.Get(),GatePreviewArrow.Get()}){Arrow->SetStaticMesh(ArrowMesh);Arrow->SetMaterial(0,Material);}
  SetActorTickEnabled(true);
 }
 if(Kind==2&&!Destination.IsNone()){Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Environment/SM_JumpRing.SM_JumpRing")));Mesh->SetRelativeScale3D(FVector(Radius));auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();const auto Axis=Sim->JumpPosition(System,Destination).GetSafeNormal();SetActorRotation(FRotator(0,-FMath::RadiansToDegrees(FMath::Atan2(Axis.Y,Axis.X)),0));}else Mesh->SetRelativeScale3D(FVector(Radius*2)); FString Material=Kind==0 ? TEXT("M_Star") : Kind==1 ? TEXT("M_Station") : TEXT("M_Jump"); Mesh->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Environment/%s.%s"),*Material,*Material)));}
void AVTWorldAnchor::Tick(float DeltaTime) {
 Super::Tick(DeltaTime);auto* PC=GetWorld()->GetFirstPlayerController();auto* Ship=PC&&PC->IsLocalController()?Cast<AVTShip>(PC->GetPawn()):nullptr;auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();auto* D=Sim?Sim->Data.Get():nullptr;
 const bool Local=Ship&&Ship->SystemIndex==System;Mesh->SetVisibility(Local||(Kind==0&&!UVTIntroComponent::IsArena(Sim,System)));if(Kind!=2)return;
 const auto Centre=D?Sim->JumpPosition(System,Destination):FVector2D::ZeroVector;
 const bool Nearby=Ship&&D&&Ship->SystemIndex==System&&(Ship->Movement->Motion.Position-Centre).SizeSquared()<FMath::Square(D->GateInteractionRange()+D->GateStartDistance());
 GateStartArrow->SetVisibility(Nearby);GatePreviewArrow->SetVisibility(false);if(!Nearby)return;
 const auto Axis=Centre.GetSafeNormal();const FRotator Outward(0,-FMath::RadiansToDegrees(FMath::Atan2(Axis.Y,Axis.X)),0);
 GateStartArrow->SetWorldLocation(VT::ToWorld(Centre-Axis*D->GateStartDistance(),System)+FVector(0,0,-100));GateStartArrow->SetWorldRotation(Outward);GateStartArrow->SetWorldScale3D(FVector(D->GateMarkerSize));
 FVTMotion Preview;const bool Visible=VTGate::Preview(Preview,Centre,D->GateStartDistance(),D->GateArrivalFraction(),D->GatePassageAcceleration,D->GateArrivalDuration,D->GateFlashDuration,D->GateMarkerPause,GetWorld()->GetTimeSeconds());
 GatePreviewArrow->SetVisibility(Visible);GatePreviewArrow->SetWorldLocation(VT::ToWorld(Preview.Position,System)+FVector(0,0,-80));GatePreviewArrow->SetWorldRotation(FRotator(0,-FMath::RadiansToDegrees(Preview.Heading),0));GatePreviewArrow->SetWorldScale3D(FVector(D->GateMarkerSize*0.65f));
}
void AVTWorldAnchor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AVTWorldAnchor,System); DOREPLIFETIME(AVTWorldAnchor,Kind); DOREPLIFETIME(AVTWorldAnchor,Destination); DOREPLIFETIME(AVTWorldAnchor,Radius);}
bool AVTWorldAnchor::IsNetRelevantFor(const AActor* RealViewer,const AActor* ViewTarget,const FVector& SrcLocation) const {auto* PC=Cast<APlayerController>(RealViewer); auto* Ship=PC ? Cast<AVTShip>(PC->GetPawn()) : Cast<AVTShip>(ViewTarget); return Ship&&(Ship->SystemIndex==System||(Kind==0&&!UVTIntroComponent::IsArena(GetWorld()->GetSubsystem<UVTSimulation>(),System)));}
void UVTSimulation::CreateAnchors() {
 if(GetWorld()->GetNetMode()==NM_Client||GetWorld()->GetMapName().Contains(TEXT("Menu"))) return;
 for(int I=0;I<Data->Systems.Num();++I) {
  if(ActiveScenario()&&I!=Data->FindSystem(Data->StartSystem)) continue;
  auto Spawn=[&](int Kind,FVector2D P,FName Destination,float Radius) {auto Transform=FTransform(VT::ToWorld(P,I)); auto* A=GetWorld()->SpawnActorDeferred<AVTWorldAnchor>(AVTWorldAnchor::StaticClass(),Transform); A->System=I; A->Kind=Kind; A->Radius=Radius; A->Destination=Destination; A->FinishSpawning(Transform);};
  for(const auto& Body:Data->Landmarks) Spawn(Body.Kind,Body.Position,NAME_None,Body.Radius);
  if(Data->Systems[I].HasStation) Spawn(1,Data->Rules.StationPosition,NAME_None,Data->Rules.StationRadius);
  for(FName Link:Data->Systems[I].Links) Spawn(2,JumpPosition(I,Link),Link,Data->GateOpeningRadius);
 }
}

AVTSky::AVTSky() {
 PrimaryActorTick.bCanEverTick=true;
 auto* Sphere=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sky")); RootComponent=Sphere; Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision); Sphere->SetCastShadow(false); Sphere->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"))); Sphere->SetRelativeScale3D(FVector(10000000)); Sphere->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Environment/M_SpaceSky.M_SpaceSky")));
}
void AVTSky::Tick(float DeltaTime) {Super::Tick(DeltaTime); if(auto* PC=GetWorld()->GetFirstPlayerController()) if(PC->PlayerCameraManager) SetActorLocation(PC->PlayerCameraManager->GetCameraLocation());}

bool VTGrid::NearestStar(const TArray<FVTLandmarkDefinition>& Landmarks,const FVector2D& Position,FVector2D& Centre) {
 bool Found=false;double Best=DBL_MAX;
 for(const auto& Body:Landmarks)if(Body.Kind==0){double Distance=(Body.Position-Position).SizeSquared();if(Distance<Best){Best=Distance;Centre=Body.Position;Found=true;}}
 return Found;
}
void AVTReferenceGrid::BeginPlay(){Super::BeginPlay();auto* Plane=CastChecked<UStaticMeshComponent>(RootComponent);GridMaterial=Plane->CreateDynamicMaterialInstance(0);}
AVTReferenceGrid::AVTReferenceGrid() {
 PrimaryActorTick.bCanEverTick=true; auto* Plane=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReferenceGrid")); RootComponent=Plane;
 Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision); Plane->SetCastShadow(false); Plane->SetCanEverAffectNavigation(false);
 Plane->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"))); Plane->SetRelativeScale3D(FVector(6000,6000,1));
 Plane->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Environment/M_ReferenceGrid.M_ReferenceGrid")));
}
void AVTReferenceGrid::Tick(float DeltaTime) {
 Super::Tick(DeltaTime); auto* PC=GetWorld()->GetFirstPlayerController(); auto* Ship=PC ? Cast<AVTShip>(PC->GetPawn()) : nullptr;
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();FVector2D Centre;
 const bool HasStar=Ship&&Sim&&Sim->Data&&VTGrid::NearestStar(Sim->Data->Landmarks,Ship->Movement->Motion.Position,Centre);
 SetActorHiddenInGame(!HasStar);
 if(HasStar){SetActorLocation(FVector(Ship->GetActorLocation().X,Ship->GetActorLocation().Y,-900));const auto WorldCentre=VT::ToWorld(Centre,Ship->SystemIndex);if(GridMaterial&&!WorldCentre.Equals(LastCentre)){GridMaterial->SetVectorParameterValue(TEXT("StarCentre"),FLinearColor(WorldCentre.X,WorldCentre.Y,0,0));LastCentre=WorldCentre;}}
}
