#include "VTBootstrapCommandlet.h"
#include "VTIntro.h"
#include "VTIntroWidget.h"
#include "VTUI.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshAttributes.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
namespace {
void CompileIntroWidget(UWidgetBlueprint* BP){TArray<UWidget*> Widgets;BP->WidgetTree->GetAllWidgets(Widgets);for(auto* W:Widgets)if(!BP->WidgetVariableNameToGuidMap.Contains(W->GetFName()))BP->WidgetVariableNameToGuidMap.Add(W->GetFName(),FGuid::NewGuid());FKismetEditorUtilities::CompileBlueprint(BP);}
bool SaveIntroAsset(UObject* Asset){auto* P=Asset->GetOutermost();P->FullyLoad();P->MarkPackageDirty();FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;auto Path=FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension());IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);return UPackage::SavePackage(P,Asset,*Path,Args);}
}
UVTIntroAssetsCommandlet::UVTIntroAssetsCommandlet(){IsEditor=true;LogToConsole=true;}
int32 UVTIntroAssetsCommandlet::Main(const FString& Params){
 if(!FParse::Param(*Params,TEXT("Apply")))return 1;
 const bool CreateWidget=!FPackageName::DoesPackageExist(TEXT("/Game/UI/WBP_Intro"));
 auto Portrait=[](const TCHAR* Name){const FString Path=FString(TEXT("/Game/UI/Intro/T_"))+Name;auto* Existing=LoadObject<UTexture2D>(nullptr,*(Path+TEXT(".T_")+Name));if(Existing)return Existing;
  auto* Factory=NewObject<UTextureFactory>();Factory->SuppressImportOverwriteDialog();const auto File=FPaths::ProjectDir()/TEXT("ArtSource/Intro")/(FString(Name)+TEXT(".png"));auto* T=Cast<UTexture2D>(UFactory::StaticImportObject(UTexture2D::StaticClass(),CreatePackage(*Path),FName(FString(TEXT("T_"))+Name),RF_Public|RF_Standalone,*File,nullptr,Factory));if(T){T->LODGroup=TEXTUREGROUP_UI;T->NeverStream=true;T->SRGB=true;T->PostEditChange();if(!SaveIntroAsset(T))return static_cast<UTexture2D*>(nullptr);}return T;};
 auto* Engineer=Portrait(TEXT("Engineer"));auto* Captain=Portrait(TEXT("EnemyCaptain"));if(!Engineer||!Captain)return 2;
 if(!FPackageName::DoesPackageExist(TEXT("/Game/Data/DA_Intro"))){auto* D=NewObject<UVTIntroData>(CreatePackage(TEXT("/Game/Data/DA_Intro")),TEXT("DA_Intro"),RF_Public|RF_Standalone);D->EngineerPortrait=Engineer;D->CaptainPortrait=Captain;if(!SaveIntroAsset(D))return 3;}
 if(FParse::Param(*Params,TEXT("RepairChoices"))){
  auto* D=LoadObject<UVTIntroData>(nullptr,TEXT("/Game/Data/DA_Intro.DA_Intro"));auto* Defaults=GetDefault<UVTIntroData>();if(!D)return 8;
  for(const auto& Beat:Defaults->Beats)if(!D->Beat(Beat.Stage))D->Beats.Add(Beat);else if(Beat.Stage==EVTIntroStage::Repairs)for(auto& Existing:D->Beats)if(Existing.Stage==Beat.Stage)Existing=Beat;
  if(D->BatteryRepairs.IsEmpty())D->BatteryRepairs=Defaults->BatteryRepairs;if(D->SpecialRepairs.IsEmpty())D->SpecialRepairs=Defaults->SpecialRepairs;if(!SaveIntroAsset(D))return 9;
 }
 if(!FPackageName::DoesPackageExist(TEXT("/Game/Environment/M_IntroDebris"))){auto* M=NewObject<UMaterial>(CreatePackage(TEXT("/Game/Environment/M_IntroDebris")),TEXT("M_IntroDebris"),RF_Public|RF_Standalone);auto* C=NewObject<UMaterialExpressionConstant3Vector>(M);C->Constant=FLinearColor(0.09f,0.065f,0.045f);M->GetExpressionCollection().AddExpression(C);M->GetEditorOnlyData()->BaseColor.Expression=C;auto* Rough=NewObject<UMaterialExpressionConstant>(M);Rough->R=0.9f;M->GetExpressionCollection().AddExpression(Rough);M->GetEditorOnlyData()->Roughness.Expression=Rough;M->PostEditChange();if(!SaveIntroAsset(M))return 6;}
 if(FParse::Param(*Params,TEXT("RefreshVisuals"))){if(auto* BP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/WBP_Intro.WBP_Intro"))){if(auto* Canvas=Cast<UCanvasPanel>(BP->WidgetTree->RootWidget))if(auto* C=Canvas->GetChildAt(0))if(auto* Slot=Cast<UCanvasPanelSlot>(C->Slot))Slot->SetPosition(FVector2D(0,82));CompileIntroWidget(BP);if(!SaveIntroAsset(BP))return 7;}}
 if(!FPackageName::DoesPackageExist(TEXT("/Game/Environment/SM_IntroDebris"))){
  auto* Mesh=NewObject<UStaticMesh>(CreatePackage(TEXT("/Game/Environment/SM_IntroDebris")),TEXT("SM_IntroDebris"),RF_Public|RF_Standalone);FMeshDescription Shape;FStaticMeshAttributes A(Shape);A.Register();auto P=A.GetVertexPositions();auto N=A.GetVertexInstanceNormals();auto UV=A.GetVertexInstanceUVs();UV.SetNumChannels(1);auto Group=Shape.CreatePolygonGroup();A.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Hull");
  const FVector3f Points[]={{-70,-30,-12},{35,-48,-10},{70,12,-8},{10,42,-12},{-60,25,-10},{-58,-25,18},{28,-40,27},{55,9,16},{8,32,22},{-52,20,28}};TArray<FVertexID> V;for(const auto& Pt:Points){auto Id=Shape.CreateVertex();P[Id]=Pt;V.Add(Id);}auto Tri=[&](int X,int Y,int Z){TArray<FVertexInstanceID> IDs;for(int I:{X,Y,Z}){auto Id=Shape.CreateVertexInstance(V[I]);N[Id]=FVector3f::CrossProduct(Points[Y]-Points[X],Points[Z]-Points[X]).GetSafeNormal();UV.Set(Id,0,FVector2f((Points[I].X+70)/140,(Points[I].Y+48)/96));IDs.Add(Id);}Shape.CreatePolygon(Group,IDs);};for(int I=1;I<4;++I){Tri(0,I+1,I);Tri(5,5+I,6+I);}for(int I=0;I<5;++I){int J=(I+1)%5;Tri(I,J,5+J);Tri(I,5+J,5+I);}
  Mesh->GetStaticMaterials().Add(FStaticMaterial(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Environment/M_IntroDebris.M_IntroDebris")),TEXT("Hull")));UStaticMesh::FBuildMeshDescriptionsParams Build;Build.bCommitMeshDescription=true;Build.bBuildSimpleCollision=false;Mesh->BuildFromMeshDescriptions({&Shape},Build);if(!SaveIntroAsset(Mesh))return 4;
 }
 if(!FPackageName::DoesPackageExist(TEXT("/Game/UI/WBP_Intro"))){
 auto* BP=CastChecked<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UVTIntroWidget::StaticClass(),CreatePackage(TEXT("/Game/UI/WBP_Intro")),TEXT("WBP_Intro"),BPTYPE_Normal,UWidgetBlueprint::StaticClass(),UWidgetBlueprintGeneratedClass::StaticClass()));auto* Tree=BP->WidgetTree.Get();
 auto* Root=Tree->ConstructWidget<UCanvasPanel>();Tree->RootWidget=Root;auto* Bounds=Tree->ConstructWidget<USizeBox>();Bounds->SetWidthOverride(780);auto* Slot=Root->AddChildToCanvas(Bounds);Slot->SetAnchors(FAnchors(0.5f,0));Slot->SetAlignment(FVector2D(0.5f,0));Slot->SetPosition(FVector2D(0,82));Slot->SetAutoSize(true);
 auto* Border=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("Comms"));Border->SetPadding(FMargin(14));Border->SetBrushColor(FLinearColor(0.018f,0.012f,0.006f,0.97f));Bounds->SetContent(Border);auto* Rows=Tree->ConstructWidget<UVerticalBox>();Border->SetContent(Rows);auto* Line=Tree->ConstructWidget<UHorizontalBox>();Rows->AddChild(Line);
 auto* FaceBounds=Tree->ConstructWidget<USizeBox>();FaceBounds->SetWidthOverride(128);FaceBounds->SetHeightOverride(128);auto* Face=Tree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("Portrait"));FaceBounds->SetContent(Face);Line->AddChild(FaceBounds);auto* Words=Tree->ConstructWidget<UVerticalBox>();Line->AddChildToHorizontalBox(Words)->SetPadding(FMargin(16,0,0,0));
 auto* Style=LoadClass<UVTUI>(nullptr,TEXT("/Game/UI/WBP_UI.WBP_UI_C"));const auto Font=Style?Style->GetDefaultObject<UVTUI>()->HudFont:FSlateFontInfo();
 auto Text=[&](UVerticalBox* Box,const TCHAR* Name,int Size,FLinearColor Colour){auto* T=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),FName(Name));auto F=Font;F.Size=Size;T->SetFont(F);T->SetColorAndOpacity(Colour);T->SetWrapTextAt(590);T->SetMargin(FMargin(0,2));Box->AddChild(T);return T;};
 const FLinearColor Amber(1,0.65f,0.08f),Pale(0.91f,0.82f,0.65f);Text(Words,TEXT("Speaker"),13,Amber);Text(Words,TEXT("Speech"),14,Pale);Text(Words,TEXT("Objective"),13,Amber);Text(Rows,TEXT("Controls"),11,Pale)->SetWrapTextAt(750);Text(Rows,TEXT("Repairs"),10,Amber)->SetWrapTextAt(750);
 auto* Buttons=Tree->ConstructWidget<UHorizontalBox>();Rows->AddChild(Buttons);for(auto Pair:{TPair<const TCHAR*,const TCHAR*>(TEXT("Continue"),TEXT("Continue  [Enter / Pad A]")),TPair<const TCHAR*,const TCHAR*>(TEXT("Skip"),TEXT("Skip intro"))}){auto* B=Tree->ConstructWidget<UButton>(UButton::StaticClass(),FName(Pair.Key));auto* T=Tree->ConstructWidget<UTextBlock>();auto F=Font;F.Size=12;T->SetFont(F);T->SetColorAndOpacity(Amber);T->SetText(FText::FromString(Pair.Value));B->SetContent(T);B->SetBackgroundColor(FLinearColor(0.1f,0.07f,0.02f));Buttons->AddChildToHorizontalBox(B)->SetPadding(FMargin(0,6,12,0));}
 CompileIntroWidget(BP);if(BP->Status==BS_Error||!SaveIntroAsset(BP))return 5;
 }
 if(CreateWidget||FParse::Param(*Params,TEXT("RepairChoices"))){auto* BP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/WBP_Intro.WBP_Intro"));if(!BP)return 10;auto* Tree=BP->WidgetTree.Get();auto* Root=Cast<UBorder>(Tree->FindWidget(TEXT("Comms")));auto* Rows=Root?Cast<UVerticalBox>(Root->GetContent()):nullptr;auto* Speech=Cast<UTextBlock>(Tree->FindWidget(TEXT("Speech")));if(!Rows||!Speech)return 11;
  for(const TCHAR* Name:{TEXT("ChoiceA"),TEXT("ChoiceB")})if(!Tree->FindWidget(FName(Name))){auto* B=Tree->ConstructWidget<UButton>(UButton::StaticClass(),FName(Name));auto* T=Tree->ConstructWidget<UTextBlock>();auto Font=Speech->GetFont();Font.Size=12;T->SetFont(Font);T->SetWrapTextAt(720);T->SetMargin(FMargin(5));T->SetColorAndOpacity(FLinearColor(1,0.65f,0.08f));B->SetContent(T);B->SetBackgroundColor(FLinearColor(0.08f,0.045f,0.01f));B->SetVisibility(ESlateVisibility::Collapsed);Rows->AddChild(B);}
  CompileIntroWidget(BP);if(!SaveIntroAsset(BP))return 12;
 }return 0;
}
