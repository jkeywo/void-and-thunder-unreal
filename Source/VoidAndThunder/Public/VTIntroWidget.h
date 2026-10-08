#pragma once
#include "Blueprint/UserWidget.h"
#include "VTIntroWidget.generated.h"
UCLASS()
class VOIDANDTHUNDER_API UVTIntroWidget:public UUserWidget {
 GENERATED_BODY()
public:
 UPROPERTY(meta=(BindWidget)) TObjectPtr<class UBorder> Comms;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<class UImage> Portrait;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<class UTextBlock> Speaker;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<class UTextBlock> Speech;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<class UTextBlock> Objective;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<class UTextBlock> Controls;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<class UTextBlock> Repairs;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<class UButton> Continue;
 UPROPERTY(meta=(BindWidget)) TObjectPtr<class UButton> Skip;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<class UButton> ChoiceA;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<class UButton> ChoiceB;
 UFUNCTION() void RepairA();
 UFUNCTION() void RepairB();
 UFUNCTION() void Advance();
 UFUNCTION() void SkipIntro();
 virtual void NativeConstruct() override;
 virtual void NativeTick(const FGeometry& Geometry,float Dt) override;
 virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Culling,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool Enabled) const override;
};
