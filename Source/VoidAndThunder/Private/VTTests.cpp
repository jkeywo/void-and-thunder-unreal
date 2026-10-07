#include "VTTypes.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTThrottleTest,"VT.Flight.ThrustAndReverse",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTThrottleTest::RunTest(const FString& Params) {
 FVTShipStats S; S.Thrust=100; S.MaxSpeed=200; S.ForwardDrag=0; S.LateralDrag=0;
 FVTPilotIntent I; I.Throttle=1; FVTMotion Forward; VT::HelmStep(Forward,S,I,0.25f,1);
 TestTrue("Forward thrust matches Rust fixture",FMath::Abs(Forward.Velocity.X-100)<0.0001);
 I.Throttle=-1; FVTMotion Reverse; VT::HelmStep(Reverse,S,I,0.25f,1);
 TestTrue("Reverse matches Rust fixture",FMath::Abs(Reverse.Velocity.X+25)<0.0001); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTDragTest,"VT.Flight.AnisotropicDragAndClamp",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTDragTest::RunTest(const FString& Params) {
 FVTShipStats S; S.Thrust=0; S.MaxSpeed=1000; S.ForwardDrag=0.5f; S.LateralDrag=4;
 FVTMotion M; M.Velocity=FVector2D(100,100); VT::HelmStep(M,S,FVTPilotIntent(),0.25f,1);
 TestTrue("Along drag is exponential",FMath::Abs(M.Velocity.X-100*FMath::Exp(-0.5f))<0.001);
 TestTrue("Lateral drag bites harder",FMath::Abs(M.Velocity.Y-100*FMath::Exp(-4.f))<0.001);
 S.Thrust=10000; S.MaxSpeed=200; FVTPilotIntent I; I.Throttle=1;
 for(int N=0;N<1000;++N) VT::HelmStep(M,S,I,0.25f,VT::Step);
 TestTrue("Top speed is bounded",M.Velocity.Size()<=200.001); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTInputTest,"VT.Network.RejectInvalidIntent",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTInputTest::RunTest(const FString& Params) {
 FVTPilotIntent I; TestTrue("Neutral intent valid",VT::ValidIntent(I));
 I.Throttle=2; TestFalse("Out of range throttle rejected",VT::ValidIntent(I));
 I.Throttle=0; I.Aim.X=std::numeric_limits<double>::quiet_NaN(); TestFalse("NaN aim rejected",VT::ValidIntent(I));
 I.Aim=FVector2D(0,1); I.Buttons=65535; TestFalse("Unknown device bits rejected",VT::ValidIntent(I)); return true;
}
#endif
