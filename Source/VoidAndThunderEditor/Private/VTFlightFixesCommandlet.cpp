#include "VTBootstrapCommandlet.h"
#include "VTGameData.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshAttributes.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
namespace {
bool SaveFlightAsset(UObject* Asset){auto* P=Asset->GetOutermost();P->FullyLoad();P->MarkPackageDirty();FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;return UPackage::SavePackage(P,Asset,*FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension()),Args);}
}
UVTFlightFixesCommandlet::UVTFlightFixesCommandlet(){IsEditor=true;LogToConsole=true;}
int32 UVTFlightFixesCommandlet::Main(const FString& Params){
 if(!FParse::Param(*Params,TEXT("Apply")))return 1;
 auto* Material=FPackageName::DoesPackageExist(TEXT("/Game/Environment/M_Torpedo"))?LoadObject<UMaterial>(nullptr,TEXT("/Game/Environment/M_Torpedo.M_Torpedo")):NewObject<UMaterial>(CreatePackage(TEXT("/Game/Environment/M_Torpedo")),TEXT("M_Torpedo"),RF_Public|RF_Standalone);Material->PreEditChange(nullptr);Material->GetExpressionCollection().Empty();Material->SetShadingModel(MSM_Unlit);auto* Red=NewObject<UMaterialExpressionConstant3Vector>(Material);Red->Constant=FLinearColor(0.4f,0.f,0.f);Material->GetExpressionCollection().AddExpression(Red);Material->GetEditorOnlyData()->EmissiveColor.Connect(0,Red);Material->PostEditChange();if(!SaveFlightAsset(Material))return 2;if(FParse::Param(*Params,TEXT("TorpedoOnly"))){auto* Data=LoadObject<UVTGameData>(nullptr,TEXT("/Game/Data/DA_GameData.DA_GameData"));if(!Data)return 4;Data->TorpedoVisualRadius=1.25f;return SaveFlightAsset(Data)?0:5;}
 auto* Ring=FPackageName::DoesPackageExist(TEXT("/Game/Environment/SM_JumpRing"))?LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Environment/SM_JumpRing.SM_JumpRing")):NewObject<UStaticMesh>(CreatePackage(TEXT("/Game/Environment/SM_JumpRing")),TEXT("SM_JumpRing"),RF_Public|RF_Standalone);Ring->GetStaticMaterials().Reset();
 FMeshDescription Mesh;FStaticMeshAttributes Attributes(Mesh);Attributes.Register();auto Positions=Attributes.GetVertexPositions();auto Normals=Attributes.GetVertexInstanceNormals();auto UVs=Attributes.GetVertexInstanceUVs();UVs.SetNumChannels(1);const auto Group=Mesh.CreatePolygonGroup();Attributes.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Gate");
 constexpr int Sides=64,TubeSides=12;TArray<FVertexID> Vertices;TArray<FVector3f> Directions;
 for(int I=0;I<Sides;++I)for(int J=0;J<TubeSides;++J){float U=2*PI*I/Sides,V=2*PI*J/TubeSides;auto Id=Mesh.CreateVertex();Positions[Id]=FVector3f(6*FMath::Sin(V),(106+6*FMath::Cos(V))*FMath::Cos(U),(106+6*FMath::Cos(V))*FMath::Sin(U));Vertices.Add(Id);Directions.Add(FVector3f(FMath::Sin(V),FMath::Cos(V)*FMath::Cos(U),FMath::Cos(V)*FMath::Sin(U)));}
 auto Triangle=[&](int A,int B,int C){TArray<FVertexInstanceID> Instances;for(int Index:{A,B,C}){auto Instance=Mesh.CreateVertexInstance(Vertices[Index]);Normals[Instance]=Directions[Index];UVs.Set(Instance,0,FVector2f(float(Index/TubeSides)/Sides,float(Index%TubeSides)/TubeSides));Instances.Add(Instance);}Mesh.CreatePolygon(Group,Instances);};
 for(int I=0;I<Sides;++I)for(int J=0;J<TubeSides;++J){int A=I*TubeSides+J,B=((I+1)%Sides)*TubeSides+J,C=((I+1)%Sides)*TubeSides+(J+1)%TubeSides,D=I*TubeSides+(J+1)%TubeSides;Triangle(A,B,C);Triangle(A,C,D);}
 Ring->GetStaticMaterials().Add(FStaticMaterial(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Environment/M_Jump.M_Jump")),TEXT("Gate")));UStaticMesh::FBuildMeshDescriptionsParams Build;Build.bBuildSimpleCollision=false;Build.bFastBuild=false;Build.bCommitMeshDescription=true;Ring->BuildFromMeshDescriptions({&Mesh},Build);if(!SaveFlightAsset(Ring))return 3;
 auto* Data=LoadObject<UVTGameData>(nullptr,TEXT("/Game/Data/DA_GameData.DA_GameData"));if(!Data)return 4;Data->TorpedoVisualRadius=1.25f;return SaveFlightAsset(Data)?0:5;
}
