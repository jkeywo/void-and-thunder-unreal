#include "VTBootstrapCommandlet.h"
#include "VTGameData.h"
#include "VTUI.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureCube.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionTextureObject.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
namespace {
bool Persist(UObject* Asset) {
 auto* P=Asset->GetOutermost(); P->MarkPackageDirty(); FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone;
 auto File=FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension()); IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);
 return UPackage::SavePackage(P,Asset,*File,Args);
}
UMaterial* Material(const TCHAR* Name) {
 FString Path=FString(TEXT("/Game/Environment/"))+Name; auto* M=LoadObject<UMaterial>(nullptr,*(Path+TEXT(".")+Name));
 if(!M){M=NewObject<UMaterial>(CreatePackage(*Path),FName(Name),RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(M);}
 M->GetExpressionCollection().Empty(); M->SetShadingModel(MSM_Unlit); return M;
}
UMaterialExpressionCustom* Shader(UMaterial* M,const TCHAR* Code,ECustomMaterialOutputType Type) {
 auto* S=NewObject<UMaterialExpressionCustom>(M); S->Code=Code; S->OutputType=Type; M->GetExpressionCollection().AddExpression(S); return S;
}
void Input(UMaterialExpressionCustom* S,const TCHAR* Name,UMaterialExpression* E) {FCustomInput I;I.InputName=Name;I.Input.Connect(0,E);S->Inputs.Add(I);}
}
UVTPlayabilityCommandlet::UVTPlayabilityCommandlet(){IsEditor=true;IsClient=false;IsServer=false;LogToConsole=true;}
int32 UVTPlayabilityCommandlet::Main(const FString& Params) {
 if(!FParse::Param(*Params,TEXT("Apply"))){UE_LOG(LogTemp,Error,TEXT("Use -Apply to update only the requested playability assets."));return 1;}
 auto* Data=LoadObject<UVTGameData>(nullptr,TEXT("/Game/Data/DA_GameData.DA_GameData")); if(!Data)return 2;
 Data->FlightSpeedMultiplier=2;Data->ProjectileVisualRadius=7;if(!Persist(Data))return 3;
 auto* Atlas=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Environment/phoenix_space_cubemap.phoenix_space_cubemap"));if(!Atlas)return 4;
 TArray64<uint8> Pixels;int32 Side=Atlas->Source.GetSizeX();if(Atlas->Source.GetSizeY()!=Side*6||!Atlas->Source.GetMipData(Pixels,0))return 5;
 auto* Cube=LoadObject<UTextureCube>(nullptr,TEXT("/Game/Environment/T_SpaceCube.T_SpaceCube"));
 if(!Cube){Cube=NewObject<UTextureCube>(CreatePackage(TEXT("/Game/Environment/T_SpaceCube")),TEXT("T_SpaceCube"),RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(Cube);}
 Cube->Source.Init(Side,Side,6,1,Atlas->Source.GetFormat(),Pixels.GetData());Cube->SRGB=true;Cube->PostEditChange();if(!Persist(Cube))return 6;
 auto* Sky=Material(TEXT("M_SpaceSky"));Sky->TwoSided=true;
 auto* Texture=NewObject<UMaterialExpressionTextureObject>(Sky);Texture->Texture=Cube;Sky->GetExpressionCollection().AddExpression(Texture);
 auto* Direction=NewObject<UMaterialExpressionCameraVectorWS>(Sky);Sky->GetExpressionCollection().AddExpression(Direction);
 auto* Sample=Shader(Sky,TEXT("return TextureCubeSample(Sky,SkySampler,normalize(float3(-Direction.x,Direction.y,-Direction.z))).rgb*0.8;"),CMOT_Float3);Input(Sample,TEXT("Sky"),Texture);Input(Sample,TEXT("Direction"),Direction);Sky->GetEditorOnlyData()->EmissiveColor.Connect(0,Sample);Sky->PostEditChange();if(!Persist(Sky))return 7;
 auto* Grid=Material(TEXT("M_ReferenceGrid"));Grid->BlendMode=BLEND_Translucent;Grid->TwoSided=true;
 auto* Position=NewObject<UMaterialExpressionWorldPosition>(Grid);Grid->GetExpressionCollection().AddExpression(Position);
 auto* Lines=Shader(Grid,TEXT("float2 p=Position.xy/20000;float2 d=abs(frac(p+0.5)-0.5);float2 w=max(fwidth(p),0.0005);float line=1-min(saturate(d.x/w.x),saturate(d.y/w.y));float fade=1-saturate(length(Parameters.AbsoluteWorldPosition-CameraPosition)/450000);return line*0.32*fade;"),CMOT_Float1);
 // World-space line width uses screen derivatives; the plane is local to the viewer's system.
 Lines->Code=TEXT("float2 p=Position.xy/20000;float2 d=abs(frac(p+0.5)-0.5);float2 w=max(fwidth(p),0.0005);return (1-min(saturate(d.x/w.x),saturate(d.y/w.y)))*0.3;");Input(Lines,TEXT("Position"),Position);
 auto* Colour=NewObject<UMaterialExpressionConstant3Vector>(Grid);Colour->Constant=FLinearColor(0.3f,0.38f,0.55f);Grid->GetExpressionCollection().AddExpression(Colour);Grid->GetEditorOnlyData()->EmissiveColor.Connect(0,Colour);Grid->GetEditorOnlyData()->Opacity.Connect(0,Lines);Grid->PostEditChange();if(!Persist(Grid))return 8;
 auto* Star=Material(TEXT("M_Star"));auto* StarPosition=NewObject<UMaterialExpressionWorldPosition>(Star);Star->GetExpressionCollection().AddExpression(StarPosition);auto* Time=NewObject<UMaterialExpressionTime>(Star);Star->GetExpressionCollection().AddExpression(Time);
 auto* Surface=Shader(Star,TEXT("float3 p=Position/850;float n=sin(p.x+sin(p.y*1.9+Time*0.17))*sin(p.y+sin(p.z*2.3-Time*0.11))*sin(p.z+sin(p.x*1.7));float fine=sin(p.x*4+p.y*3+Time*0.3)*sin(p.z*5-p.y*4);float v=saturate(0.5+n*0.6+fine*0.15);return lerp(float3(0.18,0.009,0.001),float3(3.5,0.9,0.06),v);"),CMOT_Float3);Input(Surface,TEXT("Position"),StarPosition);Input(Surface,TEXT("Time"),Time);Star->GetEditorOnlyData()->EmissiveColor.Connect(0,Surface);Star->PostEditChange();if(!Persist(Star))return 9;
 auto* Shot=Material(TEXT("M_Projectile"));auto* Glow=NewObject<UMaterialExpressionConstant3Vector>(Shot);Glow->Constant=FLinearColor(12,4,0.4f);Shot->GetExpressionCollection().AddExpression(Glow);Shot->GetEditorOnlyData()->EmissiveColor.Connect(0,Glow);Shot->PostEditChange();if(!Persist(Shot))return 10;
 auto* BP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/WBP_UI.WBP_UI"));if(!BP)return 11;auto* Tree=BP->WidgetTree.Get();auto* HUD=Cast<UVerticalBox>(Tree->FindWidget(TEXT("HUDPanel")));auto* Overlay=Cast<UOverlay>(Tree->RootWidget);if(!HUD||!Overlay)return 12;
 // Idempotent targeted upgrade: keep the authored menu and Flight variable binding.
 if(!Tree->FindWidget(TEXT("HullBar"))) {
  auto Text=[&](UPanelWidget* Parent,const TCHAR* Name,const TCHAR* Label,int Size){auto* T=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),FName(Name));T->SetText(FText::FromString(Label));auto Font=T->GetFont();Font.Size=Size;T->SetFont(Font);T->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f,0.9f,1)));Parent->AddChild(T);return T;};
  auto Bar=[&](const TCHAR* Name,const TCHAR* Label,FLinearColor Colour){auto* Row=Tree->ConstructWidget<UHorizontalBox>();auto* RS=HUD->AddChildToVerticalBox(Row);RS->SetPadding(FMargin(0,3));auto* L=Text(Row,*FString::Printf(TEXT("%sLabel"),Name),Label,12);auto* LS=CastChecked<UHorizontalBoxSlot>(L->Slot);LS->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));LS->SetPadding(FMargin(0,0,12,0));auto* Size=Tree->ConstructWidget<USizeBox>();Size->SetWidthOverride(220);Size->SetHeightOverride(12);Row->AddChild(Size);auto* B=Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(),FName(Name));B->SetFillColorAndOpacity(Colour);B->SetPercent(1);Size->SetContent(B);};
  Bar(TEXT("HullBar"),TEXT("HULL"),FLinearColor(0.25f,0.9f,0.65f));Bar(TEXT("BatteryBar"),TEXT("BATTERY"),FLinearColor(0.3f,0.65f,1));Bar(TEXT("EMPBar"),TEXT("EMP STRESS"),FLinearColor(0.85f,0.25f,1));
  Bar(TEXT("BowBar"),TEXT("BOW SHIELD"),FLinearColor(0.1f,0.8f,1));Bar(TEXT("SternBar"),TEXT("STERN SHIELD"),FLinearColor(0.1f,0.8f,1));Bar(TEXT("PortBar"),TEXT("PORT SHIELD"),FLinearColor(0.1f,0.8f,1));Bar(TEXT("StarboardBar"),TEXT("STBD SHIELD"),FLinearColor(0.1f,0.8f,1));
  Text(HUD,TEXT("FlightState"),TEXT(""),12);
  auto* Border=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("AbilityPanel"));Border->SetBrushColor(FLinearColor(0.015f,0.025f,0.04f,0.92f));Border->SetPadding(FMargin(16,12));auto* Slot=Overlay->AddChildToOverlay(Border);Slot->SetHorizontalAlignment(HAlign_Center);Slot->SetVerticalAlignment(VAlign_Bottom);Slot->SetPadding(FMargin(16,16));auto* Row=Tree->ConstructWidget<UHorizontalBox>();Border->SetContent(Row);
  const TCHAR* Names[]={TEXT("AbilityPort"),TEXT("AbilityStarboard"),TEXT("AbilityEMP"),TEXT("AbilityTorpedo"),TEXT("AbilityWarp"),TEXT("AbilityMine"),TEXT("AbilityScreen"),TEXT("AbilityBoost"),TEXT("AbilityInteract")};
  for(auto* Name:Names){auto* T=Text(Row,Name,TEXT(""),12);CastChecked<UHorizontalBoxSlot>(T->Slot)->SetPadding(FMargin(12,0));}
 }
 TArray<UWidget*> Widgets;Tree->GetAllWidgets(Widgets);for(auto* W:Widgets)if(!BP->WidgetVariableNameToGuidMap.Contains(W->GetFName()))BP->WidgetVariableNameToGuidMap.Add(W->GetFName(),FGuid::NewGuid());
 FKismetEditorUtilities::CompileBlueprint(BP);if(BP->Status==BS_Error||!Persist(BP))return 13;
 UE_LOG(LogTemp,Display,TEXT("Applied graphical HUD, flight pace, projectile, textured star, cube sky and plane grid assets."));return 0;
}
