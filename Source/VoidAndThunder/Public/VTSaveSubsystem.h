#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "Async/Future.h"
#include "GameFramework/SaveGame.h"
#include "VTTypes.h"
#include "VTSaveSubsystem.generated.h"
USTRUCT()
struct FVTSavedShip {
 GENERATED_BODY()
 UPROPERTY() FGuid Id;
 UPROPERTY() FName ClassId;
 UPROPERTY() FVTLoadoutSelection Fit;
 UPROPERTY() FName Faction;
 UPROPERTY() int32 System = 0;
 UPROPERTY() FVTMotion Motion;
 UPROPERTY() float Hull = 0;
 UPROPERTY() float Battery = 0;
 UPROPERTY() float EMPStress=0;
 UPROPERTY() FVTEquipmentState Equipment;
 UPROPERTY() bool NPC = false;
 UPROPERTY() bool Invulnerable = false;
 UPROPERTY() bool Anchored = false;
 UPROPERTY() int32 ShipRole=0;
 UPROPERTY() FVTBrainState Brain;
 UPROPERTY() float DockProgress=0;
 UPROPERTY() float JumpProgress=0;
 UPROPERTY() FName JumpDestination;
 UPROPERTY() FGuid BoardingTarget;
 UPROPERTY() float BoardingProgress = 0;
 UPROPERTY() bool Disabled = false;
 UPROPERTY() bool Docked = false;
 UPROPERTY() FVTShieldBanks Shields = FVTShieldBanks(0,0,0,0);
 UPROPERTY() FVTShieldBanks Suppression = FVTShieldBanks(0,0,0,0);
 UPROPERTY() float PortReload = 0;
 UPROPERTY() float StarboardReload = 0;
};
USTRUCT()
struct FVTSavedProjectile {
 GENERATED_BODY()
 UPROPERTY() FGuid Id;
 UPROPERTY() FGuid Source;
 UPROPERTY() int32 System = 0;
 UPROPERTY() FVector2D Position = FVector2D::ZeroVector;
 UPROPERTY() FVector2D Velocity = FVector2D::ZeroVector;
 UPROPERTY() EVTProjectileKind Kind=EVTProjectileKind::Cannon;
 UPROPERTY() FGuid Target;
 UPROPERTY() FGuid AttackerProfile;
 UPROPERTY() FName SourceFaction;
 UPROPERTY() bool SourceNPC=false;
 UPROPERTY() float Height=0;
 UPROPERTY() FVector Velocity3D=FVector::ZeroVector;
 UPROPERTY() float TurnRate=0;
 UPROPERTY() float ReportCountdown=0;
 UPROPERTY() float Damage = 0;
 UPROPERTY() float Remaining = 0;
 UPROPERTY() float Radius = 0;
};
USTRUCT()
struct FVTSavedPlayer {
 GENERATED_BODY()
 UPROPERTY() FGuid Profile;
 UPROPERTY() FGuid Token;
 UPROPERTY() FVTSavedShip Ship;
 UPROPERTY() int32 Credits = 0;
 UPROPERTY() int32 Boarded = 0;
 UPROPERTY() TArray<float> Reputation;
 UPROPERTY() TArray<float> Heat;
};
UCLASS()
class VOIDANDTHUNDER_API UVTWorldSave : public USaveGame {
 GENERATED_BODY()
public:
 UPROPERTY() int32 Version = 5;
 UPROPERTY() int32 Seed = 12345;
 UPROPERTY() FGuid WorldId;
 UPROPERTY() double SimulationTime = 0;
 UPROPERTY() TArray<FVTSavedShip> Ships;
 UPROPERTY() TArray<FVTSavedProjectile> Projectiles;
 UPROPERTY() TArray<FVTSavedPlayer> Players;
};
USTRUCT()
struct FVTReconnectToken {
 GENERATED_BODY()
 UPROPERTY() FGuid World;
 UPROPERTY() FGuid Token;
};
UCLASS()
class VOIDANDTHUNDER_API UVTPersonalSave : public USaveGame {
 GENERATED_BODY()
public:
 UPROPERTY() FGuid Profile;
 UPROPERTY() TArray<FVTReconnectToken> Tokens;
 UPROPERTY() FName PreferredHull=TEXT("corsair_cruiser");
 UPROPERTY() FVTLoadoutSelection PreferredFit;
};
UCLASS()
class VOIDANDTHUNDER_API UVTCareerSave : public USaveGame {
 GENERATED_BODY()
public:
 UPROPERTY() int32 Runs=0;
 UPROPERTY() int32 Victories=0;
 UPROPERTY() int32 DeepestWave=0;
 UPROPERTY() int32 ShipsBoarded=0;
};
UCLASS()
class VOIDANDTHUNDER_API UVTSaveSubsystem : public UGameInstanceSubsystem {
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Deinitialize() override;
 void WorldTearDown(UWorld* World);
 FDelegateHandle TearDownHandle;
 UPROPERTY() TObjectPtr<UVTPersonalSave> Personal;
 FString PersonalSlot=TEXT("Personal");
 FString CareerSlot=TEXT("Career");
 UPROPERTY() TObjectPtr<UVTCareerSave> Career;
 bool SoloRecorded=false;
 void RememberFit();
 void RecordSolo(bool Victory,int32 Wave,int32 Boarded);
 void StoreToken(FGuid World,FGuid Token);
 void ResetWorldIdentity();
 float SinceSave = 0;
 FGuid WorldId = FGuid::NewGuid();
 UPROPERTY() TArray<FVTSavedPlayer> PlayerRecords;
 FString Slot = TEXT("Campaign");
 FString CampaignPrefix;
 FVTSavedShip CaptureShip(class AVTShip* Ship) const;
 class AVTShip* RestoreShip(const FVTSavedShip& Record);
 void CapturePlayer(class AVTController* Controller);
 bool Migrate(UVTWorldSave* Snapshot) const;
 bool Validate(const UVTWorldSave* Snapshot) const;
 // Manual save returns committed success; autosave returns queued success.
 bool Save(bool Background=false);
 bool FlushPendingSave(bool Wait=true);
 TFuture<bool> PendingWrite;
 TArray<uint8> PendingBytes;
 TMap<FString,TArray<uint8>> LastGoodBytes;
 FString PendingPath;
 bool LastWriteSucceeded=true;
 bool Load();
 void Advance(float Dt);
};
