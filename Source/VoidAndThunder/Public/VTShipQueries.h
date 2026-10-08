#pragma once
#include "CoreMinimal.h"
class AVTShip;class UVTSimulation;
class VOIDANDTHUNDER_API FVTShipQueries {
 UVTSimulation* Owner=nullptr;mutable bool Dirty=true;
 mutable TArray<TArray<AVTShip*>> Systems;mutable TArray<TMap<FGuid,AVTShip*>> Ids;
 TArray<TMap<FIntPoint,TArray<AVTShip*>>> Cells;TArray<TArray<AVTShip*>> Prizes;
 float LargestRadius=0;bool SpatialReady=false,BoardingReady=false;
 void EnsureMembership() const;
public:
 void Initialize(UVTSimulation* Sim){Owner=Sim;Dirty=true;}
 void Invalidate(){Dirty=true;}
 void BeginStep();void BuildSpatial();void BuildBoarding();
 int32 SystemCount() const;
 const TArray<AVTShip*>& Ordered(int32 System) const;
 AVTShip* Find(int32 System,FGuid Id) const;
 const TArray<AVTShip*>& Boarding(int32 System) const;
 void SweptCandidates(int32 System,const FVector2D& Before,const FVector2D& After,float Radius,TArray<AVTShip*>& Out) const;
};
