#pragma once
#include "Engine/DataAsset.h"
#include "VTTypes.h"
#include "VTGameData.generated.h"
UCLASS(BlueprintType)
class VOIDANDTHUNDER_API UVTGameData : public UPrimaryDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FVTShipDefinition> Ships;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FVTSystemDefinition> Systems;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FVTRules Rules;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FName StartSystem = "the_scar";
 const FVTShipDefinition* FindShip(FName Id) const { return Ships.FindByPredicate([Id](const FVTShipDefinition& S) {return S.Id == Id;}); }
 int32 FindSystem(FName Id) const { return Systems.IndexOfByPredicate([Id](const FVTSystemDefinition& S) {return S.Id == Id;}); }
};
