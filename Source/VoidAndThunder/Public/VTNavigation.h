#pragma once
#include "CoreMinimal.h"
class UVTGameData;class AVTShip;class UVTSimulation;
namespace VTNavigation {
 VOIDANDTHUNDER_API TArray<int32> Route(const UVTGameData& Data,int32 From,int32 To);
 VOIDANDTHUNDER_API FLinearColor RelationshipColour(const AVTShip* Viewer,const AVTShip* Other,const UVTSimulation* Sim);
}
