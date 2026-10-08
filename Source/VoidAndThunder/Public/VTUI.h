#pragma once
#include "Blueprint/UserWidget.h"
#include "VTTypes.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateColor.h"
#include "VTUI.generated.h"
class UTextBlock;
class UComboBoxString;
class UEditableTextBox;
class UPanelWidget;
struct FOnAttributeChangeData;
UCLASS(BlueprintType) class VOIDANDTHUNDER_API UVTUIOption : public UObject {
 GENERATED_BODY()
public:
 UPROPERTY(BlueprintReadOnly) FName Id;
 UPROPERTY(BlueprintReadOnly) FText Label;
 UPROPERTY(BlueprintReadOnly) FSlateFontInfo Font;
 UPROPERTY(BlueprintReadOnly) FSlateColor Foreground;
};
UCLASS()
class VOIDANDTHUNDER_API UVTUI : public UUserWidget {
 GENERATED_BODY()
public:
 UPROPERTY(EditDefaultsOnly,Category="HUD|Style") TArray<TObjectPtr<class UTexture2D>> HudPanels;
 UPROPERTY(EditDefaultsOnly,Category="HUD|Style") FSlateFontInfo HudFont;
 UPROPERTY(EditDefaultsOnly,Category="HUD|Style") TObjectPtr<class UStringTable> TextTable;
 bool SessionOptions=false;
 UFUNCTION() void ToggleSessionOptions();
 bool ControlsOpen=false, ChartOpen=false;
 void ToggleControls(){if(!MenuOpen)ControlsOpen=!ControlsOpen;}
 void ToggleChart(){if(!MenuOpen){ChartOpen=!ChartOpen;ControlsOpen=false;}}
 int32 PaintFlightHUD(const FGeometry& Geometry,FSlateWindowElementList& Elements,int32 Layer) const;
 bool MenuOpen=false;
 bool FocusPending=false;
 int32 LastWorlds=-1;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> StatusText;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Flight;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UPanelWidget> MenuPanel;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UPanelWidget> HUDPanel;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UComboBoxString> HullChoice;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UComboBoxString> BatteryChoice;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UComboBoxString> GunChoice;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UComboBoxString> SpecialChoice;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UComboBoxString> BatterySecond;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UComboBoxString> BatteryThird;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UComboBoxString> SpecialSecond;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UComboBoxString> SpecialThird;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UComboBoxString> LANChoice;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UEditableTextBox> Address;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<UEditableTextBox> WorldName;
 UPROPERTY(BlueprintReadOnly) TMap<FName,TObjectPtr<UVTUIOption>> ChoiceItems;
 TMap<FName,TWeakObjectPtr<UWidget>> WidgetCache;
 TWeakObjectPtr<class UAbilitySystemComponent> ObservedAbilities;
 TArray<FDelegateHandle> AttributeHandles;
 FTimerHandle RefreshTimer;
 bool RefreshQueued=false;
 UWidget* CachedWidget(FName Name) const;
 void AddChoice(UComboBoxString* Box,FName Id,const FText& Label);
 FName SelectedId(UComboBoxString* Box) const;
 UFUNCTION() UWidget* GenerateChoice(FString Item);
 UFUNCTION(BlueprintNativeEvent,Category="UI") UWidget* GenerateChoiceWidget(FName Item);
 virtual UWidget* GenerateChoiceWidget_Implementation(FName Item);
 UFUNCTION(BlueprintPure,Category="UI") UVTUIOption* GetChoiceItem(FName Key) const {const auto* Item=ChoiceItems.Find(Key);return Item ? Item->Get() : nullptr;}
 void RequestRefresh();
 void RefreshView();
 void AttributeChanged(const FOnAttributeChangeData& Data);
 virtual void NativeDestruct() override;
 virtual void NativeConstruct() override;
 virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Culling,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool Enabled) const override;
 virtual void NativeTick(const FGeometry& Geometry,float Dt) override;
 void SetMenu(bool Open);
 bool StoreFit();
 void ApplyGraphics(int32 Preset);
 UFUNCTION() void PerformanceGraphics();
 UFUNCTION() void BalancedGraphics();
 UFUNCTION() void HighGraphics();
 UFUNCTION() void CreateWorld();
 UFUNCTION() void ContinueWorld();
 UFUNCTION() void JoinAddress();
 UFUNCTION() void FindLAN();
 UFUNCTION() void JoinLAN();
 UFUNCTION() void Skirmish();
 UFUNCTION() void TestRange();
 UFUNCTION() void Resume();
 UFUNCTION() void ToggleAutopilot();
 UFUNCTION() void Leave();
 UFUNCTION() void Quit();
 UFUNCTION() void Save();
 UFUNCTION() void Repair();
 UFUNCTION() void Recover();
 UFUNCTION() void Undock();
 UFUNCTION() void PayHeat();
 UFUNCTION() void Refit();
};
