#include "VTGameplay.h"
#include "VTCombat.h"
#include "VTSaveSubsystem.h"
const FVTScenarioDefinition* UVTSimulation::ActiveScenario() const {
 auto* GI=Cast<UVTGameInstance>(GetWorld()->GetGameInstance()); if(!GI||GetWorld()->GetNetMode()!=NM_Standalone||GI->PlayMode==TEXT("sandbox")) return nullptr;
 FName Id(GI->PlayMode); return Data->Scenarios.FindByPredicate([Id](const FVTScenarioDefinition& D){return D.Id==Id;});
}
void UVTSimulation::ScenarioStep() {
 const auto* Scenario=ActiveScenario(); if(!Scenario) return;
 AVTShip* Player=nullptr; for(AVTShip* S:Ships) if(IsValid(S)&&!S->IsNPC) {Player=S; break;} if(!Player) return;
 auto* State=GetWorld()->GetGameState<AVTGameState>(); if(!State||!State->Outcome.IsEmpty()) return;
 if(!SoloSpawned) {SoloSpawned=true; for(const auto& D:Scenario->Enemies) {FVTMotion M; M.Position=D.Position; M.Heading=D.Heading; auto* Enemy=SpawnShip(D.ClassId,Player->SystemIndex,M,true,D.Faction); Enemy->Anchored=D.Anchored; Enemy->Invulnerable=D.Invulnerable; if(D.Inert&&Enemy->Controller) Enemy->Controller->Destroy();}}
 int Enemies=0; for(AVTShip* S:Ships) if(IsValid(S)&&S->IsNPC) ++Enemies; State->EnemiesRemaining=Enemies;
 if(!Scenario->Director||Enemies>0) return;
 if(State->Wave>=Scenario->MaxWaves) {State->Outcome=TEXT("Skirmish cleared"); auto* PS=Player->GetPlayerState<AVTPlayerState>(); if(auto* Save=GetWorld()->GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()) Save->RecordSolo(true,State->Wave,PS ? PS->Boarded : 0); return;}
 ++State->Wave; bool Finale=State->Wave==Scenario->MaxWaves&&!Scenario->FinaleClass.IsNone(); int Count=Finale ? Scenario->FinaleCount : Scenario->BaseCount+State->Wave-1;
 float Jitter=VT::LcgNext(DirectorSeed);
 for(int I=0;I<Count;++I) {float Angle=(float(I)/Count+Jitter)*2*PI; FVTMotion M; M.Position=FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*Scenario->Radius*0.85f; M.Heading=Angle+PI;
  auto* Enemy=SpawnShip(Finale ? Scenario->FinaleClass : Scenario->EnemyClass,Player->SystemIndex,M,true,Scenario->EnemyFaction);
  if(!Finale) {Enemy->Definition.Hull=Scenario->BaseHull+(State->Wave-1)*Scenario->HullPerWave; Enemy->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),Enemy->Definition.Hull);}}
 State->EnemiesRemaining=Count;
}
