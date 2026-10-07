#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "VTTypes.h"
#include "VTSaveSubsystem.generated.h"
USTRUCT()
struct FVTSavedShip {
 GENERATED_BODY()
 UPROPERTY() FGuid Id;
 UPROPERTY() FName ClassId;
 UPROPERTY() FName Faction;
 UPROPERTY() int32 System = 0;
 UPROPERTY() FVTMotion Motion;
 UPROPERTY() float Hull = 0;
 UPROPERTY() float Battery = 0;
 UPROPERTY() bool NPC = false;
};
UCLASS()
class VOIDANDTHUNDER_API UVTWorldSave : public USaveGame {
 GENERATED_BODY()
public:
 UPROPERTY() int32 Version = 1;
 UPROPERTY() double SimulationTime = 0;
 UPROPERTY() TArray<FVTSavedShip> Ships;
};
UCLASS()
class VOIDANDTHUNDER_API UVTSaveSubsystem : public UGameInstanceSubsystem {
 GENERATED_BODY()
public:
 float SinceSave = 0;
 bool Save();
 bool Load();
 void Advance(float Dt);
};
