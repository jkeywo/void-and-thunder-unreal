#include "VTProjectileSpawn.h"
#include "VTGameplay.h"
#include "VTCombat.h"
#include "VTSaveSubsystem.h"
FVTProjectileSpawnSpec FVTProjectileSpawnSpec::Fired(AVTShip* S,EVTProjectileKind K,const FVector2D& P,const FVector2D& V,float D,float TTL,float R){FVTProjectileSpawnSpec X;X.Kind=K;X.System=S->SystemIndex;X.Id=FGuid::NewGuid();X.Source=S;X.SourceId=S->PersistentId;X.Faction=S->Faction;X.SourceNPC=S->IsNPC;if(auto* PS=S->GetPlayerState<AVTPlayerState>())X.Profile=PS->Profile;X.Position=P;X.Velocity=V;X.Damage=D;X.Remaining=TTL;X.Radius=R;return X;}
FVTProjectileSpawnSpec FVTProjectileSpawnSpec::Restored(const FVTSavedProjectile& R,const TMap<FGuid,AVTShip*>& Entities){FVTProjectileSpawnSpec X;X.Id=R.Id;X.SourceId=R.Source;X.Source=Entities.FindRef(R.Source);X.Kind=R.Kind;X.System=R.System;X.TargetId=R.Target;X.Profile=R.AttackerProfile;X.Faction=R.SourceFaction;X.SourceNPC=R.SourceNPC;X.Position=R.Position;X.Velocity=R.Velocity;X.Velocity3D=R.Velocity3D;X.Height=R.Height;X.TurnRate=R.TurnRate;X.Damage=R.Damage;X.Remaining=R.Remaining;X.Radius=R.Radius;X.ReportCountdown=R.ReportCountdown;return X;}
AVTProjectile* VTProjectileSpawn::Create(UWorld* World,const FVTProjectileSpawnSpec& X){
 if(!World||World->GetNetMode()==NM_Client)return nullptr;
 const FTransform Transform(VT::ToWorld(X.Position,X.System)+FVector(0,0,X.Height*100));auto* P=World->SpawnActorDeferred<AVTProjectile>(AVTProjectile::StaticClass(),Transform);if(!P)return nullptr;
 P->Kind=X.Kind;P->PersistentId=X.Id;P->SystemIndex=X.System;P->Source=X.Source;P->SourceId=X.SourceId;P->SourceFaction=X.Faction;P->SourceNPC=X.SourceNPC;P->AttackerProfile=X.Profile;P->TargetId=X.TargetId;P->Position=P->Previous=X.Position;P->Velocity=X.Velocity;P->Height=X.Height;P->Velocity3D=X.Velocity3D;P->TurnRate=X.TurnRate;P->Damage=X.Damage;P->Remaining=X.Remaining;P->Radius=X.Radius;P->ReportCountdown=X.ReportCountdown;P->FinishSpawning(Transform);return P;
}
