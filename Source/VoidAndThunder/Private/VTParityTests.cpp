#include "VTGameplay.h"
#include "VTCombat.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace {
TSharedPtr<FJsonObject> Golden(FAutomationTestBase& Test) {
 FString Text;TSharedPtr<FJsonObject> Root;
 if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectDir()/TEXT("Migration/golden-rules.json")))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root)){Test.AddError(TEXT("Missing or invalid independently exported golden rules"));return nullptr;}
 return Root;
}
FVector2D Point(const TSharedPtr<FJsonObject>& O,const TCHAR* Key){const auto& A=O->GetArrayField(Key);return FVector2D(A[0]->AsNumber(),A[1]->AsNumber());}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTGoldenFlight,"VT.Parity.LegacyFlightTrajectories",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTGoldenFlight::RunTest(const FString& Params) {
 auto Root=Golden(*this);if(!Root)return false;auto* Data=LoadObject<UVTGameData>(nullptr,TEXT("/Game/Data/DA_GameData.DA_GameData"));if(!TestNotNull(TEXT("Native authored data"),Data))return false;
 double MaxPosition=0,MaxVelocity=0,MaxHeading=0,MaxOmega=0;int Count=0;
 for(const auto& Value:Root->GetArrayField(TEXT("flight"))) {
  auto O=Value->AsObject();const auto* Definition=Data->FindShip(FName(O->GetStringField(TEXT("hull"))));if(!Definition){AddError(TEXT("Missing native hull in golden fixture"));return false;}
  auto Initial=O->GetObjectField(TEXT("initial"));FVTMotion Motion;Motion.Heading=Initial->GetNumberField(TEXT("heading"));Motion.Omega=Initial->GetNumberField(TEXT("omega"));Motion.Velocity=Point(Initial,TEXT("velocity"));int Step=0,Checkpoint=0;const auto& Expected=O->GetArrayField(TEXT("checkpoints"));
  for(const auto& Block:O->GetArrayField(TEXT("controls"))) {const auto& A=Block->AsArray();FVTPilotIntent Input;Input.Throttle=A[0]->AsNumber();Input.Turn=A[1]->AsNumber();for(int I=0;I<int(A[2]->AsNumber());++I){VT::HelmStep(Motion,Definition->Stats,Input,0.25f,VT::Step);++Step;if(Checkpoint<Expected.Num()&&Step==int(Expected[Checkpoint]->AsObject()->GetNumberField(TEXT("step")))){auto E=Expected[Checkpoint++]->AsObject();MaxPosition=FMath::Max(MaxPosition,(Motion.Position-Point(E,TEXT("position"))).Size());MaxVelocity=FMath::Max(MaxVelocity,(Motion.Velocity-Point(E,TEXT("velocity"))).Size());MaxHeading=FMath::Max(MaxHeading,double(FMath::Abs(FMath::UnwindRadians(Motion.Heading-float(E->GetNumberField(TEXT("heading")))))));MaxOmega=FMath::Max(MaxOmega,FMath::Abs(Motion.Omega-E->GetNumberField(TEXT("omega"))));++Count;}}}
 }
 TestEqual(TEXT("Forty source trajectories with eight checkpoints"),Count,320);
 TestTrue(TEXT("Ten-second position error below 0.05 source unit"),MaxPosition<0.05);TestTrue(TEXT("Velocity error below 0.02 source unit per second"),MaxVelocity<0.02);TestTrue(TEXT("Heading error below 0.001 radians"),MaxHeading<0.001);TestTrue(TEXT("Angular velocity error below 0.001 radians/second"),MaxOmega<0.001);
 AddInfo(FString::Printf(TEXT("Golden trajectory maxima: position %.8f, velocity %.8f, heading %.8f, omega %.8f"),MaxPosition,MaxVelocity,MaxHeading,MaxOmega));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTGoldenBeams,"VT.Parity.LegacyBroadsideBearings",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTGoldenBeams::RunTest(const FString& Params){auto Root=Golden(*this);if(!Root)return false;int Count=0;double Error=0;for(const auto& V:Root->GetArrayField(TEXT("broadside"))){auto O=V->AsObject();auto D=VTCombat::BroadsideDirection(O->GetNumberField(TEXT("heading")),O->GetBoolField(TEXT("port")),Point(O,TEXT("aim")),O->GetNumberField(TEXT("arc")));Error=FMath::Max(Error,(D-Point(O,TEXT("direction"))).Size());++Count;}TestEqual(TEXT("Source beam corpus"),Count,210);TestTrue(TEXT("Beam direction within 1e-4"),Error<1e-4);AddInfo(FString::Printf(TEXT("Golden beam max error %.9f"),Error));return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTGoldenShields,"VT.Parity.LegacyShieldBearings",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTGoldenShields::RunTest(const FString& Params){auto Root=Golden(*this);if(!Root)return false;int Count=0;for(const auto& V:Root->GetArrayField(TEXT("shield"))){auto O=V->AsObject();TestEqual(TEXT("Source shield bank selection"),VTCombat::ShieldArc(O->GetNumberField(TEXT("heading")),Point(O,TEXT("impact")),int(O->GetNumberField(TEXT("arcs")))),int(O->GetNumberField(TEXT("answer"))));++Count;}TestEqual(TEXT("Source shield corpus"),Count,105);return true;}
#endif
