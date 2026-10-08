#include "VTFixedStepTask.h"
UVTFixedStepTask* UVTFixedStepTask::Start(UGameplayAbility* Owner,float Seconds) {
 auto* Task=NewAbilityTask<UVTFixedStepTask>(Owner); Task->Restore(Seconds); Task->ReadyForActivation(); return Task;
}
bool UVTFixedStepTask::Advance(float Step) {Remaining=FMath::Max(0.f,Remaining-Step); return Remaining<=0;}
