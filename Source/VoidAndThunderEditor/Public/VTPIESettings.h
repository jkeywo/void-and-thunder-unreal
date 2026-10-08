#pragma once
#include "Engine/AssetUserData.h"
#include "Engine/DeveloperSettings.h"
#include "VTTypes.h"
#include "VTPIESettings.generated.h"
class UWorld;class UVTGameData;class UVTGameInstance;
UENUM()
enum class EVTPIEMode : uint8 { Sandbox,Skirmish,TestRange };
UCLASS(EditInlineNew)
class VOIDANDTHUNDEREDITOR_API UVTPIELevelModes : public UAssetUserData {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,Category="Void & Thunder") TArray<EVTPIEMode> SupportedModes={EVTPIEMode::Sandbox,EVTPIEMode::Skirmish,EVTPIEMode::TestRange};
 virtual bool IsEditorOnly() const override{return true;}
};
UCLASS(Config=EditorPerProjectUserSettings)
class VOIDANDTHUNDEREDITOR_API UVTPIESettings : public UDeveloperSettings {
 GENERATED_BODY()
public:
 UPROPERTY(Config,EditAnywhere,Category="Preview") bool SkipIntro=false;
 UPROPERTY(Config,EditAnywhere,Category="Preview") EVTPIEMode Mode=EVTPIEMode::Sandbox;
 UPROPERTY(Config,EditAnywhere,Category="Preview") FName Hull=TEXT("corsair_cruiser");
 UPROPERTY(Config,EditAnywhere,Category="Preview") FVTLoadoutSelection Fit;
 FText FitFeedback;
 static TArray<EVTPIEMode> Modes(UWorld* World);
 static FText ModeLabel(EVTPIEMode Value);
 static FString ModeId(EVTPIEMode Value);
 bool LoadoutCandidate(const UVTGameData* Data,FName Id,FVTLoadoutSelection& Candidate) const;
 bool ToggleLoadout(const UVTGameData* Data,FName Id);
 void SelectHull(const UVTGameData* Data,FName Id);
 bool Apply(UVTGameInstance* GI,UWorld* EditorWorld,const UVTGameData* Data) const;
};
