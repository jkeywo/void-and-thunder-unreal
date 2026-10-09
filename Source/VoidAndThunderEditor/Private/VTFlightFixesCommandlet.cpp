#include "VTBootstrapCommandlet.h"
#include "VTGameData.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshAttributes.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
namespace {
bool SaveFlightAsset(UObject* Asset){auto* P=Asset->GetOutermost();P->FullyLoad();P->MarkPackageDirty();FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;return UPackage::SavePackage(P,Asset,*FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension()),Args);}
}
UVTFlightFixesCommandlet::UVTFlightFixesCommandlet(){IsEditor=true;LogToConsole=true;}
int32 UVTFlightFixesCommandlet::Main(const FString& Params){
 if(!FParse::Param(*Params,TEXT("Apply")))return 1;
 if(FParse::Param(*Params,TEXT("GateMarkers"))) {
  const TCHAR* Path=TEXT("/Game/Environment/SM_GateArrow");auto* Arrow=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Environment/SM_GateArrow.SM_GateArrow"));if(!Arrow)Arrow=NewObject<UStaticMesh>(CreatePackage(Path),TEXT("SM_GateArrow"),RF_Public|RF_Standalone);
  FMeshDescription Shape;FStaticMeshAttributes Attributes(Shape);Attributes.Register();auto Positions=Attributes.GetVertexPositions();auto Normals=Attributes.GetVertexInstanceNormals();auto UVs=Attributes.GetVertexInstanceUVs();UVs.SetNumChannels(1);auto Group=Shape.CreatePolygonGroup();Attributes.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Arrow");
  const FVector3f Points[]={{-50,-12,0},{10,-12,0},{10,-25,0},{50,0,0},{10,25,0},{10,12,0},{-50,12,0}};TArray<FVertexID> Vertices;for(const auto& Point:Points){auto Id=Shape.CreateVertex();Positions[Id]=Point;Vertices.Add(Id);}
  for(const FIntVector Triangle:{FIntVector(0,1,5),FIntVector(0,5,6),FIntVector(2,3,4),FIntVector(5,1,0),FIntVector(6,5,0),FIntVector(4,3,2)}){TArray<FVertexInstanceID> Instances;for(int Index:{Triangle.X,Triangle.Y,Triangle.Z}){auto Instance=Shape.CreateVertexInstance(Vertices[Index]);Normals[Instance]=FVector3f::CrossProduct(Points[Triangle.Y]-Points[Triangle.X],Points[Triangle.Z]-Points[Triangle.X]).GetSafeNormal();UVs.Set(Instance,0,FVector2f((Points[Index].X+50)/100,(Points[Index].Y+25)/50));Instances.Add(Instance);}Shape.CreatePolygon(Group,Instances);}
  Arrow->GetStaticMaterials().Reset();Arrow->GetStaticMaterials().Add(FStaticMaterial(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Environment/M_Jump.M_Jump")),TEXT("Arrow")));UStaticMesh::FBuildMeshDescriptionsParams Build;Build.bBuildSimpleCollision=false;Build.bFastBuild=false;Build.bCommitMeshDescription=true;Arrow->BuildFromMeshDescriptions({&Shape},Build);
  auto* Data=LoadObject<UVTGameData>(nullptr,TEXT("/Game/Data/DA_GameData.DA_GameData"));return SaveFlightAsset(Arrow)&&Data&&SaveFlightAsset(Data)?0:6;
 }
 if(FParse::Param(*Params,TEXT("DataOnly"))||FParse::Param(*Params,TEXT("TorpedoOnly"))){auto* Data=LoadObject<UVTGameData>(nullptr,TEXT("/Game/Data/DA_GameData.DA_GameData"));return Data&&SaveFlightAsset(Data)?0:4;}
 auto* Ring=FPackageName::DoesPackageExist(TEXT("/Game/Environment/SM_JumpRing"))?LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Environment/SM_JumpRing.SM_JumpRing")):NewObject<UStaticMesh>(CreatePackage(TEXT("/Game/Environment/SM_JumpRing")),TEXT("SM_JumpRing"),RF_Public|RF_Standalone);Ring->GetStaticMaterials().Reset();
 FMeshDescription Mesh;FStaticMeshAttributes Attributes(Mesh);Attributes.Register();auto Positions=Attributes.GetVertexPositions();auto Normals=Attributes.GetVertexInstanceNormals();auto UVs=Attributes.GetVertexInstanceUVs();UVs.SetNumChannels(1);const auto Group=Mesh.CreatePolygonGroup();Attributes.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Gate");
 constexpr int Sides=64,TubeSides=12;TArray<FVertexID> Vertices;TArray<FVector3f> Directions;
 for(int I=0;I<Sides;++I)for(int J=0;J<TubeSides;++J){float U=2*PI*I/Sides,V=2*PI*J/TubeSides;auto Id=Mesh.CreateVertex();Positions[Id]=FVector3f(6*FMath::Sin(V),(106+6*FMath::Cos(V))*FMath::Cos(U),(106+6*FMath::Cos(V))*FMath::Sin(U));Vertices.Add(Id);Directions.Add(FVector3f(FMath::Sin(V),FMath::Cos(V)*FMath::Cos(U),FMath::Cos(V)*FMath::Sin(U)));}
 auto Triangle=[&](int A,int B,int C){TArray<FVertexInstanceID> Instances;for(int Index:{A,B,C}){auto Instance=Mesh.CreateVertexInstance(Vertices[Index]);Normals[Instance]=Directions[Index];UVs.Set(Instance,0,FVector2f(float(Index/TubeSides)/Sides,float(Index%TubeSides)/TubeSides));Instances.Add(Instance);}Mesh.CreatePolygon(Group,Instances);};
 for(int I=0;I<Sides;++I)for(int J=0;J<TubeSides;++J){int A=I*TubeSides+J,B=((I+1)%Sides)*TubeSides+J,C=((I+1)%Sides)*TubeSides+(J+1)%TubeSides,D=I*TubeSides+(J+1)%TubeSides;Triangle(A,B,C);Triangle(A,C,D);}
 Ring->GetStaticMaterials().Add(FStaticMaterial(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Environment/M_Jump.M_Jump")),TEXT("Gate")));UStaticMesh::FBuildMeshDescriptionsParams Build;Build.bBuildSimpleCollision=false;Build.bFastBuild=false;Build.bCommitMeshDescription=true;Ring->BuildFromMeshDescriptions({&Mesh},Build);if(!SaveFlightAsset(Ring))return 3;
 auto* Data=LoadObject<UVTGameData>(nullptr,TEXT("/Game/Data/DA_GameData.DA_GameData"));if(!Data)return 4;return SaveFlightAsset(Data)?0:5;
}
