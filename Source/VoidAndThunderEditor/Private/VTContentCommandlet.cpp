#include "VTBootstrapCommandlet.h"
#include "VTGameData.h"
#include "AssetToolsModule.h"
#include "AssetImportTask.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshAttributes.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionNoise.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureObject.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Engine/Texture2D.h"
#include "VTCues.h"
#include "Sound/SoundBase.h"
#include "NiagaraSystem.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/Blueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMeshSocket.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"

static bool Persist(UObject* Asset) {
 auto* Package=Asset->GetOutermost(); Package->MarkAsFullyLoaded(); Package->MarkPackageDirty(); FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
 FString Path=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension()); IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
 return UPackage::SavePackage(Package,Asset,*Path,Args);
}
UVTContentCommandlet::UVTContentCommandlet() {IsEditor=true; IsClient=false; IsServer=false; LogToConsole=true;}
int32 UVTContentCommandlet::Main(const FString& Params) {
 if(!FParse::Param(*Params,TEXT("Regenerate"))&&(FPackageName::DoesPackageExist(TEXT("/Game/Effects/Cues/GC_Fire"))||FPackageName::DoesPackageExist(TEXT("/Game/Effects/NS_ShipBurst"))||IFileManager::Get().DirectoryExists(*(FPaths::ProjectContentDir()/TEXT("Ships/challenger"))))) {
  UE_LOG(LogTemp,Display,TEXT("Authored presentation retained. Use -Regenerate to explicitly replace imported content.")); return 0;
 }

 auto* Data=LoadObject<UVTGameData>(nullptr,TEXT("/Game/Data/DA_GameData.DA_GameData")); if(!Data) return 1;
 auto& Tools=FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
 const TCHAR* Models[]={TEXT("challenger"),TEXT("imperial"),TEXT("dispatcher"),TEXT("bob"),TEXT("executioner")};
 TMap<FString,UStaticMesh*> Meshes;
 for(const auto* Name:Models) {
  auto* Task=NewObject<UAssetImportTask>(); Task->Filename=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("SourceAssets/models")/(FString(Name)+TEXT(".glb")));
  Task->DestinationPath=FString(TEXT("/Game/Ships/"))+Name; Task->DestinationName=Name; Task->bAutomated=true; Task->bReplaceExisting=true; Task->bSave=true; Task->bAsync=false;
  Tools.ImportAssetTasks({Task}); UStaticMesh* Mesh=nullptr;
  for(auto* Asset:Task->GetObjects()) if(auto* Imported=Cast<UStaticMesh>(Asset)) {Mesh=Imported; break;}
  if(!Mesh) {UE_LOG(LogTemp,Error,TEXT("GLB import failed: %s"),Name); return 2;}
  // Legacy GLBs use Z up. glTF import assumes Y up, so undo that basis rotation in the native mesh.
  if(auto* Description=Mesh->GetMeshDescription(0)) {FStaticMeshAttributes Attributes(*Description); auto Positions=Attributes.GetVertexPositions(); FQuat Rotation(FVector::ForwardVector,PI/2);
   for(auto Vertex:Description->Vertices().GetElementIDs()) Positions[Vertex]=FVector3f(Rotation.RotateVector(FVector(Positions[Vertex])));
   Mesh->CommitMeshDescription(0); Mesh->GetSourceModel(0).BuildSettings.bRecomputeNormals=true; Mesh->GetSourceModel(0).BuildSettings.bRecomputeTangents=true; Mesh->Build(false);}
  const TCHAR* Colour=FString(Name)==TEXT("challenger") ? TEXT("purple") : FString(Name)==TEXT("imperial") ? TEXT("red") : FString(Name)==TEXT("dispatcher") ? TEXT("blue") : FString(Name)==TEXT("bob") ? TEXT("orange") : TEXT("green");
  auto* TextureTask=NewObject<UAssetImportTask>(); TextureTask->Filename=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("SourceAssets/models/textures")/(FString(Name)+TEXT("_")+Colour+TEXT(".png"))); TextureTask->DestinationPath=FString(TEXT("/Game/Ships/"))+Name; TextureTask->bAutomated=true; TextureTask->bReplaceExisting=true; TextureTask->bSave=true; Tools.ImportAssetTasks({TextureTask});
  UTexture2D* Texture=nullptr; for(auto* Asset:TextureTask->GetObjects()) if(auto* T=Cast<UTexture2D>(Asset)) Texture=T;
  if(!Texture) return 6;
  FString MaterialPath=FString(TEXT("/Game/Ships/"))+Name+TEXT("/M_Hull"); auto* Material=NewObject<UMaterial>(CreatePackage(*MaterialPath),TEXT("M_Hull"),RF_Public|RF_Standalone);
  auto* Sample=NewObject<UMaterialExpressionTextureSample>(Material); Sample->Texture=Texture; Sample->SamplerType=SAMPLERTYPE_Color; Material->GetExpressionCollection().AddExpression(Sample); Material->GetEditorOnlyData()->BaseColor.Connect(0,Sample); Material->PostEditChange(); if(!Persist(Material)) return 7;
  for(auto& Slot:Mesh->GetStaticMaterials()) Slot.MaterialInterface=Material;
  const auto Bounds=Mesh->GetBounds(); UE_LOG(LogTemp,Display,TEXT("Model %s imported %s extent %s"),Name,*Mesh->GetPathName(),*Bounds.BoxExtent.ToString());
  for(int I=0;I<2;++I) {FName SocketName=FName(I==0 ? TEXT("EnginePort") : TEXT("EngineStarboard")); auto* Socket=Mesh->FindSocket(SocketName); if(!Socket) {Socket=NewObject<UStaticMeshSocket>(Mesh); Socket->SocketName=SocketName; Mesh->AddSocket(Socket);} Socket->RelativeLocation=FVector(-2000,I==0 ? -600 : 600,0);}
  if(!Persist(Mesh)) return 3; Meshes.Add(Name,Mesh);
 }
 for(FName Faction:{FName("Corsairs"),FName("Houses"),FName("Vethara"),FName("Ondrak"),FName("Caelifen"),FName("Mourne"),FName("Aethon"),FName("Ecclesiatum"),FName("Guild"),FName("Janissariat"),FName("Freebooters")}) {FString Name=Faction==FName("Corsairs") ? TEXT("executioner") : Faction==FName("Guild") ? TEXT("dispatcher") : Faction==FName("Janissariat") ? TEXT("bob") : Faction==FName("Freebooters") ? TEXT("challenger") : TEXT("imperial"); Data->FactionMeshes.Add(Faction,Meshes.FindRef(Name));}
 for(auto& Ship:Data->Ships) {FString Name=Ship.Id.ToString().StartsWith(TEXT("corsair")) ? TEXT("executioner") : TEXT("imperial"); Ship.Mesh=Meshes.FindRef(Name);}
 if(!Persist(Data)) return 4;
 auto* Strings=NewObject<UStringTable>(CreatePackage(TEXT("/Game/UI/ST_UI")),TEXT("ST_UI"),RF_Public|RF_Standalone); Strings->GetMutableStringTable()->SetNamespace(TEXT("VT.UI"));
 FString Json; FFileHelper::LoadFileToString(Json,*(FPaths::ProjectDir()/TEXT("SourceAssets/strings/en.json"))); TSharedPtr<FJsonObject> Root;
 if(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root)) for(const auto& Pair:Root->Values) if(Pair.Value->Type==EJson::String) Strings->GetMutableStringTable()->SetSourceString(FTextKey(Pair.Key),Pair.Value->AsString(),TEXT("Transferred from source UI"));
 if(!Persist(Strings)) return 5;
 auto* SkyImport=NewObject<UAssetImportTask>(); SkyImport->Filename=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("SourceAssets/skybox/phoenix_space_cubemap.png")); SkyImport->DestinationPath=TEXT("/Game/Environment"); SkyImport->bAutomated=true; SkyImport->bReplaceExisting=true; SkyImport->bSave=true; Tools.ImportAssetTasks({SkyImport});
 UTexture2D* SkyTexture=nullptr; for(auto* Object:SkyImport->GetObjects()) if(auto* T=Cast<UTexture2D>(Object)) SkyTexture=T; if(!SkyTexture) return 13;
 auto* Sky=NewObject<UMaterial>(CreatePackage(TEXT("/Game/Environment/M_SkyAtlas")),TEXT("M_SkyAtlas"),RF_Public|RF_Standalone); Sky->SetShadingModel(MSM_Unlit); Sky->TwoSided=true;
 auto* Tex=NewObject<UMaterialExpressionTextureObject>(Sky); Tex->Texture=SkyTexture; Sky->GetExpressionCollection().AddExpression(Tex);
 auto* Direction=NewObject<UMaterialExpressionCameraVectorWS>(Sky); Sky->GetExpressionCollection().AddExpression(Direction);
 auto* Sample=NewObject<UMaterialExpressionCustom>(Sky); Sample->OutputType=CMOT_Float3;
 FCustomInput AtlasInput; AtlasInput.InputName=TEXT("Atlas"); AtlasInput.Input.Connect(0,Tex); Sample->Inputs.Add(AtlasInput); FCustomInput DirectionInput; DirectionInput.InputName=TEXT("Direction"); DirectionInput.Input.Connect(0,Direction); Sample->Inputs.Add(DirectionInput);
 Sample->Code=TEXT("float3 d=-normalize(Direction); d.y=-d.y; float3 a=abs(d); float2 uv; float face; if(a.x>=a.y && a.x>=a.z){face=d.x>0?0:1;uv=float2(d.x>0?-d.z:d.z,-d.y)/a.x;}else if(a.y>=a.z){face=d.y>0?2:3;uv=float2(d.x,d.y>0?d.z:-d.z)/a.y;}else{face=d.z>0?4:5;uv=float2(d.z>0?d.x:-d.x,-d.y)/a.z;}uv=uv*0.5+0.5;uv.y=(uv.y+face)/6;return Texture2DSample(Atlas,AtlasSampler,uv).rgb*0.6;"); Sky->GetExpressionCollection().AddExpression(Sample); Sky->GetEditorOnlyData()->EmissiveColor.Connect(0,Sample); Sky->PostEditChange(); if(!Persist(Sky)) return 14;

 const TCHAR* MaterialNames[]={TEXT("M_Star"),TEXT("M_Station"),TEXT("M_Jump")};
 const FLinearColor Colours[]={FLinearColor(3,0.65f,0.08f),FLinearColor(0.9f,0.55f,0.1f),FLinearColor(0.05f,0.5f,1.4f)};
 for(int I=0;I<3;++I) {FString Path=FString(TEXT("/Game/Environment/"))+MaterialNames[I]; auto* M=NewObject<UMaterial>(CreatePackage(*Path),FName(MaterialNames[I]),RF_Public|RF_Standalone); M->SetShadingModel(MSM_Unlit);
  auto* Colour=NewObject<UMaterialExpressionConstant3Vector>(M); Colour->Constant=Colours[I]; M->GetExpressionCollection().AddExpression(Colour);
  if(I==0) {auto* Noise=NewObject<UMaterialExpressionNoise>(M); Noise->Scale=0.003f; Noise->Levels=3; Noise->OutputMin=0.05f; Noise->OutputMax=0.4f; M->GetExpressionCollection().AddExpression(Noise);
   auto* WorldPosition=NewObject<UMaterialExpressionWorldPosition>(M); M->GetExpressionCollection().AddExpression(WorldPosition);
   auto* Drift=NewObject<UMaterialExpressionPanner>(M); Drift->SpeedX=20; Drift->SpeedY=12; M->GetExpressionCollection().AddExpression(Drift);
   auto* Position=NewObject<UMaterialExpressionAdd>(M); Position->A.Connect(0,WorldPosition); Position->B.Connect(0,Drift); M->GetExpressionCollection().AddExpression(Position); Noise->Position.Connect(0,Position);
   auto* Multiply=NewObject<UMaterialExpressionMultiply>(M); Multiply->A.Connect(0,Colour); Multiply->B.Connect(0,Noise); M->GetExpressionCollection().AddExpression(Multiply); M->GetEditorOnlyData()->EmissiveColor.Connect(0,Multiply);} else M->GetEditorOnlyData()->EmissiveColor.Connect(0,Colour);
  M->PostEditChange(); if(!Persist(M)) return 12;
 }
 auto* Template=LoadObject<UNiagaraSystem>(nullptr,TEXT("/Niagara/DefaultAssets/Templates/Systems/DirectionalBurst.DirectionalBurst")); if(!Template) return 8;
 auto* Burst=DuplicateObject<UNiagaraSystem>(Template,CreatePackage(TEXT("/Game/Effects/NS_ShipBurst")),TEXT("NS_ShipBurst")); Burst->SetFlags(RF_Public|RF_Standalone); if(!Persist(Burst)) return 9;
 auto* TrailTemplate=LoadObject<UNiagaraSystem>(nullptr,TEXT("/Niagara/DefaultAssets/Templates/Systems/FountainLightweight.FountainLightweight")); if(!TrailTemplate) return 15;
 auto* Trail=DuplicateObject<UNiagaraSystem>(TrailTemplate,CreatePackage(TEXT("/Game/Effects/NS_EngineTrail")),TEXT("NS_EngineTrail")); Trail->SetFlags(RF_Public|RF_Standalone); if(!Persist(Trail)) return 16;
 const TCHAR* Sounds[]={TEXT("broadside"),TEXT("hit"),TEXT("explosion"),TEXT("board"),TEXT("boost"),TEXT("warp"),TEXT("brace"),TEXT("hullwarn")};
 const TCHAR* Cues[]={TEXT("Fire"),TEXT("Hit"),TEXT("Explosion"),TEXT("Board"),TEXT("Boost"),TEXT("Warp"),TEXT("Brace"),TEXT("HullWarning")};
 for(int I=0;I<8;++I) {
  auto* Task=NewObject<UAssetImportTask>(); Task->Filename=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("SourceAssets/audio")/(FString(Sounds[I])+TEXT(".wav"))); Task->DestinationPath=TEXT("/Game/Audio"); Task->bAutomated=true; Task->bReplaceExisting=true; Task->bSave=true; Tools.ImportAssetTasks({Task});
  USoundBase* Sound=nullptr; for(auto* Asset:Task->GetObjects()) if(auto* S=Cast<USoundBase>(Asset)) Sound=S; if(!Sound) return 10;
  FString Path=FString(TEXT("/Game/Effects/Cues/GC_"))+Cues[I]; auto* BP=FKismetEditorUtilities::CreateBlueprint(UVTShipCue::StaticClass(),CreatePackage(*Path),FName(FString(TEXT("GC_"))+Cues[I]),BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass()); FKismetEditorUtilities::CompileBlueprint(BP);
  auto* Cue=CastChecked<UVTShipCue>(BP->GeneratedClass->GetDefaultObject()); Cue->GameplayCueTag=FGameplayTag::RequestGameplayTag(FName(FString(TEXT("GameplayCue.Ship."))+Cues[I])); Cue->GameplayCueName=Cue->GameplayCueTag.GetTagName(); Cue->Sound=Sound; Cue->Effect=(I==0||I==1||I==2||I==5) ? Burst : nullptr;
  FAssetRegistryModule::AssetCreated(BP); if(!Persist(BP)) return 11;
 }
 UE_LOG(LogTemp,Display,TEXT("Native models, engine sockets and UI string table transferred.")); return 0;
}
