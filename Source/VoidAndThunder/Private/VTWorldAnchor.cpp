#include "VTWorldAnchor.h"
#include "VTGameplay.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
AVTWorldAnchor::AVTWorldAnchor() {bReplicates=true; SetReplicateMovement(true); Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Landmark")); RootComponent=Mesh; Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetCanEverAffectNavigation(false); Mesh->SetCastShadow(false); Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));}
void AVTWorldAnchor::BeginPlay() {Super::BeginPlay(); Mesh->SetRelativeScale3D(FVector(Radius*2)); FString Material=Kind==0 ? TEXT("M_Star") : Kind==1 ? TEXT("M_Station") : TEXT("M_Jump"); Mesh->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Environment/%s.%s"),*Material,*Material)));}
void AVTWorldAnchor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AVTWorldAnchor,System); DOREPLIFETIME(AVTWorldAnchor,Kind); DOREPLIFETIME(AVTWorldAnchor,Destination); DOREPLIFETIME(AVTWorldAnchor,Radius);}
bool AVTWorldAnchor::IsNetRelevantFor(const AActor* RealViewer,const AActor* ViewTarget,const FVector& SrcLocation) const {auto* PC=Cast<APlayerController>(RealViewer); auto* Ship=PC ? Cast<AVTShip>(PC->GetPawn()) : Cast<AVTShip>(ViewTarget); return Ship&&Ship->SystemIndex==System;}
void UVTSimulation::CreateAnchors() {
 if(GetWorld()->GetNetMode()==NM_Client||GetWorld()->GetMapName().Contains(TEXT("Menu"))) return;
 for(int I=0;I<Data->Systems.Num();++I) {
  if(ActiveScenario()&&I!=Data->FindSystem(Data->StartSystem)) continue;
  auto Spawn=[&](int Kind,FVector2D P,FName Destination,float Radius) {auto Transform=FTransform(VT::ToWorld(P,I)); auto* A=GetWorld()->SpawnActorDeferred<AVTWorldAnchor>(AVTWorldAnchor::StaticClass(),Transform); A->System=I; A->Kind=Kind; A->Radius=Radius; A->Destination=Destination; A->FinishSpawning(Transform);};
  for(const auto& Body:Data->Landmarks) Spawn(Body.Kind,Body.Position,NAME_None,Body.Radius);
  if(Data->Systems[I].HasStation) Spawn(1,Data->Rules.StationPosition,NAME_None,Data->Rules.StationRadius);
  for(FName Link:Data->Systems[I].Links) Spawn(2,JumpPosition(I,Link),Link,25);
 }
}

AVTSky::AVTSky() {
 PrimaryActorTick.bCanEverTick=true;
 auto* Sphere=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sky")); RootComponent=Sphere; Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision); Sphere->SetCastShadow(false); Sphere->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"))); Sphere->SetRelativeScale3D(FVector(40000)); Sphere->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Environment/M_SpaceSky.M_SpaceSky")));
}
void AVTSky::Tick(float DeltaTime) {Super::Tick(DeltaTime); if(auto* PC=GetWorld()->GetFirstPlayerController()) if(PC->PlayerCameraManager) SetActorLocation(PC->PlayerCameraManager->GetCameraLocation());}

AVTReferenceGrid::AVTReferenceGrid() {
 PrimaryActorTick.bCanEverTick=true; auto* Plane=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReferenceGrid")); RootComponent=Plane;
 Plane->SetCollisionEnabled(ECollisionEnabled::NoCollision); Plane->SetCastShadow(false); Plane->SetCanEverAffectNavigation(false);
 Plane->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"))); Plane->SetRelativeScale3D(FVector(6000,6000,1));
 Plane->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Environment/M_ReferenceGrid.M_ReferenceGrid")));
}
void AVTReferenceGrid::Tick(float DeltaTime) {
 Super::Tick(DeltaTime); auto* PC=GetWorld()->GetFirstPlayerController(); auto* Ship=PC ? Cast<AVTShip>(PC->GetPawn()) : nullptr;
 if(Ship) SetActorLocation(VT::ArenaOrigin(Ship->SystemIndex)+FVector(0,0,-900));
}
