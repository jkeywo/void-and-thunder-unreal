#pragma once
#include "Engine/DataAsset.h"
#include "VTTypes.h"
#include "VTDefinitionAssets.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "VTGameData.generated.h"
UCLASS(BlueprintType)
class VOIDANDTHUNDER_API UVTGameData : public UPrimaryDataAsset {
 GENERATED_BODY()
public:
 virtual FPrimaryAssetId GetPrimaryAssetId() const override {return FPrimaryAssetId(TEXT("VTGameData"),GetFName());}
 UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(AssetBundles="Gameplay")) TArray<TSoftObjectPtr<UVTShipAsset>> ShipAssets;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(AssetBundles="Gameplay")) TArray<TSoftObjectPtr<UVTEquipmentAsset>> EquipmentAssets;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(AssetBundles="Gameplay")) TArray<TSoftObjectPtr<UVTSystemAsset>> SystemAssets;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(AssetBundles="Gameplay")) TArray<TSoftObjectPtr<UVTScenarioAsset>> ScenarioAssets;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(AssetBundles="Gameplay,Presentation")) TSoftClassPtr<class AVTShip> ShipClass;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,meta=(AssetBundles="Presentation")) TSoftClassPtr<class UVTUI> UIClass;
 void LoadCatalog();
 TSharedPtr<struct FStreamableHandle> CatalogHandle,PresentationHandle;
 bool CatalogLoaded=false;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TMap<FName,int32> PopulationProfiles={{FName("Authored"),-1},{FName("Shared sandbox"),500}};
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FVTShipDefinition> Ships;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FVTSystemDefinition> Systems;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FVTScenarioDefinition> Scenarios;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FVTLandmarkDefinition> Landmarks={{FVector2D::ZeroVector,120,0},{FVector2D(-700,500),60,3},{FVector2D(820,-420),44,3}};
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TMap<FName,TSoftObjectPtr<UStaticMesh>> FactionMeshes;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FVTRules Rules;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTAITuning AI;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTFeel Feel;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTWorldTuning World;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FVTLoadoutOption> Loadouts;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FName> TrackedFactions;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<float> InitialReputation;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FVTFactionRelation> Relations;
 int32 FactionIndex(FName Faction) const {return TrackedFactions.IndexOfByKey(Faction);}
 float StandingBetween(FName A,FName B) const;
 TArray<EVTDevice> CrewForFit(FName ClassId,const FVTLoadoutSelection& Fit) const;
 bool ResolveFit(FName ClassId,const FVTLoadoutSelection& Fit,FVTShipDefinition& Out) const;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FName StartSystem = "the_scar";
 #if WITH_EDITOR
 virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
 const FVTShipDefinition* FindShip(FName Id) const { return Ships.FindByPredicate([Id](const FVTShipDefinition& S) {return S.Id == Id;}); }
 int32 FindSystem(FName Id) const { return Systems.IndexOfByPredicate([Id](const FVTSystemDefinition& S) {return S.Id == Id;}); }
};
