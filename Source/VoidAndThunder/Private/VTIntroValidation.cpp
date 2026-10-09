#include "VTGameplay.h"
#include "VTIntro.h"
#include "VTSaveSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#if !UE_BUILD_SHIPPING
namespace {
bool SawWake=false,SawActive=false,SawMotion=false,SawFourArenas=false,SawIndependentSkip=false,Wrote=false;
int MaxCaptains=0;FVector2D FirstPosition;bool HavePosition=false;
}
void VTIntroValidationInput(AVTController* PC){
 PC->LocalIntent={};if(FParse::Param(FCommandLine::Get(),TEXT("VTIntroReconnect")))return;auto* S=Cast<AVTShip>(PC->GetPawn());if(!S)return;
 FString Role;FParse::Value(FCommandLine::Get(),TEXT("VTProbe="),Role);const double Age=PC->GetWorld()->GetRealTimeSeconds();
 if(Role==TEXT("IntroClient1"))return;
 if(Role==TEXT("IntroClient2")){
  auto* I=PC->Intro.Get();
  if(I->Progress.Stage==EVTIntroStage::BatteryChoice)I->ServerChooseRepair(0);
  if(I->Progress.Stage==EVTIntroStage::BatteryTrial){PC->LocalIntent.Throttle=0.5f;PC->LocalIntent.Buttons=VTButtons::Boost;}
  if(I->Progress.Stage==EVTIntroStage::SpecialChoice)I->ServerChooseRepair(1);
  if(I->Progress.Stage==EVTIntroStage::SpecialTrial){PC->LocalIntent.CursorOffset=FVector2D(80,0);if(I->Progress.Elapsed<0.7f)PC->LocalIntent.Buttons=VTButtons::Warp;}
 }
 if(Age>12&&PC->Intro->Progress.Stage==EVTIntroStage::Wake)PC->Intro->ServerAdvance();
 if(Age>14&&Age<18){PC->LocalIntent.Throttle=0.5f;PC->LocalIntent.Turn=0.5f;}
 if(Age>22&&Role==TEXT("IntroClient0")&&PC->Intro->Active())PC->Intro->ServerSkip();
}
void VTIntroValidationTick(UVTSimulation* Sim,const FString& Role){
 if(Wrote)return;auto* World=Sim->GetWorld();auto* PC=Cast<AVTController>(World->GetFirstPlayerController());auto* S=PC?Cast<AVTShip>(PC->GetPawn()):nullptr;if(!S)return;
 const double Age=World->GetRealTimeSeconds();const bool Reconnect=FParse::Param(FCommandLine::Get(),TEXT("VTIntroReconnect"));
 SawWake|=PC->Intro->Progress.Stage==EVTIntroStage::Wake;SawActive|=PC->Intro->Active();if(!HavePosition){HavePosition=true;FirstPosition=S->Movement->Motion.Position;}SawMotion|=(S->Movement->Motion.Position-FirstPosition).Size()>5;
 if(World->GetNetMode()!=NM_Client){TSet<int32> Private;int Captains=0,Complete=0;for(auto It=World->GetPlayerControllerIterator();It;++It)if(auto* C=Cast<AVTController>(It->Get()))if(auto* Ship=Cast<AVTShip>(C->GetPawn())){++Captains;if(C->Intro->Active()&&UVTIntroComponent::IsArena(Sim,Ship->SystemIndex))Private.Add(Ship->SystemIndex);else if(!C->Intro->Active())++Complete;}MaxCaptains=FMath::Max(MaxCaptains,Captains);SawFourArenas|=Private.Num()==4;SawIndependentSkip|=Captains==4&&Private.Num()==3&&Complete==1;}
 // After network helm validation, place remote captains at the repair checkpoint.
 // The choices and actual-use trials still travel through normal client intent/RPCs.
 if(World->GetNetMode()!=NM_Client&&Age>30)for(auto It=World->GetPlayerControllerIterator();It;++It)if(auto* C=Cast<AVTController>(It->Get()))if(!C->IsLocalController()&&C->Intro->Progress.Stage==EVTIntroStage::Helm)C->Intro->Change(EVTIntroStage::BatteryChoice);
 if(Age<(Role==TEXT("IntroHost")?85:Reconnect?8:40))return;
 bool Scoped=true;if(World->GetNetMode()==NM_Client)for(const AVTShip* Other:Sim->Ships)if(IsValid(Other)&&Other->SystemIndex!=S->SystemIndex)Scoped=false;
 bool Passed=Scoped&&SawActive&&(Reconnect?PC->Intro->Active()&&PC->Intro->Progress.Stage!=EVTIntroStage::Wake:SawWake&&(Role==TEXT("IntroClient1")||SawMotion));
 bool Saved=true;if(Role==TEXT("IntroHost")){Saved=World->GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->Save();Passed&=MaxCaptains==4&&SawFourArenas&&SawIndependentSkip&&Saved;}
 if(Role==TEXT("IntroClient0"))Passed&=!PC->Intro->Active()&&!UVTIntroComponent::IsArena(Sim,S->SystemIndex);
 const bool RepairsRetained=PC->Intro->Progress.BatteryChoice==FName("loadout.boost")&&PC->Intro->Progress.SpecialChoice==FName("loadout.microwarp")&&S->Definition.Equipment.Boost&&S->Definition.Equipment.Warp&&!S->Definition.Equipment.EMP&&!S->Definition.Equipment.Torpedoes;
 if(Role==TEXT("IntroClient2"))Passed&=RepairsRetained&&PC->Intro->Progress.Stage==EVTIntroStage::Challenge;
 auto J=MakeShared<FJsonObject>();J->SetBoolField(TEXT("repair_choices_and_fit_retained"),RepairsRetained);J->SetBoolField(TEXT("passed"),Passed);J->SetBoolField(TEXT("owner_only_arena_relevancy"),Scoped);J->SetBoolField(TEXT("wake_seen"),SawWake);J->SetBoolField(TEXT("motion_seen"),SawMotion);J->SetBoolField(TEXT("four_private_arenas"),SawFourArenas);J->SetBoolField(TEXT("independent_skip"),SawIndependentSkip);J->SetBoolField(TEXT("saved"),Saved);J->SetBoolField(TEXT("reconnect"),Reconnect);J->SetNumberField(TEXT("stage"),int(PC->Intro->Progress.Stage));J->SetNumberField(TEXT("system"),S->SystemIndex);J->SetNumberField(TEXT("simulation_time"),Sim->SimulationTime);J->SetNumberField(TEXT("captains"),MaxCaptains);
 FString Text;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Text));FString Directory;FParse::Value(FCommandLine::Get(),TEXT("VTProbeDir="),Directory);FFileHelper::SaveStringToFile(Text,*(Directory/(Role+(Reconnect?TEXT("-Reconnect"):TEXT(""))+TEXT(".json"))));Wrote=true;FPlatformMisc::RequestExitWithStatus(false,Passed?0:1);
}
#endif
