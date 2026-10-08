#include "VTIntro.h"
#include "VTFitEditor.h"
#include "VTGameplay.h"
#include "VTCombat.h"
#include "VTSaveSubsystem.h"
#include "VTUI.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
namespace {void ResetIntroMotion(AVTShip* S){FVTSavedShip R;R.Motion=S->Movement->Motion;S->GatePassage->Restore(R,S->Movement->Motion.SimulationTime);}}
UVTIntroData::UVTIntroData(){
 auto Add=[this](EVTIntroStage S,bool Enemy,const TCHAR* Key,const TCHAR* Speech,const TCHAR* Objective){FVTIntroBeat B;B.Stage=S;B.Enemy=Enemy;B.Speech=FText::FromString(Speech);B.Objective=FText::FromString(Objective);
#if WITH_EDITORONLY_DATA
 B.Speech=FText::ChangeKey(TEXT("VTIntro"),Key,B.Speech);B.Objective=FText::ChangeKey(TEXT("VTIntro"),FString(Key)+TEXT("Objective"),B.Objective);
#endif
Beats.Add(B);};
 Add(EVTIntroStage::Wake,false,TEXT("Wake"),TEXT("You're awake. Good. The captain didn't make it. The bridge is gone and most systems are dead. I've routed the helm to your station. We're still here. You have command."),TEXT("Take command. Engineering is restoring propulsion."));
 Add(EVTIntroStage::Helm,false,TEXT("Helm"),TEXT("Engines answering. Keep us clear of the wreckage. Change speed in steps, turn into the gap, then bring us to a halt at the marker. I'll work on the guns."),TEXT("Escape the wreck field: move, turn, and halt at CLEAR SPACE."));
 Add(EVTIntroStage::Debris,false,TEXT("Debris"),TEXT("Broadside power restored. That broken hull is blocking our route. Turn a side toward it, hold to aim, then release to fire. Mind which way the targeting lines point."),TEXT("Destroy the marked wreckage with your broadside."));
 Add(EVTIntroStage::Repairs,false,TEXT("Repairs"),TEXT("Path clear. Shield circuits are holding and the hull's patched. The optional systems are wrecked. We have a few usable parts, but we'll have to choose what to bring back."),TEXT("Shield and hull repairs completing. The engineer will offer a choice of systems."));
 Add(EVTIntroStage::Challenge,true,TEXT("Challenge"),TEXT("A survivor? Your captain should have surrendered when I asked. Turn that wreck around. I won't offer twice."),TEXT("The attacker is still here, damaged from the fight. Prepare to engage."));
 Add(EVTIntroStage::Duel,false,TEXT("Duel"),TEXT("That's the ship that hit us. Their shields are down. Keep moving, bring your broadsides to bear, and finish this. If we take a bad hit, I'll pull us back and patch the hull."),TEXT("Defeat the enemy captain. Disable their ship or destroy it."));
 Add(EVTIntroStage::Salvage,false,TEXT("Salvage"),TEXT("Their drive is dead. Bring us close and hold the interaction control to send the boarding party. Recover what we can; then we're leaving."),TEXT("Approach the disabled attacker and hold INTERACT to board."));
 Add(EVTIntroStage::Gate,false,TEXT("Gate"),TEXT("It's over. Jump drive is ready. Head for the ring. When the staging arrow appears, hold the interaction control; the helm will line us up and drive through. Keep holding until we cross."),TEXT("Leave through the marked jump gate."));
 Add(EVTIntroStage::Handoff,false,TEXT("Handoff"),TEXT("We made it. Find a station for repairs and refitting. Beyond it: traffic, patrols, prizes. Other captains may help us or hunt us. Where we go next is your call, Captain."),TEXT("Welcome to the sandbox. Open the chart to choose your course."));
 Add(EVTIntroStage::BatteryChoice,false,TEXT("BatteryChoice"),TEXT("I've got one working power coupler. I can rebuild the boost circuit for speed, or the EMP gun to knock out enemy systems. Only enough parts for one. Your call."),TEXT("Choose a battery system to repair."));
 Add(EVTIntroStage::BatteryTrial,false,TEXT("BatteryTrial"),TEXT("It's online. Give it a test before we put it to work. I've marked a derelict hull if you need a target."),TEXT("Try the system you chose."));
 Add(EVTIntroStage::SpecialChoice,false,TEXT("SpecialChoice"),TEXT("That works. Next problem: one intact control assembly. It can run the torpedo tubes, or the microwarp drive. Guided firepower or a short jump out of trouble. Which do you want?"),TEXT("Choose a special system to repair."));
 Add(EVTIntroStage::SpecialTrial,false,TEXT("SpecialTrial"),TEXT("Repairs finished. Test the new system. Then we'll see who left us in this mess."),TEXT("Try your second repaired system."));
 auto Option=[](const TCHAR* Id,const TCHAR* Label,const TCHAR* Trial,const TCHAR* Input){FVTIntroRepair R;R.Equipment=FName(Id);R.Label=FText::FromString(Label);R.Trial=FText::FromString(Trial);R.InputAction=FName(Input);return R;};
 BatteryRepairs={Option(TEXT("loadout.boost"),TEXT("Repair BOOST - spend battery for speed"),TEXT("Set half or full throttle and hold BOOST for one second."),TEXT("IA_Boost")),Option(TEXT("loadout.disruptor"),TEXT("Repair EMP - disable enemy systems"),TEXT("Point your bow at the marked derelict and hold EMP to fire."),TEXT("IA_EMP"))};
 SpecialRepairs={Option(TEXT("loadout.torpedoes"),TEXT("Repair TORPEDOES - guided firepower"),TEXT("Hold TORPEDOES with your cursor over the marked derelict. Release to launch."),TEXT("IA_Torpedo")),Option(TEXT("loadout.microwarp"),TEXT("Repair MICROWARP - short aimed jump"),TEXT("Hold MICROWARP, aim at open space, then release to jump."),TEXT("IA_Warp"))};
}
const FVTIntroBeat* UVTIntroData::Beat(EVTIntroStage S) const{return Beats.FindByPredicate([S](const FVTIntroBeat& B){return B.Stage==S;});}
UVTIntroComponent::UVTIntroComponent(){SetIsReplicatedByDefault(true);PrimaryComponentTick.bCanEverTick=false;}
void UVTIntroComponent::BeginPlay(){Super::BeginPlay();Data=LoadObject<UVTIntroData>(nullptr,TEXT("/Game/Data/DA_Intro.DA_Intro"));if(!Data)Data=NewObject<UVTIntroData>(this);}
UVTIntroComponent* UVTIntroComponent::For(const AVTShip* S){const auto* PC=S?Cast<AVTController>(S->GetController()):nullptr;return PC?PC->Intro.Get():nullptr;}
bool UVTIntroComponent::IsArena(const UVTSimulation* Sim,int32 I){return Sim&&Sim->Data&&Sim->Data->Systems.IsValidIndex(I)&&Sim->Data->Systems[I].Id.ToString().StartsWith(TEXT("__intro_"));}
void UVTIntroComponent::PrepareArenas(UVTSimulation* Sim){
 if(!Sim||!Sim->Data||Sim->Data->Systems.IsEmpty()||IsArena(Sim,Sim->Data->Systems.Num()-1))return;
 Sim->Data=DuplicateObject<UVTGameData>(Sim->Data,Sim);
 const int32 Start=FMath::Max(0,Sim->Data->FindSystem(Sim->Data->StartSystem));
 for(int32 I=0;I<4;++I){FVTSystemDefinition S;S.Id=FName(*FString::Printf(TEXT("__intro_%d"),I));S.DisplayName=NSLOCTEXT("VTIntro","Arena","Wreck field");S.Radius=1400;S.ChartPosition=Sim->Data->Systems[Start].ChartPosition-FVector2D(5,0);S.Links.Add(Sim->Data->StartSystem);Sim->Data->Systems.Add(S);}
}
void UVTIntroComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(UVTIntroComponent,Progress);DOREPLIFETIME(UVTIntroComponent,ObjectivePosition);DOREPLIFETIME(UVTIntroComponent,HasWaypoint);}
void UVTIntroComponent::OnRep_Progress(){VTNotifyHUD(GetWorld());}
void UVTIntroComponent::ClientResetHelm_Implementation(){auto* PC=CastChecked<AVTController>(GetOwner());PC->ThrottleControl.SetAnalog(0);PC->LocalIntent={};PC->SendAccumulator=0;}
void UVTIntroComponent::ClientKeepFit_Implementation(FName Hull,const FVTLoadoutSelection& Fit){
 auto* PC=CastChecked<AVTController>(GetOwner());auto* GI=Cast<UVTGameInstance>(GetWorld()->GetGameInstance());if(!GI)return;
 GI->SelectedHull=Hull;GI->SelectedFit=Fit;
 if(PC->UI){PC->UI->FitEditor.Initialize(*GetWorld()->GetSubsystem<UVTSimulation>()->Data,Hull,Fit);PC->UI->RefreshFitEditor();}
 GI->GetSubsystem<UVTSaveSubsystem>()->RememberFit();
}
bool UVTIntroComponent::CanAdvance() const{return Progress.Stage==EVTIntroStage::Wake||Progress.Stage==EVTIntroStage::Challenge||Progress.Stage==EVTIntroStage::Handoff;}
void UVTIntroComponent::Filter(FVTPilotIntent& I) const{
 if(!Active())return;
 if(Progress.Stage==EVTIntroStage::Wake){I.Throttle=I.Turn=0;I.Buttons=0;return;}
 if(!WeaponsOnline())I.Buttons&=~(VTButtons::Port|VTButtons::Starboard|VTButtons::AimPort|VTButtons::AimStarboard);
 if(!SystemsOnline())I.Buttons&=VTButtons::Port|VTButtons::Starboard|VTButtons::AimPort|VTButtons::AimStarboard;
 if(!JumpOnline()&&Progress.Stage!=EVTIntroStage::Salvage)I.Buttons&=~VTButtons::Interact;
}
void UVTIntroComponent::Start(){if(!GetOwner()->HasAuthority())return;FVTIntroProgress New;New.Stage=EVTIntroStage::Wake;if(auto* S=Cast<AVTShip>(CastChecked<AVTController>(GetOwner())->GetPawn())){FVTLoadoutSelection Empty;Empty.OverrideBatteries=Empty.OverrideSpecials=true;S->ApplyFit(Empty,true);}Restore(New);}
void UVTIntroComponent::Restore(FVTIntroProgress Saved){
 if(!GetOwner()->HasAuthority())return;Cleanup();Progress=Saved;if(!Active())return;
 if(!Data){Data=LoadObject<UVTIntroData>(nullptr,TEXT("/Game/Data/DA_Intro.DA_Intro"));if(!Data)Data=NewObject<UVTIntroData>(this);}
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();PrepareArenas(Sim);
 if(InArena()){
  Arena=INDEX_NONE;for(int32 I=0;I<Sim->Data->Systems.Num();++I)if(IsArena(Sim,I)){
   bool Occupied=false;for(AVTShip* S:Sim->Ships)if(IsValid(S)&&!S->IsNPC&&S->SystemIndex==I&&S->GetController()!=GetOwner())Occupied=true;
   if(!Occupied){Arena=I;break;}
  }
  if(Arena==INDEX_NONE){ServerSkip();return;}
  Rebuild();
 }
 ClientResetHelm();GetOwner()->ForceNetUpdate();
}
void UVTIntroComponent::Cleanup(){
 if(!GetOwner()->HasAuthority())return;auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();
 if(IsArena(Sim,Arena)){auto Ships=Sim->Ships;for(AVTShip* S:Ships)if(IsValid(S)&&S->IsNPC&&S->SystemIndex==Arena){if(S->Controller)S->Controller->Destroy();S->Destroy();}auto Shots=Sim->Projectiles;for(AVTProjectile* P:Shots)if(IsValid(P)&&P->SystemIndex==Arena)P->Destroy();}
 Target.Reset();Arena=INDEX_NONE;
}
AVTShip* UVTIntroComponent::SpawnFixture(const FVector2D& P,bool Debris,bool Decorative){
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();FVTMotion M;M.Position=P;M.Heading=PI;
 auto* S=Sim->SpawnShip(TEXT("house_patrol"),Arena,M,true,TEXT("Freebooters"));S->IntroFixture=Debris?1:2;S->OnRep_IntroFixture();
 if(Debris||Progress.Stage!=EVTIntroStage::Duel){if(S->Controller)S->Controller->Destroy();}
 S->Anchored=Debris;S->Invulnerable=Decorative;S->Combat->Shields=FVTShieldBanks(0,0,0,0);
 S->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),Debris?FMath::Min(S->Definition.Hull,Data->DebrisHull):S->Definition.Hull*Data->EnemyHullFraction);
 return S;
}
void UVTIntroComponent::Rebuild(){
 auto* PC=CastChecked<AVTController>(GetOwner());auto* S=Cast<AVTShip>(PC->GetPawn());if(!S)return;
 const int32 Slot=Arena;Cleanup();Arena=Slot;auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();
 S->SystemIndex=Arena;S->Disabled=S->Docked=S->Autopilot=false;S->Anchored=false;S->Invulnerable=false;
 S->Movement->Motion.Position=Progress.Stage<=EVTIntroStage::Helm?Data->Start:Data->ClearPoint;
 S->Movement->Motion.Heading=0;S->Movement->Motion.Velocity=FVector2D::ZeroVector;S->Movement->Motion.Omega=0;ResetIntroMotion(S);Sim->Queries.Invalidate();
 S->Abilities->CancelAllAbilities();S->Combat->EquipmentState.LaunchQueue.Reset();S->PortReload=S->StarboardReload=0;S->Combat->PortCharge=S->Combat->StarboardCharge=0;
 S->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),S->Definition.Hull*(SystemsOnline()?1:Data->InitialHullFraction));
 S->Abilities->SetNumericAttributeBase(UVTAttributes::GetEMPStressAttribute(),0);S->Combat->Shields=SystemsOnline()?S->Definition.ShieldMax:FVTShieldBanks(0,0,0,0);
 SpawnFixture(FVector2D(-440,-350),true,true);SpawnFixture(FVector2D(-400,-690),true,true);SpawnFixture(FVector2D(-650,-670),true,true);
 if(Progress.Stage<=EVTIntroStage::Debris)Target=SpawnFixture(Data->DebrisPoint,true);
 if(Progress.Stage==EVTIntroStage::BatteryTrial||Progress.Stage==EVTIntroStage::SpecialTrial)Target=SpawnFixture(Data->DebrisPoint,false,true);
 if(Progress.Stage==EVTIntroStage::Challenge||Progress.Stage==EVTIntroStage::Duel||Progress.Stage==EVTIntroStage::Salvage){Target=SpawnFixture(Data->EnemyPoint,false);if(Progress.Stage==EVTIntroStage::Salvage)Target->Disabled=true;}
 S->ForceNetUpdate();ClientResetHelm();
}
void UVTIntroComponent::Change(EVTIntroStage Stage){
 if((Progress.Stage==EVTIntroStage::BatteryTrial||Progress.Stage==EVTIntroStage::SpecialTrial)&&Target.IsValid()){if(Target->Controller)Target->Controller->Destroy();Target->Destroy();Target.Reset();}
 Progress.Stage=Stage;Progress.Elapsed=0;Progress.DeviceSeconds=0;
 auto* S=Cast<AVTShip>(CastChecked<AVTController>(GetOwner())->GetPawn());if(!S)return;
 if(Stage==EVTIntroStage::Repairs){S->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),S->Definition.Hull);S->Combat->Shields=S->Definition.ShieldMax;}
 if(Stage==EVTIntroStage::BatteryChoice||Stage==EVTIntroStage::SpecialChoice)ClientResetHelm();
 if(Stage==EVTIntroStage::BatteryTrial||Stage==EVTIntroStage::SpecialTrial)Target=SpawnFixture(Data->DebrisPoint,false,true);
 if(Stage==EVTIntroStage::Challenge)Target=SpawnFixture(Data->EnemyPoint,false);
 if(Stage==EVTIntroStage::Duel&&Target.IsValid())Target->SpawnDefaultController();
 if(Stage==EVTIntroStage::Handoff||Stage==EVTIntroStage::Complete){Cleanup();HasWaypoint=false;ClientKeepFit(S->ClassId,S->Fit);}
 GetOwner()->ForceNetUpdate();OnRep_Progress();
}
void UVTIntroComponent::ServerAdvance_Implementation(){if(!CanAdvance())return;Change(Progress.Stage==EVTIntroStage::Wake?EVTIntroStage::Helm:Progress.Stage==EVTIntroStage::Challenge?EVTIntroStage::Duel:EVTIntroStage::Complete);}
void UVTIntroComponent::ServerSkip_Implementation(){
 if(!Active())return;auto* PC=CastChecked<AVTController>(GetOwner());auto* S=Cast<AVTShip>(PC->GetPawn());if(!S)return;
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();if(InArena()){S->SystemIndex=FMath::Max(0,Sim->Data->FindSystem(Sim->Data->StartSystem));S->Movement->Motion.Position=FVector2D(0,-270);S->Movement->Motion.Velocity=FVector2D::ZeroVector;ResetIntroMotion(S);S->Disabled=false;S->Abilities->SetNumericAttributeBase(UVTAttributes::HullAttribute(),S->Definition.Hull);S->Combat->Shields=S->Definition.ShieldMax;Sim->Queries.Invalidate();S->ForceNetUpdate();}
 Change(EVTIntroStage::Complete);ClientResetHelm();
}
void UVTIntroComponent::Retry(){if(!InArena())return;Rebuild();Progress.Elapsed=0;Progress.Distance=Progress.Turn=Progress.DeviceSeconds=0;}
void UVTIntroComponent::FixedStep(){
 if(!GetOwner()->HasAuthority()||!Active()||!Data)return;auto* PC=CastChecked<AVTController>(GetOwner());auto* S=Cast<AVTShip>(PC->GetPawn());if(!S)return;
 if(InArena()&&(S->Disabled||S->Attributes->Hull.GetCurrentValue()<=S->Definition.Hull*GetWorld()->GetSubsystem<UVTSimulation>()->Data->Rules.CrippleThreshold)){Retry();return;}
 Progress.Elapsed+=VT::Step;HasWaypoint=false;
 if(Progress.Stage==EVTIntroStage::Helm){
  Progress.Distance+=float((S->Movement->Motion.Position-S->Movement->Previous.Position).Size());Progress.Turn+=FMath::Abs(FMath::UnwindRadians(S->Movement->Motion.Heading-S->Movement->Previous.Heading));
  ObjectivePosition=Data->ClearPoint;HasWaypoint=true;
  if(Progress.Distance>=Data->HelmDistance&&Progress.Turn>=Data->HelmTurn&&S->Movement->Motion.Velocity.Size()<5&&FMath::Abs(S->Intent.Throttle)<0.01f&&(S->Movement->Motion.Position-ObjectivePosition).Size()<Data->WaypointRadius)Change(EVTIntroStage::Debris);
 }else if(Progress.Stage==EVTIntroStage::Debris){ObjectivePosition=Data->DebrisPoint;HasWaypoint=true;if(!Target.IsValid())Change(EVTIntroStage::Repairs);}
 else if(Progress.Stage==EVTIntroStage::Repairs&&Progress.Elapsed>=Data->RepairSeconds)Change(EVTIntroStage::BatteryChoice);
 else if(Progress.Stage==EVTIntroStage::BatteryTrial){HasWaypoint=true;ObjectivePosition=Data->DebrisPoint;if(S->Combat->BoostPowered)Progress.DeviceSeconds+=VT::Step;if((Progress.BatteryChoice==FName("loadout.boost")&&Progress.DeviceSeconds>=1)||(Progress.BatteryChoice==FName("loadout.disruptor")&&S->Combat->EquipmentState.EMPCooldown>0))Change(EVTIntroStage::SpecialChoice);}
 else if(Progress.Stage==EVTIntroStage::SpecialTrial){HasWaypoint=true;ObjectivePosition=Data->DebrisPoint;bool Fired=false;for(const AVTProjectile* P:GetWorld()->GetSubsystem<UVTSimulation>()->Projectiles)if(IsValid(P)&&P->SourceId==S->PersistentId&&P->Kind==EVTProjectileKind::Torpedo)Fired=true;if((Progress.SpecialChoice==FName("loadout.microwarp")&&S->Combat->EquipmentState.WarpCooldown>0)||(Progress.SpecialChoice==FName("loadout.torpedoes")&&Fired))Change(EVTIntroStage::Challenge);}
 else if(Progress.Stage==EVTIntroStage::Duel){if(!Target.IsValid())Change(EVTIntroStage::Gate);else{ObjectivePosition=Target->Movement->Motion.Position;HasWaypoint=true;if(Target->Disabled)Change(EVTIntroStage::Salvage);}}
 else if(Progress.Stage==EVTIntroStage::Salvage){if(!Target.IsValid())Change(EVTIntroStage::Gate);else{ObjectivePosition=Target->Movement->Motion.Position;HasWaypoint=true;}}
 else if(Progress.Stage==EVTIntroStage::Gate){auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();if(!IsArena(Sim,S->SystemIndex)){if(!S->GatePassage->Arriving())Change(EVTIntroStage::Handoff);}else{HasWaypoint=true;ObjectivePosition=Sim->JumpPosition(S->SystemIndex,Sim->Data->StartSystem);}}
}

void AVTShip::OnRep_IntroFixture(){
 if(!IntroFixture)return;auto* D=LoadObject<UVTIntroData>(nullptr,TEXT("/Game/Data/DA_Intro.DA_Intro"));if(!D)return;
 GetWorld()->GetSubsystem<UVTSimulation>()->Data->ResolveFit(ClassId,Fit,Definition);
 Definition.ShieldMax=FVTShieldBanks(0,0,0,0);Definition.ShieldRegen=0;
 if(IntroFixture==1){Definition.Radius=D->DebrisRadius;Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Environment/SM_IntroDebris.SM_IntroDebris")));Mesh->SetRelativeScale3D(FVector(D->DebrisRadius));Mesh->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Environment/M_IntroDebris.M_IntroDebris")));}
 else {Definition.Damage*=D->EnemyDamageScale;Definition.AIAbilities=false;}
}

const TArray<FVTIntroRepair>* UVTIntroComponent::RepairOptions() const{if(!Data)return nullptr;return Progress.Stage==EVTIntroStage::BatteryChoice?&Data->BatteryRepairs:Progress.Stage==EVTIntroStage::SpecialChoice?&Data->SpecialRepairs:nullptr;}
const FVTIntroRepair* UVTIntroComponent::CurrentRepair() const{if(!Data)return nullptr;const auto& Options=Progress.Stage==EVTIntroStage::BatteryTrial?Data->BatteryRepairs:Data->SpecialRepairs;const auto Id=Progress.Stage==EVTIntroStage::BatteryTrial?Progress.BatteryChoice:Progress.SpecialChoice;return Options.FindByPredicate([Id](const FVTIntroRepair& O){return O.Equipment==Id;});}
void UVTIntroComponent::ServerChooseRepair_Implementation(int32 Option){
 const auto* Options=RepairOptions();if(!Options||!Options->IsValidIndex(Option))return;auto* PC=CastChecked<AVTController>(GetOwner());auto* S=Cast<AVTShip>(PC->GetPawn());if(!S)return;
 FVTFitEditor Editor;FText Reason;auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();const FName Id=(*Options)[Option].Equipment;
 if(!Editor.Initialize(*Sim->Data,S->ClassId,S->Fit)||!Editor.Toggle(*Sim->Data,Id,Reason)||!S->ApplyFit(Editor.Preview().Selection,true))return;
 if(Progress.Stage==EVTIntroStage::BatteryChoice){Progress.BatteryChoice=Id;Change(EVTIntroStage::BatteryTrial);}else{Progress.SpecialChoice=Id;Change(EVTIntroStage::SpecialTrial);}
}
