#pragma once
#include "VTTypes.h"
namespace VTGate {
 VOIDANDTHUNDER_API FVTPilotIntent Guide(const FVTMotion& Motion,const FVTShipStats& Stats,const FVTPilotIntent& Intent,const FVector2D& Centre,bool Entering,float Approach,float Cruise,float Tolerance,float Reverse);
 VOIDANDTHUNDER_API void Depart(FVTMotion& Motion,const FVector2D& Centre,float Acceleration,float Dt);
 VOIDANDTHUNDER_API bool Arrive(FVTMotion& Motion,const FVector2D& Origin,const FVector2D& Target,double Elapsed,float Duration);
 VOIDANDTHUNDER_API bool Preview(FVTMotion& Motion,const FVector2D& Centre,float Approach,float ArrivalFraction,float Acceleration,float ArrivalDuration,float FlashDuration,float Pause,double Time);
 VOIDANDTHUNDER_API bool Crossed(const FVTMotion& Before,const FVTMotion& After,const FVector2D& Centre,float Clearance,float Tolerance);
}
