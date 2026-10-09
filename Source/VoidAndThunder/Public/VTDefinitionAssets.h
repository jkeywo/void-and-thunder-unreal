#pragma once
#include "Engine/DataAsset.h"
#include "VTTypes.h"
#include "VTDefinitionAssets.generated.h"
UCLASS(BlueprintType) class VOIDANDTHUNDER_API UVTShipAsset : public UPrimaryDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTShipDefinition Definition;
 virtual FPrimaryAssetId GetPrimaryAssetId() const override {return FPrimaryAssetId(TEXT("VTShip"),GetFName());}
};
UCLASS(BlueprintType) class VOIDANDTHUNDER_API UVTEquipmentAsset : public UPrimaryDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTLoadoutOption Definition;
 virtual FPrimaryAssetId GetPrimaryAssetId() const override {return FPrimaryAssetId(TEXT("VTEquipment"),GetFName());}
};
UCLASS(BlueprintType) class VOIDANDTHUNDER_API UVTSystemAsset : public UPrimaryDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTSystemDefinition Definition;
 virtual FPrimaryAssetId GetPrimaryAssetId() const override {return FPrimaryAssetId(TEXT("VTSystem"),GetFName());}
};
UCLASS(BlueprintType) class VOIDANDTHUNDER_API UVTScenarioAsset : public UPrimaryDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTScenarioDefinition Definition;
 virtual FPrimaryAssetId GetPrimaryAssetId() const override {return FPrimaryAssetId(TEXT("VTScenario"),GetFName());}
};
