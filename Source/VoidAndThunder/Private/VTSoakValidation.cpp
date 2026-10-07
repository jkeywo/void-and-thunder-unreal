#include "VTGameplay.h"
#include "VTSaveSubsystem.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMemory.h"
#include "Serialization/JsonSerializer.h"
void UVTSimulation::SoakTick(const FString& ProbeRole) {
#if !UE_BUILD_SHIPPING
 if(ProbeWrote)return;
 const bool Host=GetWorld()->GetNetMode()!=NM_Client;
 const double Age=GetWorld()->GetRealTimeSeconds();float Limit=7200;FParse::Value(FCommandLine::Get(),TEXT("VTProbeSeconds="),Limit);
 auto* State=GetWorld()->GetGameState<AVTGameState>();if(State)MaxPlayersObserved=FMath::Max(MaxPlayersObserved,State->PlayerArray.Num());
 auto* PC=Cast<AVTController>(GetWorld()->GetFirstPlayerController());auto* Ship=PC ? Cast<AVTShip>(PC->GetPawn()) : nullptr;
 if(Host)for(AVTShip* S:Ships)if(IsValid(S)&&!S->IsNPC)S->Invulnerable=true;
 if(Host) {
  if(SoakFirstTime==0&&Age>1) {int32 Population=500;FParse::Value(FCommandLine::Get(),TEXT("VTSoakPopulation="),Population);if(Population>=0)Bootstrap(FMath::Clamp(Population,0,2000),false);SoakFirstTime=SimulationTime;for(AVTShip* S:Ships)if(IsValid(S)&&S->IsNPC)++SoakInitialNPCs;}
  for(AVTShip* S:Ships)if(IsValid(S)) {if(!FMath::IsFinite(S->Movement->Motion.SimulationTime)||S->Movement->Motion.Position.ContainsNaN()||S->Movement->Motion.Velocity.ContainsNaN())SoakFailed=true; if(S->IsNPC&&S->Movement->Motion.SimulationTime>SoakFirstTime+10)SoakSystemsAdvanced.Add(S->SystemIndex);}
  if(SimulationTime>=SoakNextSave) {
   SoakNextSave+=300;auto* Save=GetWorld()->GetGameInstance()->GetSubsystem<UVTSaveSubsystem>();const auto World=Save->WorldId;const double Clock=SimulationTime;
   TMap<FGuid,FGuid> IDs;for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It)if(auto* S=Cast<AVTShip>(It->Get()->GetPawn()))if(auto* PS=S->GetPlayerState<AVTPlayerState>())IDs.Add(PS->Profile,S->PersistentId);
   bool Passed=Save->Save()&&Save->Load()&&Save->WorldId==World&&SimulationTime==Clock;
   for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It)if(auto* S=Cast<AVTShip>(It->Get()->GetPawn()))if(auto* PS=S->GetPlayerState<AVTPlayerState>())Passed&=IDs.Contains(PS->Profile)&&IDs[PS->Profile]==S->PersistentId;
   SoakFailed|=!Passed;++SoakSnapshots;UE_LOG(LogTemp,Display,TEXT("Campaign soak snapshot %d at %.2f: %s"),SoakSnapshots,SimulationTime,Passed?TEXT("passed"):TEXT("FAILED"));
  }
  if(Age>=SoakNextSample) {
   SoakNextSample+=60;auto Sample=MakeShared<FJsonObject>();Sample->SetNumberField(TEXT("wall_seconds"),Age);Sample->SetNumberField(TEXT("simulation_seconds"),SimulationTime);Sample->SetNumberField(TEXT("captains"),State ? State->PlayerArray.Num() : 0);Sample->SetNumberField(TEXT("ships"),Ships.Num());Sample->SetNumberField(TEXT("projectiles"),Projectiles.Num());Sample->SetNumberField(TEXT("used_physical_mb"),double(FPlatformMemory::GetStats().UsedPhysical)/1048576.);
   auto Times=StepMilliseconds;Times.Sort();Sample->SetNumberField(TEXT("step_p95_ms"),Times.IsEmpty()?0:Times[FMath::Min(Times.Num()-1,FMath::FloorToInt(Times.Num()*0.95))]);StepMilliseconds.Reset();FString JSON;FJsonSerializer::Serialize(Sample,TJsonWriterFactory<>::Create(&JSON));SoakSamples.Add(JSON);
  }
 }
 if(Age<Limit)return;ProbeWrote=true;
 auto O=MakeShared<FJsonObject>();bool Passed=!SoakFailed&&Ship&&MaxPlayersObserved==4;
 if(Host)Passed&=SoakSnapshots>=FMath::FloorToInt((Limit-10)/300)&&SoakSystemsAdvanced.Num()==Data->Systems.Num()&&SimulationTime>=Limit*0.95;
 O->SetBoolField(TEXT("passed"),Passed);O->SetNumberField(TEXT("wall_seconds"),Age);O->SetNumberField(TEXT("simulation_seconds"),Host?SimulationTime:State?State->SimulationTime:0);O->SetNumberField(TEXT("initial_npcs"),SoakInitialNPCs);O->SetNumberField(TEXT("save_load_cycles"),SoakSnapshots);O->SetNumberField(TEXT("systems_advanced"),SoakSystemsAdvanced.Num());O->SetNumberField(TEXT("max_captains"),MaxPlayersObserved);
 if(auto* PS=PC ? PC->GetPlayerState<AVTPlayerState>() : nullptr){O->SetStringField(TEXT("profile"),PS->Profile.ToString());O->SetStringField(TEXT("ship_id"),Ship?Ship->PersistentId.ToString():TEXT(""));}
 TArray<TSharedPtr<FJsonValue>> Samples;for(const auto& Text:SoakSamples){TSharedPtr<FJsonObject> Sample;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Sample);Samples.Add(MakeShared<FJsonValueObject>(Sample));}O->SetArrayField(TEXT("samples"),Samples);
 FString Dir,ReportRole=ProbeRole;FParse::Value(FCommandLine::Get(),TEXT("VTProbeDir="),Dir);FParse::Value(FCommandLine::Get(),TEXT("VTReportRole="),ReportRole);FString JSON;FJsonSerializer::Serialize(O,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(Dir/(ReportRole+TEXT(".json"))));FPlatformMisc::RequestExitWithStatus(false,Passed?0:1);
#endif
}
