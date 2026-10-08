#pragma once
#include "CoreMinimal.h"
class UVTSimulation;class AVTShip;
struct FVTStanding {float Reputation=0,Heat=0;bool Found=false;};
class VOIDANDTHUNDER_API FVTCaptainStandings {
 UVTSimulation* Owner=nullptr;
 struct FAdapter {TArray<float>* Reputation=nullptr;TArray<float>* Heat=nullptr;explicit operator bool() const{return Reputation&&Heat;}};
 FAdapter Resolve(FGuid Profile) const;
public:
 void Initialize(UVTSimulation* Sim){Owner=Sim;}
 FVTStanding Read(FGuid Profile,FName Faction) const;
 void Crime(FGuid Profile,const AVTShip* Victim,float PreShieldDamage);
 void Rescue(FGuid Profile,FName Faction);
 void Decay(float Dt);
 int32 HeatPaymentCost(FGuid Profile,FName Faction) const;
 bool PayHeat(FGuid Profile,FName Faction,int32& Credits);
};
