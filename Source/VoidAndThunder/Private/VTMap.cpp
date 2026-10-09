#include "VTUI.h"
#include "VTGameplay.h"
#include "VTNavigation.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Kismet/GameplayStatics.h"
void UVTUI::ToggleFullMap(){if(!MenuOpen)SetChart(!(ChartOpen&&FullMap),true);}
void UVTUI::SetChart(bool Open,bool Full){
 if(CastChecked<UVTGameInstance>(GetGameInstance())->PlayMode!=TEXT("sandbox"))return;
 ChartOpen=Open;FullMap=Open&&Full;ControlsOpen=false;
 SetVisibility(Open?ESlateVisibility::Visible:ESlateVisibility::SelfHitTestInvisible);SetIsFocusable(Open);
 if(auto* PC=Cast<AVTController>(GetOwningPlayer())){if(FullMap)PC->LocalIntent={};if(FullMap&&GetWorld()->GetNetMode()==NM_Standalone){PC->AimDilation=1;UGameplayStatics::SetGlobalTimeDilation(this,1);}PC->UpdateInputContexts();if(Open){FInputModeGameAndUI Mode;Mode.SetWidgetToFocus(TakeWidget());Mode.SetHideCursorDuringCapture(false);PC->SetInputMode(Mode);}else{FInputModeGameOnly Mode;Mode.SetConsumeCaptureMouseDown(false);PC->SetInputMode(Mode);}PC->bShowMouseCursor=true;}
}
FVector2D UVTUI::ChartPoint(int32 System,const FVector2D& Size) const{
 auto* D=GetWorld()->GetSubsystem<UVTSimulation>()->Data.Get();FVector2D Min(DBL_MAX,DBL_MAX),Max(-DBL_MAX,-DBL_MAX);
 for(const auto& S:D->Systems)if(!S.Id.ToString().StartsWith(TEXT("__intro_"))){Min.X=FMath::Min(Min.X,S.ChartPosition.X);Min.Y=FMath::Min(Min.Y,S.ChartPosition.Y);Max.X=FMath::Max(Max.X,S.ChartPosition.X);Max.Y=FMath::Max(Max.Y,S.ChartPosition.Y);}
 const FVector2D Offset=FullMap?FVector2D(70,120):FVector2D(Size.X-405,85),Span=FullMap?Size-FVector2D(290,255):FVector2D(205,145);
 auto P=D->Systems[System].ChartPosition;return Offset+FVector2D((P.X-Min.X)/FMath::Max(1.,Max.X-Min.X)*Span.X,(P.Y-Min.Y)/FMath::Max(1.,Max.Y-Min.Y)*Span.Y);
}
FReply UVTUI::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E){if(ChartOpen&&(E.GetKey()==EKeys::Escape||E.GetKey()==EKeys::F||E.GetKey()==EKeys::G)){if(E.GetKey()==EKeys::G&&!FullMap)SetChart(true,true);else SetChart(false,false);return FReply::Handled();}return Super::NativeOnKeyDown(G,E);}
FReply UVTUI::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E){
 if(!ChartOpen||E.GetEffectingButton()!=EKeys::LeftMouseButton)return Super::NativeOnMouseButtonDown(G,E);
 auto P=G.AbsoluteToLocal(E.GetScreenSpacePosition());const auto Size=G.GetLocalSize();
 if(!FullMap&&(P.X<Size.X-430||P.Y<35||P.Y>300))return Super::NativeOnMouseButtonDown(G,E);
 if(FullMap&&P.Y<80){SetChart(false,false);return FReply::Handled();}
 if(!FullMap&&P.X>Size.X-430&&P.Y>=35&&P.Y<75){SetChart(true,true);return FReply::Handled();}
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();int Selected=INDEX_NONE;double Best=DBL_MAX;
 for(int N=0;N<Sim->Data->Systems.Num();++N)if(!UVTIntroComponent::IsArena(Sim,N)){auto Delta=P-ChartPoint(N,Size);if(Delta.X>=-18&&Delta.X<180&&FMath::Abs(Delta.Y)<20&&Delta.SizeSquared()<Best){Best=Delta.SizeSquared();Selected=N;}}
 if(Selected!=INDEX_NONE)RouteDestination=Sim->Data->Systems[Selected].Id;
 return FReply::Handled();
}
int32 UVTUI::PaintNavigation(const FGeometry& G,FSlateWindowElementList& Out,int32 Layer) const{
 auto* PC=Cast<AVTController>(GetOwningPlayer());auto* Ship=PC?Cast<AVTShip>(PC->GetPawn()):nullptr;if(!Ship)return Layer;
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();auto* D=Sim->Data.Get();const FVector2D Size=G.GetLocalSize();const FLinearColor Amber(1,0.7f,0.1f),Dim(0.4f,0.3f,0.1f);
 auto Text=[&](FVector2D P,const FString& T,int Pt=14){auto Font=HudFont;Font.Size=Pt;FSlateDrawElement::MakeText(Out,Layer+2,G.ToPaintGeometry(FVector2D(800,50),FSlateLayoutTransform(P)),T,Font,ESlateDrawEffect::None,Amber);};
 auto Line=[&](FVector2D A,FVector2D B,FLinearColor C,float W=2){TArray<FVector2D> P={A,B};FSlateDrawElement::MakeLines(Out,Layer+1,G.ToPaintGeometry(),P,ESlateDrawEffect::None,C,true,W);};
 const int Destination=D->FindSystem(RouteDestination);const auto Route=VTNavigation::Route(*D,Ship->SystemIndex,Destination);
 if(ChartOpen){
  const FVector2D Offset=FullMap?FVector2D::ZeroVector:FVector2D(Size.X-430,35),Extent=FullMap?Size:FVector2D(415,265);
  FSlateDrawElement::MakeBox(Out,Layer,G.ToPaintGeometry(Extent,FSlateLayoutTransform(Offset)),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,FLinearColor(0.015f,0.01f,0.006f,0.98f));
  Text(Offset+FVector2D(16,10),FullMap?TEXT("STAR CHART   [G / F / ESC] CLOSE"):TEXT("STAR CHART  [G / CLICK] EXPAND"),FullMap?20:12);
  for(int N=0;N<D->Systems.Num();++N)if(!UVTIntroComponent::IsArena(Sim,N))for(auto Link:D->Systems[N].Links){int J=D->FindSystem(Link);if(J>N&&!UVTIntroComponent::IsArena(Sim,J))Line(ChartPoint(N,Size),ChartPoint(J,Size),Dim,1);}
  for(int N=1;N<Route.Num();++N)if(!UVTIntroComponent::IsArena(Sim,Route[N-1]))Line(ChartPoint(Route[N-1],Size),ChartPoint(Route[N],Size),Amber,3);
  for(int N=0;N<D->Systems.Num();++N)if(!UVTIntroComponent::IsArena(Sim,N)){auto P=ChartPoint(N,Size);const auto C=N==Ship->SystemIndex?FLinearColor(0.2f,0.8f,1):N==Destination?FLinearColor(0.2f,1,0.4f):Amber;FSlateDrawElement::MakeBox(Out,Layer+2,G.ToPaintGeometry(FVector2D(10,10),FSlateLayoutTransform(P-FVector2D(5,5))),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,C);Text(P+FVector2D(12,-9),D->Systems[N].DisplayName.ToString(),FullMap?16:9);}
  FString Status=Destination==INDEX_NONE?TEXT("Click a system to plot a gate route"):Route.IsEmpty()?TEXT("No connected gate route"):Route.Num()==1?TEXT("Destination reached"):FString::Printf(TEXT("%s: %d jumps | next: %s"),*D->Systems[Destination].DisplayName.ToString(),Route.Num()-1,*D->Systems[Route[1]].DisplayName.ToString());
  Text(FullMap?FVector2D(35,Size.Y-80):Offset+FVector2D(12,233),Status,FullMap?14:9);
  if(FullMap)Text({35,Size.Y-45},TEXT("RINGS: GREEN friendly | AMBER neutral | RED hostile | BLUE your ship.  World remains live."),12);
 }else if(Route.Num()>1&&(!PC->Intro->Active()||PC->Intro->JumpOnline())){
  const auto Gate=Sim->JumpPosition(Ship->SystemIndex,D->Systems[Route[1]].Id);FVector2D P;const bool Front=PC->ProjectWorldLocationToScreen(VT::ToWorld(Gate,Ship->SystemIndex),P,true);int W=0,H=0;PC->GetViewportSize(W,H);P*=Size/FVector2D(FMath::Max(1,W),FMath::Max(1,H));if(!Front)P=FVector2D(Size.X/2,Size.Y-220);P.X=FMath::Clamp(P.X,40.,Size.X-270);P.Y=FMath::Clamp(P.Y,100.,Size.Y-210);
  Line(P+FVector2D(0,-14),P+FVector2D(14,0),Amber);Line(P+FVector2D(14,0),P+FVector2D(0,14),Amber);Line(P+FVector2D(0,14),P+FVector2D(-14,0),Amber);Line(P+FVector2D(-14,0),P+FVector2D(0,-14),Amber);
  Text(P+FVector2D(22,-10),FString::Printf(TEXT("GATE: %s | %.0f m"),*D->Systems[Route[1]].DisplayName.ToString(),(Gate-Ship->Movement->Motion.Position).Size()),12);
 }
 return Layer+3;
}
