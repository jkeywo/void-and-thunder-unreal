#pragma once
#include "CoreMinimal.h"

// Presentation/input state only: simulation continues to consume scalar intent.
struct FVTThrottleControl {
 int32 Notch=2;
 bool Analog=false;
 float Stick=0;
 static float ValueAt(int32 Index) {const float Values[]={-1.f,0.f,0.5f,1.f};return Values[FMath::Clamp(Index,0,3)];}
 float Value() const {return Analog?Stick:ValueAt(Notch);}
 void Step(int32 Delta) {Analog=false;Notch=FMath::Clamp(Notch+Delta,0,3);}
 void SetAnalog(float Value) {
  Analog=true;Stick=FMath::Clamp(Value,-1.f,1.f);float Best=MAX_flt;
  for(int32 I=0;I<4;++I) {float Gap=FMath::Abs(Stick-ValueAt(I));if(Gap<Best){Best=Gap;Notch=I;}}
 }
 const TCHAR* Label() const {const TCHAR* Labels[]={TEXT("REVERSE"),TEXT("HALT"),TEXT("HALF"),TEXT("FULL")};return Labels[Notch];}
};
