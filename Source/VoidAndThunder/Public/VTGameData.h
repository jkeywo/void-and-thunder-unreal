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
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Flight",meta=(ClampMin="0.1",ClampMax="4")) float FlightSpeedMultiplier=2.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Presentation",meta=(ClampMin="1",ClampMax="20")) float ProjectileVisualRadius=7.f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Presentation",meta=(AssetBundles="Presentation")) TSoftObjectPtr<class UMaterialInterface> ProjectileMaterial= TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Environment/M_Projectile.M_Projectile")));
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel",meta=(ClampMin="50")) float GateOpeningRadius=80;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel",meta=(ClampMin="1")) float GateApproachDistance=80;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel",meta=(ClampMin="1",ClampMax="4")) float GateDistanceScale=2;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Presentation",meta=(ClampMin="1")) float GateMarkerSize=30;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Presentation",meta=(ClampMin="0.1")) float GateMarkerPause=0.6f;
 float GateStartDistance() const{return GateApproachDistance*GateDistanceScale;}
 float GateInteractionRange() const{return Rules.JumpRange*GateDistanceScale;}
 float GateArrivalFraction() const{return 0.15f*GateDistanceScale;}
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel",meta=(ClampMin="1")) float GateCruiseSpeed=45;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel",meta=(ClampMin="1")) float GatePassageAcceleration=1200;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel",meta=(ClampMin="0.05")) float GateArrivalDuration=0.35f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel",meta=(ClampMin="0.05")) float GateFlashDuration=0.3f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel",meta=(ClampMin="1")) float GateArrivalTolerance=12;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Travel",meta=(ClampMin="0.01",ClampMax="0.5")) float GateAlignmentTolerance=0.18f;
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
