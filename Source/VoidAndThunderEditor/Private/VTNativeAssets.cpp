#include "VTNativeAssets.h"
#include "VTGameData.h"
#include "VTGameplay.h"
#include "VTCues.h"
#include "Engine/Blueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "InputAction.h"
#include "PlayerMappableKeySettings.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "VTUI.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "NiagaraSystem.h"
#include "NiagaraEffectType.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/Paths.h"
namespace {
bool Save(UObject* Asset) {
 auto* P=Asset->GetOutermost(); P->MarkPackageDirty(); FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
 auto File=FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension());
 IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);
 return UPackage::SavePackage(P,Asset,*File,Args);
}
template<class T> T* Seed(const FString& Path,bool& Created) {
 Created=false; if(auto* Existing=LoadObject<T>(nullptr,*(Path+TEXT(".")+FPackageName::GetShortName(Path)))) return Existing;
 Created=true; auto* Asset=NewObject<T>(CreatePackage(*Path),FName(FPackageName::GetShortName(Path)),RF_Public|RF_Standalone);
 FAssetRegistryModule::AssetCreated(Asset); return Asset;
}
template<class T,class D> bool Definitions(const TArray<D>& Source,TArray<TSoftObjectPtr<T>>& Refs,const TCHAR* Kind) {
 for(const auto& Definition:Source) {
  FString Name=Definition.Id.ToString(); for(TCHAR& C:Name) if(!FChar::IsAlnum(C)&&C!=TCHAR('_')) C=TCHAR('_');
  const FString Path=FString::Printf(TEXT("/Game/Data/%s/DA_%s"),Kind,*Name); bool Created;
  auto* Asset=Seed<T>(Path,Created); if(Created) {Asset->Definition=Definition; if(!Save(Asset)) return false;}
  Refs.AddUnique(TSoftObjectPtr<T>(Asset));
 }
 return true;
}
}
bool VTSeedNativeAssets(UVTGameData* Data) {
 if(!Definitions<UVTShipAsset>(Data->Ships,Data->ShipAssets,TEXT("Ships"))||!Definitions<UVTEquipmentAsset>(Data->Loadouts,Data->EquipmentAssets,TEXT("Equipment"))||!Definitions<UVTSystemAsset>(Data->Systems,Data->SystemAssets,TEXT("Systems"))||!Definitions<UVTScenarioAsset>(Data->Scenarios,Data->ScenarioAssets,TEXT("Scenarios"))) return false;
 if(Data->ShipClass.IsNull()) {
  auto* BP=LoadObject<UBlueprint>(nullptr,TEXT("/Game/Ships/BP_Ship.BP_Ship"));
  if(!BP) {BP=FKismetEditorUtilities::CreateBlueprint(AVTShip::StaticClass(),CreatePackage(TEXT("/Game/Ships/BP_Ship")),TEXT("BP_Ship"),BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass()); FKismetEditorUtilities::CompileBlueprint(BP); if(!Save(BP)) return false;}
  Data->ShipClass=FSoftObjectPath(TEXT("/Game/Ships/BP_Ship.BP_Ship_C"));
 }
 if(Data->UIClass.IsNull()) Data->UIClass=FSoftObjectPath(TEXT("/Game/UI/WBP_UI.WBP_UI_C"));
 // Recompile bindings without rebuilding or replacing the designer's widget tree.
 if(auto* UI=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/WBP_UI.WBP_UI"))) {
  if(auto* Status=UI->WidgetTree->FindWidget(TEXT("Status"))) {
   FGuid Guid=UI->WidgetVariableNameToGuidMap.FindRef(TEXT("Status"));if(!Guid.IsValid())Guid=FGuid::NewGuid();
   UI->WidgetVariableNameToGuidMap.Remove(TEXT("Status"));UI->WidgetVariableNameToGuidMap.Add(TEXT("StatusText"),Guid);
   Status->Rename(TEXT("StatusText"),UI->WidgetTree);
  }
  // Repair a previously interrupted upgrade while retaining the widget's stable GUID.
  if(UI->WidgetTree->FindWidget(TEXT("StatusText"))) {UI->WidgetVariableNameToGuidMap.Remove(TEXT("Status"));if(!UI->WidgetVariableNameToGuidMap.Contains(TEXT("StatusText")))UI->WidgetVariableNameToGuidMap.Add(TEXT("StatusText"),FGuid::NewGuid());}
  FKismetEditorUtilities::CompileBlueprint(UI); if(UI->Status==BS_Error||!Save(UI)) return false;
 }
 if(!Save(Data)) return false;
 bool Created; auto* Attenuation=Seed<USoundAttenuation>(TEXT("/Game/Audio/SA_Ship"),Created);
 if(Created) {auto& S=Attenuation->Attenuation; S.bAttenuate=true; S.bSpatialize=true; S.AttenuationShapeExtents=FVector(10000); S.FalloffDistance=80000; S.DistanceAlgorithm=EAttenuationDistanceModel::Linear; if(!Save(Attenuation)) return false;}
 auto* Concurrency=Seed<USoundConcurrency>(TEXT("/Game/Audio/SC_Combat"),Created);
 if(Created) {Concurrency->Concurrency.MaxCount=32; Concurrency->Concurrency.ResolutionRule=EMaxConcurrentResolutionRule::StopQuietest; if(!Save(Concurrency)) return false;}
 for(const TCHAR* Name:{TEXT("Fire"),TEXT("Hit"),TEXT("Explosion"),TEXT("Board"),TEXT("Boost"),TEXT("Warp"),TEXT("Brace"),TEXT("HullWarning")}) {
  auto Path=FString::Printf(TEXT("/Game/Effects/Cues/GC_%s.GC_%s"),Name,Name); auto* BP=LoadObject<UBlueprint>(nullptr,*Path);
  if(BP) if(auto* Cue=Cast<UVTShipCue>(BP->GeneratedClass->GetDefaultObject())) {bool Changed=false; if(!Cue->Attenuation){Cue->Attenuation=Attenuation;Changed=true;} if(!Cue->Concurrency){Cue->Concurrency=Concurrency;Changed=true;} if(Changed&&!Save(BP)) return false;}
 }
 for(const auto& Pair:{TPair<const TCHAR*,int32>(TEXT("Burst"),128),TPair<const TCHAR*,int32>(TEXT("Trail"),256)}) {
  auto Path=FString::Printf(TEXT("/Game/Effects/NET_%s"),Pair.Key); auto* Type=Seed<UNiagaraEffectType>(Path,Created);
  if(Created) {Type->SignificanceHandler=NewObject<UNiagaraSignificanceHandlerDistance>(Type); FNiagaraSystemScalabilitySettings Settings; Settings.bCullByDistance=true; Settings.MaxDistance=160000; Settings.bCullMaxInstanceCount=true; Settings.MaxInstances=Pair.Value; Type->SystemScalabilitySettings.Settings.Add(Settings); if(!Save(Type)) return false;}
  const TCHAR* Name=Pair.Value==128 ? TEXT("NS_ShipBurst") : TEXT("NS_EngineTrail");
  auto* System=LoadObject<UNiagaraSystem>(nullptr,*FString::Printf(TEXT("/Game/Effects/%s.%s"),Name,Name));
  if(System&&!System->GetEffectType()) {System->SetEffectType(Type); if(!Save(System)) return false;}
 }
 // Common actions remain available in all UI/gameplay states; flight context is removed in menus.
 auto* Common=Seed<UInputMappingContext>(TEXT("/Game/Input/IMC_Common"),Created); bool CommonCreated=Created;
 const TCHAR* Names[]={TEXT("Menu"),TEXT("Autopilot"),TEXT("Recover")};
 const FKey Keys[]={EKeys::Escape,EKeys::T,EKeys::R}; const FKey Pads[]={EKeys::Gamepad_Special_Right,EKeys::Gamepad_LeftThumbstick,EKeys::Gamepad_DPad_Down};
 for(int I=0;I<3;++I) {auto* Action=Seed<UInputAction>(FString(TEXT("/Game/Input/IA_"))+Names[I],Created); if(Created) {Action->ValueType=EInputActionValueType::Boolean; Action->bTriggerWhenPaused=I==0; if(!Save(Action)) return false;} if(CommonCreated){Common->MapKey(Action,Keys[I]);Common->MapKey(Action,Pads[I]);}}
 if(CommonCreated&&!Save(Common)) return false;
 for(const TCHAR* Name:{TEXT("Menu"),TEXT("Docked")}) {auto* Context=Seed<UInputMappingContext>(FString(TEXT("/Game/Input/IMC_"))+Name,Created); if(Created&&!Save(Context)) return false;}
 for(const TCHAR* Name:{TEXT("Common"),TEXT("Flight")}) {
  auto* Context=LoadObject<UInputMappingContext>(nullptr,*FString::Printf(TEXT("/Game/Input/IMC_%s.IMC_%s"),Name,Name)); if(!Context) continue; bool Changed=false;
  for(int I=0;I<Context->GetMappings().Num();++I) {auto& Mapping=Context->GetMapping(I); if(Mapping.IsPlayerMappable()) continue;
   auto* Settings=NewObject<UPlayerMappableKeySettings>(Context); FString Action=Mapping.Action->GetName().RightChop(3);
   Settings->Name=FName(Action+TEXT("_")+Mapping.Key.GetFName().ToString()); Settings->DisplayName=FText::FromString(Action);Settings->DisplayCategory=NSLOCTEXT("VTInput","Flight","Ship controls");
   // The editor-facing mapping settings are reflected protected properties in UE 5.8.
   auto* Behavior=FindFProperty<FEnumProperty>(FEnhancedActionKeyMapping::StaticStruct(),TEXT("SettingBehavior"));
   auto* Mappable=FindFProperty<FObjectPropertyBase>(FEnhancedActionKeyMapping::StaticStruct(),TEXT("PlayerMappableKeySettings"));
   check(Behavior&&Mappable);Behavior->GetUnderlyingProperty()->SetIntPropertyValue(Behavior->ContainerPtrToValuePtr<void>(&Mapping),uint64(EPlayerMappableKeySettingBehaviors::OverrideSettings));
   Mappable->SetObjectPropertyValue_InContainer(&Mapping,Settings);Changed=true;
  }
  if(Changed&&!Save(Context)) return false;
 }
 return true;
}
