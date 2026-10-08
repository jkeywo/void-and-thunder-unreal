#pragma once
#include "VTTypes.h"
namespace VTGate {
 VOIDANDTHUNDER_API FVTPilotIntent Guide(const FVTMotion& Motion,const FVTShipStats& Stats,const FVTPilotIntent& Intent,const FVector2D& Centre,bool Entering,float Approach,float Cruise,float Tolerance,float Reverse);
 VOIDANDTHUNDER_API bool Crossed(const FVTMotion& Before,const FVTMotion& After,const FVector2D& Centre,float Clearance,float Tolerance);
}
