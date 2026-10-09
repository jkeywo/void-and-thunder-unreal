#include "VTBootstrapCommandlet.h"
#include "VTPIESettings.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
UVTPIEAuthoringCommandlet::UVTPIEAuthoringCommandlet(){IsEditor=true;LogToConsole=true;}
int32 UVTPIEAuthoringCommandlet::Main(const FString& Params){if(!FParse::Param(*Params,TEXT("Apply")))return 1;auto* World=LoadObject<UWorld>(nullptr,TEXT("/Game/Maps/Sandbox.Sandbox"));if(!World)return 2;auto* Settings=World->GetWorldSettings();if(Settings->GetAssetUserDataOfClass(UVTPIELevelModes::StaticClass()))return 0;Settings->AddAssetUserData(NewObject<UVTPIELevelModes>(Settings));auto* P=World->GetOutermost();P->MarkPackageDirty();FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;return UPackage::SavePackage(P,World,*FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetMapPackageExtension()),Args)?0:3;}
