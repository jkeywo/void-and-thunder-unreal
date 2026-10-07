#pragma once
#include "CoreMinimal.h"
#include "VTTypes.generated.h"

USTRUCT(BlueprintType)
struct FVTShieldBanks {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float X = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float Y = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float Z = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float W = 0;
 FVTShieldBanks() = default;
 FVTShieldBanks(float Fore,float Aft,float Port,float Starboard):X(Fore),Y(Aft),Z(Port),W(Starboard) {}
 float& operator[](int32 I) {check(I>=0&&I<4); return I==0 ? X : I==1 ? Y : I==2 ? Z : W;}
 float operator[](int32 I) const {check(I>=0&&I<4); return I==0 ? X : I==1 ? Y : I==2 ? Z : W;}
};
USTRUCT(BlueprintType)
struct FVTShipStats {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float Thrust = 115;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float TurnRate = 0.6875f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxSpeed = 127.5f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float ForwardDrag = 0.9f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float LateralDrag = 4.5f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float TurnRateSlow = 1.55f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float TurnRateFast = 0.55f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float TurnAccel = 4;
};
USTRUCT(BlueprintType)
struct FVTPilotIntent {
 GENERATED_BODY()
 UPROPERTY(BlueprintReadWrite) float Throttle = 0;
 UPROPERTY(BlueprintReadWrite) float Turn = 0;
 UPROPERTY(BlueprintReadWrite) FVector2D Aim = FVector2D(0, 1);
 UPROPERTY() uint16 Buttons = 0;
 UPROPERTY() uint32 Sequence = 0;
};
UENUM(BlueprintType)
enum class EVTDevice : uint8 { Port, Starboard, EMP, Torpedo, Microwarp, Boost, Brace, Board, Mine, PointDefense };
namespace VTButtons {
 constexpr uint16 Port=1, Starboard=2, EMP=4, Torpedo=8, Warp=16, Boost=32, Brace=64, Interact=128, Mine=256, PointDefense=512;
}
USTRUCT(BlueprintType)
struct FVTMotion {
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) FVector2D Position = FVector2D::ZeroVector;
 UPROPERTY(BlueprintReadOnly) FVector2D Velocity = FVector2D::ZeroVector;
 UPROPERTY(BlueprintReadOnly) float Heading = 0;
 UPROPERTY(BlueprintReadOnly) float Omega = 0;
 UPROPERTY() uint32 Ack = 0;
 UPROPERTY() double SimulationTime = 0;
};
USTRUCT(BlueprintType)
struct FVTShipDefinition {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FVTShipStats Stats;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float Hull = 50;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float Radius = 26;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float Damage = 12;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float Reload = 10;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float MuzzleSpeed = 325;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float Arc = 1.1780972f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float ChargeTime = 0;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Guns = 3;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float BatteryMax = 3;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float BatteryRecharge = 0.6f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ShieldArcs = 2;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FVTShieldBanks ShieldMax = FVTShieldBanks(0,0,0,0);
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float ShieldRegen = 7;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float ShieldDelay = 2.5f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UStaticMesh> Mesh;
};
USTRUCT(BlueprintType)
struct FVTSystemDefinition {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Owner;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Security = 0;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float Danger = 0;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float Radius = 1400;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) bool HasStation = false;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> Links;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector2D ChartPosition = FVector2D::ZeroVector;
};
USTRUCT(BlueprintType)
struct FVTRules {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float RamDamagePerSpeed = 0.22f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float RamThreshold = 45;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float RamRestitution = 0.35f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float RamSeparation = 0.6f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float BoardRepairFraction = 0.1f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float HullLength = 40;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float MuzzleStandoff = 22;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float ReverseThrottle = 0.25f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float BoundsSpring = 3;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float BraceDamageFactor = 0.35f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float ProjectileTTL = 2.5f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float ProjectileRadius = 5;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float BoardRange = 95;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float BoardDwell = 3;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float CrippleThreshold = 0.25f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float JumpRange = 120;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float JumpDwell = 6;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float AutosaveSeconds = 60;
};
namespace VT {
 constexpr float Step = 1.0f / 64.0f;
 VOIDANDTHUNDER_API void HelmStep(FVTMotion& Motion, const FVTShipStats& Stats, const FVTPilotIntent& Intent, float Reverse, float Dt);
 VOIDANDTHUNDER_API float LcgNext(uint32& Seed);
 VOIDANDTHUNDER_API bool ValidIntent(const FVTPilotIntent& Intent);
 VOIDANDTHUNDER_API FVector ArenaOrigin(int32 System);
 VOIDANDTHUNDER_API FVector ToWorld(const FVector2D& Position, int32 System);
}
