#include "VTUI.h"
#include "VTGameplay.h"
#include "VTCombat.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"

// Native readout rendering over the original, once-rasterised metal/CRT artwork.
// Coordinates match hud.html's five clusters in logical pixels; no gameplay state is stored here.
int32 UVTUI::PaintFlightHUD(const FGeometry& G,FSlateWindowElementList& Out,int32 Layer) const {
 auto* PC=Cast<AVTController>(GetOwningPlayer()); auto* S=PC?Cast<AVTShip>(PC->GetPawn()):nullptr;
 if(!S||HudPanels.Num()!=5)return Layer;
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();auto* GS=GetWorld()->GetGameState<AVTGameState>();auto* PS=PC->GetPlayerState<AVTPlayerState>();
 const auto& D=S->Definition;const auto& E=S->Combat->EquipmentState;
 int VX=0,VY=0;PC->GetViewportSize(VX,VY);
 const FVector2D View=G.GetLocalSize();
 // The legacy panels use CSS pixels: compensate UMG DPI once, then fit narrow viewports.
 const float Scale=FMath::Min(1.f,float(VX)/900)*float(View.X/FMath::Max(1,VX));
 const FVector2D Size=View/Scale;
 const FLinearColor Amber(FColor(255,178,0)),Dim(FColor(181,116,2)),Red(1,0.18f,0.08f),Blue(0.15f,0.62f,1);
 auto Rect=[&](FVector2D P,FVector2D Ext,FLinearColor C,int L=0){FSlateDrawElement::MakeBox(Out,Layer+L,G.ToPaintGeometry(Ext*Scale,FSlateLayoutTransform(P*Scale)),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,C);};
 auto Text=[&](FVector2D P,const FString& Str,int Pt=10,FLinearColor C=FLinearColor(FColor(255,178,0))){
  auto Font=HudFont;Font.Size=Pt;
  // One soft underlay keeps the amber phosphor glow without animated layout or web rendering.
  FSlateDrawElement::MakeText(Out,Layer+2,G.ToPaintGeometry(FVector2D(400,30)*Scale,FSlateLayoutTransform(Scale,P*Scale+FVector2D(1,1))),Str,Font,ESlateDrawEffect::None,FLinearColor(C.R,C.G,C.B,0.2f));
  FSlateDrawElement::MakeText(Out,Layer+3,G.ToPaintGeometry(FVector2D(400,30)*Scale,FSlateLayoutTransform(Scale,P*Scale)),Str,Font,ESlateDrawEffect::None,C);
 };
 auto Art=[&](int I,FVector2D P,FVector2D Ext){FSlateBrush Brush;Brush.SetResourceObject(HudPanels[I]);Brush.ImageSize=Ext;Brush.DrawAs=ESlateBrushDrawType::Image;FSlateDrawElement::MakeBox(Out,Layer+1,G.ToPaintGeometry(Ext*Scale,FSlateLayoutTransform(P*Scale)),&Brush);};
 auto Bar=[&](FVector2D P,float W,float H,float Fraction,FLinearColor Colour){
  Fraction=FMath::Clamp(Fraction,0.f,1.f);Rect(P,FVector2D(W,H),FLinearColor(0.18f,0.105f,0.01f,0.8f),2);
  Rect(P+FVector2D(1,1),FVector2D(FMath::Max(0.f,(W-2)*Fraction),H-2),Colour,3);
  for(float X=8;X<W-1;X+=10)Rect(P+FVector2D(X,1),FVector2D(2,H-2),FLinearColor(0.01f,0.006f,0,0.9f),4);
 };
 auto Arc=[&](FVector2D C,float Radius,float Start,float Sweep,FLinearColor Colour,float Width){TArray<FVector2D> Points;int N=FMath::Max(2,FMath::CeilToInt(FMath::Abs(Sweep)*12));for(int I=0;I<=N;++I){float A=Start+Sweep*I/N;Points.Add((C+FVector2D(FMath::Cos(A),FMath::Sin(A))*Radius)*Scale);}FSlateDrawElement::MakeLines(Out,Layer+3,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Colour,true,Width*Scale);};
 auto Dial=[&](FVector2D C,float Remaining,float Duration,const TCHAR* Label,bool Fitted=true,FLinearColor Colour=FLinearColor(FColor(255,178,0))){
  Arc(C,18,-PI/2,2*PI,FLinearColor(Colour.R,Colour.G,Colour.B,0.14f),4);
  if(Fitted)Arc(C,18,-PI/2,2*PI*(Remaining>0?1-FMath::Clamp(Remaining/FMath::Max(0.001f,Duration),0.f,1.f):1),FLinearColor(Colour.R,Colour.G,Colour.B,Remaining>0?1:0.28f),4);
  Text(C+FVector2D(-13,-6),!Fitted?TEXT("--"):Remaining>0?FString::Printf(TEXT("%.1f"),Remaining):TEXT("RDY"),9,Colour);
  Text(C+FVector2D(-20,30),Label,7,Dim);
 };
 const float Hull=FMath::Clamp(S->Attributes->Hull.GetCurrentValue()/FMath::Max(1.f,D.Hull),0.f,1.f);
 const FLinearColor HullColour=Hull<0.25f?Red:Hull<0.5f?FLinearColor(1,0.48f,0.09f):Amber;
 // Shield edge illumination and damage vignette stay behind the panel chrome.
 for(int Bank=0;Bank<4;++Bank){float Charge=D.ShieldMax[Bank]>0?S->Combat->Shields[Bank]/D.ShieldMax[Bank]:0;if(Charge<=0)continue;
  for(int I=0;I<12;++I){float Alpha=Charge*0.5f*(1-float(I)/12);FLinearColor C(Blue.R,Blue.G,Blue.B,Alpha);float Margin=I*2;
   if(Bank==0)Rect(FVector2D(0,Margin),FVector2D(Size.X,2),C);
   if(Bank==1)Rect(FVector2D(0,Size.Y-Margin-2),FVector2D(Size.X,2),C);
   if(Bank==2)Rect(FVector2D(Margin,0),FVector2D(2,Size.Y),C);
   if(Bank==3)Rect(FVector2D(Size.X-Margin-2,0),FVector2D(2,Size.Y),C);
  }
 }
 if(Hull<0.5f)for(int I=0;I<20;++I){float A=(0.5f-Hull)*0.075f*(1-float(I)/20);FLinearColor C(Red.R,Red.G,Red.B,A);float M=I*3;Rect(FVector2D(M,M),FVector2D(Size.X-2*M,3),C);Rect(FVector2D(M,Size.Y-M-3),FVector2D(Size.X-2*M,3),C);Rect(FVector2D(M,M),FVector2D(3,Size.Y-2*M),C);Rect(FVector2D(Size.X-M-3,M),FVector2D(3,Size.Y-2*M),C);}
 // Rings are projected on the flight plane, so they inherit the camera's real perspective.
 FVector2D PixelToLocal(View.X/FMath::Max(1,VX),View.Y/FMath::Max(1,VY));
 auto Project=[&](const FVector& P,FVector2D& Local){FVector2D Pixel;if(!PC->ProjectWorldLocationToScreen(P,Pixel,true))return false;Local=Pixel*PixelToLocal;return true;};
 auto Ring=[&](AVTShip* Ship,bool Player){FVector Origin=Ship->GetActorLocation();Origin.Z=-900;FVector2D Centre,EX,EY;float Radius=Player?62:46;
  if(!Project(Origin,Centre)||Centre.X<-100||Centre.Y<-100||Centre.X>View.X+100||Centre.Y>View.Y+100||!Project(Origin+FVector(Radius*100,0,0),EX)||!Project(Origin+FVector(0,Radius*100,0),EY))return;
  EX-=Centre;EY-=Centre;auto Band=[&](float R,float Start,float Sweep,FLinearColor C,float Thickness){TArray<FVector2D> Points;int N=FMath::Max(2,FMath::CeilToInt(FMath::Abs(Sweep)*10));for(int I=0;I<=N;++I){float A=Start+Sweep*I/N;Points.Add(Centre+(EX*FMath::Cos(A)+EY*FMath::Sin(A))*R);}FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,C,true,Thickness);};
  auto FillBand=[&](float Inner,float Outer,float Start,float Sweep,FLinearColor C){if(Outer<=Inner)return;TArray<FSlateVertex> Vertices;TArray<SlateIndex> Indices;int N=FMath::Max(2,FMath::CeilToInt(FMath::Abs(Sweep)*12));for(int I=0;I<=N;++I){float A=Start+Sweep*I/N;FVector2D Axis=EX*FMath::Cos(A)+EY*FMath::Sin(A);for(float R:{Inner,Outer})Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(G.GetAccumulatedRenderTransform(),FVector2f(Centre+Axis*R),FVector2f(0.5f,0.5f),C.ToFColor(false)));if(I<N){SlateIndex V=I*2;Indices.Append({V,SlateIndex(V+1),SlateIndex(V+2),SlateIndex(V+1),SlateIndex(V+3),SlateIndex(V+2)});}}auto Resource=FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush("WhiteBrush"));FSlateDrawElement::MakeCustomVerts(Out,Layer,Resource,Vertices,Indices,nullptr,0,0);};
  Band(1,0,2*PI,FLinearColor(0.35f,0.29f,0.2f,0.6f),1);Band(0.42f,0,2*PI,FLinearColor(1,0.698f,0,0.16f),1);
  float Heading=-Ship->Movement->Motion.Heading;const auto& Def=Ship->Definition;
  float H=FMath::Clamp(Ship->Attributes->Hull.GetCurrentValue()/FMath::Max(1.f,Def.Hull),0.f,1.f);FLinearColor HC=Ship->Disabled?FLinearColor(0.4f,0.75f,0.8f,0.4f):H<0.25f?FLinearColor(1,0.18f,0.08f,0.5f):H<0.5f?FLinearColor(1,0.48f,0.09f,0.35f):FLinearColor(1,0.698f,0,0.3f);
  FillBand(0,0.42f*(1-H),0,2*PI,HC);
  for(int Bank=0;Bank<4;++Bank){if(Def.ShieldMax[Bank]<=0)continue;float Charge=FMath::Clamp(Ship->Combat->Shields[Bank]/Def.ShieldMax[Bank],0.f,1.f);float Sweep=Def.ShieldArcs==1?2*PI:Def.ShieldArcs==2?PI:PI/2;float A=Bank==0?0:Bank==1?PI:Bank==2?-PI/2:PI/2;Band(0.72f,Heading+A-Sweep/2+0.025f,Sweep-0.05f,FLinearColor(0.1f,0.4f,0.6f,0.15f),1);FillBand(0.44f,0.44f+Charge*0.28f,Heading+A-Sweep/2+0.025f,Sweep-0.05f,FLinearColor(0.15f,0.62f,1,0.35f));}
  if(Player){for(bool Port:{true,false}){float A=Heading+(Port?-PI/2:PI/2);float Reload=Port?Ship->PortReload:Ship->StarboardReload;float Ready=1-FMath::Clamp(Reload/FMath::Max(0.001f,Def.Reload),0.f,1.f);Band(0.94f,A-Def.Arc/2,Def.Arc,FLinearColor(1,0.698f,0,0.12f),2);Band(0.94f,A-Def.Arc/2,Def.Arc*Ready,FLinearColor(1,0.698f,0,0.65f),3);}}
 };
 Ring(S,true);int RingCount=0;
 for(AVTShip* Other:Sim->Queries.Ordered(S->SystemIndex))if(IsValid(Other)&&Other!=S&&(Other->GetActorLocation()-S->GetActorLocation()).SizeSquared2D()<FMath::Square(60000.f)){Ring(Other,false);if(++RingCount>=24)break;}
 FVector2D Top(Size.X/2-160,14),Status(18,14),Coords(Size.X-194,14),Left(18,Size.Y-178),Right(Size.X-270,Size.Y-140);
 Art(0,Top,{320,54});Art(1,Status,{210,92});Art(2,Coords,{176,62});Art(3,Left,{262,160});Art(4,Right,{252,122});
 Text(Top+FVector2D(24,22),TEXT("H U L L"),7,Dim);Bar(Top+FVector2D(60,22),192,10,Hull,HullColour);Text(Top+FVector2D(260,19),FString::Printf(TEXT("%3.0f%%"),Hull*100),10,HullColour);
 auto Stat=[&](int Y,const TCHAR* Label,const FString& Value){Text(Status+FVector2D(24,Y),Label,7,Dim);Text(Status+FVector2D(145,Y-2),Value,10);};
 if(Sim->ActiveScenario()){Stat(21,TEXT("W A V E"),FString::FromInt(GS?GS->Wave:0));Stat(36,TEXT("ENEMIES"),FString::FromInt(GS?GS->EnemiesRemaining:0));Stat(51,TEXT("PLUNDER"),FString::FromInt(PS?PS->Boarded:0));}
 else {Text(Status+FVector2D(24,18),Sim->Data->Systems[S->SystemIndex].DisplayName.ToString().ToUpper(),9);Stat(36,TEXT("CREDITS"),FString::FromInt(PS?PS->Credits:0));Stat(51,TEXT("PLUNDER"),FString::FromInt(PS?PS->Boarded:0));}
 Text(Status+FVector2D(24,69),S->Combat->BoardingProgress>0?TEXT("BOARDING"):S->Autopilot?TEXT("AI PILOT"):TEXT("MANUAL HELM"),7,S->Autopilot?Amber:Dim);
 Text(Coords+FVector2D(60,17),TEXT("X"),7,Dim);Text(Coords+FVector2D(80,12),FString::Printf(TEXT("%+06.0f"),S->Movement->Motion.Position.X),14);
 Text(Coords+FVector2D(60,36),TEXT("Y"),7,Dim);Text(Coords+FVector2D(80,31),FString::Printf(TEXT("%+06.0f"),S->Movement->Motion.Position.Y),14);
 Dial(Left+FVector2D(36,32),S->PortReload,D.Reload,TEXT("< PORT"));Dial(Left+FVector2D(96,32),S->StarboardReload,D.Reload,TEXT("STBD >"));
 Text(Left+FVector2D(24,92),D.Equipment.Torpedoes?FString::Printf(TEXT("TORPEDO TUBES  %d  LOCKS %d"),E.TorpedoMagazine,E.Locks.Num()):TEXT("TORPEDO TUBES --"),7,Dim);
 int Count=D.Equipment.Torpedoes?FMath::Clamp(D.Equipment.Tubes,1,16):0;float TubeW=Count?float(226-(Count-1)*5)/Count:0;
 for(int I=0;I<Count;++I){FVector2D P=Left+FVector2D(24+I*(TubeW+5),108);float Fill=FMath::Clamp(E.Loaded-I,0.f,1.f);Rect(P,{TubeW,28},FLinearColor(0.3f,0.19f,0.01f,0.7f),2);Rect(P+FVector2D(1,1),{TubeW-2,26},FLinearColor(0.01f,0.007f,0.002f,1),3);if(Fill>0)Rect(P+FVector2D(1,27-26*Fill),{TubeW-2,26*Fill},Fill>=1?Amber:FLinearColor(1,0.48f,0.09f),4);Text(P+FVector2D(3,9),Fill>=1?TEXT("RDY"):Fill>0?FString::Printf(TEXT("%.0f%%"),Fill*100):TEXT("--"),7,Fill>=1?FLinearColor(0.1f,0.06f,0):Dim);}
 float Battery=S->Attributes->Battery.GetCurrentValue()/FMath::Max(1.f,D.BatteryMax);float Aim=PC->AimBattery/FMath::Max(0.001f,Sim->Data->Feel.time.battery_max);
 float Speed=FMath::Clamp(float(S->Movement->Motion.Velocity.Size())/FMath::Max(1.f,D.Stats.MaxSpeed*Sim->Data->FlightSpeedMultiplier*S->Combat->SpeedScale),0.f,1.f);
 float Agility=FMath::Lerp(D.Stats.TurnRateSlow,D.Stats.TurnRateFast,Speed)/FMath::Max(0.001f,D.Stats.TurnRateSlow);
 auto Row=[&](int Y,const TCHAR* Label,float Value,const FString& ValueText){Text(Right+FVector2D(24,Y),Label,7,Dim);Bar(Right+FVector2D(79,Y+2),58,8,Value,Amber);Text(Right+FVector2D(144,Y-1),ValueText,9);};
 Row(38,TEXT("BATTERY"),Battery,FString::Printf(TEXT("%.0f%%"),Battery*100));Row(56,TEXT("AIM"),Aim,GetWorld()->GetNetMode()==NM_Standalone?FString::Printf(TEXT("%.0f%%"),Aim*100):TEXT("RT"));
 Row(74,TEXT("HELM"),Agility,FMath::Abs(PC->LocalIntent.Throttle)>0.75f?TEXT("FULL"):FMath::Abs(PC->LocalIntent.Throttle)>0.1f?TEXT("HALF"):TEXT("STOP"));
 Dial(Right+FVector2D(204,58),E.WarpCooldown,D.Equipment.WarpCooldown,TEXT("WARP"),D.Equipment.Warp,FLinearColor(1,0.48f,0.09f));
 auto DeviceBinding=[&](const TCHAR* ActionName){FString Result;auto* Sub=PC->GetLocalPlayer()?ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()):nullptr;if(Sub)for(const UInputAction* Action:PC->Actions)if(Action&&Action->GetFName()==FName(ActionName))for(const auto& Key:Sub->QueryKeysMappedToAction(Action))if(Key.IsGamepadKey()==PC->UsingGamepadAim){Result=Key.GetDisplayName().ToString();break;}return Result;};
 if(D.Equipment.Warp){const auto Key=DeviceBinding(TEXT("IA_Warp"));if(!Key.IsEmpty())Text({Size.X/2-210,Size.Y-53},FString::Printf(TEXT("[%s] HOLD TO AIM WARP / RELEASE TO JUMP"),*Key),8,Amber);}
 if(D.Equipment.Torpedoes){const auto Key=DeviceBinding(TEXT("IA_Torpedo"));if(!Key.IsEmpty())Text({Size.X/2-210,Size.Y-38},FString::Printf(TEXT("[%s] HOLD TO LOCK / RELEASE TORPEDOES"),*Key),8,Dim);}
 Text({Size.X/2-140,Size.Y-22},TEXT("[TAB] CONTROLS   [F] CHART   [ESC] MENU"),7,Dim);
 if(S->Disabled||S->GatePassage->Status().Charge>0||S->DockProgress>0||(GS&&!GS->Outcome.IsEmpty())){FString Message=S->Disabled?TEXT("SHIP DISABLED  |  [R] RECOVER"):S->GatePassage->Status().Charge>0?(S->GatePassage->Departing()?TEXT("FLYING THROUGH GATE"):TEXT("ALIGNING / CHARGING GATE")):S->DockProgress>0?TEXT("DOCKING"):GS->Outcome;Text({Size.X/2-160,Size.Y*0.36},Message.ToUpper(),16,S->Disabled?Red:Amber);}
 const auto Hint=VTInteractionHint(S,Sim);
 if(Hint.Kind!=EVTInteractionHint::None){FVector2D Anchor;if(Project(VT::ToWorld(Hint.Position,S->SystemIndex),Anchor)){
  FString Keys;auto* Sub=PC->GetLocalPlayer()?ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()):nullptr;
  if(Sub)for(const UInputAction* Action:PC->Actions)if(Action&&Action->GetFName()==FName(TEXT("IA_Interact")))for(const auto& Key:Sub->QueryKeysMappedToAction(Action)){
   const FString Name=Key==EKeys::Gamepad_FaceButton_Right?TEXT("PAD B"):Key==EKeys::Gamepad_FaceButton_Bottom?TEXT("PAD A"):Key==EKeys::Gamepad_FaceButton_Top?TEXT("PAD Y"):Key==EKeys::Gamepad_FaceButton_Left?TEXT("PAD X"):Key.GetDisplayName().ToString();Keys+=(Keys.IsEmpty()?TEXT(""):TEXT(" / "))+FString::Printf(TEXT("[%s]"),*Name);
  }
  const bool NeedsKey=Hint.Kind!=EVTInteractionHint::Dock;
  if(!NeedsKey||!Keys.IsEmpty()){
   const FString Prompt=NeedsKey?FString::Printf(TEXT("Hold %s  %s"),*Keys,*Hint.Label.ToString()):Hint.Label.ToString();auto PromptFont=HudFont;PromptFont.Size=10;const double Width=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Prompt,PromptFont).X;const FVector2D Ext(FMath::Min(Size.X-30,FMath::Max(420.,Width+24)),58);FVector2D P=Anchor/Scale+FVector2D(-Ext.X/2,65);P.X=FMath::Clamp(P.X,15.,FMath::Max(15.,Size.X-Ext.X-15));P.Y=FMath::Clamp(P.Y,115.,FMath::Max(115.,Size.Y-230));
   Rect(P,Ext,FLinearColor(0.025f,0.016f,0.005f,0.94f),5);Layer+=6;Text(P+FVector2D(12,8),Prompt,10);Bar(P+FVector2D(12,38),Ext.X-24,8,Hint.Progress,Amber);
  }
 }}
 if(ControlsOpen){FVector2D P(Size.X-340,Size.Y/2-180);Rect(P,{322,360},FLinearColor(0.025f,0.016f,0.005f,0.97f),5);Layer+=6;Text(P+FVector2D(24,20),TEXT("C O N T R O L S"),12);const TCHAR* Keys[]={TEXT("W / S"),TEXT("A / D"),TEXT("LMB / RMB"),TEXT("SPACE / Q"),TEXT("SHIFT / CTRL"),TEXT("C / B"),TEXT("M / X"),TEXT("T / R"),TEXT("TAB / F"),TEXT("ESC")};const TCHAR* Desc[]={TEXT("Thrust"),TEXT("Steer"),TEXT("Aim / fire broadsides"),TEXT("Boost / EMP"),TEXT("Warp / torpedoes"),TEXT("Brace / interact"),TEXT("Mines / point defence"),TEXT("AI pilot / recover"),TEXT("Controls / chart"),TEXT("Pause / menu")};for(int I=0;I<10;++I){Text(P+FVector2D(24,56+I*27),Keys[I],9);Text(P+FVector2D(142,56+I*27),Desc[I],8,FLinearColor(0.85f,0.7f,0.5f));}}
 return Layer+8;
}
