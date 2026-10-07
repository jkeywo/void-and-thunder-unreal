#include "VTBootstrapCommandlet.h"
#include "VTGameData.h"
#include "VTGameplay.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/Engine.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"

static bool SaveAsset(UObject* Asset, const FString& Name) {
 Asset->SetFlags(RF_Public|RF_Standalone);
 UPackage* Package=Asset->GetOutermost(); Package->FullyLoad(); Package->MarkPackageDirty();
 FAssetRegistryModule::AssetCreated(Asset);
 const FString File=FPackageName::LongPackageNameToFilename(Name,Asset->IsA<UWorld>() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension());
 IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);
 FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_None;
 return UPackage::SavePackage(Package,Asset,*File,Args);
}
static float Number(const TSharedPtr<FJsonObject>& O,const TCHAR* Key,float Default) {
 double Value; return O->TryGetNumberField(Key,Value) ? float(Value) : Default;
}
UVTBootstrapCommandlet::UVTBootstrapCommandlet() { IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true; }
int32 UVTBootstrapCommandlet::Main(const FString& Params) {
 FString Json;
 const FString Input=FPaths::ProjectDir()/TEXT("Migration/resolved-baseline.json");
 if(!FFileHelper::LoadFileToString(Json,*Input)) { UE_LOG(LogTemp,Error,TEXT("Missing typed migration baseline: %s"),*Input); return 1; }
 TSharedPtr<FJsonObject> Root;
 if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root)) return 2;
 const FString DataPath=TEXT("/Game/Data/DA_GameData");
 UPackage* Package=CreatePackage(*DataPath);
 UVTGameData* Data=NewObject<UVTGameData>(Package,TEXT("DA_GameData"),RF_Public|RF_Standalone);
 const auto ShipTable=Root->GetObjectField(TEXT("ships"));
 for(const auto& Item:ShipTable->GetArrayField(TEXT("classes"))) {
  auto Entry=Item->AsObject(); auto Class=Entry->GetObjectField(TEXT("class"));
  FVTShipDefinition Def; Def.Id=FName(Entry->GetStringField(TEXT("name")));
  auto Stats=Class->GetObjectField(TEXT("stats"));
  Def.Stats.Thrust=Number(Stats,TEXT("thrust"),Def.Stats.Thrust);
  Def.Stats.TurnRate=Number(Stats,TEXT("turn_rate"),Def.Stats.TurnRate);
  Def.Stats.MaxSpeed=Number(Stats,TEXT("max_speed"),Def.Stats.MaxSpeed);
  Def.Stats.ForwardDrag=Number(Stats,TEXT("forward_drag"),Def.Stats.ForwardDrag);
  Def.Stats.LateralDrag=Number(Stats,TEXT("lateral_drag"),Def.Stats.LateralDrag);
  Def.Stats.TurnRateSlow=Number(Stats,TEXT("turn_rate_slow"),Def.Stats.TurnRateSlow);
  Def.Stats.TurnRateFast=Number(Stats,TEXT("turn_rate_fast"),Def.Stats.TurnRateFast);
  Def.Stats.TurnAccel=Number(Stats,TEXT("turn_accel"),Def.Stats.TurnAccel);
  Def.Hull=Number(Class,TEXT("hull"),Def.Hull);
  Def.Radius=Number(Class->GetObjectField(TEXT("collider")),TEXT("radius"),Def.Radius);
  auto Broadside=Class->GetObjectField(TEXT("loadout"))->GetObjectField(TEXT("broadside"));
  Def.Damage=Number(Broadside,TEXT("damage"),Def.Damage);
  Def.Reload=Number(Broadside,TEXT("cooldown"),Def.Reload);
  Def.MuzzleSpeed=Number(Broadside,TEXT("muzzle_speed"),Def.MuzzleSpeed);
  Def.Arc=Number(Broadside,TEXT("arc"),Def.Arc);
  Def.ChargeTime=Number(Broadside,TEXT("charge_time"),Def.ChargeTime);
  Def.Guns=int32(Number(Broadside,TEXT("guns"),Def.Guns));
  Data->Ships.Add(Def);
 }
 auto Map=Root->GetObjectField(TEXT("world"));
 Data->StartSystem=FName(Map->GetStringField(TEXT("start")));
 for(const auto& Item:Map->GetArrayField(TEXT("systems"))) {
  auto O=Item->AsObject(); FVTSystemDefinition Def;
  Def.Id=FName(O->GetStringField(TEXT("id"))); Def.DisplayName=FText::FromString(O->GetStringField(TEXT("name")));
  FString Owner; if(O->TryGetStringField(TEXT("owner"),Owner)) Def.Owner=FName(Owner);
  FString Security=O->GetStringField(TEXT("security"));
  Def.Security=Security=="High" ? 2 : Security=="Medium" ? 1 : 0;
  Def.Danger=Number(O,TEXT("danger"),0); Def.Radius=Number(O,TEXT("bounds_radius"),1400); Def.HasStation=O->GetBoolField(TEXT("has_starbase"));
  Def.ChartPosition=FVector2D(Number(O,TEXT("x"),0),Number(O,TEXT("y"),0));
  for(const auto& Link:O->GetArrayField(TEXT("links"))) Def.Links.Add(FName(Link->AsString()));
  Data->Systems.Add(Def);
 }
 auto Rules=Root->GetObjectField(TEXT("tuning"));
 Data->Rules.ReverseThrottle=Number(Rules,TEXT("reverse_throttle"),0.25);
 Data->Rules.BoundsSpring=Number(Rules,TEXT("bounds_spring"),3);
 Data->Rules.BraceDamageFactor=Number(Rules,TEXT("brace_damage_factor"),0.35f);
 Data->Rules.ProjectileTTL=Number(Rules,TEXT("projectile_ttl"),2.5);
 Data->Rules.ProjectileRadius=Number(Rules,TEXT("projectile_radius"),5);
 Data->Rules.BoardRange=Number(Rules,TEXT("board_range"),95);
 Data->Rules.BoardDwell=Number(Rules,TEXT("board_dwell"),3);
 Data->Rules.CrippleThreshold=Number(Rules,TEXT("cripple_threshold"),0.25);
 Data->Rules.JumpRange=Number(Rules,TEXT("jump_range"),120);
 Data->Rules.JumpDwell=Number(Rules,TEXT("jump_charge_time"),6);
 if(!SaveAsset(Data,DataPath)) return 3;
 UPackage* InputPackage=CreatePackage(TEXT("/Game/Input/IMC_Flight"));
 auto* Mapping=NewObject<UInputMappingContext>(InputPackage,TEXT("IMC_Flight"),RF_Public|RF_Standalone);
 const TCHAR* Names[]={TEXT("Throttle"),TEXT("Turn"),TEXT("Aim"),TEXT("Port"),TEXT("Starboard"),TEXT("EMP"),TEXT("Torpedo"),TEXT("Warp"),TEXT("Boost"),TEXT("Brace"),TEXT("Interact"),TEXT("Mine"),TEXT("PointDefense")};
 const FKey Keys[]={EKeys::W,EKeys::D,EKeys::Gamepad_Right2D,EKeys::LeftMouseButton,EKeys::RightMouseButton,EKeys::Q,EKeys::LeftControl,EKeys::LeftShift,EKeys::SpaceBar,EKeys::C,EKeys::B,EKeys::M,EKeys::X};
 for(int32 I=0;I<UE_ARRAY_COUNT(Names);++I) {
  const FString Path=FString::Printf(TEXT("/Game/Input/IA_%s"),Names[I]);
  auto* A=NewObject<UInputAction>(CreatePackage(*Path),FName(FString("IA_")+Names[I]),RF_Public|RF_Standalone);
  A->ValueType=I<2 ? EInputActionValueType::Axis1D : I==2 ? EInputActionValueType::Axis2D : EInputActionValueType::Boolean;
  Mapping->MapKey(A,Keys[I]);
  if(I<2) {
   auto& Negative=Mapping->MapKey(A,I==0 ? EKeys::S : EKeys::A);
   Negative.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
   Mapping->MapKey(A,I==0 ? EKeys::Gamepad_LeftY : EKeys::Gamepad_LeftX);
  }
  if(I>=3) {
   const FKey Pad[]={EKeys::Gamepad_LeftTrigger,EKeys::Gamepad_RightTrigger,EKeys::Gamepad_FaceButton_Left,EKeys::Gamepad_LeftShoulder,EKeys::Gamepad_RightShoulder,EKeys::Gamepad_FaceButton_Bottom,EKeys::Gamepad_FaceButton_Top,EKeys::Gamepad_FaceButton_Right,EKeys::Gamepad_DPad_Left,EKeys::Gamepad_DPad_Right};
   Mapping->MapKey(A,Pad[I-3]);
  }
  if(!SaveAsset(A,Path)) return 4;
 }
 if(!SaveAsset(Mapping,TEXT("/Game/Input/IMC_Flight"))) return 5;
 UPackage* MapPackage=CreatePackage(TEXT("/Game/Maps/Sandbox"));
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false,FName("Sandbox"),MapPackage);
 auto* Light=World->SpawnActor<ADirectionalLight>(); Light->SetActorRotation(FRotator(-50,-30,0)); Light->GetLightComponent()->SetIntensity(5);
 World->SpawnActor<ASkyLight>();
 const bool Saved=SaveAsset(World,TEXT("/Game/Maps/Sandbox"));
 World->DestroyWorld(false);
 UE_LOG(LogTemp,Display,TEXT("Imported %d ship classes and %d systems"),Data->Ships.Num(),Data->Systems.Num());
 return Saved ? 0 : 6;
}
UVTBenchmarkCommandlet::UVTBenchmarkCommandlet() { IsClient=false; IsServer=true; IsEditor=true; LogToConsole=true; }
int32 UVTBenchmarkCommandlet::Main(const FString& Params) {
 int32 Count=500; FParse::Value(*Params,TEXT("Population="),Count);
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,FName("VTBenchmark"));
 FWorldContext& Context=GEngine->CreateNewWorldContext(EWorldType::Game); Context.SetCurrentWorld(World);
 World->SetGameInstance(NewObject<UVTGameInstance>(GEngine));
 auto* Sim=World->GetSubsystem<UVTSimulation>(); if(!Sim->Data) return 2;
 World->SetGameMode(FURL());
 World->InitializeActorsForPlay(FURL());
 World->BeginPlay();
 Sim->Bootstrap(Count);
 if(Sim->Ships.Num()!=Count) { UE_LOG(LogTemp,Error,TEXT("Benchmark spawned %d instead of %d"),Sim->Ships.Num(),Count); return 3; }
 for(int32 I=0;I<1200;++I) Sim->FixedStep();
 auto Samples=Sim->StepMilliseconds; Samples.RemoveAt(0,200); Samples.Sort();
 const double P95=Samples[FMath::FloorToInt(Samples.Num()*0.95)];
 FString Report=FString::Printf(TEXT("{\"population\":%d,\"systems\":%d,\"samples\":%d,\"p95_ms\":%.6f,\"budget_ms\":8,\"cpu\":\"%s\",\"passed\":%s}"),Sim->Ships.Num(),Sim->Data->Systems.Num(),Samples.Num(),P95,FPlatformMisc::GetCPUBrand().GetCharArray().GetData(),P95<8 ? TEXT("true") : TEXT("false"));
 const FString Path=FPaths::ProjectSavedDir()/FString::Printf(TEXT("Validation/scale-%d.json"),Count);
 IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true); FFileHelper::SaveStringToFile(Report,*Path);
 UE_LOG(LogTemp,Display,TEXT("Population %d simulation p95 %.3f ms"),Sim->Ships.Num(),P95);
 World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
 return P95<8 ? 0 : 1;
}
