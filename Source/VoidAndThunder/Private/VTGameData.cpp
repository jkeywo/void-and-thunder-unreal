#include "VTFitEditor.h"
#include "VTGameData.h"
#include "VTGameplay.h"
#include "VTUI.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Misc/App.h"
#if WITH_EDITOR
EDataValidationResult UVTGameData::IsDataValid(FDataValidationContext& Context) const {
 bool Good=true;
 auto EffectiveShips=Ships;auto EffectiveLoadouts=Loadouts;auto EffectiveSystems=Systems;auto EffectiveScenarios=Scenarios;
 auto CheckDefinitions=[&](const auto& References,auto& Definitions) {
  TSet<FName> IDs;
  for(const auto& Ref:References) {
   auto* Asset=Ref.LoadSynchronous();
   if(!Asset||Asset->Definition.Id.IsNone()||IDs.Contains(Asset->Definition.Id)) {Good=false;Context.AddError(NSLOCTEXT("VTData","InvalidDefinition","Primary definitions must resolve and have unique, nonempty IDs."));continue;}
   IDs.Add(Asset->Definition.Id);auto* Existing=Definitions.FindByPredicate([&](const auto& D){return D.Id==Asset->Definition.Id;});
   if(Existing)*Existing=Asset->Definition;else Definitions.Add(Asset->Definition);
  }
 };
 CheckDefinitions(ShipAssets,EffectiveShips);CheckDefinitions(EquipmentAssets,EffectiveLoadouts);CheckDefinitions(SystemAssets,EffectiveSystems);CheckDefinitions(ScenarioAssets,EffectiveScenarios);
 auto EffectiveFindSystem=[&](FName Id){return EffectiveSystems.IndexOfByPredicate([Id](const auto& D){return D.Id==Id;});};
 auto Require=[&](bool Condition,const FString& Message) {if(!Condition) {Good=false; Context.AddError(FText::FromString(Message));}};
 Require(FMath::IsFinite(FlightSpeedMultiplier)&&FlightSpeedMultiplier>0&&FlightSpeedMultiplier<=4,TEXT("Flight speed multiplier must be in (0,4]."));
 Require(FMath::IsFinite(ProjectileVisualRadius)&&ProjectileVisualRadius>=1&&ProjectileVisualRadius<=20,TEXT("Projectile visual radius must be in [1,20]."));
 auto Nonnegative=[](float Value) {return FMath::IsFinite(Value)&&Value>=0;};
 Require(EffectiveLoadouts.Num()>=8&&EffectiveScenarios.Num()==2,TEXT("Native loadouts and both solo scenarios are required."));
 Require(TrackedFactions.Num()==InitialReputation.Num()&&Relations.Num()>=36,TEXT("Faction standings must match the migrated catalogue."));
 Require(FactionMeshes.Num()>=5,TEXT("Imported faction model references are required."));
 Require(EffectiveShips.Num()>=5,TEXT("The migrated ship catalogue must contain all five baseline classes."));
 Require(EffectiveSystems.Num()==10,TEXT("The baseline sandbox must contain all ten systems."));
 TSet<FName> ShipIDs,SystemIDs;
 for(const auto& Ship:EffectiveShips) {
  FString Prefix=Ship.Id.ToString()+TEXT(": ");
  Require(!Ship.Mesh.IsNull(),Prefix+TEXT("native hull mesh is required"));
  Require(Ship.Mounts>=1&&Ship.Crewed>=1&&Ship.Crewed<=Ship.Mounts,Prefix+TEXT("mount/crew allocation is invalid"));
  const auto& E=Ship.Equipment; Require(E.Tubes>=0&&E.Tubes<=6&&E.TorpedoMagazine>=0&&E.MineMagazine>=0&&Nonnegative(E.EMPDrain)&&Nonnegative(E.PDDrain)&&Nonnegative(E.BoostDrain),Prefix+TEXT("equipment ammunition or shared costs are invalid"));
  Require(!Ship.Id.IsNone()&&!ShipIDs.Contains(Ship.Id),Prefix+TEXT("ship ID must be nonempty and unique")); ShipIDs.Add(Ship.Id);
  Require(FMath::IsFinite(Ship.Hull)&&Ship.Hull>0&&FMath::IsFinite(Ship.Radius)&&Ship.Radius>0,Prefix+TEXT("hull and collision radius must be positive and finite"));
  const auto& S=Ship.Stats;
  Require(Nonnegative(S.Thrust)&&Nonnegative(S.TurnRate)&&Nonnegative(S.MaxSpeed)&&Nonnegative(S.ForwardDrag)&&Nonnegative(S.LateralDrag)&&Nonnegative(S.TurnRateSlow)&&Nonnegative(S.TurnRateFast)&&Nonnegative(S.TurnAccel),Prefix+TEXT("movement values must be finite and nonnegative"));
  Require(Nonnegative(Ship.Damage)&&FMath::IsFinite(Ship.Reload)&&Ship.Reload>0&&FMath::IsFinite(Ship.MuzzleSpeed)&&Ship.MuzzleSpeed>0&&Nonnegative(Ship.ChargeTime)&&Nonnegative(Ship.Arc)&&Ship.Arc<=PI&&Ship.Guns>=1,Prefix+TEXT("broadside fit is invalid"));
  Require(Nonnegative(Ship.BatteryMax)&&Nonnegative(Ship.BatteryRecharge)&&Nonnegative(Ship.ShieldRegen)&&Nonnegative(Ship.ShieldDelay),Prefix+TEXT("battery or shield tuning is invalid"));
  Require(Ship.ShieldArcs==1||Ship.ShieldArcs==2||Ship.ShieldArcs==4,Prefix+TEXT("shield arcs must be 1, 2 or 4"));
  for(int I=0;I<4;++I) Require(Nonnegative(Ship.ShieldMax[I]),Prefix+TEXT("shield capacities must be finite and nonnegative"));
 }
 bool Station=false;
 for(const auto& System:EffectiveSystems) {
  Require(!System.Id.IsNone()&&!SystemIDs.Contains(System.Id),TEXT("System IDs must be nonempty and unique.")); SystemIDs.Add(System.Id);
  Require(FMath::IsFinite(System.Radius)&&System.Radius>0&&!System.ChartPosition.ContainsNaN()&&Nonnegative(System.Danger)&&System.Security>=0&&System.Security<=2,System.Id.ToString()+TEXT(": system tuning is invalid")); Station|=System.HasStation;
 }
 Require(EffectiveFindSystem(StartSystem)!=INDEX_NONE,TEXT("Start system is missing.")); Require(Station,TEXT("A recovery station is required."));
 for(const auto& System:EffectiveSystems) for(FName Link:System.Links) {
  int32 Other=EffectiveFindSystem(Link);
  Require(Other!=INDEX_NONE&&Link!=System.Id,System.Id.ToString()+TEXT(": jump link is missing or points to itself"));
  if(Other!=INDEX_NONE) Require(EffectiveSystems[Other].Links.Contains(System.Id),System.Id.ToString()+TEXT(": jump link must be reciprocal"));
 }
 Require(FMath::IsFinite(GateOpeningRadius)&&GateOpeningRadius>0&&FMath::IsFinite(GateApproachDistance)&&GateApproachDistance>0&&FMath::IsFinite(GateArrivalTolerance)&&GateArrivalTolerance>0&&GateApproachDistance+GateArrivalTolerance*1.5f<Rules.JumpRange&&FMath::IsFinite(GateCruiseSpeed)&&GateCruiseSpeed>0&&FMath::IsFinite(GateAlignmentTolerance)&&GateAlignmentTolerance>0&&GateAlignmentTolerance<PI/2&&FMath::IsFinite(GatePassageAcceleration)&&GatePassageAcceleration>0&&FMath::IsFinite(GateArrivalDuration)&&GateArrivalDuration>=0.05f&&FMath::IsFinite(GateFlashDuration)&&GateFlashDuration>=0.05f,TEXT("Gate geometry/approach and passage tuning must be finite and usable."));
 for(const auto& Ship:EffectiveShips)if(Ship.Id.ToString().StartsWith(TEXT("corsair")))Require(Ship.Radius<GateOpeningRadius,TEXT("All captain hulls must fit through the gate aperture."));
 Require(Nonnegative(Rules.BraceDamageFactor)&&Rules.BraceDamageFactor<=1&&Nonnegative(Rules.CrippleThreshold)&&Rules.CrippleThreshold<=1,TEXT("Combat fractions must remain in [0, 1]."));
 return Good ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif

bool UVTGameData::ResolveFit(FName ClassId,const FVTLoadoutSelection& Fit,FVTShipDefinition& Out) const {return FVTFitEditor::Resolve(*this,ClassId,Fit,Out);}

float UVTGameData::StandingBetween(FName A,FName B) const {
 static const FName FreebootersFaction(TEXT("Freebooters"));
 if(A==B) return 100; if(A==FreebootersFaction||B==FreebootersFaction) return -70;
 for(const auto& R:Relations) if((R.A==A&&R.B==B)||(R.A==B&&R.B==A)) return R.Standing;
 return 0;
}

TArray<EVTDevice> UVTGameData::CrewForFit(FName ClassId,const FVTLoadoutSelection& Fit) const {return FVTFitEditor::Crew(*this,ClassId,Fit);}

void UVTGameData::LoadCatalog() {
 if(CatalogLoaded) return;
 auto& Manager=UAssetManager::Get();
 CatalogHandle=Manager.LoadPrimaryAsset(GetPrimaryAssetId(),{FName("Gameplay")});
 if(CatalogHandle) CatalogHandle->WaitUntilComplete();
 auto Overlay=[](const auto& References,auto& Definitions) {
  for(const auto& Ref:References) if(auto* Asset=Ref.Get()) {
   auto* Existing=Definitions.FindByPredicate([&](const auto& D){return D.Id==Asset->Definition.Id;});
   if(Existing) *Existing=Asset->Definition; else Definitions.Add(Asset->Definition);
  }
 };
 Overlay(ShipAssets,Ships); Overlay(EquipmentAssets,Loadouts); Overlay(SystemAssets,Systems); Overlay(ScenarioAssets,Scenarios);
 if(FApp::CanEverRender()&&!IsRunningCommandlet()) {
  TArray<FSoftObjectPath> Paths;
  if(!ProjectileMaterial.IsNull()) Paths.AddUnique(ProjectileMaterial.ToSoftObjectPath());
  for(const auto& Ship:Ships) if(!Ship.Mesh.IsNull()) Paths.AddUnique(Ship.Mesh.ToSoftObjectPath());
  for(const auto& Pair:FactionMeshes) if(!Pair.Value.IsNull()) Paths.AddUnique(Pair.Value.ToSoftObjectPath());
  if(!ShipClass.IsNull()) Paths.AddUnique(ShipClass.ToSoftObjectPath());
  if(!UIClass.IsNull()) Paths.AddUnique(UIClass.ToSoftObjectPath());
  // Loading-screen boundary: complete presentation before any ship is spawned.
  PresentationHandle=Manager.GetStreamableManager().RequestAsyncLoad(Paths);
  if(PresentationHandle) PresentationHandle->WaitUntilComplete();
 }
 CatalogLoaded=true;
}
