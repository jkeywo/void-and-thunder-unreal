#include "VTGate.h"
FVTPilotIntent VTGate::Guide(const FVTMotion& Motion,const FVTShipStats& Stats,const FVTPilotIntent& Intent,const FVector2D& Centre,bool Entering,float Approach,float Cruise,float Tolerance,float Reverse) {
 auto Result=Intent;const auto Normal=Centre.GetSafeNormal();const auto Target=Centre+Normal*(Entering?Approach:-Approach);const auto Delta=Target-Motion.Position;const float Distance=Delta.Size();
 const auto Direction=!Entering&&Distance<Tolerance?Normal:Delta.GetSafeNormal();float Heading=FMath::Atan2(Direction.Y,Direction.X);bool Backing=false;
 if(!Entering&&Distance>=Tolerance&&Reverse>0) {
  const float Outward=FMath::Atan2(Normal.Y,Normal.X),TurnRate=FMath::Max(0.01f,Stats.TurnRate*Stats.TurnRateSlow);
  auto Estimate=[&](float Bearing,float Factor){const float Acceleration=FMath::Max(0.01f,Stats.Thrust*Factor),Speed=FMath::Max(0.01f,FMath::Min(Cruise,Acceleration/FMath::Max(0.01f,Stats.ForwardDrag)));const float Travel=Distance<Speed*Speed/Acceleration?2*FMath::Sqrt(Distance/Acceleration):Distance/Speed+Speed/Acceleration;return Travel+(FMath::Abs(FMath::UnwindRadians(Bearing-Motion.Heading))+FMath::Abs(FMath::UnwindRadians(Outward-Bearing)))/TurnRate;};
  const float ReverseHeading=FMath::UnwindRadians(Heading+PI);Backing=Estimate(ReverseHeading,Reverse)<Estimate(Heading,1);if(Backing)Heading=ReverseHeading;
 }
 const float Error=FMath::UnwindRadians(Heading-Motion.Heading);
 Result.Turn=FMath::Clamp((Error*3-Motion.Omega)/FMath::Max(0.01f,Stats.TurnRate),-1.f,1.f);
 const float Desired=(Backing?-1.f:1.f)*(FMath::Abs(Error)<0.35f?FMath::Min(Cruise,!Entering?FMath::Max(0.f,(Distance-Tolerance)*1.5f):Cruise):0.f);
 const float Speed=FVector2D::DotProduct(Motion.Velocity,FVector2D(FMath::Cos(Motion.Heading),FMath::Sin(Motion.Heading)));
 const float Accel=Desired*Stats.ForwardDrag+(Desired-Speed)*3;
 Result.Throttle=FMath::Clamp(Accel/FMath::Max(0.01f,Stats.Thrust*(Accel<0?Reverse:1.f)),-1.f,1.f);return Result;
}
bool VTGate::Crossed(const FVTMotion& Before,const FVTMotion& After,const FVector2D& Centre,float Clearance,float Tolerance) {
 if(Clearance<=0)return false;const auto Normal=Centre.GetSafeNormal();const auto Side=FVector2D(-Normal.Y,Normal.X);const float A=FVector2D::DotProduct(Before.Position-Centre,Normal),B=FVector2D::DotProduct(After.Position-Centre,Normal);
 if(A>0||B<=0||B-A<=0||FVector2D::DotProduct(FVector2D(FMath::Cos(After.Heading),FMath::Sin(After.Heading)),Normal)<FMath::Cos(Tolerance))return false;
 const auto Crossing=FMath::Lerp(Before.Position,After.Position,double(-A/(B-A)));return FMath::Abs(FVector2D::DotProduct(Crossing-Centre,Side))<=Clearance;
}
