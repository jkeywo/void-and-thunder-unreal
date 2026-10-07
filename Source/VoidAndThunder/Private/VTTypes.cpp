#include "VTTypes.h"
void VT::HelmStep(FVTMotion& M, const FVTShipStats& S, const FVTPilotIntent& I, float Reverse, float Dt) {
 const float SpeedFraction = S.MaxSpeed > 0 ? FMath::Clamp(float(M.Velocity.Size()) / S.MaxSpeed, 0.f, 1.f) : 0;
 const float Agility = FMath::Lerp(S.TurnRateSlow, S.TurnRateFast, SpeedFraction);
 const float Target = FMath::Clamp(I.Turn, -1.f, 1.f) * S.TurnRate * Agility;
 M.Omega += (Target - M.Omega) * (1 - FMath::Exp(-S.TurnAccel * Dt));
 M.Heading = FMath::UnwindRadians(M.Heading + M.Omega * Dt);
 const FVector2D Forward(FMath::Cos(M.Heading), FMath::Sin(M.Heading));
 const FVector2D Right(Forward.Y, -Forward.X);
 const float Throttle = FMath::Clamp(I.Throttle, -1.f, 1.f);
 const FVector2D V = M.Velocity + Forward * (Throttle * (Throttle < 0 ? Reverse : 1) * S.Thrust * Dt);
 M.Velocity = Forward * (FVector2D::DotProduct(V, Forward) * FMath::Exp(-S.ForwardDrag * Dt))
  + Right * (FVector2D::DotProduct(V, Right) * FMath::Exp(-S.LateralDrag * Dt));
 if (M.Velocity.Size() > S.MaxSpeed) M.Velocity = M.Velocity.GetSafeNormal() * S.MaxSpeed;
 M.Position += M.Velocity * Dt;
}
bool VT::ValidIntent(const FVTPilotIntent& I) {
 return FMath::IsFinite(I.Throttle) && FMath::IsFinite(I.Turn) && FMath::IsFinite(I.Aim.X) && FMath::IsFinite(I.Aim.Y)
  && FMath::Abs(I.Throttle) <= 1 && FMath::Abs(I.Turn) <= 1 && I.Aim.SizeSquared() <= 1.01 && (I.Buttons & ~uint16(1023)) == 0;
}
FVector VT::ArenaOrigin(int32 S) { return FVector((S % 5) * 1000000.0, (S / 5) * 1000000.0, 0); }
FVector VT::ToWorld(const FVector2D& P, int32 S) { return ArenaOrigin(S) + FVector(P.X * 100, -P.Y * 100, 0); }
