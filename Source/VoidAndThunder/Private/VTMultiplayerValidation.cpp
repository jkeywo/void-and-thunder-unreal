#include "VTGameplay.h"
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
#include "Components/Button.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

void AVTController::ValidationInput(float Dt) {
#if !UE_BUILD_SHIPPING
 FString ProbeRole; if(!FParse::Value(FCommandLine::Get(),TEXT("VTProbe="),ProbeRole)) return;
 if(ProbeRole.StartsWith(TEXT("Soak"))) {LocalIntent=FVTPilotIntent();LocalIntent.Throttle=0.6f;LocalIntent.Turn=0.15f;return;}
 if(ProbeRole.StartsWith(TEXT("Gameplay"))) {for(TActorIterator<AVTGameplayProbe> It(GetWorld());It;++It){It->DriveLocal(this);break;}return;}
 if(ProbeRole.StartsWith(TEXT("Render"))) return;
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
  if(auto* Player=GetWorld()->GetFirstPlayerController()) if(auto* Pawn=Cast<AVTShip>(Player->GetPawn())) Pawn->Invulnerable=true;
  double Now=FPlatformTime::Seconds(), Age=GetWorld()->GetRealTimeSeconds();
  if(LastRenderFrame>0&&Age>20) RenderFrameMilliseconds.Add((Now-LastRenderFrame)*1000); LastRenderFrame=Now;
  if(ProbeRole==TEXT("Render")&&!ProbeScaled&&Age>1) {ConfigurePopulationFixture(500,FParse::Param(FCommandLine::Get(),TEXT("Busy")),FParse::Param(FCommandLine::Get(),TEXT("Armed"))); if(FParse::Param(FCommandLine::Get(),TEXT("Busy"))) if(auto* Player=GetWorld()->GetFirstPlayerController()) if(auto* Pawn=Cast<AVTShip>(Player->GetPawn())) {Pawn->SystemIndex=0; Pawn->Movement->Motion.Position=FVector2D(0,-500); Pawn->Movement->Previous=Pawn->Movement->Motion; Pawn->Movement->Authority=Pawn->Movement->Motion;} ProbeScaled=true;}
  if(ProbeRole==TEXT("RenderEnvironment")&&!ProbeScaled&&Age>1) {if(auto* Player=GetWorld()->GetFirstPlayerController())if(auto* Pawn=Cast<AVTShip>(Player->GetPawn())) {Pawn->SystemIndex=0;Pawn->Movement->Motion=FVTMotion();Pawn->Movement->Motion.Position=FVector2D(-600,0);Pawn->Movement->Previous=Pawn->Movement->Motion;Pawn->Movement->Authority=Pawn->Movement->Motion;Pawn->Anchored=true;}ProbeScaled=true;}
  if(ProbeRole==TEXT("RenderMenu")&&Age>22&&!UIProbeStarted){UIProbeStarted=true;if(auto* Player=Cast<AVTController>(GetWorld()->GetFirstPlayerController()))if(Player->UI)if(auto* Create=Player->UI->GetWidgetFromName(TEXT("Create"))){UIInitialFocus=Create->HasUserFocus(Player);FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Gamepad_DPad_Right,FModifierKeysState(),0,false,0,0));}}
  if(ProbeRole==TEXT("RenderMenu")&&Age>22.5&&!UIProbeFinished){UIProbeFinished=true;if(auto* Player=Cast<AVTController>(GetWorld()->GetFirstPlayerController()))if(Player->UI)if(auto* Continue=Player->UI->GetWidgetFromName(TEXT("Continue")))UINavigationPassed=UIInitialFocus&&Continue->HasUserFocus(Player);}
  if(!ScreenshotRequested&&Age>25) {ScreenshotRequested=true; FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Validation/")+ProbeRole+TEXT(".png"),true,false);}
  if(!ProbeWrote&&Age>35&&!RenderFrameMilliseconds.IsEmpty()) {
   ProbeWrote=true; auto Samples=RenderFrameMilliseconds; Samples.Sort(); double P95=Samples[FMath::FloorToInt(Samples.Num()*0.95)]; double Total=0; for(double Ms:Samples) Total+=Ms;
   FString Report=FString::Printf(TEXT("{\"frames\":%d,\"p95_frame_ms\":%.6f,\"mean_fps\":%.3f,\"width\":1920,\"height\":1080,\"population\":%d,\"gpu\":\"%s\",\"passed\":%s}"),Samples.Num(),P95,Samples.Num()*1000/Total,Ships.Num(),*GRHIAdapterName,P95<=1000./60 ? TEXT("true") : TEXT("false"));
   TSharedPtr<FJsonObject> Parsed; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Report),Parsed); if(GEngine&&GEngine->GameViewport&&GEngine->GameViewport->Viewport){auto Size=GEngine->GameViewport->Viewport->GetSizeXY();Parsed->SetNumberField(TEXT("width"),Size.X);Parsed->SetNumberField(TEXT("height"),Size.Y);}
   Parsed->SetBoolField(TEXT("initial_menu_focus"),UIInitialFocus);Parsed->SetBoolField(TEXT("controller_menu_navigation"),UINavigationPassed);const bool Passed=P95<=1000./60&&(ProbeRole!=TEXT("RenderMenu")||UINavigationPassed);Parsed->SetBoolField(TEXT("passed"),Passed);Parsed->SetNumberField(TEXT("simulation_time"),SimulationTime); Parsed->SetBoolField(TEXT("busy"),FParse::Param(FCommandLine::Get(),TEXT("Busy"))); Parsed->SetBoolField(TEXT("armed"),FParse::Param(FCommandLine::Get(),TEXT("Armed"))); auto Times=StepMilliseconds; if(!Times.IsEmpty()) {Times.Sort(); Parsed->SetNumberField(TEXT("p95_simulation_ms"),Times[FMath::FloorToInt(Times.Num()*0.95)]);} FJsonSerializer::Serialize(Parsed.ToSharedRef(),TJsonWriterFactory<>::Create(&Report)); FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("Validation/")+ProbeRole+TEXT(".json"))); FPlatformMisc::RequestExitWithStatus(false,Passed ? 0 : 1);
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
