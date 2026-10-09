#include "VTFitEditor.h"
#include "VTGameData.h"
bool FVTFitEditor::Resolve(const UVTGameData& Data,FName ClassId,const FVTLoadoutSelection& Fit,FVTShipDefinition& Out) {
 const auto& Loadouts=Data.Loadouts;
 const auto* Base=Data.FindShip(ClassId); if(!Base) return false; Out=*Base;
 const FName PrimaryNames[]={Fit.Broadside,Fit.Battery,Fit.Special};
 for(int I=0;I<3;++I) {
  if(PrimaryNames[I].IsNone()) continue;
  const auto* Option=Loadouts.FindByPredicate([&](const FVTLoadoutOption& O){return O.Id==PrimaryNames[I]&&int(O.Slot)==I;}); if(!Option) return false;
  const auto& D=Option->Definition; auto& E=Out.Equipment;
  if(I==0) {Out.Damage=D.Damage; Out.Reload=D.Reload; Out.MuzzleSpeed=D.MuzzleSpeed; Out.Arc=D.Arc; Out.ChargeTime=D.ChargeTime; Out.Guns=D.Guns;}
  if(I==1) {Out.BatteryMax=FMath::Max(Base->BatteryMax,D.BatteryMax); Out.BatteryRecharge=FMath::Max(Base->BatteryRecharge,D.BatteryRecharge); E.EMP=D.Equipment.EMP; E.Boost=D.Equipment.Boost; E.PointDefense=D.Equipment.PointDefense;
   E.EMPCooldown=D.Equipment.EMPCooldown; E.EMPDrain=D.Equipment.EMPDrain; E.EMPRange=D.Equipment.EMPRange; E.EMPSwivel=D.Equipment.EMPSwivel; E.EMPArc=D.Equipment.EMPArc; E.EMPSpeed=D.Equipment.EMPSpeed; E.EMPFraction=D.Equipment.EMPFraction; E.BoostMultiplier=D.Equipment.BoostMultiplier; E.BoostDrain=D.Equipment.BoostDrain; E.PDRadius=D.Equipment.PDRadius; E.PDRate=D.Equipment.PDRate; E.PDDrain=D.Equipment.PDDrain;}
  if(I==2) {E.Torpedoes=D.Equipment.Torpedoes; E.Warp=D.Equipment.Warp; E.Mines=D.Equipment.Mines;
   E.TorpedoDamage=D.Equipment.TorpedoDamage; E.LockInterval=D.Equipment.LockInterval; E.LockRadius=D.Equipment.LockRadius; E.TorpedoMagazine=D.Equipment.TorpedoMagazine; E.Tubes=D.Equipment.Tubes; E.TubeReload=D.Equipment.TubeReload; E.TorpedoRange=D.Equipment.TorpedoRange; E.TorpedoSpeed=D.Equipment.TorpedoSpeed; E.TorpedoTurn=D.Equipment.TorpedoTurn; E.TorpedoResupplyMin=D.Equipment.TorpedoResupplyMin; E.TorpedoResupplyMax=D.Equipment.TorpedoResupplyMax;
   E.WarpCooldown=D.Equipment.WarpCooldown; E.WarpRange=D.Equipment.WarpRange; E.MineCooldown=D.Equipment.MineCooldown; E.MineDamage=D.Equipment.MineDamage; E.MineMagazine=D.Equipment.MineMagazine; E.MineRadius=D.Equipment.MineRadius; E.MineTTL=D.Equipment.MineTTL; E.MineResupplyMin=D.Equipment.MineResupplyMin; E.MineResupplyMax=D.Equipment.MineResupplyMax;}
 }
 // Multi-mount sets use catalogue order, matching the original mask iteration.
 for(int Slot=1;Slot<3;++Slot) {
  const auto& Names=Slot==1 ? Fit.Batteries : Fit.Specials; const bool Override=Slot==1?Fit.OverrideBatteries:Fit.OverrideSpecials; if(Names.IsEmpty()&&!Override) continue;
  if(Names.Num()>Out.Mounts) return false; TSet<FName> UniqueNames;
  if(Slot==1) {Out.Equipment.EMP=false; Out.Equipment.Boost=false; Out.Equipment.PointDefense=false;}
  else {Out.Equipment.Torpedoes=false; Out.Equipment.Warp=false; Out.Equipment.Mines=false;}
  for(FName Name:Names) {
   if(UniqueNames.Contains(Name)) return false; UniqueNames.Add(Name);
   FVTLoadoutSelection Single; if(Slot==1) Single.Battery=Name; else Single.Special=Name; FVTShipDefinition D;
   if(!Resolve(Data,ClassId,Single,D)) return false;
   auto& E=Out.Equipment; const auto& Add=D.Equipment;
   if(Slot==1) {Out.BatteryMax=FMath::Max(Out.BatteryMax,D.BatteryMax); Out.BatteryRecharge=FMath::Max(Out.BatteryRecharge,D.BatteryRecharge); E.EMP|=Add.EMP; E.Boost|=Add.Boost; E.PointDefense|=Add.PointDefense;
    if(Add.EMP) {E.EMPCooldown=Add.EMPCooldown; E.EMPDrain=Add.EMPDrain; E.EMPRange=Add.EMPRange; E.EMPSwivel=Add.EMPSwivel; E.EMPArc=Add.EMPArc; E.EMPSpeed=Add.EMPSpeed; E.EMPFraction=Add.EMPFraction;}
    if(Add.Boost) {E.BoostMultiplier=Add.BoostMultiplier; E.BoostDrain=Add.BoostDrain;} if(Add.PointDefense) {E.PDRadius=Add.PDRadius; E.PDRate=Add.PDRate; E.PDDrain=Add.PDDrain;}}
   else {E.Torpedoes|=Add.Torpedoes; E.Warp|=Add.Warp; E.Mines|=Add.Mines;
    if(Add.Torpedoes) {E.TorpedoDamage=Add.TorpedoDamage; E.LockInterval=Add.LockInterval; E.LockRadius=Add.LockRadius; E.TorpedoMagazine=Add.TorpedoMagazine; E.Tubes=Add.Tubes; E.TubeReload=Add.TubeReload; E.TorpedoRange=Add.TorpedoRange; E.TorpedoSpeed=Add.TorpedoSpeed; E.TorpedoTurn=Add.TorpedoTurn; E.TorpedoResupplyMin=Add.TorpedoResupplyMin; E.TorpedoResupplyMax=Add.TorpedoResupplyMax;}
    if(Add.Warp) {E.WarpCooldown=Add.WarpCooldown; E.WarpRange=Add.WarpRange;} if(Add.Mines) {E.MineCooldown=Add.MineCooldown; E.MineDamage=Add.MineDamage; E.MineMagazine=Add.MineMagazine; E.MineRadius=Add.MineRadius; E.MineTTL=Add.MineTTL; E.MineResupplyMin=Add.MineResupplyMin; E.MineResupplyMax=Add.MineResupplyMax;}}
  }
 }
 if(Fit.CrewedDevices.Num()>10) return false;
 TSet<EVTDevice> Unique;
 for(auto Device:Fit.CrewedDevices) {if(Unique.Contains(Device)||Device==EVTDevice::Boost||Device==EVTDevice::Microwarp||Device==EVTDevice::Brace||Device==EVTDevice::Board||uint8(Device)>uint8(EVTDevice::PointDefense)) return false; Unique.Add(Device);}
 return true;
}
TArray<EVTDevice> FVTFitEditor::Crew(const UVTGameData& Data,FName ClassId,const FVTLoadoutSelection& Fit) {
 const auto& Loadouts=Data.Loadouts;
 TArray<EVTDevice> Crew; const auto* Base=Data.FindShip(ClassId); if(!Base) return Crew;
 for(int Slot=1;Slot<3;++Slot) {
  const auto& Names=Slot==1 ? Fit.Batteries : Fit.Specials; int Mount=0;
  for(const auto& O:Loadouts) if(int(O.Slot)==Slot&&Names.Contains(O.Id)) {
   if(Mount++<Base->Crewed) continue; const auto& E=O.Definition.Equipment;
   if(Slot==1) {if(E.EMP) Crew.AddUnique(EVTDevice::EMP); if(E.PointDefense) Crew.AddUnique(EVTDevice::PointDefense);}
   else {if(E.Torpedoes) Crew.AddUnique(EVTDevice::Torpedo); if(E.Mines) Crew.AddUnique(EVTDevice::Mine);}
  }
 }
 return Crew;
}

FVTFitPreview FVTFitEditor::Inspect(const UVTGameData& Data,FName Hull,const FVTLoadoutSelection& Fit){
 FVTFitPreview P;P.Hull=Hull;P.Selection=Fit;P.Valid=Resolve(Data,Hull,Fit,P.Resolved);
 if(!P.Valid){P.Reason=NSLOCTEXT("VTFit","Invalid","Invalid equipment or mount limit exceeded.");return P;}
 P.Crew=Crew(Data,Hull,Fit);P.BatteryMounts=Fit.Batteries.IsEmpty()&&!Fit.OverrideBatteries?int(!Fit.Battery.IsNone()):Fit.Batteries.Num();P.SpecialMounts=Fit.Specials.IsEmpty()&&!Fit.OverrideSpecials?int(!Fit.Special.IsNone()):Fit.Specials.Num();return P;
}
FText FVTFitPreview::Summary() const{
 const auto& E=Resolved.Equipment;TArray<FString> Devices;
 if(E.EMP)Devices.Add(TEXT("EMP"));if(E.Boost)Devices.Add(TEXT("Boost"));if(E.PointDefense)Devices.Add(TEXT("Point defence"));if(E.Torpedoes)Devices.Add(TEXT("Torpedoes"));if(E.Warp)Devices.Add(TEXT("Microwarp"));if(E.Mines)Devices.Add(TEXT("Mines"));
 auto Mounts=[&](int Count,bool Override,FName Primary){return !Override&&Count==0&&Primary.IsNone()?NSLOCTEXT("VTFit","Inherited","hull default"):FText::Format(NSLOCTEXT("VTFit","MountCount","{0}/{1}"),FText::AsNumber(Count),FText::AsNumber(Resolved.Mounts));};
 return FText::Format(NSLOCTEXT("VTFit","ResolvedSummary","Battery {0} | Special {1} | Crewed devices {2}\n{3}"),Mounts(BatteryMounts,Selection.OverrideBatteries,Selection.Battery),Mounts(SpecialMounts,Selection.OverrideSpecials,Selection.Special),FText::AsNumber(Crew.Num()),Devices.IsEmpty()?NSLOCTEXT("VTFit","NoOptional","No optional devices"):FText::FromString(FString::Join(Devices,TEXT(", "))));
}

bool FVTFitEditor::Initialize(const UVTGameData& Data,FName Hull,const FVTLoadoutSelection& Fit){auto P=Inspect(Data,Hull,Fit);if(!P.Valid)return false;Accepted=MoveTemp(P);return true;}
bool FVTFitEditor::Toggle(const UVTGameData& Data,FName Id,FText& Reason){
 const auto* O=Data.Loadouts.FindByPredicate([Id](const FVTLoadoutOption& Item){return Item.Id==Id;});if(!O){Reason=NSLOCTEXT("VTFit","Unknown","Unknown equipment.");return false;}
 auto Candidate=Accepted.Selection;
 // Preserve explicit legacy primary choices when editing their checkbox representation.
 if(Candidate.Batteries.IsEmpty()&&!Candidate.OverrideBatteries&&!Candidate.Battery.IsNone())Candidate.Batteries.Add(Candidate.Battery);
 if(Candidate.Specials.IsEmpty()&&!Candidate.OverrideSpecials&&!Candidate.Special.IsNone())Candidate.Specials.Add(Candidate.Special);
 Candidate.OverrideBatteries=Candidate.OverrideSpecials=true;Candidate.Battery=Candidate.Special=NAME_None;
 if(O->Slot==EVTLoadoutSlot::Broadside)Candidate.Broadside=Candidate.Broadside==Id?NAME_None:Id;
 else{auto& Items=O->Slot==EVTLoadoutSlot::Battery?Candidate.Batteries:Candidate.Specials;if(Items.Contains(Id))Items.Remove(Id);else Items.Add(Id);}
 auto P=Inspect(Data,Accepted.Hull,Candidate);Reason=P.Reason;if(!P.Valid)return false;Accepted=MoveTemp(P);return true;
}
bool FVTFitEditor::SelectHull(const UVTGameData& Data,FName Hull,FText& Reason){
 const auto* Ship=Data.FindShip(Hull);if(!Ship)return false;auto Candidate=Accepted.Selection;bool Pruned=false;
 for(int Slot=1;Slot<3;++Slot){auto& Items=Slot==1?Candidate.Batteries:Candidate.Specials;TArray<FName> Kept;for(const auto& O:Data.Loadouts)if(int(O.Slot)==Slot&&Items.Contains(O.Id)&&Kept.Num()<Ship->Mounts)Kept.Add(O.Id);Pruned|=Kept.Num()!=Items.Num();Items=MoveTemp(Kept);}
 auto P=Inspect(Data,Hull,Candidate);if(!P.Valid){Reason=P.Reason;return false;}Accepted=MoveTemp(P);Reason=Pruned?NSLOCTEXT("VTFit","Pruned","Extra modules removed in catalogue order for this hull."):FText::GetEmpty();return true;
}
