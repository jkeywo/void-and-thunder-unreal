#include "VTBootstrapCommandlet.h"
#include "VTUI.h"
#include "AssetToolsModule.h"
#include "AssetImportTask.h"
#include "Engine/Texture2D.h"
#include "Engine/FontFace.h"
#include "Engine/Font.h"
#include "Misc/FileHelper.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Brushes/SlateColorBrush.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
namespace {
bool SaveHUD(UObject* A){auto* P=A->GetOutermost();P->MarkPackageDirty();FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;auto File=FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension());IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);return UPackage::SavePackage(P,A,*File,Args);}
UObject* ImportHUD(const FString& File,const FString& Name){auto* Task=NewObject<UAssetImportTask>();Task->Filename=File;Task->DestinationPath=TEXT("/Game/UI/Legacy");Task->DestinationName=Name;Task->bAutomated=true;Task->bReplaceExisting=true;Task->bSave=true;FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().ImportAssetTasks({Task});return Task->GetObjects().IsEmpty()?nullptr:Task->GetObjects()[0];}
void ReadableMenu(UWidgetBlueprint* BP){
 auto* Menu=BP->WidgetTree->FindWidget(TEXT("MenuPanel"));
 TArray<UWidget*> Widgets;BP->WidgetTree->GetAllWidgets(Widgets);
 for(auto* W:Widgets){bool InMenu=false;for(auto* P=W;P;P=P->GetParent())if(P==Menu){InMenu=true;break;}if(!InMenu)continue;
  if(auto* T=Cast<UTextBlock>(W)){auto F=T->GetFont();F.Size=W->GetFName()==TEXT("Title")?36:W->GetFName().ToString().EndsWith(TEXT("Heading"))?22:18;F.LetterSpacing=0;T->SetFont(F);if(W->GetFName()==TEXT("StatusText")){T->SetWrapTextAt(980);T->SetAutoWrapText(true);}}
  if(auto* B=Cast<UButton>(W)){auto Style=B->GetStyle();Style.SetNormalPadding(FMargin(16,10));Style.SetPressedPadding(FMargin(16,11,16,9));B->SetStyle(Style);}
  if(auto* C=Cast<UComboBoxString>(W)){auto F=C->GetFont();F.Size=18;F.LetterSpacing=0;*FindFProperty<FStructProperty>(C->GetClass(),TEXT("Font"))->ContainerPtrToValuePtr<FSlateFontInfo>(C)=F;C->SetContentPadding(FMargin(12,8));}
  if(auto* E=Cast<UEditableTextBox>(W)){auto Style=E->GetWidgetStyle();auto F=Style.TextStyle.Font;F.Size=18;Style.SetFont(F);E->SetWidgetStyle(Style);}
  if(auto* Bounds=Cast<USizeBox>(W))if(Bounds->GetWidthOverride()>0&&Bounds->GetWidthOverride()<500)Bounds->SetWidthOverride(280);
 }
}

}
UVTHUDStyleCommandlet::UVTHUDStyleCommandlet(){IsEditor=true;IsClient=false;IsServer=false;LogToConsole=true;}
int32 UVTHUDStyleCommandlet::Main(const FString& Params){
 if(!FParse::Param(*Params,TEXT("Apply"))){UE_LOG(LogTemp,Error,TEXT("Use -Apply for the explicit original HUD style upgrade."));return 1;}
 auto* BP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/WBP_UI.WBP_UI"));if(!BP)return 2;
 if(FParse::Param(*Params,TEXT("MenuReadability"))){ReadableMenu(BP);FKismetEditorUtilities::CompileBlueprint(BP);return BP->Status!=BS_Error&&SaveHUD(BP)?0:8;}
 TArray<TObjectPtr<UTexture2D>> Panels;
 for(const TCHAR* Name:{TEXT("pTop"),TEXT("pStatus"),TEXT("pCoords"),TEXT("pLeft"),TEXT("pRight")}){auto* T=Cast<UTexture2D>(ImportHUD(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("SourceAssets/ui")/(FString(Name)+TEXT(".png"))),Name));if(!T)return 3;T->CompressionSettings=TC_EditorIcon;T->LODGroup=TEXTUREGROUP_UI;T->MipGenSettings=TMGS_NoMipmaps;T->NeverStream=true;T->SRGB=true;T->PostEditChange();if(!SaveHUD(T))return 4;Panels.Add(T);}
 auto* Face=LoadObject<UFontFace>(nullptr,TEXT("/Game/UI/Legacy/F_HUDMono.F_HUDMono"));
 if(!Face){Face=NewObject<UFontFace>(CreatePackage(TEXT("/Game/UI/Legacy/F_HUDMono")),TEXT("F_HUDMono"),RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(Face);}
 TArray<uint8> FontBytes;Face->SourceFilename=FPaths::EngineContentDir()/TEXT("Slate/Fonts/DroidSansMono.ttf");if(!FFileHelper::LoadFileToArray(FontBytes,*Face->SourceFilename))return 5;
 Face->FontFaceData->SetData(MoveTemp(FontBytes));Face->LoadingPolicy=EFontLoadingPolicy::Inline;if(!SaveHUD(Face))return 6;
 auto* FontAsset=LoadObject<UFont>(nullptr,TEXT("/Game/UI/Legacy/F_HUD.F_HUD"));if(!FontAsset){FontAsset=NewObject<UFont>(CreatePackage(TEXT("/Game/UI/Legacy/F_HUD")),TEXT("F_HUD"),RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(FontAsset);}
 FontAsset->FontCacheType=EFontCacheType::Runtime;auto& Typeface=FontAsset->GetMutableInternalCompositeFont().DefaultTypeface;Typeface.Fonts.Reset();{auto& Entry=Typeface.Fonts.AddDefaulted_GetRef();Entry.Name=TEXT("Regular");Entry.Font=FFontData(Face);};if(!SaveHUD(FontAsset))return 6;
 FSlateFontInfo Font(FontAsset,12);FLinearColor Amber(FColor(255,178,0)),Dim(FColor(181,116,2));
 TArray<UWidget*> Widgets;BP->WidgetTree->GetAllWidgets(Widgets);
 for(auto* W:Widgets){
  if(auto* T=Cast<UTextBlock>(W)){Font.Size=W->GetFName()==TEXT("Title")?36:W->GetFName()==TEXT("Subtitle")?11:W->GetFName().ToString().EndsWith(TEXT("Heading"))?11:10;Font.LetterSpacing=W->GetFName()==TEXT("Title")?180:W->GetFName()==TEXT("Subtitle")?160:30;T->SetFont(Font);T->SetColorAndOpacity(Amber);T->SetShadowColorAndOpacity(FLinearColor(0.5f,0.22f,0,0.5f));T->SetShadowOffset({1,1});if(W->GetFName()==TEXT("Title")){T->SetText(FText::FromString(TEXT("VOID & THUNDER")));T->SetJustification(ETextJustify::Center);}if(W->GetFName()==TEXT("Subtitle")){T->SetText(FText::FromString(TEXT("P I R A C Y   I N   T H E   S E T T L E D   D A R K")));T->SetJustification(ETextJustify::Center);T->SetColorAndOpacity(FLinearColor(0.85f,0.7f,0.5f));}if(auto* Slot=Cast<UVerticalBoxSlot>(T->Slot))Slot->SetPadding(FMargin(0,6,0,3));}
  if(auto* B=Cast<UBorder>(W)){B->SetBrushColor(B->GetFName()==TEXT("MenuBackdrop")?FLinearColor(0.008f,0.006f,0.002f,0.8f):FLinearColor::Transparent);}
  if(auto* B=Cast<UButton>(W)){auto Style=B->GetStyle();Style.SetNormal(FSlateColorBrush(FLinearColor(0.07f,0.045f,0.009f,0.9f)));Style.SetHovered(FSlateColorBrush(FLinearColor(0.25f,0.16f,0.02f)));Style.SetPressed(FSlateColorBrush(FLinearColor(0.35f,0.21f,0.03f)));Style.SetDisabled(FSlateColorBrush(FLinearColor(0.035f,0.025f,0.012f)));Style.SetNormalPadding(FMargin(12,6));Style.SetPressedPadding(FMargin(12,7,12,5));B->SetStyle(Style);}
  if(auto* C=Cast<UComboBoxString>(W)){Font.Size=10;Font.LetterSpacing=30;*FindFProperty<FStructProperty>(C->GetClass(),TEXT("Font"))->ContainerPtrToValuePtr<FSlateFontInfo>(C)=Font;*FindFProperty<FStructProperty>(C->GetClass(),TEXT("ForegroundColor"))->ContainerPtrToValuePtr<FSlateColor>(C)=FSlateColor(Amber);auto Style=C->GetWidgetStyle();Style.ComboButtonStyle.ButtonStyle.SetNormal(FSlateColorBrush(FLinearColor(0.08f,0.05f,0.01f)));Style.ComboButtonStyle.ButtonStyle.SetHovered(FSlateColorBrush(FLinearColor(0.2f,0.12f,0.02f)));C->SetWidgetStyle(Style);}
  if(auto* E=Cast<UEditableTextBox>(W)){auto Style=E->GetWidgetStyle();Font.Size=10;Font.LetterSpacing=0;Style.SetFont(Font);Style.SetForegroundColor(Amber);Style.SetBackgroundColor(FLinearColor(0.04f,0.025f,0.005f));E->SetWidgetStyle(Style);}
 }
 if(!BP->WidgetTree->FindWidget(TEXT("MenuBackdrop"))){auto* Backdrop=BP->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("MenuBackdrop"));Backdrop->SetBrushColor(FLinearColor(0.008f,0.006f,0.002f,0.8f));auto* Root=CastChecked<UOverlay>(BP->WidgetTree->RootWidget);Root->InsertChildAt(0,Backdrop);auto* Slot=CastChecked<UOverlaySlot>(Backdrop->Slot);Slot->SetHorizontalAlignment(HAlign_Fill);Slot->SetVerticalAlignment(VAlign_Fill);}
 auto* Menu=Cast<UVerticalBox>(BP->WidgetTree->FindWidget(TEXT("MenuPanel")));auto* Solo=Cast<UHorizontalBox>(BP->WidgetTree->FindWidget(TEXT("Skirmish"))->GetParent());if(!Menu||!Solo)return 7;
 if(!BP->WidgetTree->FindWidget(TEXT("WorldOptions"))){auto* B=BP->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("WorldOptions"));auto Style=CastChecked<UButton>(BP->WidgetTree->FindWidget(TEXT("Skirmish")))->GetStyle();B->SetStyle(Style);auto* T=BP->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("WorldOptionsLabel"));T->SetText(FText::FromString(TEXT("SHARED WORLD")));Font.Size=10;T->SetFont(Font);T->SetColorAndOpacity(Amber);B->SetContent(T);Solo->AddChild(B);}
 if(auto* T=Cast<UTextBlock>(BP->WidgetTree->FindWidget(TEXT("SkirmishLabel"))))T->SetText(FText::FromString(TEXT("CAST OFF")));
 if(auto* T=Cast<UTextBlock>(BP->WidgetTree->FindWidget(TEXT("RangeLabel"))))T->SetText(FText::FromString(TEXT("TEST RANGE")));
 // Keep the bound native controls, but assemble them like the original centered title card.
 TArray<UWidget*> Rows=Menu->GetAllChildren();auto* Fit=BP->WidgetTree->FindWidget(TEXT("FitRow"));auto* Mount=BP->WidgetTree->FindWidget(TEXT("MountRow"));
 TArray<UWidget*> First={BP->WidgetTree->FindWidget(TEXT("Title")),BP->WidgetTree->FindWidget(TEXT("Subtitle")),BP->WidgetTree->FindWidget(TEXT("FitHeading")),Fit,BP->WidgetTree->FindWidget(TEXT("MountHeading")),Mount,Solo};
 Menu->ClearChildren();for(auto* W:First)if(W){auto* Slot=Menu->AddChildToVerticalBox(W);Slot->SetHorizontalAlignment(HAlign_Center);Slot->SetPadding(FMargin(0,W==Solo?18:7,0,4));}
 for(auto* W:Rows)if(!First.Contains(W)){auto* Slot=Menu->AddChildToVerticalBox(W);Slot->SetHorizontalAlignment(HAlign_Center);Slot->SetPadding(FMargin(0,4));}
 BP->WidgetTree->GetAllWidgets(Widgets);for(auto* W:Widgets){if(auto* Bounds=Cast<USizeBox>(W))if(Bounds->GetWidthOverride()>200&&Bounds->GetWidthOverride()<500)Bounds->SetWidthOverride(180);if(!BP->WidgetVariableNameToGuidMap.Contains(W->GetFName()))BP->WidgetVariableNameToGuidMap.Add(W->GetFName(),FGuid::NewGuid());}
 auto* Strings=LoadObject<UStringTable>(nullptr,TEXT("/Game/UI/ST_UI.ST_UI"));if(!Strings)return 7;
 FString Json;if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectDir()/TEXT("SourceAssets/strings/en.json"))))return 7;TSharedPtr<FJsonObject> Source;if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Source))return 7;
 for(const auto& Pair:Source->Values){FString Existing;if(Pair.Value->Type==EJson::String&&!Strings->GetStringTable()->GetSourceString(FTextKey(Pair.Key),Existing))Strings->GetMutableStringTable()->SetSourceString(FTextKey(Pair.Key),Pair.Value->AsString(),TEXT("Original HUD source"));}if(!SaveHUD(Strings))return 7;
 ReadableMenu(BP);FKismetEditorUtilities::CompileBlueprint(BP);if(BP->Status==BS_Error)return 7;
 auto* Defaults=Cast<UVTUI>(BP->GeneratedClass->GetDefaultObject());Defaults->TextTable=Strings;Defaults->HudPanels=Panels;Defaults->HudFont=FSlateFontInfo(FontAsset,10);if(!SaveHUD(BP))return 8;
 auto* Mapping=LoadObject<UInputMappingContext>(nullptr,TEXT("/Game/Input/IMC_Flight.IMC_Flight"));if(!Mapping)return 9;
 int I=0;for(const TCHAR* Name:{TEXT("HUDControls"),TEXT("HUDChart")}){FString Path=FString(TEXT("/Game/Input/IA_"))+Name;auto* A=LoadObject<UInputAction>(nullptr,*(Path+TEXT(".IA_")+Name));if(!A){A=NewObject<UInputAction>(CreatePackage(*Path),FName(FString(TEXT("IA_"))+Name),RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(A);}A->ValueType=EInputActionValueType::Boolean;Mapping->UnmapAllKeysFromAction(A);Mapping->MapKey(A,I==0?EKeys::Tab:EKeys::F);Mapping->MapKey(A,I==0?EKeys::Gamepad_Special_Left:EKeys::Gamepad_RightThumbstick);if(!SaveHUD(A))return 10;++I;}
 if(!SaveHUD(Mapping))return 11;
 UE_LOG(LogTemp,Display,TEXT("Original metal/CRT artwork, amber menu, native readouts and overlay mappings saved."));return 0;
}

