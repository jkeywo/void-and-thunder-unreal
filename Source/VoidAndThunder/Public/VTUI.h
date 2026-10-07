#pragma once
#include "Blueprint/UserWidget.h"
#include "VTTypes.h"
#include "VTUI.generated.h"
class UTextBlock;
class UComboBoxString;
class UEditableTextBox;
class UPanelWidget;
UCLASS()
class VOIDANDTHUNDER_API UVTUI : public UUserWidget {
 GENERATED_BODY()
public:
 bool MenuOpen=false;
 bool FocusPending=false;
 int32 LastWorlds=-1;
 UPROPERTY() TObjectPtr<UTextBlock> StatusText;
 UPROPERTY() TObjectPtr<UTextBlock> FlightText;
 UPROPERTY() TObjectPtr<UPanelWidget> MenuPanel;
 UPROPERTY() TObjectPtr<UPanelWidget> HUDPanel;
 UPROPERTY() TObjectPtr<UComboBoxString> HullChoice;
 UPROPERTY() TObjectPtr<UComboBoxString> BatteryChoice;
 UPROPERTY() TObjectPtr<UComboBoxString> GunChoice;
 UPROPERTY() TObjectPtr<UComboBoxString> SpecialChoice;
 UPROPERTY() TObjectPtr<UComboBoxString> BatterySecond;
 UPROPERTY() TObjectPtr<UComboBoxString> BatteryThird;
 UPROPERTY() TObjectPtr<UComboBoxString> SpecialSecond;
 UPROPERTY() TObjectPtr<UComboBoxString> SpecialThird;
 UPROPERTY() TObjectPtr<UComboBoxString> LANChoice;
 UPROPERTY() TObjectPtr<UEditableTextBox> Address;
 UPROPERTY() TObjectPtr<UEditableTextBox> WorldName;
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
