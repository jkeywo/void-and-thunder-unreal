#include "VTNavigation.h"
#include "VTGameplay.h"
TArray<int32> VTNavigation::Route(const UVTGameData& D,int32 From,int32 To){
 if(!D.Systems.IsValidIndex(From)||!D.Systems.IsValidIndex(To))return {};
 TArray<int32> Parent;Parent.Init(INDEX_NONE,D.Systems.Num());TArray<int32> Queue;Queue.Add(From);Parent[From]=From;
 for(int32 Head=0;Head<Queue.Num()&&Parent[To]==INDEX_NONE;++Head)for(FName Link:D.Systems[Queue[Head]].Links){int32 N=D.FindSystem(Link);if(N!=INDEX_NONE&&Parent[N]==INDEX_NONE){Parent[N]=Queue[Head];Queue.Add(N);}}
 if(Parent[To]==INDEX_NONE)return {};TArray<int32> Result;for(int32 N=To;;N=Parent[N]){Result.Insert(N,0);if(N==From)break;}return Result;
}
FLinearColor VTNavigation::RelationshipColour(const AVTShip* Viewer,const AVTShip* Other,const UVTSimulation* Sim){
 if(!Viewer||!Other||!Sim)return FLinearColor(1,0.7f,0.1f);
 if(Viewer==Other)return FLinearColor(0.3f,0.8f,1);
 const auto* PS=Viewer->GetPlayerState<AVTPlayerState>();const auto S=Sim->Standings.Read(PS?PS->Profile:FGuid(),Other->Faction);
 const float Reputation=S.Found?S.Reputation:Sim->Data->StandingBetween(Viewer->Faction,Other->Faction);
 if(Reputation<Sim->Data->World.hostile_threshold||(S.Found&&S.Heat>=Sim->Data->World.heat_engage_threshold))return FLinearColor(1,0.15f,0.08f);
 if(Viewer->Faction==Other->Faction||Reputation>20)return FLinearColor(0.2f,0.95f,0.4f);
 return FLinearColor(1,0.7f,0.1f);
}
