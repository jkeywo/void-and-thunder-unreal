#include "VTGameplayProbe.h"
#include "VTGameplay.h"
#include "VTCombat.h"
#include "VTSaveSubsystem.h"
#include "VTUI.h"
#include "Net/UnrealNetwork.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

AVTGameplayProbe::AVTGameplayProbe() {bReplicates=true; bAlwaysRelevant=true; SetNetUpdateFrequency(20);}
void AVTGameplayProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 DOREPLIFETIME(AVTGameplayProbe,Phase); DOREPLIFETIME(AVTGameplayProbe,Profiles); DOREPLIFETIME(AVTGameplayProbe,StationSystem); DOREPLIFETIME(AVTGameplayProbe,DestinationSystem); DOREPLIFETIME(AVTGameplayProbe,Failure);
}
void AVTGameplayProbe::Enter(int32 Next) {Phase=Next;PhaseStarted=GetWorld()->GetRealTimeSeconds();ForceNetUpdate();UE_LOG(LogTemp,Display,TEXT("Gameplay acceptance phase %d"),Phase);}
void AVTGameplayProbe::Place(AVTShip* Ship,int32 System,const FVector2D& Position,float Heading) {
 Ship->SystemIndex=System; Ship->Movement->Motion.Position=Position;Ship->Movement->Motion.Velocity=FVector2D::ZeroVector;Ship->Movement->Motion.Heading=Heading;Ship->Movement->Motion.Omega=0;
 Ship->Movement->Previous=Ship->Movement->Motion;Ship->Movement->Authority=Ship->Movement->Motion;Ship->Docked=false;Ship->DockProgress=0;Ship->Intent=FVTPilotIntent();Ship->InputQueue.Reset();Ship->ForceNetUpdate();
}
void AVTGameplayProbe::ServerStep() {
#if !UE_BUILD_SHIPPING
 if(!HasAuthority()||Reported) return;
 const double Now=GetWorld()->GetRealTimeSeconds(); if(Started==0) Started=Now;
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();
 auto Fail=[&](const FString& Reason){Failure=Reason;Checks.Add(Reason,false);Enter(11);FinishAt=Now+5;};
 if(Phase<11&&Now-Started>180) {Fail(FString::Printf(TEXT("phase_%d_timeout"),Phase));return;}
 if(Phase==11) {if(Now>=FinishAt&&FinishAt>0) ReportLocal(Captains.IsEmpty()?nullptr:Captains[0].Get());return;}
 if(Phase==0) {
  TArray<AVTController*> Players;
  for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It) if(auto* PC=Cast<AVTController>(It->Get())) if(auto* PS=PC->GetPlayerState<AVTPlayerState>()) if(PS->Profile.IsValid()&&PC->GetPawn()) Players.Add(PC);
  if(Players.Num()!=4) return;
  Players.Sort([](const AVTController& A,const AVTController& B){return A.PlayerState->GetPlayerId()<B.PlayerState->GetPlayerId();});
  for(auto* PC:Players) {Captains.Add(PC);auto* PS=PC->GetPlayerState<AVTPlayerState>();Profiles.Add(PS->Profile);PS->Credits=1000;PS->Boarded=0;}
  for(int I=0;I<Sim->Data->Systems.Num();++I) if(Sim->Data->Systems[I].HasStation&&!Sim->Data->Systems[I].Links.IsEmpty()&&Sim->Data->FactionIndex(Sim->Data->Systems[I].Owner)>=0) {StationSystem=I;DestinationSystem=Sim->Data->FindSystem(Sim->Data->Systems[I].Links[0]);break;}
  if(StationSystem<0) {Fail(TEXT("no_station_fixture"));return;}
  for(int I=0;I<4;++I) {auto* S=CastChecked<AVTShip>(Players[I]->GetPawn());Place(S,0,FVector2D(-600-160*I,-400));S->Invulnerable=true;S->Faction=FName("Vethara");}
  auto* Victim=CastChecked<AVTShip>(Players[1]->GetPawn());Place(Victim,0,FVector2D(-600,-320));Victim->Invulnerable=false;Victim->Combat->Shields=FVTShieldBanks(0,0,0,0);Victim->Definition.ShieldRegen=0;
  InitialHull=Victim->Attributes->Hull.GetCurrentValue();OldVictim=Victim->PersistentId;Enter(1);return;
 }
 if(Captains.Num()!=4) return;
 AVTShip* S[4];AVTPlayerState* PS[4];for(int I=0;I<4;++I) {if(!Captains[I].IsValid()) {Fail(TEXT("unexpected_disconnect"));return;}S[I]=Cast<AVTShip>(Captains[I]->GetPawn());PS[I]=Captains[I]->GetPlayerState<AVTPlayerState>();if(!S[I]||!PS[I])return;}
 const double Age=Now-PhaseStarted;
 if(Phase==1&&Age>2&&S[1]->Attributes->Hull.GetCurrentValue()<InitialHull) {
  Checks.Add(TEXT("same_faction_pvp_damage"),true);Checks.Add(TEXT("remote_invalid_station_refit_rejected"),S[2]->ClassId==FName("corsair_cruiser")&&PS[2]->Credits==1000&&!S[2]->Docked);Checks.Add(TEXT("pvp_profile_attribution"),S[1]->Brain.LastAttackerProfile==PS[0]->Profile);
  int Faction=Sim->Data->FactionIndex(S[1]->Faction);Checks.Add(TEXT("personal_crime_attribution"),PS[0]->Heat.IsValidIndex(Faction)&&PS[0]->Heat[Faction]>0&&PS[2]->Heat[Faction]==0);
  S[1]->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),1);S[1]->Disabled=true;S[1]->Movement->Motion.Velocity=FVector2D::ZeroVector;
  Place(S[0],0,FVector2D(-600,-400));Place(S[2],0,FVector2D(-680,-320));InitialPrizes=PS[0]->Boarded+PS[2]->Boarded;InitialCredits=PS[0]->Credits+PS[2]->Credits;Enter(2);
 } else if(Phase==2&&Age>2&&S[1]->PersistentId!=OldVictim) {
  Checks.Add(TEXT("simultaneous_boarding_single_prize"),PS[0]->Boarded+PS[2]->Boarded==InitialPrizes+1);
  Checks.Add(TEXT("simultaneous_boarding_single_bounty"),PS[0]->Credits+PS[2]->Credits==InitialCredits+Sim->Data->Rules.BoardingBounty);
  Checks.Add(TEXT("boarded_captain_free_recovery"),PS[1]->Credits==1000&&S[1]->Attributes->Hull.GetCurrentValue()==S[1]->Definition.Hull);
  for(int I=0;I<4;++I) Place(S[I],0,FVector2D(-1100-160*I,-600));
  Place(S[1],0,FVector2D(-600,-400));Place(S[3],0,FVector2D(-600,-320));S[3]->Invulnerable=false;S[3]->Combat->Shields=FVTShieldBanks(0,0,0,0);S[3]->Definition.ShieldRegen=0;S[3]->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),1);OldVictim=S[3]->PersistentId;Enter(3);
 } else if(Phase==3&&Age>2&&S[3]->PersistentId!=OldVictim) {
  Checks.Add(TEXT("network_weapon_death_recovery"),S[3]->Attributes->Hull.GetCurrentValue()==S[3]->Definition.Hull&&PS[3]->Credits==1000&&Sim->Data->Systems[S[3]->SystemIndex].HasStation);
  Place(S[1],StationSystem,Sim->Data->Rules.StationPosition+FVector2D(0,-Sim->Data->Rules.StationRadius-40));S[1]->Invulnerable=true;S[1]->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),S[1]->Definition.Hull*0.5f);
  int OwnerIndex=Sim->Data->FactionIndex(Sim->Data->Systems[StationSystem].Owner);if(PS[1]->Heat.IsValidIndex(OwnerIndex)) {PS[1]->Heat[OwnerIndex]=20;PS[1]->Reputation[OwnerIndex]=0;}InitialCredits=PS[1]->Credits;Enter(4);
 } else if(Phase==4&&Age>2&&S[1]->Docked) {Checks.Add(TEXT("network_station_dwell"),true);Enter(5);}
 else if(Phase==5&&Age>2&&S[1]->Attributes->Hull.GetCurrentValue()==S[1]->Definition.Hull) {
  int OwnerIndex=Sim->Data->FactionIndex(Sim->Data->Systems[StationSystem].Owner);
  if(!PS[1]->Heat.IsValidIndex(OwnerIndex)||PS[1]->Heat[OwnerIndex]>0||PS[1]->Credits>=InitialCredits)return;
  Checks.Add(TEXT("network_station_repair_heat_purchase"),PS[1]->Credits>=0);RefitVictim=S[1]->PersistentId;Enter(6);
 } else if(Phase==6&&Age>2&&S[1]->PersistentId!=RefitVictim&&S[1]->ClassId==FName("corsair_frigate")) {Checks.Add(TEXT("network_station_refit"),S[1]->Docked);Enter(7);}
 else if(Phase==7&&Age>2&&!S[1]->Docked) {
  Checks.Add(TEXT("network_station_undock"),true);Place(S[1],StationSystem,Sim->JumpPosition(StationSystem,Sim->Data->Systems[DestinationSystem].Id));OtherSystems.Reset();for(int I=0;I<4;++I) OtherSystems.Add(S[I]->SystemIndex);Enter(8);
 } else if(Phase==8&&S[1]->SystemIndex==DestinationSystem) {
  Checks.Add(TEXT("normal_charged_independent_jump"),Age+VT::Step>=Sim->Data->Rules.JumpDwell&&S[0]->SystemIndex==OtherSystems[0]&&S[2]->SystemIndex==OtherSystems[2]&&S[3]->SystemIndex==OtherSystems[3]);MovementOrigins.Reset();for(int I=0;I<4;++I){Place(S[I],S[I]->SystemIndex,FVector2D(-900-160*I,-900));MovementOrigins.Add(S[I]->Movement->Motion.Position);}Enter(9);
 } else if(Phase==9&&Age>30) {
  bool AllMoved=true;for(int I=0;I<4;++I) AllMoved&=(S[I]->Movement->Motion.Position-MovementOrigins[I]).Size()>100&&S[I]->Movement->Authority.Ack>0;Checks.Add(TEXT("all_captains_moved_after_loss_burst"),AllMoved);auto* Save=GetGameInstance()->GetSubsystem<UVTSaveSubsystem>();Checks.Add(TEXT("post_gameplay_snapshot"),Save->Save());Place(S[1],S[1]->SystemIndex,FVector2D(-900,-900));FVTMotion TargetMotion;TargetMotion.Position=FVector2D(-900,-700);auto* Target=Sim->SpawnShip("house_patrol",S[1]->SystemIndex,TargetMotion,true,"Freebooters");Target->Invulnerable=true;Target->Anchored=true;S[1]->PortReload=0;S[1]->StarboardReload=0;Enter(10);
 }
 else if(Phase==10&&Age>3&&S[1]->Autopilot&&S[1]->PortReload>0) {Checks.Add(TEXT("network_ai_pilot_keeps_captain_and_operates_guns"),S[1]->Controller==Captains[1].Get()&&S[1]->GetPlayerState<AVTPlayerState>()==PS[1]);NetworkPilotAck=S[1]->Movement->Authority.Ack;Enter(12);}
 else if(Phase==12&&Age>2&&!S[1]->Autopilot&&S[1]->Movement->Authority.Ack>NetworkPilotAck+32) {Checks.Add(TEXT("network_ai_pilot_returns_to_manual"),S[1]->Controller==Captains[1].Get());Enter(11);FinishAt=Now+5;}
 if(Phase==11&&Now>=FinishAt&&FinishAt>0) ReportLocal(Captains[0].Get());
#endif
}
void AVTGameplayProbe::DriveLocal(AVTController* PC) {
#if !UE_BUILD_SHIPPING
 if(!PC||Reported) return;
 auto* PS=PC->GetPlayerState<AVTPlayerState>();auto* Ship=Cast<AVTShip>(PC->GetPawn());if(!PS||!Ship)return;
 const int Index=Profiles.Find(PS->Profile); if(Index<0)return;
 if(Phase!=LastLocalPhase) {LastLocalPhase=Phase;SentAction=false;SeenPhases|=1<<Phase;}
 PC->LocalIntent=FVTPilotIntent();
 if((Phase==1&&Index==0)||(Phase==3&&Index==1)) {PC->LocalIntent.Buttons=VTButtons::Port;PC->LocalIntent.Aim=FVector2D(0,1);PC->LocalIntent.CursorOffset=FVector2D(0,80);}
 if(Phase==2&&(Index==0||Index==2)) PC->LocalIntent.Buttons=VTButtons::Interact;
 if(Phase==1&&Index==2&&!SentAction) {PC->ServerStationAction(FName("pay_heat"));PC->ServerRefit(FName("house_patrol"),FVTLoadoutSelection());PC->ServerRecover();SentAction=true;}
 if(!SentAction&&Index==1) {
  if(Phase==5) {PC->ServerStationAction(FName("repair"));PC->ServerStationAction(FName("pay_heat"));SentAction=true;}
  if(Phase==6) {PC->ServerRefit(FName("corsair_frigate"),FVTLoadoutSelection());SentAction=true;}
  if(Phase==10) {PC->ServerSetAutopilot(true);SentAction=true;}
  if(Phase==12) {PC->ServerSetAutopilot(false);SentAction=true;}
  if(Phase==7) {PC->ServerStationAction(FName("undock"));SentAction=true;}
 }
 if(Phase>=8&&PC->UI&&PC->UI->MenuOpen) PC->UI->SetMenu(false);
 if(Phase==8&&Index==1) PC->LocalIntent.Buttons=VTButtons::Interact;
 if(Phase==9) {
  Ship->Movement->MeasureCorrections=true;PC->LocalIntent.Throttle=0.7f;PC->LocalIntent.Turn=0.15f;
  // A bounded five-second loss burst exercises recovery after disruption.
  if(!BurstStarted) {BurstStarted=true;PhaseStarted=GetWorld()->GetRealTimeSeconds();LocalMovementOrigin=Ship->Movement->Motion.Position;}
  double Age=GetWorld()->GetRealTimeSeconds()-PhaseStarted;
  if(Age>10&&!SentAction) {PC->ConsoleCommand(FParse::Param(FCommandLine::Get(),TEXT("VTBlackout"))?TEXT("Net PktLoss=100"):TEXT("Net PktLoss=50"),false);SentAction=true;}
  if(Age>15&&!BurstEnded) {int Loss=0;FParse::Value(FCommandLine::Get(),TEXT("PktLoss="),Loss);PC->ConsoleCommand(FString::Printf(TEXT("Net PktLoss=%d"),Loss),false);BurstEnded=true;BurstEndAck=Ship->Movement->Authority.Ack;}
 }
 if(Phase==11&&!HasAuthority()) {if(FinishAt==0)FinishAt=GetWorld()->GetRealTimeSeconds()+2;if(GetWorld()->GetRealTimeSeconds()>=FinishAt)ReportLocal(PC);}
#endif
}
void AVTGameplayProbe::ReportLocal(AVTController* PC) {
#if !UE_BUILD_SHIPPING
 if(Reported)return;Reported=true;
 auto* Ship=PC ? Cast<AVTShip>(PC->GetPawn()) : nullptr;
 auto O=MakeShared<FJsonObject>();bool Passed=Failure.IsEmpty();
 if(HasAuthority()) {auto C=MakeShared<FJsonObject>();for(const auto& Check:Checks){C->SetBoolField(Check.Key,Check.Value);Passed&=Check.Value;}O->SetObjectField(TEXT("checks"),C);Passed&=Checks.Num()>=12;}
 else {Passed&=Ship&&BurstEnded&&Ship->Movement->Authority.Ack>BurstEndAck+300&&(Ship->Movement->Motion.Position-LocalMovementOrigin).Size()>100;O->SetNumberField(TEXT("observed_phases"),SeenPhases);}
 O->SetStringField(TEXT("failure"),Failure);O->SetBoolField(TEXT("passed"),Passed);
 if(Ship) {auto Values=Ship->Movement->CorrectionDistances;Values.Sort();O->SetNumberField(TEXT("reconciliation_samples"),Values.Num());O->SetNumberField(TEXT("correction_p95_units"),Values.IsEmpty()?0:Values[FMath::Min(Values.Num()-1,FMath::FloorToInt(Values.Num()*0.95))]);O->SetNumberField(TEXT("correction_max_units"),Values.IsEmpty()?0:Values.Last());O->SetNumberField(TEXT("max_pending_inputs"),Ship->Movement->MaxPendingObserved);O->SetNumberField(TEXT("final_ack"),Ship->Movement->Authority.Ack);}
 FString ProbeRole,Dir;FParse::Value(FCommandLine::Get(),TEXT("VTProbe="),ProbeRole);FParse::Value(FCommandLine::Get(),TEXT("VTProbeDir="),Dir);FString JSON;FJsonSerializer::Serialize(O,TJsonWriterFactory<>::Create(&JSON));FFileHelper::SaveStringToFile(JSON,*(Dir/(ProbeRole+TEXT(".json"))));FPlatformMisc::RequestExitWithStatus(false,Passed?0:1);
#endif
}
