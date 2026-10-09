#include "VTIntroWidget.h"
#include "VTIntro.h"
#include "VTGameplay.h"
#include "VTUI.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
void UVTIntroWidget::NativeConstruct(){Super::NativeConstruct();if(ChoiceA)ChoiceA->OnClicked.AddDynamic(this,&UVTIntroWidget::RepairA);if(ChoiceB)ChoiceB->OnClicked.AddDynamic(this,&UVTIntroWidget::RepairB);if(Continue)Continue->OnClicked.AddDynamic(this,&UVTIntroWidget::Advance);if(Skip)Skip->OnClicked.AddDynamic(this,&UVTIntroWidget::SkipIntro);SetVisibility(ESlateVisibility::SelfHitTestInvisible);}
void UVTIntroWidget::RepairA(){if(auto* PC=Cast<AVTController>(GetOwningPlayer()))PC->Intro->ServerChooseRepair(0);}
void UVTIntroWidget::RepairB(){if(auto* PC=Cast<AVTController>(GetOwningPlayer()))PC->Intro->ServerChooseRepair(1);}
void UVTIntroWidget::Advance(){if(auto* PC=Cast<AVTController>(GetOwningPlayer()))PC->Intro->ServerAdvance();}
void UVTIntroWidget::SkipIntro(){if(auto* PC=Cast<AVTController>(GetOwningPlayer()))PC->Intro->ServerSkip();}
void UVTIntroWidget::NativeTick(const FGeometry& Geometry,float Dt){
 Super::NativeTick(Geometry,Dt);auto* PC=Cast<AVTController>(GetOwningPlayer());auto* I=PC?PC->Intro.Get():nullptr;
 if(!Comms)return;bool Show=I&&I->Active()&&I->Data&&PC->GetPawn()&&(!PC->UI||!(PC->UI->MenuOpen||PC->UI->ChartOpen));Comms->SetVisibility(Show?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed);if(!Show)return;
 const auto* Beat=I->Data->Beat(I->Progress.Stage);if(!Beat)return;
 Speaker->SetText(Beat->Enemy?NSLOCTEXT("VTIntro","Enemy","ENEMY CAPTAIN  /  INCOMING TRANSMISSION"):NSLOCTEXT("VTIntro","Engineer","ENGINEER  /  INTERNAL COMMS"));
 Portrait->SetBrushFromTexture(Beat->Enemy?I->Data->CaptainPortrait:I->Data->EngineerPortrait);
 Speech->SetText(Beat->Speech);Objective->SetText(Beat->Objective);
 const auto* Options=I->RepairOptions();for(int N=0;N<2;++N)if(auto* Button=N==0?ChoiceA.Get():ChoiceB.Get()){Button->SetVisibility(Options&&Options->IsValidIndex(N)?ESlateVisibility::Visible:ESlateVisibility::Collapsed);if(Options&&Options->IsValidIndex(N))if(auto* Label=Cast<UTextBlock>(Button->GetContent()))Label->SetText(FText::Format(NSLOCTEXT("VTIntro","RepairLabel","[{0}] {1}"),FText::FromString(N==0?TEXT("1 / Pad X"):TEXT("2 / Pad Y")),(*Options)[N].Label));}

 Continue->SetVisibility(I->CanAdvance()?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
 auto* Sub=PC->GetLocalPlayer()?ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()):nullptr;
 auto Binding=[&](const UInputAction* A){FString Text;if(Sub&&A)for(const auto& K:Sub->QueryKeysMappedToAction(A)){if(!Text.IsEmpty())Text+=TEXT(" / ");Text+=K.GetDisplayName().ToString();}return Text;};
 auto Action=[&](const TCHAR* Name){for(const UInputAction* A:PC->Actions)if(A&&A->GetFName()==FName(Name))return Binding(A);auto* A=LoadObject<UInputAction>(nullptr,*FString::Printf(TEXT("/Game/Input/%s.%s"),Name,Name));return Binding(A);};
 FString Hint;
 if(I->Progress.Stage==EVTIntroStage::Helm)Hint=FString::Printf(TEXT("Tap [%s] / [%s]: reverse, halt, half, full.  [%s]: steer.  Stick: analogue helm."),*Binding(PC->ThrottleUp),*Binding(PC->ThrottleDown),*Action(TEXT("IA_Turn")));
 else if(I->Progress.Stage==EVTIntroStage::Debris||I->Progress.Stage==EVTIntroStage::Duel)Hint=FString::Printf(TEXT("[%s] / [%s]: hold to aim a broadside; release to fire. Steer to bring the target alongside."),*Action(TEXT("IA_Port")),*Action(TEXT("IA_Starboard")));
 else if(I->Progress.Stage==EVTIntroStage::Gate||I->Progress.Stage==EVTIntroStage::Salvage)Hint=FString::Printf(TEXT("Hold [%s] when the interaction prompt appears."),*Action(TEXT("IA_Interact")));
 else if(I->Progress.Stage==EVTIntroStage::Repairs)Hint=FString::Printf(TEXT("[%s]: show all controls. The engineer will offer repairs next."),*Action(TEXT("IA_HUDControls")));
 else if(I->Progress.Stage==EVTIntroStage::Handoff)Hint=FString::Printf(TEXT("[%s]: chart. At a station, hold position to dock and open services."),*Action(TEXT("IA_HUDChart")));
 else Hint=TEXT("[Enter / Pad A] Continue    [Esc] Menu");
 if(Options)Hint=TEXT("Only enough parts for one. Choose with a button, 1 / 2, or Pad X / Y.");
 if(I->Progress.Stage==EVTIntroStage::BatteryTrial||I->Progress.Stage==EVTIntroStage::SpecialTrial)if(const auto* Repair=I->CurrentRepair()){Objective->SetText(Repair->Trial);Hint=FString::Printf(TEXT("[%s]  %s"),*Action(*Repair->InputAction.ToString()),*Repair->Trial.ToString());}
 Controls->SetText(FText::FromString(Hint));
 Repairs->SetText(FText::FromString(FString::Printf(TEXT("HELM %s   |   BROADSIDES %s   |   SHIELDS %s   |   JUMP %s"),I->Progress.Stage==EVTIntroStage::Wake?TEXT("REPAIRING"):TEXT("ONLINE"),I->WeaponsOnline()?TEXT("ONLINE"):TEXT("OFFLINE"),I->SystemsOnline()?TEXT("ONLINE"):TEXT("OFFLINE"),I->JumpOnline()?TEXT("ONLINE"):TEXT("OFFLINE"))));
}
int32 UVTIntroWidget::NativePaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Cull,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled) const{
 auto Top=Super::NativePaint(Args,G,Cull,Out,Layer,Style,Enabled);auto* PC=Cast<AVTController>(GetOwningPlayer());auto* S=PC?Cast<AVTShip>(PC->GetPawn()):nullptr;
 if(!S||!PC->Intro->Active()||!PC->Intro->HasWaypoint||(PC->UI&&(PC->UI->MenuOpen||PC->UI->ChartOpen)))return Top;
 FVector2D P;PC->ProjectWorldLocationToScreen(VT::ToWorld(PC->Intro->ObjectivePosition,S->SystemIndex),P,true);int W=0,H=0;PC->GetViewportSize(W,H);P*=G.GetLocalSize()/FVector2D(FMath::Max(1,W),FMath::Max(1,H));
 P.X=FMath::Clamp(P.X,35.,G.GetLocalSize().X-180);P.Y=FMath::Clamp(P.Y,365.,G.GetLocalSize().Y-240);
 const FLinearColor Amber(1,0.65f,0.08f);TArray<FVector2D> Points={P+FVector2D(0,-16),P+FVector2D(16,0),P+FVector2D(0,16),P+FVector2D(-16,0),P+FVector2D(0,-16)};FSlateDrawElement::MakeLines(Out,Top+1,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Amber,true,2);
 const TCHAR* Label=PC->Intro->Progress.Stage==EVTIntroStage::Helm?TEXT("CLEAR SPACE"):PC->Intro->Progress.Stage==EVTIntroStage::Debris?TEXT("WRECKAGE"):PC->Intro->Progress.Stage==EVTIntroStage::Gate?TEXT("JUMP GATE"):(PC->Intro->Progress.Stage==EVTIntroStage::BatteryTrial||PC->Intro->Progress.Stage==EVTIntroStage::SpecialTrial)?TEXT("DERELICT"):TEXT("ATTACKER");
 const auto Text=FString::Printf(TEXT("%s  %.0f m"),Label,(PC->Intro->ObjectivePosition-S->Movement->Motion.Position).Size());FSlateDrawElement::MakeText(Out,Top+1,G.ToPaintGeometry(FVector2D(180,24),FSlateLayoutTransform(P+FVector2D(22,-8))),Text,FCoreStyle::GetDefaultFontStyle("Regular",12),ESlateDrawEffect::None,Amber);return Top+1;
}
