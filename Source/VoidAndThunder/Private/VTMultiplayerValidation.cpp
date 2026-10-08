#include "VTGameplay.h"
#include "VTCombat.h"
#include "VTGameplayProbe.h"
#include "EngineUtils.h"
#include "VTSaveSubsystem.h"
#include "VTSessionSubsystem.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/World.h"
#include "UnrealClient.h"
#include "RHI.h"
#include "VTUI.h"
#include "VTWorldAnchor.h"
#include "Components/Button.h"
#include "Components/StaticMeshComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"

#if !UE_BUILD_SHIPPING
void VTIntroValidationInput(AVTController* PC);
void VTIntroValidationTick(UVTSimulation* Sim,const FString& Role);
#endif
void AVTController::ValidationInput(float Dt) {
#if !UE_BUILD_SHIPPING
 FString ProbeRole; if(!FParse::Value(FCommandLine::Get(),TEXT("VTProbe="),ProbeRole)) return;
 if(ProbeRole.StartsWith(TEXT("Intro"))){VTIntroValidationInput(this);return;}
 if(ProbeRole.StartsWith(TEXT("Soak"))) {LocalIntent=FVTPilotIntent();LocalIntent.Throttle=0.6f;LocalIntent.Turn=0.15f;return;}
 if(ProbeRole.StartsWith(TEXT("Gameplay"))) {for(TActorIterator<AVTGameplayProbe> It(GetWorld());It;++It){It->DriveLocal(this);break;}return;}
 if(ProbeRole==TEXT("RenderGate")){LocalIntent=FVTPilotIntent();if(auto* Ship=Cast<AVTShip>(GetPawn()))if(Ship->SystemIndex==0&&GetWorld()->GetRealTimeSeconds()>2)LocalIntent.Buttons=VTButtons::Interact;return;}
 if(ProbeRole.StartsWith(TEXT("Render"))) {
  if(FParse::Param(FCommandLine::Get(),TEXT("VTThrottleProbe"))&&GetPawn()) {
   const double Age=GetWorld()->GetRealTimeSeconds();
   const float Before[]={0.5f,0.f,0.f,0.f,0.5f,0.5f,1.f,1.f,0.5f,0.5f,0.f,0.f,-1.f,-1.f,-1.f};
   const bool Down[]={true,true,false,true,false,true,false,true,false,true,false,true,false,true,false};
   const FKey Keys[]={EKeys::S,EKeys::S,EKeys::S,EKeys::W,EKeys::W,EKeys::W,EKeys::W,EKeys::S,EKeys::S,EKeys::S,EKeys::S,EKeys::S,EKeys::S,EKeys::S,EKeys::S};
   if(ThrottleProbeStage<UE_ARRAY_COUNT(Before)&&Age>3+ThrottleProbeStage*0.6) {
    ThrottleProbePassed&=FMath::IsNearlyEqual(LocalIntent.Throttle,Before[ThrottleProbeStage]);
    FKeyEvent Event(Keys[ThrottleProbeStage],FModifierKeysState(),0,ThrottleProbeStage==1,0,0);
    if(Down[ThrottleProbeStage])FSlateApplication::Get().ProcessKeyDownEvent(Event);else FSlateApplication::Get().ProcessKeyUpEvent(Event);
    ++ThrottleProbeStage;
   }
  }
  if(FParse::Param(FCommandLine::Get(),TEXT("VTFlightProbe"))&&GEngine&&GEngine->GameViewport){auto* Ship=Cast<AVTShip>(GetPawn());const double Age=GetWorld()->GetRealTimeSeconds();auto View=GEngine->GameViewport->GetGameViewportWidget();auto Window=GEngine->GameViewport->GetWindow();if(Ship&&View.IsValid()&&Window.IsValid()){
   const auto& Geometry=View->GetCachedGeometry();const auto Point=Geometry.LocalToAbsolute(Geometry.GetLocalSize()*FVector2D(0.7,0.35));auto Mouse=[&](bool Down){TSet<FKey> Keys;if(Down)Keys.Add(EKeys::LeftMouseButton);FPointerEvent Event(0,Point,Point,Keys,EKeys::LeftMouseButton,0,FModifierKeysState());if(Down)FSlateApplication::Get().ProcessMouseButtonDownEvent(Window->GetNativeWindow(),Event);else FSlateApplication::Get().ProcessMouseButtonUpEvent(Event);};
   if(FlightProbeStage==0&&Age>20){FlightProbeOrigin=Ship->Movement->Motion.Position;FSlateApplication::Get().SetCursorPos(Point);FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::LeftShift,FModifierKeysState(),0,false,0,0));FlightProbeStage=1;}
   else if(FlightProbeStage==1&&Age>20.5){FlightProbeHeld=(LocalIntent.Buttons&VTButtons::Warp)&&Ship->Definition.Equipment.Warp&&!Ship->Definition.Equipment.EMP;Mouse(true);FlightProbeStage=2;}
   else if(FlightProbeStage==2&&Age>21){FlightProbeHeld&=(LocalIntent.Buttons&(VTButtons::AimPort|VTButtons::Port))==0;Mouse(false);FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(EKeys::LeftShift,FModifierKeysState(),0,false,0,0));FlightProbeStage=3;}
   else if(FlightProbeStage==3&&Age>22){FlightProbePassed=FlightProbeHeld&&Ship->Combat->EquipmentState.WarpCooldown>0&&(Ship->Movement->Motion.Position-FlightProbeOrigin).Size()>1&&Ship->PortReload==0&&Ship->Combat->PortCharge==0;FlightProbeStage=4;}
  }}
  if(FParse::Param(FCommandLine::Get(),TEXT("VTInteractionProbe"))) {
   const double Age=GetWorld()->GetRealTimeSeconds();auto* Ship=Cast<AVTShip>(GetPawn());auto* PS=GetPlayerState<AVTPlayerState>();auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();
   if(Ship&&PS&&InteractionProbeStage==0&&Age>24){auto* Sub=GetLocalPlayer()?ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()):nullptr;if(Sub)for(const UInputAction* Action:Actions)if(Action&&Action->GetFName()==FName(TEXT("IA_Interact")))for(const auto& Key:Sub->QueryKeysMappedToAction(Action))if(!Key.IsGamepadKey()){InteractionProbeKey=Key;break;}InteractionProbePrompt=VTInteractionHint(Ship,Sim).Kind==EVTInteractionHint::Loot;InteractionProbeBoarded=PS->Boarded;FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(InteractionProbeKey,FModifierKeysState(),0,false,0,0));InteractionProbeStage=1;}
   else if(Ship&&PS&&InteractionProbeStage==1&&Age>25.5){auto Hint=VTInteractionHint(Ship,Sim);InteractionProbePrompt&=Hint.Kind==EVTInteractionHint::Loot&&Hint.Progress>0;InteractionProbeStage=2;}
   else if(Ship&&PS&&InteractionProbeStage==2&&Age>28.5){InteractionProbeLooted=PS->Boarded==InteractionProbeBoarded+1&&VTInteractionHint(Ship,Sim).Kind!=EVTInteractionHint::Loot;FSlateApplication::Get().ProcessKeyUpEvent(FKeyEvent(InteractionProbeKey,FModifierKeysState(),0,false,0,0));InteractionProbeStage=3;}
  }
  if(FParse::Param(FCommandLine::Get(),TEXT("VTBroadsideProbe"))&&GEngine&&GEngine->GameViewport) {
   auto* Ship=Cast<AVTShip>(GetPawn());auto View=GEngine->GameViewport->GetGameViewportWidget();auto Window=GEngine->GameViewport->GetWindow();
   if(Ship&&View.IsValid()&&Window.IsValid()) {
    const double Age=GetWorld()->GetRealTimeSeconds();const auto& Geometry=View->GetCachedGeometry();const FVector2D Point=Geometry.LocalToAbsolute(Geometry.GetLocalSize()*0.5);
    auto Mouse=[&](bool Down){TSet<FKey> Keys;if(Down)Keys.Add(EKeys::LeftMouseButton);FPointerEvent Event(0,Point,Point,Keys,EKeys::LeftMouseButton,0,FModifierKeysState());if(Down)FSlateApplication::Get().ProcessMouseButtonDownEvent(Window->GetNativeWindow(),Event);else FSlateApplication::Get().ProcessMouseButtonUpEvent(Event);};
    if(BroadsideProbeStage==0&&Age>20){BroadsideProbeHeld=GEngine->GameViewport->GetMouseCaptureMode()==EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown;Mouse(true);BroadsideProbeStage=1;}
    else if(BroadsideProbeStage==1&&Age>20.4){BroadsideProbeHeld&=(LocalIntent.Buttons&VTButtons::AimPort)!=0&&(LocalIntent.Buttons&VTButtons::Port)==0;Mouse(false);BroadsideProbeStage=2;}
    else if(BroadsideProbeStage==2&&Age>20.8){BroadsideProbePassed=BroadsideProbeHeld&&(LocalIntent.Buttons&VTButtons::AimPort)==0&&(Ship->PortReload>0||Ship->Combat->PortCharge>0);BroadsideProbeStage=3;}
    else if(BroadsideProbeStage==3&&Age>23){Mouse(true);BroadsideProbeStage=4;}
    else if(BroadsideProbeStage==4&&Age>27){Mouse(false);BroadsideProbeStage=5;}
   }
  }
  return;
 }
 LocalIntent.Throttle=0.8f; LocalIntent.Turn=0.2f;
 FString Destination;
 if(GetWorld()->GetRealTimeSeconds()>4 && !ProbeJumped && FParse::Value(FCommandLine::Get(),TEXT("VTProbeSystem="),Destination)) {
  ServerJump(FName(Destination)); ProbeJumped=true;
 }
#endif
}
void UVTSimulation::ValidationTick() {
#if !UE_BUILD_SHIPPING
 FString ProbeRole; if(!FParse::Value(FCommandLine::Get(),TEXT("VTProbe="),ProbeRole)) return;
 if(ProbeRole.StartsWith(TEXT("Intro"))){VTIntroValidationTick(this,ProbeRole);return;}
 if(ProbeRole.StartsWith(TEXT("Soak"))) {SoakTick(ProbeRole);return;}
 if(ProbeRole.StartsWith(TEXT("Gameplay"))) {
  AVTGameplayProbe* Fixture=nullptr;for(TActorIterator<AVTGameplayProbe> It(GetWorld());It;++It){Fixture=*It;break;}
  if(GetWorld()->GetNetMode()!=NM_Client) {if(!Fixture) {Bootstrap(0);Fixture=GetWorld()->SpawnActor<AVTGameplayProbe>();}Fixture->ServerStep();}return;
 }
 if(ProbeRole.StartsWith(TEXT("Flow"))&&GetWorld()->GetMapName().Contains(TEXT("Menu"))) {
  auto* GI=CastChecked<UVTGameInstance>(GetWorld()->GetGameInstance()); auto* Session=GI->GetSubsystem<UVTSessionSubsystem>();
  if(ProbeRole==TEXT("FlowGuest")&&GI->ValidationSessionStarted&&FParse::Param(FCommandLine::Get(),TEXT("VTExpectHostDeparture"))&&Session->Status.Contains(TEXT("Host connection ended"))) {
   FString RunDir; FParse::Value(FCommandLine::Get(),TEXT("VTProbeDir="),RunDir);
   const bool Passed=!Session->Sessions.IsValid()||!Session->Sessions->GetNamedSession(NAME_GameSession);
   FFileHelper::SaveStringToFile(Passed ? TEXT("{\"passed\":true,\"returned_to_menu\":true,\"session_removed\":true}") : TEXT("{\"passed\":false}"),*(RunDir/TEXT("FlowDepartureGuest.json")));
   FPlatformMisc::RequestExitWithStatus(false,Passed ? 0 : 1); return;
  }
  if(GetWorld()->GetRealTimeSeconds()>2) {
   if(ProbeRole==TEXT("FlowHost")&&!GI->ValidationSessionStarted) {GI->ValidationSessionStarted=true; GI->PopulationProfile=TEXT("Authored"); Session->CreateWorld(FParse::Param(FCommandLine::Get(),TEXT("VTContinue")),TEXT("Validation-Frontend"));}
   if(ProbeRole==TEXT("FlowGuest")&&!GI->ValidationDiscoveryStarted) {GI->ValidationDiscoveryStarted=true; Session->Discover();}
   if(ProbeRole==TEXT("FlowGuest")&&!GI->ValidationSessionStarted&&!Session->Worlds.IsEmpty()) {GI->ValidationSessionStarted=true; Session->JoinWorld(0);}
  }
  return;
 }

 if(ProbeRole.StartsWith(TEXT("Render"))) {

  if(auto* Player=Cast<AVTController>(GetWorld()->GetFirstPlayerController())) {if(auto* Pawn=Cast<AVTShip>(Player->GetPawn()))Pawn->Invulnerable=true;if(ProbeRole==TEXT("RenderIntro")&&FParse::Param(FCommandLine::Get(),TEXT("VTIntroChoices"))&&!ProbeScaled&&Player->GetPawn()){Player->Intro->Change(EVTIntroStage::BatteryChoice);ProbeScaled=true;}}
  double Now=FPlatformTime::Seconds(), Age=GetWorld()->GetRealTimeSeconds();
  if(LastRenderFrame>0&&Age>20) RenderFrameMilliseconds.Add((Now-LastRenderFrame)*1000); LastRenderFrame=Now;
  if(ProbeRole==TEXT("Render")&&!ProbeScaled&&Age>1) {ConfigurePopulationFixture(500,FParse::Param(FCommandLine::Get(),TEXT("Busy")),FParse::Param(FCommandLine::Get(),TEXT("Armed"))); if(FParse::Param(FCommandLine::Get(),TEXT("Busy"))) if(auto* Player=GetWorld()->GetFirstPlayerController()) if(auto* Pawn=Cast<AVTShip>(Player->GetPawn())) {Pawn->SystemIndex=0; Pawn->Movement->Motion.Position=FVector2D(0,-500); Pawn->Movement->Previous=Pawn->Movement->Motion; Pawn->Movement->Authority=Pawn->Movement->Motion;} ProbeScaled=true;}
  if(ProbeRole==TEXT("RenderEnvironment")&&!ProbeScaled&&Age>1) {if(auto* Player=GetWorld()->GetFirstPlayerController())if(auto* Pawn=Cast<AVTShip>(Player->GetPawn())) {Pawn->SystemIndex=0;Pawn->Movement->Motion=FVTMotion();Pawn->Movement->Motion.Position=FVector2D(-600,0);Pawn->Movement->Previous=Pawn->Movement->Motion;Pawn->Movement->Authority=Pawn->Movement->Motion;Pawn->Anchored=true;if(FParse::Param(FCommandLine::Get(),TEXT("VTFlightProbe"))){FVTLoadoutSelection Fit;Fit.OverrideBatteries=Fit.OverrideSpecials=true;Fit.Batteries={FName("loadout.boost")};Fit.Specials={FName("loadout.microwarp")};Pawn->ApplyFit(Fit,true);const auto Link=Data->Systems[0].Links[0];const auto Centre=JumpPosition(0,Link),Axis=Centre.GetSafeNormal();Pawn->Movement->Motion.Position=Centre-Axis*100;Pawn->Movement->Motion.Heading=FMath::Atan2(Axis.Y,Axis.X);Pawn->Movement->Previous=Pawn->Movement->Motion;Pawn->Movement->Authority=Pawn->Movement->Motion;auto* Shot=Pawn->Combat->SpawnDeviceProjectile(EVTProjectileKind::Torpedo,Centre+FVector2D(-Axis.Y,Axis.X)*40,FVector2D::ZeroVector,1,100,12);Shot->TargetId=Pawn->PersistentId;Shot->Position=Centre+FVector2D(-Axis.Y,Axis.X)*40;Shot->Height=30;Shot->Remaining=100;}
if(FParse::Param(FCommandLine::Get(),TEXT("VTInteractionProbe"))){Bootstrap(0);FVTMotion M;M.Position=Pawn->Movement->Motion.Position+FVector2D(0,-65);auto* Prize=SpawnShip(TEXT("house_patrol"),Pawn->SystemIndex,M,true,Pawn->Faction);Prize->Disabled=true;Prize->Anchored=true;Prize->Invulnerable=false;}}ProbeScaled=true;}
  if(ProbeRole==TEXT("RenderGateMarker"))if(auto* PC=GetWorld()->GetFirstPlayerController())if(auto* Pawn=Cast<AVTShip>(PC->GetPawn())) {
   const auto Centre=JumpPosition(0,Data->Systems[0].Links[0]),Axis=Centre.GetSafeNormal();
   if(!ProbeScaled&&Age>1){Pawn->SystemIndex=0;Pawn->Anchored=true;Pawn->Movement->Motion=FVTMotion();Pawn->Movement->Motion.Position=Centre-Axis*(Data->GateStartDistance()+40)+FVector2D(-Axis.Y,Axis.X)*60;Pawn->Movement->Motion.Heading=FMath::Atan2(Axis.Y,Axis.X);Pawn->Movement->Previous=Pawn->Movement->Authority=Pawn->Movement->Motion;ProbeScaled=true;}
   for(TActorIterator<AVTWorldAnchor> It(GetWorld());It;++It)if(It->System==0&&It->Destination==Data->Systems[0].Links[0]&&It->GateStartArrow->IsVisible()) {
    const auto Start=VT::ToWorld(Centre-Axis*Data->GateStartDistance(),0)+FVector(0,0,-100);
    GateMarkerSeen|=It->GateStartArrow->GetComponentLocation().Equals(Start,1)&&It->GateStartArrow->GetStaticMesh()!=nullptr;
    GateMarkerAnimated|=It->GatePreviewArrow->IsVisible()&&(It->GatePreviewArrow->GetComponentLocation()-Start).Size()>2000;
   }
  }
  if(ProbeRole==TEXT("RenderGate"))if(auto* PC=Cast<AVTController>(GetWorld()->GetFirstPlayerController()))if(auto* Pawn=Cast<AVTShip>(PC->GetPawn())){
   if(!ProbeScaled&&Age>1){const auto Link=Data->Systems[0].Links[0];const auto Centre=JumpPosition(0,Link);Pawn->SystemIndex=0;Pawn->Anchored=false;Pawn->Movement->Motion=FVTMotion();Pawn->Movement->Motion.Position=Centre-Centre.GetSafeNormal()*Data->GateStartDistance();Pawn->Movement->Motion.Heading=FMath::Atan2(Centre.Y,Centre.X);Pawn->Movement->Motion.SimulationTime=SimulationTime;Pawn->Movement->Previous=Pawn->Movement->Authority=Pawn->Movement->Motion;FVTSavedShip GateRecord;GateRecord.JumpDestination=Link;GateRecord.JumpProgress=Data->Rules.JumpDwell;Pawn->GatePassage->Restore(GateRecord,SimulationTime);ProbeScaled=true;}
   if(auto* Camera=Cast<AVTCameraManager>(PC->PlayerCameraManager)) {
    if(Pawn->GatePassage->Departing()&&Camera->GateDepartureFocus){const auto Start=JumpPosition(0,Data->Systems[0].Links[0]);GateCameraDepartureSeen|=Camera->Focus.Equals(VT::ToWorld(Start-Start.GetSafeNormal()*Data->GateStartDistance(),0),200);}
    if(Pawn->GatePassage->Arriving())GateCameraArrivalSeen|=Camera->Focus.Equals(VT::ToWorld(Pawn->GatePassage->Status().ArrivalTarget,Pawn->SystemIndex),1);
   }
   if(Pawn->SystemIndex!=0){GateBrakingSeen|=Pawn->GatePassage->Arriving();if(!Pawn->GatePassage->Arriving()&&GateBrakingSeen)GateArrivalPassed|=Pawn->Movement->Motion.Position.Equals(Pawn->GatePassage->Status().ArrivalTarget,0.05)&&Pawn->Movement->Motion.Velocity.Size()<1;if(PC->PlayerCameraManager&&PC->PlayerCameraManager->FadeAmount>0.9f&&PC->PlayerCameraManager->FadeColor.Equals(FLinearColor::White)){GateFlashSeen=true;if(!ScreenshotRequested){ScreenshotRequested=true;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/RenderGate.png"),true,false);}}}
  }
  if(ProbeRole==TEXT("RenderMenu")&&Age>22&&!UIProbeStarted){UIProbeStarted=true;if(auto* Player=Cast<AVTController>(GetWorld()->GetFirstPlayerController()))if(Player->UI)if(auto* Create=Player->UI->GetWidgetFromName(TEXT("Skirmish"))){UIInitialFocus=Create->HasUserFocus(Player);FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Gamepad_DPad_Right,FModifierKeysState(),0,false,0,0));}}
  if(ProbeRole==TEXT("RenderMenu")&&Age>22.5&&!UIProbeFinished){UIProbeFinished=true;if(auto* Player=Cast<AVTController>(GetWorld()->GetFirstPlayerController()))if(Player->UI)if(auto* Continue=Player->UI->GetWidgetFromName(TEXT("Range")))UINavigationPassed=UIInitialFocus&&Continue->HasUserFocus(Player);}
  if((ProbeRole!=TEXT("RenderGateMarker")||GateMarkerAnimated)&&ProbeRole!=TEXT("RenderGate")&&!ScreenshotRequested&&Age>(FParse::Param(FCommandLine::Get(),TEXT("VTFlightProbe"))?19:25)) {ScreenshotRequested=true; FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/")+ProbeRole+TEXT(".png"),true,false);}
  if(!ProbeWrote&&Age>35&&!RenderFrameMilliseconds.IsEmpty()) {
   ProbeWrote=true; auto Samples=RenderFrameMilliseconds; Samples.Sort(); double P95=Samples[FMath::FloorToInt(Samples.Num()*0.95)]; double Total=0; for(double Ms:Samples) Total+=Ms;
   FString Report=FString::Printf(TEXT("{\"frames\":%d,\"p95_frame_ms\":%.6f,\"mean_fps\":%.3f,\"width\":1920,\"height\":1080,\"population\":%d,\"gpu\":\"%s\",\"passed\":%s}"),Samples.Num(),P95,Samples.Num()*1000/Total,Ships.Num(),*GRHIAdapterName,P95<=1000./60 ? TEXT("true") : TEXT("false"));
   TSharedPtr<FJsonObject> Parsed; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Report),Parsed); if(GEngine&&GEngine->GameViewport&&GEngine->GameViewport->Viewport){auto Size=GEngine->GameViewport->Viewport->GetSizeXY();Parsed->SetNumberField(TEXT("width"),Size.X);Parsed->SetNumberField(TEXT("height"),Size.Y);}
   Parsed->SetBoolField(TEXT("initial_menu_focus"),UIInitialFocus);Parsed->SetBoolField(TEXT("controller_menu_navigation"),UINavigationPassed);const auto* Captain=Cast<AVTController>(GetWorld()->GetFirstPlayerController());const bool MousePassed=!FParse::Param(FCommandLine::Get(),TEXT("VTBroadsideProbe"))||(Captain&&Captain->BroadsideProbePassed);Parsed->SetBoolField(TEXT("broadside_first_press_release"),MousePassed);const bool InteractionPassed=!FParse::Param(FCommandLine::Get(),TEXT("VTInteractionProbe"))||(Captain&&Captain->InteractionProbePrompt&&Captain->InteractionProbeLooted);Parsed->SetBoolField(TEXT("interaction_prompt_and_loot"),InteractionPassed);const bool FlightPassed=!FParse::Param(FCommandLine::Get(),TEXT("VTFlightProbe"))||(Captain&&Captain->FlightProbePassed);Parsed->SetBoolField(TEXT("mapped_warp_release_and_exclusive_aim"),FlightPassed);bool TorpedoVisualPassed=true;if(ProbeRole==TEXT("RenderEnvironment")&&FParse::Param(FCommandLine::Get(),TEXT("VTFlightProbe"))){TorpedoVisualPassed=false;for(AVTProjectile* Shot:Projectiles)if(IsValid(Shot)&&Shot->Kind==EVTProjectileKind::Torpedo){auto* Mesh=Cast<UStaticMeshComponent>(Shot->GetRootComponent());TorpedoVisualPassed=Mesh&&Mesh->GetRelativeScale3D().Equals(FVector(2*Data->ProjectileVisualRadius),0.001)&&Mesh->GetMaterial(0)==Data->ProjectileMaterial.Get()&&FMath::IsNearlyEqual(Data->ProjectileVisualRadius,7.f);if(TorpedoVisualPassed)break;}}Parsed->SetBoolField(TEXT("equipment_torpedo_original_visuals"),TorpedoVisualPassed);const bool GateCameraPassed=ProbeRole!=TEXT("RenderGate")||(GateCameraDepartureSeen&&GateCameraArrivalSeen);Parsed->SetBoolField(TEXT("gate_camera_departure_and_arrival_focus"),GateCameraPassed);const bool GatePassed=ProbeRole!=TEXT("RenderGate")||(GateCameraPassed&&GateFlashSeen&&GateBrakingSeen&&GateArrivalPassed);Parsed->SetBoolField(TEXT("gate_white_flash_and_arrival"),GatePassed);const bool ThrottlePassed=!FParse::Param(FCommandLine::Get(),TEXT("VTThrottleProbe"))||(Captain&&Captain->ThrottleProbePassed&&Captain->ThrottleProbeStage==15&&FMath::IsNearlyEqual(Captain->LocalIntent.Throttle,-1.f));Parsed->SetBoolField(TEXT("tap_throttle_hold_release_and_repeat"),ThrottlePassed);const bool MarkerPassed=ProbeRole!=TEXT("RenderGateMarker")||(GateMarkerSeen&&GateMarkerAnimated);Parsed->SetBoolField(TEXT("gate_start_arrow_and_animation"),MarkerPassed);const bool Passed=MarkerPassed&&ThrottlePassed&&GatePassed&&TorpedoVisualPassed&&FlightPassed&&P95<=1000./60&&(ProbeRole!=TEXT("RenderMenu")||UINavigationPassed)&&MousePassed&&InteractionPassed;Parsed->SetBoolField(TEXT("passed"),Passed);Parsed->SetNumberField(TEXT("simulation_time"),SimulationTime); Parsed->SetBoolField(TEXT("busy"),FParse::Param(FCommandLine::Get(),TEXT("Busy"))); Parsed->SetBoolField(TEXT("armed"),FParse::Param(FCommandLine::Get(),TEXT("Armed"))); auto Times=StepMilliseconds; if(!Times.IsEmpty()) {Times.Sort(); Parsed->SetNumberField(TEXT("p95_simulation_ms"),Times[FMath::FloorToInt(Times.Num()*0.95)]);} FJsonSerializer::Serialize(Parsed.ToSharedRef(),TJsonWriterFactory<>::Create(&Report)); FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("Validation/")+ProbeRole+TEXT(".json"))); FPlatformMisc::RequestExitWithStatus(false,Passed ? 0 : 1);
  }
  return;
 }
 auto* State=GetWorld()->GetGameState<AVTGameState>();
 if(State) MaxPlayersObserved=FMath::Max(MaxPlayersObserved,State->PlayerArray.Num());
 auto* PC=Cast<AVTController>(GetWorld()->GetFirstPlayerController());
 auto* Ship=PC ? Cast<AVTShip>(PC->GetPawn()) : nullptr;
 if(Ship) {
  if(Ship->HasAuthority()) Ship->Invulnerable=true;
  if(!ProbeOriginSet){ProbeOrigin=Ship->Movement->Motion.Position;ProbeOriginSet=true;}
  if((Ship->Movement->Motion.Position-ProbeOrigin).Size()>25) ProbeMoved=true;
 }
 if(ProbeRole==TEXT("Host") && !ProbeScaled && GetWorld()->GetRealTimeSeconds()>1) { Bootstrap(500);ProbeScaled=true; }
 if(ProbeRole==TEXT("Host")&&!ProbeLoaded&&GetWorld()->GetRealTimeSeconds()>20) {
  ProbeLoaded=true; auto* Save=GetWorld()->GetGameInstance()->GetSubsystem<UVTSaveSubsystem>();
  if(!Save->Save()||!Save->Load()) {UE_LOG(LogTemp,Error,TEXT("Network snapshot round trip failed")); FPlatformMisc::RequestExitWithStatus(false,2);}
 }
 if(ProbeRole==TEXT("FlowGuest")&&FParse::Param(FCommandLine::Get(),TEXT("VTExpectHostDeparture"))) return;
 float Limit=(ProbeRole==TEXT("Host")||ProbeRole==TEXT("FlowHost")) ? 45 : 25;
 FParse::Value(FCommandLine::Get(),TEXT("VTProbeSeconds="),Limit);
 if(ProbeRole==TEXT("Host")&&GetWorld()->GetRealTimeSeconds()<20) {
  for(AVTShip* S:Ships) if(IsValid(S)&&!S->IsNPC) if(auto* PS=S->GetPlayerState<AVTPlayerState>()) {S->Invulnerable=true; PS->Credits=777; PS->Boarded=7;}
 }
 if(ProbeWrote || GetWorld()->GetRealTimeSeconds()<Limit) return;
 ProbeWrote=true;
 TSharedRef<FJsonObject> O=MakeShared<FJsonObject>();
 O->SetStringField(TEXT("role"),ProbeRole); O->SetNumberField(TEXT("max_players"),MaxPlayersObserved);
 O->SetBoolField(TEXT("moved"),ProbeMoved); O->SetNumberField(TEXT("system"),Ship ? Ship->SystemIndex : -1);
 O->SetStringField(TEXT("ship_id"),Ship ? Ship->PersistentId.ToString() : TEXT(""));
 O->SetNumberField(TEXT("position_x"),Ship ? Ship->Movement->Motion.Position.X : 0); O->SetNumberField(TEXT("position_y"),Ship ? Ship->Movement->Motion.Position.Y : 0);
 O->SetNumberField(TEXT("origin_x"),ProbeOrigin.X); O->SetNumberField(TEXT("origin_y"),ProbeOrigin.Y);
 if(auto* PS=PC ? PC->GetPlayerState<AVTPlayerState>() : nullptr) {O->SetStringField(TEXT("profile"),PS->Profile.ToString()); O->SetNumberField(TEXT("credits"),PS->Credits); O->SetNumberField(TEXT("boarded"),PS->Boarded);}
 O->SetNumberField(TEXT("ack"),Ship ? Ship->Movement->Authority.Ack : 0);
 int32 NPC=0; for(AVTShip* S:Ships) if(IsValid(S) && S->IsNPC) ++NPC;
 O->SetNumberField(TEXT("npcs"),NPC); O->SetNumberField(TEXT("simulation_time"),State ? State->SimulationTime : SimulationTime);
 bool Passed=ProbeMoved && Ship;
 if(ProbeRole==TEXT("Host")) Passed=Passed && MaxPlayersObserved==4 && NPC==500;
 else {
  bool Rejoined=FParse::Param(FCommandLine::Get(),TEXT("VTRejoined"));
  Passed=Passed && (Rejoined ? MaxPlayersObserved>=2 : MaxPlayersObserved==4) && Ship->Movement->Authority.Ack>0 && NPC==50;
  if(Rejoined) {auto* PS=PC->GetPlayerState<AVTPlayerState>(); Passed=Passed&&PS&&PS->Credits==777&&PS->Boarded==7;}
  FString Desired;
  if(FParse::Value(FCommandLine::Get(),TEXT("VTProbeSystem="),Desired) && Data) Passed=Passed && Data->FindSystem(FName(Desired))==Ship->SystemIndex;
  for(AVTShip* S:Ships) if(IsValid(S) && S!=Ship && S->SystemIndex!=Ship->SystemIndex) Passed=false;
 }
 if(ProbeRole.StartsWith(TEXT("Flow"))) {Passed=Ship&&ProbeMoved&&MaxPlayersObserved==2&&NPC>0; if(ProbeRole==TEXT("FlowHost")) {auto* Save=GetWorld()->GetGameInstance()->GetSubsystem<UVTSaveSubsystem>(); Passed=Passed&&Save->Save(); O->SetStringField(TEXT("world_id"),Save->WorldId.ToString());}}
 O->SetBoolField(TEXT("passed"),Passed);
 FString Json; FJsonSerializer::Serialize(O,TJsonWriterFactory<>::Create(&Json));
 FString RunDir; FParse::Value(FCommandLine::Get(),TEXT("VTProbeDir="),RunDir);
 if(RunDir.IsEmpty()) RunDir=FPaths::ProjectSavedDir()/TEXT("Validation/Multiplayer");
 FString ReportRole=ProbeRole; FParse::Value(FCommandLine::Get(),TEXT("VTReportRole="),ReportRole);
 IFileManager::Get().MakeDirectory(*RunDir,true);FFileHelper::SaveStringToFile(Json,*(RunDir/(ReportRole+TEXT(".json"))));
 UE_LOG(LogTemp,Display,TEXT("VT multiplayer probe %s %s"),*ProbeRole,Passed ? TEXT("passed") : TEXT("failed"));
 FPlatformMisc::RequestExitWithStatus(false,Passed ? 0 : 1);
#endif
}
