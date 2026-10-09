#pragma once
#include "GameplayCueNotify_Static.h"
#include "VTCues.generated.h"
UCLASS(Blueprintable)
class VOIDANDTHUNDER_API UVTShipCue : public UGameplayCueNotify_Static {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TObjectPtr<class USoundBase> Sound;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TObjectPtr<class USoundAttenuation> Attenuation;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TObjectPtr<class USoundConcurrency> Concurrency;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TObjectPtr<class UNiagaraSystem> Effect;
 virtual bool OnExecute_Implementation(AActor* Target,const FGameplayCueParameters& Parameters) const override;
};
