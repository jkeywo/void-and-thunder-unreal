#include "VTGameplay.h"
#include "VTSaveSubsystem.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/World.h"

void AVTController::ValidationInput(float Dt) {
#if !UE_BUILD_SHIPPING
 FString ProbeRole; if(!FParse::Value(FCommandLine::Get(),TEXT("VTProbe="),ProbeRole)) return;
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
 auto* State=GetWorld()->GetGameState<AVTGameState>();
 if(State) MaxPlayersObserved=FMath::Max(MaxPlayersObserved,State->PlayerArray.Num());
 auto* PC=Cast<AVTController>(GetWorld()->GetFirstPlayerController());
 auto* Ship=PC ? Cast<AVTShip>(PC->GetPawn()) : nullptr;
 if(Ship) {
  if(!ProbeOriginSet){ProbeOrigin=Ship->Movement->Motion.Position;ProbeOriginSet=true;}
  if((Ship->Movement->Motion.Position-ProbeOrigin).Size()>25) ProbeMoved=true;
 }
 if(ProbeRole==TEXT("Host") && !ProbeScaled && GetWorld()->GetRealTimeSeconds()>1) { Bootstrap(500);ProbeScaled=true; }
 if(ProbeRole==TEXT("Host")&&!ProbeLoaded&&GetWorld()->GetRealTimeSeconds()>20) {
  ProbeLoaded=true; auto* Save=GetWorld()->GetGameInstance()->GetSubsystem<UVTSaveSubsystem>();
  if(!Save->Save()||!Save->Load()) {UE_LOG(LogTemp,Error,TEXT("Network snapshot round trip failed")); FPlatformMisc::RequestExitWithStatus(false,2);}
 }
 float Limit=ProbeRole==TEXT("Host") ? 45 : 25;
 FParse::Value(FCommandLine::Get(),TEXT("VTProbeSeconds="),Limit);
 if(ProbeRole==TEXT("Host")&&GetWorld()->GetRealTimeSeconds()<20) {
  for(AVTShip* S:Ships) if(IsValid(S)&&!S->IsNPC) if(auto* PS=S->GetPlayerState<AVTPlayerState>()) {PS->Credits=777; PS->Boarded=7;}
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
