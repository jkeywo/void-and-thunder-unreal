#pragma once
#include "GameFramework/Actor.h"
#include "VTTypes.h"
#include "VTWorldAnchor.generated.h"
namespace VTGrid { VOIDANDTHUNDER_API bool NearestStar(const TArray<FVTLandmarkDefinition>& Landmarks,const FVector2D& Position,FVector2D& Centre); }
UCLASS()
class VOIDANDTHUNDER_API AVTWorldAnchor : public AActor {
 GENERATED_BODY()
public:
 AVTWorldAnchor();
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
 UPROPERTY(Replicated,BlueprintReadOnly) int32 System=0;
 UPROPERTY(Replicated,BlueprintReadOnly) FName Destination;
 UPROPERTY(Replicated,BlueprintReadOnly) int32 Kind=0;
 UPROPERTY(Replicated,BlueprintReadOnly) float Radius=120;
 virtual void BeginPlay() override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 virtual bool IsNetRelevantFor(const AActor* RealViewer,const AActor* ViewTarget,const FVector& SrcLocation) const override;
};

UCLASS()
class VOIDANDTHUNDER_API AVTSky : public AActor {
 GENERATED_BODY()
public:
 AVTSky();
 virtual void Tick(float DeltaTime) override;
};

UCLASS()
class VOIDANDTHUNDER_API AVTReferenceGrid : public AActor {
 GENERATED_BODY()
public:
 AVTReferenceGrid();
 UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> GridMaterial;
 FVector LastCentre=FVector(DBL_MAX,DBL_MAX,DBL_MAX);
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaTime) override;
};
