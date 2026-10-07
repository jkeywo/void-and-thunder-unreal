#include "VTCues.h"
#include "VTGameplay.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Misc/App.h"
#include "TimerManager.h"
bool UVTShipCue::OnExecute_Implementation(AActor* Target,const FGameplayCueParameters& Parameters) const {
 if(IsRunningCommandlet()||!FApp::CanEverRender()) return false;
 if(!Target||Target->GetWorld()->GetNetMode()==NM_DedicatedServer) return false;
 FVector Position=Parameters.Location.IsNearlyZero() ? Target->GetActorLocation() : FVector(Parameters.Location);
 auto* Ship=Cast<AVTShip>(Target); auto* PC=Target->GetWorld()->GetFirstPlayerController(); auto* Viewer=PC ? Cast<AVTShip>(PC->GetPawn()) : nullptr;
 if(Viewer&&Ship&&(Viewer->SystemIndex!=Ship->SystemIndex||(Viewer->Movement->Motion.Position-Ship->Movement->Motion.Position).Size()>900)) return false;
 if(Sound) {float Distance=Viewer&&Ship ? float((Viewer->Movement->Motion.Position-Ship->Movement->Motion.Position).Size()) : 0; UGameplayStatics::PlaySoundAtLocation(Target,Sound,Position,FMath::Clamp(1-Distance/900,0.f,1.f),FMath::FRandRange(0.94f,1.06f));}
 if(Effect) if(auto* Burst=UNiagaraFunctionLibrary::SpawnSystemAtLocation(Target,Effect,Position,Target->GetActorRotation(),FVector(8),true,true,ENCPoolMethod::AutoRelease)) {
  TWeakObjectPtr<UNiagaraComponent> WeakBurst(Burst); FTimerHandle Timer; float Life=Ship ? Ship->GetWorld()->GetSubsystem<UVTSimulation>()->Data->Feel.impact.spark_life : 0.18f;
  Target->GetWorld()->GetTimerManager().SetTimer(Timer,[WeakBurst](){if(WeakBurst.IsValid()) WeakBurst->DeactivateImmediate();},Life,false);
 }
 if(Ship&&Viewer==Ship&&GameplayCueTag.GetTagName()==FName("GameplayCue.Ship.Hit")) if(auto* Captain=Cast<AVTController>(PC)) {const auto& Impact=Ship->GetWorld()->GetSubsystem<UVTSimulation>()->Data->Feel.impact; Captain->HitStop=Impact.own_hit_hitstop; if(auto* Camera=Cast<AVTCameraManager>(Captain->PlayerCameraManager)) Camera->AddImpactKick(Ship->GetActorLocation()-Position,Impact.own_hit_kick); Captain->CameraTrauma=FMath::Min(1.f,Captain->CameraTrauma+Impact.own_hit_trauma+Parameters.RawMagnitude*Impact.own_hit_trauma_per_damage); Captain->PlayDynamicForceFeedback(FMath::Clamp(Parameters.RawMagnitude*Impact.rumble_scale/100,0.f,1.f),Impact.rumble_hit_secs,true,true,true,true);}
 return true;
}
