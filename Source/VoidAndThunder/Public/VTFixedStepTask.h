#pragma once
#include "Abilities/Tasks/AbilityTask.h"
#include "VTFixedStepTask.generated.h"
UCLASS() class VOIDANDTHUNDER_API UVTFixedStepTask : public UAbilityTask {
 GENERATED_BODY()
public:
 static UVTFixedStepTask* Start(UGameplayAbility* Owner,float Seconds);
 virtual void Activate() override {}
 bool Advance(float Step);
 float GetRemaining() const {return Remaining;}
 void Restore(float Seconds) {Remaining=FMath::Max(0.f,Seconds);}
private:
 float Remaining=0;
};
