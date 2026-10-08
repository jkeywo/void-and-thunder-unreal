#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "VTFitEditor.h"
#include "VTGameData.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVTFitEditTest,"VT.Modules.FitAcceptedSelection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FVTFitEditTest::RunTest(const FString&){auto* D=LoadObject<UVTGameData>(nullptr,TEXT("/Game/Data/DA_GameData.DA_GameData"));if(!D)return false;D->LoadCatalog();FVTFitEditor E;TestTrue("Initialize defaults",E.Initialize(*D,FName("corsair_frigate"),{}));FText Reason;TestFalse("Unknown module rejected",E.Toggle(*D,FName("invalid"),Reason));TestTrue("Default remains accepted",E.Preview().Selection.Broadside.IsNone());
 for(const auto& O:D->Loadouts)if(O.Slot==EVTLoadoutSlot::Battery){TestTrue("First module accepted",E.Toggle(*D,O.Id,Reason));TestTrue("Explicit optional fit",E.Preview().Selection.OverrideBatteries&&E.Preview().Selection.OverrideSpecials);TestTrue("Untick accepted",E.Toggle(*D,O.Id,Reason));TestTrue("Empty stays explicit",E.Preview().Selection.Batteries.IsEmpty()&&E.Preview().Selection.OverrideBatteries);break;}return true;}
#endif
