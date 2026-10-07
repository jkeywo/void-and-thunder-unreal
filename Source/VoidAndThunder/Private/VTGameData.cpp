#include "VTGameData.h"
#if WITH_EDITOR
EDataValidationResult UVTGameData::IsDataValid(FDataValidationContext& Context) const {
 bool Good=true;
 auto Require=[&](bool Condition,const FString& Message) {if(!Condition) {Good=false; Context.AddError(FText::FromString(Message));}};
 auto Nonnegative=[](float Value) {return FMath::IsFinite(Value)&&Value>=0;};
 Require(Ships.Num()>=5,TEXT("The migrated ship catalogue must contain all five baseline classes."));
 Require(Systems.Num()==10,TEXT("The baseline sandbox must contain all ten systems."));
 TSet<FName> ShipIDs,SystemIDs;
 for(const auto& Ship:Ships) {
  FString Prefix=Ship.Id.ToString()+TEXT(": ");
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
 for(const auto& System:Systems) {
  Require(!System.Id.IsNone()&&!SystemIDs.Contains(System.Id),TEXT("System IDs must be nonempty and unique.")); SystemIDs.Add(System.Id);
  Require(FMath::IsFinite(System.Radius)&&System.Radius>0&&!System.ChartPosition.ContainsNaN()&&Nonnegative(System.Danger)&&System.Security>=0&&System.Security<=2,System.Id.ToString()+TEXT(": system tuning is invalid")); Station|=System.HasStation;
 }
 Require(FindSystem(StartSystem)!=INDEX_NONE,TEXT("Start system is missing.")); Require(Station,TEXT("A recovery station is required."));
 for(const auto& System:Systems) for(FName Link:System.Links) {
  int32 Other=FindSystem(Link);
  Require(Other!=INDEX_NONE&&Link!=System.Id,System.Id.ToString()+TEXT(": jump link is missing or points to itself"));
  if(Other!=INDEX_NONE) Require(Systems[Other].Links.Contains(System.Id),System.Id.ToString()+TEXT(": jump link must be reciprocal"));
 }
 Require(Nonnegative(Rules.BraceDamageFactor)&&Rules.BraceDamageFactor<=1&&Nonnegative(Rules.CrippleThreshold)&&Rules.CrippleThreshold<=1,TEXT("Combat fractions must remain in [0, 1]."));
 return Good ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
