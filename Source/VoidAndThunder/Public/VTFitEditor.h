#pragma once
#include "VTTypes.h"
class UVTGameData;
struct VOIDANDTHUNDER_API FVTFitPreview {
 FName Hull; FVTLoadoutSelection Selection; FVTShipDefinition Resolved; TArray<EVTDevice> Crew;
 int32 BatteryMounts=0,SpecialMounts=0; FText Reason; bool Valid=false;
 FText Summary() const;
};
// The accepted selection changes only after a successful operation.
class VOIDANDTHUNDER_API FVTFitEditor {
 FVTFitPreview Accepted;
public:
 static bool Resolve(const UVTGameData& Data,FName Hull,const FVTLoadoutSelection& Fit,FVTShipDefinition& Out);
 static TArray<EVTDevice> Crew(const UVTGameData& Data,FName Hull,const FVTLoadoutSelection& Fit);
 static FVTFitPreview Inspect(const UVTGameData& Data,FName Hull,const FVTLoadoutSelection& Fit);
 bool Initialize(const UVTGameData& Data,FName Hull,const FVTLoadoutSelection& Fit);
 bool Toggle(const UVTGameData& Data,FName Equipment,FText& Reason);
 bool SelectHull(const UVTGameData& Data,FName Hull,FText& Reason);
 const FVTFitPreview& Preview() const{return Accepted;}
};
