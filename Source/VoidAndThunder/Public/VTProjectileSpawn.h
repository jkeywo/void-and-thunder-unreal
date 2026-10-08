#pragma once
#include "VTTypes.h"
class AVTShip;class AVTProjectile;class UWorld;struct FVTSavedProjectile;
struct VOIDANDTHUNDER_API FVTProjectileSpawnSpec {
 EVTProjectileKind Kind=EVTProjectileKind::Cannon;int32 System=0;
 FGuid Id,SourceId,TargetId,Profile;FName Faction;bool SourceNPC=false;
 AVTShip* Source=nullptr;FVector2D Position=FVector2D::ZeroVector,Velocity=FVector2D::ZeroVector;
 FVector Velocity3D=FVector::ZeroVector;float Height=0,TurnRate=0,Damage=0,Remaining=0,Radius=5,ReportCountdown=0;
 static FVTProjectileSpawnSpec Fired(AVTShip* Source,EVTProjectileKind Kind,const FVector2D& Position,const FVector2D& Velocity,float Damage,float TTL,float Radius);
 static FVTProjectileSpawnSpec Restored(const FVTSavedProjectile& Record,const TMap<FGuid,AVTShip*>& Entities);
};
namespace VTProjectileSpawn { VOIDANDTHUNDER_API AVTProjectile* Create(UWorld* World,const FVTProjectileSpawnSpec& Spec); }
