#include "VTGameplay.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "VTCombat.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "VTSaveSubsystem.h"
#include "GameFramework/GameSession.h"
#include "VTSessionSubsystem.h"
#include "VTUI.h"
#include "VTWorldAnchor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"

FGameplayAttribute UVTAttributes::HullAttribute() { return FGameplayAttribute(FindFProperty<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UVTAttributes, Hull))); }
FGameplayAttribute UVTAttributes::BatteryAttribute() { return FGameplayAttribute(FindFProperty<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UVTAttributes, Battery))); }
void UVTAttributes::OnRep_EMPStress(const FGameplayAttributeData& Old) {GAMEPLAYATTRIBUTE_REPNOTIFY(UVTAttributes,EMPStress,Old);}
void UVTAttributes::OnRep_Hull(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UVTAttributes, Hull, Old); }
void UVTAttributes::OnRep_Battery(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UVTAttributes, Battery, Old); }
void UVTAttributes::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 DOREPLIFETIME_CONDITION_NOTIFY(UVTAttributes, EMPStress, COND_None, REPNOTIFY_Always);
 DOREPLIFETIME_CONDITION_NOTIFY(UVTAttributes, Hull, COND_None, REPNOTIFY_Always);
 DOREPLIFETIME_CONDITION_NOTIFY(UVTAttributes, Battery, COND_None, REPNOTIFY_Always);
}

UVTShipMovement::UVTShipMovement() {
 SetIsReplicatedByDefault(true);
 PrimaryComponentTick.bCanEverTick = true;
 PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}
void UVTShipMovement::Step(const FVTPilotIntent& Value, bool Predict) {
 AVTShip* S = CastChecked<AVTShip>(GetOwner());
 const bool Frozen=S->Docked||S->Anchored;
 auto* Sim = GetWorld()->GetSubsystem<UVTSimulation>();
 const float Reverse = Sim->Data ? Sim->Data->Rules.ReverseThrottle : 0.25f;
 Previous = Motion;
 FVTPilotIntent Effective=S->Disabled ? FVTPilotIntent() : Value;
 FVTShipStats Stats=S->Definition.Stats; Stats.Thrust*=S->Combat->SpeedScale; Stats.MaxSpeed*=S->Combat->SpeedScale;
 if(Sim->Data) {Stats.Thrust*=Sim->Data->FlightSpeedMultiplier; Stats.MaxSpeed*=Sim->Data->FlightSpeedMultiplier;}
 if(!Frozen) VT::HelmStep(Motion, Stats, Effective, Reverse, VT::Step);
 if (!Frozen && Predict && Sim->Data && Sim->Data->Systems.IsValidIndex(S->SystemIndex)) {
  float Radius = Sim->Data->Systems[S->SystemIndex].Radius;
  const float Length = float(Motion.Position.Size());
  if (Length > Radius) Motion.Velocity -= Motion.Position.GetSafeNormal() * ((Length - Radius) * Sim->Data->Rules.BoundsSpring * VT::Step);
 }
 Motion.Ack = Value.Sequence;
 Motion.SimulationTime=Predict ? Motion.SimulationTime+VT::Step : Sim->SimulationTime;
 if (Predict) {
  Pending.Add({Value});
  if (Pending.Num() > 256) Pending.RemoveAt(0, Pending.Num() - 256);
  if(MeasureCorrections)MaxPendingObserved=FMath::Max(MaxPendingObserved,uint32(Pending.Num()));
 } else Authority = Motion;
}
void UVTShipMovement::OnRep_Authority() {
 AVTShip* S = CastChecked<AVTShip>(GetOwner());
 const bool ChangedSystem=ReplicaSystem!=S->SystemIndex;
 if(ChangedSystem) {ReplicaFrames.Reset();RenderCorrection=FVector2D::ZeroVector;RenderHeadingCorrection=0;ReplicaSystem=S->SystemIndex;}
 LastAuthorityReceived=FPlatformTime::Seconds(); ReplicaFrames.Add(Authority);
 if(ReplicaFrames.Num()>8) ReplicaFrames.RemoveAt(0);
 if (!S->IsLocallyControlled()||S->Autopilot) { Pending.Reset();RenderCorrection=FVector2D::ZeroVector;RenderHeadingCorrection=0;Previous = Motion; Motion = Authority; return; }
 const FVector2D PredictedPosition=Motion.Position;const float PredictedHeading=Motion.Heading;
 Pending.RemoveAll([this](const FPending& P) { return int32(P.Intent.Sequence - Authority.Ack) <= 0; });
 Motion = Authority;
 auto Replay = Pending;
 Pending.Reset();
 for (const auto& P : Replay) Step(P.Intent, true);
 if(!ChangedSystem&&!S->HasAuthority()){auto Difference=PredictedPosition-Motion.Position;if(Difference.SizeSquared()<200*200){RenderCorrection=(RenderCorrection+Difference).GetClampedToMaxSize(100);RenderHeadingCorrection=FMath::Clamp(FMath::UnwindRadians(RenderHeadingCorrection+PredictedHeading-Motion.Heading),-PI/4,PI/4);}else{RenderCorrection=FVector2D::ZeroVector;RenderHeadingCorrection=0;}}
 if(MeasureCorrections) {if(CorrectionDistances.Num()<20000) CorrectionDistances.Add((Motion.Position-PredictedPosition).Size()); MaxPendingObserved=FMath::Max(MaxPendingObserved,uint32(Pending.Num()));}
}
void UVTShipMovement::ApplyPose() {
 AVTShip* S = CastChecked<AVTShip>(GetOwner());
 if(!FApp::CanEverRender()) return;
 auto* PC=GetWorld()->GetFirstPlayerController(); auto* Viewer=PC ? Cast<AVTShip>(PC->GetPawn()) : nullptr;
 bool Visible=Viewer&&Viewer->SystemIndex==S->SystemIndex; if(S->Mesh->IsVisible()!=Visible) S->Mesh->SetVisibility(Visible);
 if(!Visible) {S->PresentEngines(); return;}
 const double Alpha = FMath::Clamp(GetWorld()->GetSubsystem<UVTSimulation>()->Accumulator / VT::Step, 0., 1.);
 FVTMotion Render = Motion;
 if(S->IsLocallyControlled()&&!S->HasAuthority()){Render.Position+=RenderCorrection;Render.Heading+=RenderHeadingCorrection;}
 if (!S->IsLocallyControlled()||S->Autopilot) {
  Render.Position=FMath::Lerp(Previous.Position,Motion.Position,Alpha);
  Render.Heading=Previous.Heading+FMath::UnwindRadians(Motion.Heading-Previous.Heading)*float(Alpha);
  if(!S->HasAuthority()&&!ReplicaFrames.IsEmpty()) {
   double Target=Authority.SimulationTime+(FPlatformTime::Seconds()-LastAuthorityReceived)-0.1;
   Render=ReplicaFrames[0];
   for(int I=1;I<ReplicaFrames.Num();++I) {
    const auto& A=ReplicaFrames[I-1]; const auto& B=ReplicaFrames[I];
    if(Target>=B.SimulationTime) {Render=B; continue;}
    double T=FMath::Clamp((Target-A.SimulationTime)/FMath::Max(1e-6,B.SimulationTime-A.SimulationTime),0.,1.);
    Render.Position=FMath::Lerp(A.Position,B.Position,T); Render.Heading=A.Heading+FMath::UnwindRadians(B.Heading-A.Heading)*float(T); break;
   }
  }
 }
 S->SetActorLocation(VT::ToWorld(Render.Position, S->SystemIndex));
 S->SetActorRotation(FRotator(0, -FMath::RadiansToDegrees(Render.Heading), 0));
 S->PresentEngines();
}
void UVTShipMovement::TickComponent(float Dt, ELevelTick Tick, FActorComponentTickFunction* Fn) { Super::TickComponent(Dt, Tick, Fn);RenderCorrection*=FMath::Exp(-12*Dt);RenderHeadingCorrection*=FMath::Exp(-12*Dt);ApplyPose(); }
void UVTShipMovement::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const { Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(UVTShipMovement, Authority); }

AVTShip::AVTShip() {
 bReplicates = true; SetReplicateMovement(false); SetNetUpdateFrequency(20); SetMinNetUpdateFrequency(10);
 Mesh = CreateDefaultSubobject<UStaticMeshComponent>("HullMesh"); RootComponent = Mesh;
 Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetCanEverAffectNavigation(false); Mesh->SetCastShadow(false);
 Movement = CreateDefaultSubobject<UVTShipMovement>("ShipMovement");
 Combat = CreateDefaultSubobject<UVTCombatComponent>("Combat");
 Abilities = CreateDefaultSubobject<UAbilitySystemComponent>("Abilities");
 Abilities->SetIsReplicated(true); Abilities->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
 Attributes = CreateDefaultSubobject<UVTAttributes>("Attributes");
 CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
 CameraBoom->SetupAttachment(RootComponent); CameraBoom->TargetArmLength = 48000;
 CameraBoom->SetRelativeRotation(FRotator(-35,0,0)); CameraBoom->bDoCollisionTest = false;
 Camera = CreateDefaultSubobject<UCameraComponent>("Camera"); Camera->SetupAttachment(CameraBoom);
 AutoPossessAI = EAutoPossessAI::Disabled; AIControllerClass = AVTShipAI::StaticClass();
}
void AVTShip::InitializeShip(FName ShipId, int32 System, const FVTMotion& Initial) {
 ClassId = ShipId; SystemIndex = System; PersistentId = FGuid::NewGuid();
 Movement->Motion = Initial; Movement->Previous = Initial; Movement->Authority = Initial;
 auto* Sim = GetWorld()->GetSubsystem<UVTSimulation>();
 if (Sim->Data) if (const auto* D = Sim->Data->FindShip(ShipId)) Definition = *D;
 Attributes->Hull.SetBaseValue(Definition.Hull); Attributes->Hull.SetCurrentValue(Definition.Hull);
 Attributes->Battery.SetBaseValue(Definition.BatteryMax); Attributes->Battery.SetCurrentValue(Definition.BatteryMax);
 SetActorLocation(VT::ToWorld(Initial.Position, System));
}
void AVTShip::BeginPlay() {
 Super::BeginPlay();
 Abilities->AddAttributeSetSubobject(Attributes.Get()); Abilities->InitAbilityActorInfo(this, this);
 auto* Sim = GetWorld()->GetSubsystem<UVTSimulation>(); Sim->Ships.AddUnique(this);
 if (Sim->Data) if (const auto* D = Sim->Data->FindShip(ClassId)) Definition = *D;
 UStaticMesh* HullMesh = Sim->Data&&Sim->Data->FactionMeshes.Contains(Faction) ? Sim->Data->FactionMeshes[Faction].Get() : Definition.Mesh.Get();
 if (!HullMesh) {
  HullMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
  Mesh->SetRelativeScale3D(FVector(40,18,8));
 }
 if(HullMesh&&HullMesh->GetPathName()==TEXT("/Engine/BasicShapes/Cube.Cube")) Mesh->SetRelativeScale3D(FVector(40,18,8));
 Mesh->SetStaticMesh(HullMesh);
 Combat->Initialize();
}
void AVTShip::OnRep_Fit() {GetWorld()->GetSubsystem<UVTSimulation>()->Data->ResolveFit(ClassId,Fit,Definition);}
bool AVTShip::ApplyFit(const FVTLoadoutSelection& Selected,bool Refill) {
 if(!HasAuthority()) return false; FVTShipDefinition Resolved; if(!GetWorld()->GetSubsystem<UVTSimulation>()->Data->ResolveFit(ClassId,Selected,Resolved)) return false;
 Fit=Selected; Fit.CrewedDevices=GetWorld()->GetSubsystem<UVTSimulation>()->Data->CrewForFit(ClassId,Selected); Definition=Resolved;
 if(Refill) {Abilities->CancelAllAbilities(); PortReload=0; StarboardReload=0; Combat->EquipmentState=FVTEquipmentState(); Combat->EquipmentState.Loaded=Definition.Equipment.Tubes; Combat->EquipmentState.TorpedoMagazine=Definition.Equipment.TorpedoMagazine; Combat->EquipmentState.MineMagazine=Definition.Equipment.MineMagazine; Abilities->SetNumericAttributeBase(UVTAttributes::BatteryAttribute(),Definition.BatteryMax);}
 ForceNetUpdate(); return true;
}
void AVTShip::PossessedBy(AController* NewController) {Super::PossessedBy(NewController); Abilities->InitAbilityActorInfo(this,this);}
void AVTShip::OnRep_Controller() {Super::OnRep_Controller(); Abilities->InitAbilityActorInfo(this,this);}
void AVTShip::EndPlay(const EEndPlayReason::Type Reason) {
 if (GetWorld()) if (auto* Sim = GetWorld()->GetSubsystem<UVTSimulation>()) Sim->Ships.Remove(this);
 Super::EndPlay(Reason);
}
void AVTShip::ServerIntent_Implementation(FVTPilotIntent Value) {
 if (Autopilot || !VT::ValidIntent(Value) || !VT::SequenceAdvanceAllowed(Value.Sequence,LastReceived,GetWorld()->GetRealTimeSeconds()-LastInputTime)) return;
 LastReceived=Value.Sequence; if(InputQueue.Num()<256) InputQueue.Add(Value); LastInputTime=GetWorld()->GetRealTimeSeconds();
}
void AVTShip::ServerIntentBatch_Implementation(const TArray<FVTPilotIntent>& Values) {if(Values.Num()>12) return; for(const auto& Value:Values) ServerIntent_Implementation(Value);}
bool AVTShip::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const {
 const APlayerController* PC = Cast<APlayerController>(RealViewer);
 const AVTShip* Viewer = PC ? Cast<AVTShip>(PC->GetPawn()) : Cast<AVTShip>(ViewTarget);
 return Viewer && Viewer->SystemIndex == SystemIndex;
}
void AVTShip::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 DOREPLIFETIME(AVTShip,Autopilot); DOREPLIFETIME(AVTShip,DockProgress); DOREPLIFETIME(AVTShip,JumpProgress); DOREPLIFETIME(AVTShip,JumpDestination); DOREPLIFETIME(AVTShip, ShipRole); DOREPLIFETIME(AVTShip, Fit); DOREPLIFETIME(AVTShip, IsNPC); DOREPLIFETIME(AVTShip, SystemIndex); DOREPLIFETIME(AVTShip, ClassId); DOREPLIFETIME(AVTShip, Faction);
 DOREPLIFETIME(AVTShip, PersistentId); DOREPLIFETIME(AVTShip, Docked); DOREPLIFETIME(AVTShip, Disabled);
 DOREPLIFETIME(AVTShip, PortReload); DOREPLIFETIME(AVTShip, StarboardReload);
}

AVTShipAI::AVTShipAI() { PrimaryActorTick.bCanEverTick = false; }
void AVTPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 DOREPLIFETIME_CONDITION(AVTPlayerState, Credits, COND_OwnerOnly); DOREPLIFETIME_CONDITION(AVTPlayerState, Boarded, COND_OwnerOnly);
 DOREPLIFETIME(AVTPlayerState, Profile); DOREPLIFETIME_CONDITION(AVTPlayerState, Reputation, COND_OwnerOnly); DOREPLIFETIME_CONDITION(AVTPlayerState, Heat, COND_OwnerOnly);
}
void AVTGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
 Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AVTGameState,Wave); DOREPLIFETIME(AVTGameState,EnemiesRemaining); DOREPLIFETIME(AVTGameState,Outcome); DOREPLIFETIME(AVTGameState, SimulationTime); DOREPLIFETIME(AVTGameState, Populations);
}
AVTController::AVTController() { bShowMouseCursor = true; PlayerCameraManagerClass=AVTCameraManager::StaticClass(); }
void AVTController::BeginPlay() {
 Super::BeginPlay();
 if (IsLocalController()) {
  if(!IsRunningCommandlet()&&FApp::CanEverRender()) {GetWorld()->SpawnActor<AVTSky>(); GetWorld()->SpawnActor<AVTReferenceGrid>();}
  if(UClass* UIClass=GetWorld()->GetSubsystem<UVTSimulation>()->Data->UIClass.Get() ? GetWorld()->GetSubsystem<UVTSimulation>()->Data->UIClass.Get() : LoadClass<UVTUI>(nullptr,TEXT("/Game/UI/WBP_UI.WBP_UI_C"))) {UI=CreateWidget<UVTUI>(this,UIClass); UI->AddToViewport();}

  FlightMapping=LoadObject<UInputMappingContext>(nullptr,TEXT("/Game/Input/IMC_Flight.IMC_Flight"));
  CommonMapping=LoadObject<UInputMappingContext>(nullptr,TEXT("/Game/Input/IMC_Common.IMC_Common"));
  MenuMapping=LoadObject<UInputMappingContext>(nullptr,TEXT("/Game/Input/IMC_Menu.IMC_Menu"));
  DockedMapping=LoadObject<UInputMappingContext>(nullptr,TEXT("/Game/Input/IMC_Docked.IMC_Docked"));
  if(auto* Sub=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer())) if(CommonMapping) Sub->AddMappingContext(CommonMapping,100);
  InputContextState=-1;
  if(auto* Sub=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer())) if(auto* Settings=Sub->GetUserSettings()) {if(FlightMapping)Settings->RegisterInputMappingContext(FlightMapping);if(CommonMapping)Settings->RegisterInputMappingContext(CommonMapping);}
  UpdateInputContexts();
 }
}
void AVTController::ReadFlight(const FInputActionValue& Value, int32 Index) {
 if(auto* Ship=Cast<AVTShip>(GetPawn()))if(Ship->Autopilot)return;
 if (Index == 0) LocalIntent.Throttle = Value.Get<float>();
 else if (Index == 1) LocalIntent.Turn = VT::PlayerTurnInput(Value.Get<float>());
 else if (Index == 2) {
  GamepadAim=Value.Get<FVector2D>(); if(GamepadAim.SizeSquared()>0.0025) UsingGamepadAim=true;
 } else if(Index==3||Index==4) {
  const uint16 Aim=Index==3?VTButtons::AimPort:VTButtons::AimStarboard;const uint16 Fire=Index==3?VTButtons::Port:VTButtons::Starboard;
  if(Value.Get<bool>())LocalIntent.Buttons|=Aim;
  else {if(LocalIntent.Buttons&Aim)LocalIntent.Buttons|=Fire;LocalIntent.Buttons&=~Aim;}
 } else {
  const uint16 Bit = uint16(1 << (Index - 3));
  if (Value.Get<bool>()) LocalIntent.Buttons |= Bit; else LocalIntent.Buttons &= ~Bit;
 }
}
void AVTController::SetupInputComponent() {
 Super::SetupInputComponent();
 auto* Input = Cast<UEnhancedInputComponent>(InputComponent);
 if (!Input) return;
 const TCHAR* CommonNames[]={TEXT("Menu"),TEXT("Autopilot"),TEXT("Recover")};
 for(int I=0;I<3;++I) if(auto* Action=LoadObject<UInputAction>(nullptr,*FString::Printf(TEXT("/Game/Input/IA_%s.IA_%s"),CommonNames[I],CommonNames[I]))) {
  if(I==0) Input->BindAction(Action,ETriggerEvent::Started,this,&AVTController::ToggleMenu);
  if(I==1) Input->BindAction(Action,ETriggerEvent::Started,this,&AVTController::ToggleAutopilot);
  if(I==2) Input->BindAction(Action,ETriggerEvent::Started,this,&AVTController::VTRecover);
 }

 for(const TCHAR* Name:{TEXT("HUDControls"),TEXT("HUDChart")}) if(auto* Action=LoadObject<UInputAction>(nullptr,*FString::Printf(TEXT("/Game/Input/IA_%s.IA_%s"),Name,Name))) {
  Input->BindAction(Action,ETriggerEvent::Started,this,&AVTController::ToggleHUD,Name==FString(TEXT("HUDChart")));
 }
 const TCHAR* Names[] = {TEXT("Throttle"),TEXT("Turn"),TEXT("Aim"),TEXT("Port"),TEXT("Starboard"),TEXT("EMP"),TEXT("Torpedo"),TEXT("Warp"),TEXT("Boost"),TEXT("Brace"),TEXT("Interact"),TEXT("Mine"),TEXT("PointDefense")};
 for (int32 I=0; I<UE_ARRAY_COUNT(Names); ++I) {
  const FString Path = FString::Printf(TEXT("/Game/Input/IA_%s.IA_%s"), Names[I], Names[I]);
  auto* Action = LoadObject<UInputAction>(nullptr,*Path); Actions.Add(Action);
  if (Action) {
   Input->BindAction(Action,ETriggerEvent::Triggered,this,&AVTController::ReadFlight,I);
   Input->BindAction(Action,ETriggerEvent::Completed,this,&AVTController::ReadFlight,I);
   Input->BindAction(Action,ETriggerEvent::Canceled,this,&AVTController::CancelFlight,I);
  }
 }
}
void AVTController::PlayerTick(float Dt) {
 Super::PlayerTick(Dt);
 ValidationInput(Dt);
 if(UI&&UI->MenuOpen) {LocalIntent=FVTPilotIntent();}
 AVTShip* Ship = Cast<AVTShip>(GetPawn()); if (!IsLocalController() || !Ship) return;
 if((UI&&UI->MenuOpen)||Ship->Autopilot) LocalIntent=FVTPilotIntent();
 float MouseX=0,MouseY=0; GetInputMouseDelta(MouseX,MouseY); if(FMath::Abs(MouseX)+FMath::Abs(MouseY)>0.1f) UsingGamepadAim=false;
 FVector Origin, Direction;
 if (!UsingGamepadAim && DeprojectMousePositionToWorld(Origin, Direction) && FMath::Abs(Direction.Z) > 0.0001) {
  const FVector Hit = Origin + Direction * (-Origin.Z / Direction.Z);
  const FVector Delta = Hit - Ship->GetActorLocation();
  LocalIntent.CursorOffset=FVector2D(Delta.X,-Delta.Y)/100.;
  LocalIntent.CursorOffset=LocalIntent.CursorOffset.GetClampedToMaxSize(1300);
  if (Delta.SizeSquared2D() > 1) LocalIntent.Aim = LocalIntent.CursorOffset.GetSafeNormal();
 }

 double Now=FPlatformTime::Seconds(); float RealDt=LastRealTick>0 ? float(FMath::Min(0.1,Now-LastRealTick)) : Dt; LastRealTick=Now;
 if(UsingGamepadAim&&(LocalIntent.Buttons&(VTButtons::AimPort|VTButtons::AimStarboard|VTButtons::Torpedo|VTButtons::Warp))) {
  const auto& Controls=GetWorld()->GetSubsystem<UVTSimulation>()->Data->Feel.controls;
  auto Axis=[&](double V){return FMath::Sign(V)*FMath::Clamp((FMath::Abs(V)-Controls.deadzone)/FMath::Max(0.001f,Controls.saturation-Controls.deadzone),0.,1.);};
  FRotator Rotation=PlayerCameraManager ? PlayerCameraManager->GetCameraRotation() : FRotator::ZeroRotator; FRotationMatrix Basis(Rotation); auto Right=Basis.GetScaledAxis(EAxis::Y),Up=Basis.GetScaledAxis(EAxis::Z);
  auto Delta=FVector2D(Right.X,-Right.Y).GetSafeNormal()*Axis(GamepadAim.X)+FVector2D(Up.X,-Up.Y).GetSafeNormal()*Axis(GamepadAim.Y);
  LocalIntent.CursorOffset=(LocalIntent.CursorOffset+Delta*Controls.aim_cursor_rate*RealDt).GetClampedToMaxSize(Controls.aim_cursor_max); LocalIntent.Aim=LocalIntent.CursorOffset.GetSafeNormal();
 }
 bool Aiming=(LocalIntent.Buttons&(VTButtons::AimPort|VTButtons::AimStarboard|VTButtons::Warp))!=0;
 if(GetWorld()->GetNetMode()==NM_Standalone&&!(UI&&UI->MenuOpen)) {
  const auto& Time=GetWorld()->GetSubsystem<UVTSimulation>()->Data->Feel.time;
  float Target=Aiming&&AimBattery>0 ? Time.aim_timescale : 1;
  AimBattery=FMath::Clamp(AimBattery+(Target<1 ? -RealDt*Time.battery_drain_per_sec : RealDt*Time.battery_recharge_per_sec),0.f,Time.battery_max); AimDilation+=(Target-AimDilation)*(1-FMath::Exp(-10*RealDt));
  UGameplayStatics::SetGlobalTimeDilation(this,HitStop>0 ? Time.hitstop_timescale : FMath::Max(0.02f,AimDilation));
 }
 HitStop=FMath::Max(0.f,HitStop-RealDt); CameraTrauma=FMath::Max(0.f,CameraTrauma-RealDt*GetWorld()->GetSubsystem<UVTSimulation>()->Data->Feel.camera.trauma_decay);
 const auto& CameraFeel=GetWorld()->GetSubsystem<UVTSimulation>()->Data->Feel.camera;
 Ship->Camera->SetFieldOfView(FMath::RadiansToDegrees(CameraFeel.base_fov+(Ship->Combat->BoostPowered ? CameraFeel.boost_fov_gain : 0)));
 if(Ship->Autopilot){SendAccumulator=0;return;}
 SendAccumulator += Dt;
 while (SendAccumulator >= VT::Step) {
  SendAccumulator -= VT::Step;
  LocalIntent.Sequence = ++NextSequence;
  if (Ship->HasAuthority()) Ship->ServerIntent_Implementation(LocalIntent);
  else {Ship->Movement->Step(LocalIntent,true); TArray<FVTPilotIntent> Batch; const auto& Pending=Ship->Movement->Pending; for(int I=FMath::Max(0,Pending.Num()-8);I<Pending.Num();++I) Batch.Add(Pending[I].Intent); Ship->ServerIntentBatch(Batch);}
  LocalIntent.Buttons&=~(VTButtons::Port|VTButtons::Starboard);
 }
}
void AVTController::VTJump(FString Destination) { ServerJump(FName(Destination)); }
void AVTController::ServerJump_Implementation(FName Destination) {
#if UE_BUILD_SHIPPING
 return;
#else
 AVTShip* Ship = Cast<AVTShip>(GetPawn()); auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();
 if (!Ship || !Sim->Data || !Sim->Data->Systems.IsValidIndex(Ship->SystemIndex)) return;
 const auto& Current = Sim->Data->Systems[Ship->SystemIndex];
 const int32 Next = Sim->Data->FindSystem(Destination);
 if (Next < 0 || !Current.Links.Contains(Destination)) return;
 // Development travel command; the gameplay interaction path supplies the charge/range gate.
 Ship->SystemIndex = Next; Ship->Movement->Motion.Position = FVector2D(0,-200); Ship->Movement->Motion.Velocity = FVector2D::ZeroVector;
 Ship->Movement->Authority = Ship->Movement->Motion; Ship->Movement->Pending.Reset(); Ship->ForceNetUpdate();
#endif
}
void AVTController::VTHost() { CastChecked<UVTGameInstance>(GetGameInstance())->Host(); }
void AVTController::VTJoin(FString Address) { CastChecked<UVTGameInstance>(GetGameInstance())->Join(Address); }
void AVTController::VTScale(int32 Count) {
 if (HasAuthority()) GetWorld()->GetSubsystem<UVTSimulation>()->Bootstrap(FMath::Clamp(Count, 0, 2000));
}
void AVTController::VTSave() { if (HasAuthority()) GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->Save(); }
void AVTController::VTLoad() { if (HasAuthority()) GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->Load(); }

void UVTSimulation::Initialize(FSubsystemCollectionBase& Collection) {
 Super::Initialize(Collection);
 auto& Manager=UAssetManager::Get();
 auto Handle=Manager.LoadPrimaryAsset(FPrimaryAssetId(TEXT("VTGameData"),TEXT("DA_GameData")));
 if(Handle) Handle->WaitUntilComplete();
 Data=Cast<UVTGameData>(Manager.GetPrimaryAssetObject(FPrimaryAssetId(TEXT("VTGameData"),TEXT("DA_GameData"))));
 if(!Data) Data=LoadObject<UVTGameData>(nullptr,TEXT("/Game/Data/DA_GameData.DA_GameData"));
 if(Data) Data->LoadCatalog();
}
AVTShip* UVTSimulation::SpawnShip(FName Id, int32 System, const FVTMotion& Motion, bool NPC, FName Faction) {
 AVTShip* Ship = GetWorld()->SpawnActorDeferred<AVTShip>(Data&&Data->ShipClass.Get() ? Data->ShipClass.Get() : AVTShip::StaticClass(),FTransform(VT::ToWorld(Motion.Position,System)),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
 Ship->Faction = Faction; Ship->IsNPC = NPC; Ship->InitializeShip(Id,System,Motion);
 UGameplayStatics::FinishSpawningActor(Ship,FTransform(VT::ToWorld(Motion.Position,System)));
 if (NPC) Ship->SpawnDefaultController();
 return Ship;
}
void UVTSimulation::Bootstrap(int32 Population,bool Synthetic) {
 if (!Data || GetWorld()->GetNetMode() == NM_Client) return;
 if (Bootstrapped && Population < 0) return;
 auto Existing = Ships;
 for (AVTShip* S : Existing) if (IsValid(S) && S->IsNPC) { if(S->Controller) S->Controller->Destroy(); S->Destroy(); }
 Bootstrapped = true;
 if(Population<0&&ActiveScenario()) return;
 uint32 Random=uint32(WorldSeed);
 for (int32 I=0; I<Data->Systems.Num(); ++I) {
  const auto& Def = Data->Systems[I];
  const int32 Civilians = Def.Security == 2 ? 4 : Def.Security == 1 ? 2 : 1;
  const int32 Patrols = Def.Owner.IsNone() ? 0 : Def.Security == 2 ? 3 : Def.Security == 1 ? 1 : 0;
  const int32 Danger = FMath::RoundToInt(Def.Danger * 4);
  const int32 Count = Population >= 0 ? Population / Data->Systems.Num() + (I < Population % Data->Systems.Num() ? 1 : 0) : Civilians + Patrols + Danger;
  const int32 Baseline=FMath::Max(1,Civilians+Patrols+Danger);
  const int32 CivilianLimit=Population>=0&&!Synthetic ? FMath::RoundToInt(float(Count)*Civilians/Baseline) : Civilians;
  const int32 PatrolLimit=Population>=0&&!Synthetic ? CivilianLimit+FMath::RoundToInt(float(Count)*Patrols/Baseline) : Civilians+Patrols;
  for (int32 N=0; N<Count; ++N) {
   const float Angle = VT::LcgNext(Random)*2*PI;
   const float Radius = 200+VT::LcgNext(Random)*(Def.Radius*0.7f-200);
   FVTMotion Motion; Motion.Position = FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*Radius; Motion.Heading = Angle;
   auto* NPC=SpawnShip("house_patrol",I,Motion,true,N<CivilianLimit ? FName("Guild") : N<PatrolLimit ? Def.Owner : FName("Freebooters"));
   NPC->ShipRole=N<CivilianLimit ? 1 : N<PatrolLimit ? 2 : 0; NPC->Invulnerable=Population>=0&&Synthetic;
  }
 }
}
void UVTSimulation::Tick(float Dt) {
 if (GetWorld()->GetNetMode() == NM_Client) { Accumulator = FMath::Fmod(Accumulator + Dt,double(VT::Step)); ValidationTick(); return; }
 if (!GetWorld()->HasBegunPlay()) return;
 if(GetWorld()->GetMapName().Contains(TEXT("Menu"))) {ValidationTick(); return;}
 auto* GI=CastChecked<UVTGameInstance>(GetWorld()->GetGameInstance());
 if(GI->ContinueWorld) {
  GI->ContinueWorld=false;
  if(!GI->GetSubsystem<UVTSaveSubsystem>()->Load()) {Bootstrapped=false; GI->GetSubsystem<UVTSessionSubsystem>()->SetStatus(TEXT("World failed validation; previous snapshots retained.")); UE_LOG(LogTemp,Error,TEXT("Continue world failed validation")); GI->ReturnToMenu(); return;}
  for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It) if(auto* PC=Cast<AVTController>(It->Get())) PC->ClientIdentify(GI->GetSubsystem<UVTSaveSubsystem>()->WorldId);
 }
 Accumulator += Dt;
 int32 Steps = 0;
 while (Accumulator >= VT::Step && Steps++ < 16) { Accumulator -= VT::Step; FixedStep(); }
 ValidationTick();
}
void UVTSimulation::FixedStep() {
 const double Start = FPlatformTime::Seconds();
 double PhaseStart=Start; auto Phase=[&](int I){double Now=FPlatformTime::Seconds(); PhaseTotals[I]+=(Now-PhaseStart)*1000; PhaseStart=Now;};
 ScenarioStep();
 if(auto* State=GetWorld()->GetGameState<AVTGameState>()) if(!State->Outcome.IsEmpty()) return;
 SimulationTime += VT::Step;
 SystemShips.SetNum(Data->Systems.Num());
 for(auto& Bucket:SystemShips) Bucket.Reset();
 for(AVTShip* S:Ships) if(IsValid(S)&&SystemShips.IsValidIndex(S->SystemIndex)) SystemShips[S->SystemIndex].Add(S);
 for (AVTShip* S : Ships) if (IsValid(S)) {
  if (auto* Brain = Cast<AVTShipAI>(S->Controller)) Brain->Decide(VT::Step);
  else if(S->Autopilot) {uint32 Ack=S->Movement->Authority.Ack;AVTShipAI::DecideShip(S,VT::Step);S->Intent.Sequence=Ack;}
  else {if(!S->InputQueue.IsEmpty()) {S->Intent=S->InputQueue[0]; S->InputQueue.RemoveAt(0);}
   if (GetWorld()->GetRealTimeSeconds() - S->LastInputTime > 0.25) { S->Intent.Throttle=0; S->Intent.Turn=0; S->Intent.Buttons=0; } AVTShipAI::CrewStep(S);}
 }
 Phase(0);
 for (AVTShip* S : Ships) if (IsValid(S)) S->Combat->SystemsStep();
 Phase(1);
 for (AVTShip* S : Ships) if (IsValid(S)) S->Movement->Step(S->Intent, false);
 Phase(2);
 ContactStep(); LandmarkStep();
 Phase(3);
 // Bounds follows contacts, matching the legacy simulation ordering.
 for(AVTShip* S:Ships) if(IsValid(S)&&!S->Docked&&!S->Anchored) {
  float Length=float(S->Movement->Motion.Position.Size()), Radius=Data->Systems[S->SystemIndex].Radius;
  if(Length>Radius) S->Movement->Motion.Velocity-=S->Movement->Motion.Position.GetSafeNormal()*((Length-Radius)*Data->Rules.BoundsSpring*VT::Step);
 }
 for (AVTShip* S : Ships) if (IsValid(S)) S->Movement->Authority=S->Movement->Motion;
 for (AVTShip* S : Ships) if (IsValid(S)) {S->Combat->WeaponsStep();S->Intent.Buttons&=~(VTButtons::Port|VTButtons::Starboard);}
 Phase(4);
 ProjectileStep();
 Phase(5);
 auto Survivors=Ships;
 for(AVTShip* S:Survivors) if(IsValid(S)) {
  if(S->Attributes->Hull.GetCurrentValue()<=0) {
   S->Combat->Cue(TEXT("GameplayCue.Ship.Explosion"));
   AwardAvenging(S,Survivors);
   if(S->IsNPC) {if(S->Controller) S->Controller->Destroy(); S->Destroy();}
   else if(ActiveScenario()) {if(auto* State=GetWorld()->GetGameState<AVTGameState>()) {State->Outcome=TEXT("Ship lost"); auto* PS=S->GetPlayerState<AVTPlayerState>(); if(auto* Save=GetWorld()->GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()) Save->RecordSolo(false,State->Wave,PS ? PS->Boarded : 0);} S->Disabled=true; S->Intent=FVTPilotIntent();}
   else if(auto* Mode=GetWorld()->GetAuthGameMode<AVTGameMode>()) Mode->RecoverShip(S);
  }
  else if(((S->IsNPC&&S->ShipRole!=1)||(!S->IsNPC&&!ActiveScenario()))&&!S->Invulnerable&&S->Attributes->Hull.GetCurrentValue()<=S->Definition.Hull*Data->Rules.CrippleThreshold) {S->Disabled=true; S->Intent=FVTPilotIntent();}
 }
 Phase(6);
 PiracyStep();
 WorldStep();
 if (auto* State = GetWorld()->GetGameState<AVTGameState>()) {
  State->SimulationTime = SimulationTime;
  if (Data) {
   State->Populations.Init(0,Data->Systems.Num());
   for (AVTShip* S : Ships) if (IsValid(S) && State->Populations.IsValidIndex(S->SystemIndex)) ++State->Populations[S->SystemIndex];
  }
 }
 Phase(7);
 if (auto* Save = GetWorld()->GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()) Save->Advance(VT::Step);
 VTNotifyHUD(GetWorld());
 const double Ms = (FPlatformTime::Seconds()-Start)*1000;
 if (StepMilliseconds.Num() < 20000) StepMilliseconds.Add(Ms);

}

AVTGameMode::AVTGameMode() {
 DefaultPawnClass = AVTShip::StaticClass(); PlayerControllerClass = AVTController::StaticClass();
 PlayerStateClass = AVTPlayerState::StaticClass(); GameStateClass = AVTGameState::StaticClass();
}
void AVTGameMode::BeginPlay() {
 Super::BeginPlay(); if(GetWorld()->GetMapName().Contains(TEXT("Menu"))) return;
 auto* GI=CastChecked<UVTGameInstance>(GetGameInstance()); auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();
 Sim->Bootstrap(GI->ContinueWorld ? 0 : GI->NewWorldPopulation,false); Sim->CreateAnchors();
 // Restore at the first boundary after every actor has completed BeginPlay.
}

void AVTGameMode::PostLogin(APlayerController* NewPlayer) {
 Super::PostLogin(NewPlayer);
 if(GetWorld()->GetMapName().Contains(TEXT("Menu"))) return;
 if(CastChecked<UVTGameInstance>(GetGameInstance())->ContinueWorld) return;
 if(auto* PC=Cast<AVTController>(NewPlayer)) PC->ClientIdentify(GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->WorldId);
}
void AVTGameMode::RestartPlayer(AController* Player) {
 auto* Sim = GetWorld()->GetSubsystem<UVTSimulation>(); Sim->Bootstrap();
 if (!Sim->Data || Sim->Data->Systems.IsEmpty()) return;
 FVTMotion Motion; Motion.Position = FVector2D(0,-200 - GetNumPlayers()*70);
 const int32 System = FMath::Max(0,Sim->Data->FindSystem(Sim->Data->StartSystem));
 auto* GI=CastChecked<UVTGameInstance>(GetGameInstance());
 if(const auto* Scenario=Sim->ActiveScenario()) {Motion.Position=Scenario->Player.Position; Motion.Heading=Scenario->Player.Heading;}
 FName Hull=GI->SelectedHull; FVTLoadoutSelection Fit=GI->SelectedFit;
 if(auto* PC=Cast<AVTController>(Player)) if(!PC->InitialHull.IsNone()) {Hull=PC->InitialHull; Fit=PC->InitialFit;}
 AVTShip* Ship = Sim->SpawnShip(Hull,System,Motion,false,"Corsairs"); Ship->ApplyFit(Fit,true);
 Player->Possess(Ship);
}
void AVTGameMode::RecoverShip(AVTShip* Ship) {
 AController* Captain=Ship->Controller; if(!Captain) return;
 auto* Sim=GetWorld()->GetSubsystem<UVTSimulation>();
 int32 Station=INDEX_NONE; TArray<int32> Frontier; TSet<int32> Seen;
 Frontier.Add(Ship->SystemIndex);
 for(int32 I=0;I<Frontier.Num();++I) {
  int32 System=Frontier[I]; if(Seen.Contains(System)||!Sim->Data->Systems.IsValidIndex(System)) continue;
  Seen.Add(System); const auto& D=Sim->Data->Systems[System];
  if(D.HasStation) {Station=System; break;}
  for(FName Link:D.Links) Frontier.Add(Sim->Data->FindSystem(Link));
 }
 if(Station==INDEX_NONE) return;
 FVTMotion M; M.Position=Sim->Data->Rules.StationPosition+FVector2D(0,-Sim->Data->Rules.StationRadius-60);
 AVTShip* Replacement=Sim->SpawnShip(Ship->ClassId,Station,M,false,Ship->Faction);
 Replacement->ApplyFit(Ship->Fit,true);
 Replacement->LastReceived=Ship->Movement->Motion.Ack; Replacement->Movement->Motion.Ack=Ship->Movement->Motion.Ack; Replacement->Movement->Authority=Replacement->Movement->Motion;
 Captain->Possess(Replacement); Ship->Destroy();
 if(auto* PC=Cast<AVTController>(Captain)) {PC->LocalIntent=FVTPilotIntent(); PC->SendAccumulator=0;}
}
void AVTGameMode::Logout(AController* Exiting) {
 auto* Save=GetGameInstance()->GetSubsystem<UVTSaveSubsystem>();
 Save->CapturePlayer(Cast<AVTController>(Exiting));
 if (auto* Ship=Cast<AVTShip>(Exiting->GetPawn())) Ship->Destroy();
 Save->Save();
 Super::Logout(Exiting);
}
void UVTGameInstance::Host() {GetSubsystem<UVTSessionSubsystem>()->CreateWorld(false,TEXT("Campaign"));}
void UVTGameInstance::StartSolo(const FString& Mode) {
 if(Mode!=TEXT("skirmish")&&Mode!=TEXT("range")) return;
 auto* Save=GetSubsystem<UVTSaveSubsystem>(); Save->Save();
 auto* Session=GetSubsystem<UVTSessionSubsystem>(); if(Session->Sessions.IsValid()&&Session->Sessions->GetNamedSession(NAME_GameSession)) Session->Sessions->DestroySession(NAME_GameSession);
 Save->ResetWorldIdentity(); Save->SoloRecorded=false; PlayMode=Mode; ContinueWorld=false; NewWorldPopulation=-1;
 UGameplayStatics::OpenLevel(this,TEXT("/Game/Maps/Sandbox"));
}

void UVTGameInstance::ReturnToMenu() {if(auto* Save=GetSubsystem<UVTSaveSubsystem>()) Save->Save(); if(auto* Session=GetSubsystem<UVTSessionSubsystem>()) if(Session->Sessions.IsValid()) Session->Sessions->DestroySession(NAME_GameSession); UGameplayStatics::OpenLevel(this,TEXT("/Game/Maps/Menu"));}
void UVTGameInstance::Join(const FString& Address) {
 if (APlayerController* PC = GetFirstLocalPlayerController()) PC->ClientTravel(Address,TRAVEL_Absolute);
}

void AVTGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) {
 // Possession follows the persistent profile handshake rather than transient connection identity.
}
void AVTGameMode::PreLogin(const FString& Options,const FString& Address,const FUniqueNetIdRepl& UniqueId,FString& ErrorMessage) {
 Super::PreLogin(Options,Address,UniqueId,ErrorMessage);
 if(GetNumPlayers()>=4) ErrorMessage=TEXT("This world already has four captains.");
}
void AVTController::ClientIdentify_Implementation(FGuid World) {
 auto* Save=GetGameInstance()->GetSubsystem<UVTSaveSubsystem>();
 const auto* Record=Save->Personal->Tokens.FindByPredicate([World](const FVTReconnectToken& R){return R.World==World;});
 auto* GI=CastChecked<UVTGameInstance>(GetGameInstance()); ServerIdentify(Save->Personal->Profile,Record ? Record->Token : FGuid(),GI->SelectedHull,GI->SelectedFit);
}
void AVTController::ServerIdentify_Implementation(FGuid Profile,FGuid Token,FName Hull,FVTLoadoutSelection Selection) {
 auto* PS=GetPlayerState<AVTPlayerState>(); if(!PS||PS->Profile.IsValid()) return;
 auto* Save=GetGameInstance()->GetSubsystem<UVTSaveSubsystem>();
 bool Valid=Profile.IsValid();
 for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It) if(It->Get()!=this) if(auto* Other=It->Get()->GetPlayerState<AVTPlayerState>()) if(Other->Profile==Profile) Valid=false;
 const auto* Record=Save->PlayerRecords.FindByPredicate([Profile](const FVTSavedPlayer& R){return R.Profile==Profile;});
 if(Record&&Record->Token!=Token) Valid=false;
 FVTShipDefinition Resolved; if(!Record&&(!Hull.ToString().StartsWith(TEXT("corsair"))||!GetWorld()->GetSubsystem<UVTSimulation>()->Data->ResolveFit(Hull,Selection,Resolved))) Valid=false;
 if(!Valid) {if(auto* Mode=GetWorld()->GetAuthGameMode<AVTGameMode>()) Mode->GameSession->KickPlayer(this,FText::FromString(TEXT("Profile already connected or reconnect token invalid."))); return;}
 InitialHull=Record ? Record->Ship.ClassId : Hull; InitialFit=Record ? Record->Ship.Fit : Selection;
 PS->Profile=Profile; PS->Reputation=GetWorld()->GetSubsystem<UVTSimulation>()->Data->InitialReputation; PS->Heat.Init(0,PS->Reputation.Num());
 if(Record) {
  Possess(Save->RestoreShip(Record->Ship)); PS->Credits=Record->Credits; PS->Boarded=Record->Boarded; PS->Heat=Record->Heat; PS->Reputation=Record->Reputation;
 } else if(auto* Mode=GetWorld()->GetAuthGameMode<AVTGameMode>()) Mode->RestartPlayer(this);
 Save->CapturePlayer(this);
 const auto* Accepted=Save->PlayerRecords.FindByPredicate([Profile](const FVTSavedPlayer& R){return R.Profile==Profile;});
 if(Accepted) ClientAcceptIdentity(Save->WorldId,Accepted->Token);
}
void AVTController::ClientAcceptIdentity_Implementation(FGuid World,FGuid Token) {
 GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->StoreToken(World,Token);
 LocalIntent=FVTPilotIntent(); NextSequence=0; SendAccumulator=0;
}
void AVTController::VTRecover() {ServerRecover();}
void AVTController::ServerRecover_Implementation() {
 if(GetWorld()->GetSubsystem<UVTSimulation>()->ActiveScenario()) return;
 if(auto* Ship=Cast<AVTShip>(GetPawn())) if(Ship->Disabled) if(auto* Mode=GetWorld()->GetAuthGameMode<AVTGameMode>()) Mode->RecoverShip(Ship);
}
void UVTGameInstance::OnStart() {
 Super::OnStart(); FString Name,Address;
 if(FParse::Value(FCommandLine::Get(),TEXT("VTHostWorld="),Name)) GetSubsystem<UVTSessionSubsystem>()->CreateWorld(FParse::Param(FCommandLine::Get(),TEXT("VTContinueWorld")),Name);
 else if(FParse::Value(FCommandLine::Get(),TEXT("VTJoinAddress="),Address)) Join(Address);
}
void UVTGameInstance::Init() {Super::Init(); if(GEngine) NetworkFailureHandle=GEngine->OnNetworkFailure().AddUObject(this,&UVTGameInstance::NetworkFailed);}
void UVTGameInstance::NetworkFailed(UWorld* World,UNetDriver* Driver,ENetworkFailure::Type Type,const FString& Message) {
 if(World!=GetWorld()||World->GetNetMode()!=NM_Client) return;
 auto* Session=GetSubsystem<UVTSessionSubsystem>(); Session->SetStatus(TEXT("Host connection ended. Rejoin the same world to restore your ship."));
 if(Session->Sessions.IsValid()&&Session->Sessions->GetNamedSession(NAME_GameSession)) Session->Sessions->DestroySession(NAME_GameSession);
 // Unreal's disconnect handler travels to the configured Menu default map.
}
void UVTGameInstance::Shutdown() {
 if(GEngine) GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
 if(auto* Save=GetSubsystem<UVTSaveSubsystem>()) Save->Save();
 Super::Shutdown();
}

#if WITH_EDITOR
void UVTGameInstance::InitializeHeadlessWorld(UWorld* World) {
 WorldContext=GEngine->GetWorldContextFromWorld(World); check(WorldContext);
 WorldContext->OwningGameInstance=this; World->SetGameInstance(this); Init();
}
#endif

void AVTController::PawnLeavingGame() {
 if(HasAuthority()) GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->CapturePlayer(this);
 Super::PawnLeavingGame();
}

bool AVTGameMode::SetPause(APlayerController* PC,FCanUnpause CanUnpauseDelegate) {
 return GetNetMode()==NM_Standalone&&Super::SetPause(PC,CanUnpauseDelegate);
}

void AVTCameraManager::AddImpactKick(const FVector& Direction,float Magnitude) {
 const auto& Feel=GetWorld()->GetSubsystem<UVTSimulation>()->Data->Feel.camera;
 ImpactKick=(ImpactKick+Direction.GetSafeNormal()*Magnitude*100).GetClampedToMaxSize(Feel.kick_max*100);
}
void AVTCameraManager::UpdateViewTarget(FTViewTarget& OutVT,float DeltaTime) {
 Super::UpdateViewTarget(OutVT,DeltaTime);
 auto* PC=Cast<AVTController>(PCOwner); auto* Ship=PC ? Cast<AVTShip>(PC->GetPawn()) : nullptr; if(!Ship) return;
 auto* Data=GetWorld()->GetSubsystem<UVTSimulation>()->Data.Get(); const auto& C=Data->Feel.camera;
 double Now=GetWorld()->GetRealTimeSeconds(),Dt=LastCameraReal>0 ? FMath::Clamp(Now-LastCameraReal,0.,0.1) : 0; LastCameraReal=Now;
 if(GetWorld()->IsPaused()) Dt=0;
 const auto& M=Ship->Movement->Motion;
 if(!RigReady||RigShip!=Ship->PersistentId||RigSystem!=Ship->SystemIndex) {RigReady=true; RigShip=Ship->PersistentId; RigSystem=Ship->SystemIndex; OrbitYaw=M.Heading; OrbitPitch=FreePitch=C.pitch_base; OrbitDistance=C.distance; OrbitFov=C.base_fov; Focus=Ship->GetActorLocation(); FreeYaw=LookIdle=MenuOrbit=0; ImpactKick=FVector::ZeroVector;}
 float MX=0,MY=0; int32 W=0,H=0; PC->GetViewportSize(W,H); bool Mouse=PC->GetMousePosition(MX,MY)&&W>0&&H>0;
 double LX=Mouse ? FMath::Clamp(double(MX)/W*2-1,-1.,1.) : 0,LY=Mouse ? FMath::Clamp(double(MY)/H*2-1,-1.,1.) : 0;
 bool Active=Mouse&&!PC->UsingGamepadAim&&(FVector2D(MX,MY)-LastCursor).Size()>1; LastCursor=FVector2D(MX,MY);
 const auto& Controls=Data->Feel.controls; auto Axis=[&](double V){return FMath::Abs(V)<=Controls.deadzone ? 0. : FMath::Clamp((FMath::Abs(V)-Controls.deadzone)/(Controls.saturation-Controls.deadzone),0.,1.)*FMath::Sign(V);};
 double SX=Axis(PC->GamepadAim.X),SY=Axis(PC->GamepadAim.Y); if(PC->UsingGamepadAim) Active=SX!=0||SY!=0;
 uint16 Buttons=PC->LocalIntent.Buttons; bool Menu=(PC->UI&&PC->UI->MenuOpen)||Ship->Docked,Locked=false;
 double Yaw=OrbitYaw,Pitch=C.pitch_base,Distance=C.distance;
 auto Lead=(M.Velocity*C.lead_secs).GetClampedToMaxSize(C.lead_max); FVector DesiredFocus=Ship->GetActorLocation()+FVector(Lead.X,-Lead.Y,0)*100;
 if(Menu) {DesiredFocus=Ship->GetActorLocation(); MenuOrbit=FMath::UnwindRadians(MenuOrbit+C.menu_orbit_rate*Dt); Yaw=M.Heading+MenuOrbit;}
 else if(Buttons&(VTButtons::Torpedo|VTButtons::Warp)) {Yaw=M.Heading; Pitch=C.topdown_pitch; Distance=Data->Rules.EngagementRange/FMath::Tan(FMath::Max(0.05,OrbitFov*0.5))*C.topdown_margin; Locked=true;}
 else if(Buttons&(VTButtons::AimPort|VTButtons::AimStarboard)) {auto Direction=VTCombat::BroadsideDirection(M.Heading,(Buttons&VTButtons::AimPort)!=0,PC->LocalIntent.Aim,Ship->Definition.Arc); Yaw=FMath::Atan2(Direction.Y,Direction.X); Pitch=C.aim_pitch; Distance=C.distance*C.aim_dist; Locked=true;}
 else if(PC->UsingGamepadAim) {FreeYaw=FMath::Clamp(FreeYaw-SX*C.look_yaw_rate*Dt,-double(PI),double(PI)); FreePitch=FMath::Clamp(FreePitch-SY*C.look_pitch_rate*Dt,double(C.pitch_min),double(C.pitch_max)); Yaw=M.Heading+FreeYaw; Pitch=FreePitch;}
 else {Yaw=M.Heading-LX*0.9; Pitch=FMath::Clamp(C.pitch_base+LY*0.5,double(C.pitch_min),double(C.pitch_max));}
 LookIdle=Locked||Active ? 0 : LookIdle+Dt;
 if(!Locked&&!Menu&&LookIdle>(PC->UsingGamepadAim ? C.recenter_delay_pad : C.recenter_delay)) {bool Reverse=FVector2D::DotProduct(M.Velocity,FVector2D(FMath::Cos(M.Heading),FMath::Sin(M.Heading)))<-5; double K=1-FMath::Exp(-C.recenter_lerp*Dt); FreeYaw+=FMath::UnwindRadians((Reverse ? PI : 0)-FreeYaw)*K; FreePitch+=(C.pitch_base-FreePitch)*K; Yaw=M.Heading+FreeYaw; Pitch=FreePitch;}
 double K=1-FMath::Exp(-(Locked ? C.aim_lerp : C.yaw_lerp)*Dt); OrbitYaw=FMath::UnwindRadians(OrbitYaw+FMath::UnwindRadians(Yaw-OrbitYaw)*K); OrbitPitch+=(Pitch-OrbitPitch)*K;
 OrbitDistance+=(Distance-OrbitDistance)*(1-FMath::Exp(-C.dist_lerp*Dt)); Focus+=(DesiredFocus-Focus)*(1-FMath::Exp(-C.focus_lerp*Dt));
 OrbitFov+=(C.base_fov+(Ship->Combat->BoostPowered ? C.boost_fov_gain : 0)-OrbitFov)*(1-FMath::Exp(-C.fov_lerp*Dt)); ImpactKick*=FMath::Exp(-C.kick_decay*Dt);
 double Time=Now*C.shake_freq; float Shake=PC->CameraTrauma*PC->CameraTrauma*C.shake_magnitude*100;
 FVector Back(-FMath::Cos(OrbitYaw),FMath::Sin(OrbitYaw),0); FVector Eye=Focus+(Back*FMath::Cos(OrbitPitch)+FVector::UpVector*FMath::Sin(OrbitPitch))*OrbitDistance*100+ImpactKick+FVector(FMath::Sin(Time)*Shake,FMath::Sin(Time*1.37)*Shake,0);
 OutVT.POV.Location=Eye; OutVT.POV.Rotation=(Focus-Eye).Rotation(); OutVT.POV.FOV=FMath::RadiansToDegrees(OrbitFov); OutVT.POV.AspectRatioAxisConstraint=EAspectRatioAxisConstraint::AspectRatio_MaintainYFOV;
}

void AVTShip::PresentEngines() {
 if(IsRunningCommandlet()||!FApp::CanEverRender()) return;
 auto* PC=GetWorld()->GetFirstPlayerController(); auto* Viewer=PC ? Cast<AVTShip>(PC->GetPawn()) : nullptr;
 bool Visible=Viewer&&Viewer->SystemIndex==SystemIndex&&(Viewer->Movement->Motion.Position-Movement->Motion.Position).SizeSquared()<900*900;
 bool On=Visible&&!Docked&&!Disabled&&Movement->Motion.Velocity.SizeSquared()>100;
 if(On&&EngineTrails.IsEmpty()) if(auto* Asset=LoadObject<UNiagaraSystem>(nullptr,TEXT("/Game/Effects/NS_EngineTrail.NS_EngineTrail"))) for(FName Socket:{FName(TEXT("EnginePort")),FName(TEXT("EngineStarboard"))}) if(auto* Effect=UNiagaraFunctionLibrary::SpawnSystemAttached(Asset,Mesh,Socket,FVector::ZeroVector,FRotator(-90,0,0),EAttachLocation::SnapToTarget,false)) {Effect->SetRelativeScale3D(FVector(3)); EngineTrails.Add(Effect);}
 for(UNiagaraComponent* Effect:EngineTrails) if(Effect) {if(On&&!Effect->IsActive()) Effect->Activate(); else if(!On&&Effect->IsActive()) Effect->Deactivate();}
}

void UVTSimulation::ConfigurePopulationFixture(int32 Count,bool Busy,bool Armed) {
 Bootstrap(Count);
 if(Busy) for(int I=0;I<Count/2;++I) {auto* Ship=Ships[I].Get(); Ship->SystemIndex=0; float Angle=float(I)*2.399963f; Ship->Movement->Motion.Position=FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*(200+float(I%20)*40);}
 if(Armed) for(int I=0;I<Count;++I) if(I%4==0) {auto* Ship=Ships[I].Get(); FVTLoadoutSelection Fit; Fit.Battery=I%8==0 ? FName("loadout.disruptor") : FName("loadout.point_defense"); Fit.Special=I%12==0 ? FName("loadout.mines") : FName("loadout.torpedoes"); Ship->ApplyFit(Fit,true); Ship->Definition.AIAbilities=true;}
}

void AVTShip::OnRep_Autopilot() {
 Movement->Pending.Reset();Movement->RenderCorrection=FVector2D::ZeroVector;Movement->RenderHeadingCorrection=0;
 if(auto* PC=Cast<AVTController>(GetController())){PC->LocalIntent=FVTPilotIntent();PC->SendAccumulator=0;}
}
void AVTController::ToggleAutopilot(){if(auto* Ship=Cast<AVTShip>(GetPawn()))ServerSetAutopilot(!Ship->Autopilot);}
void AVTController::ServerSetAutopilot_Implementation(bool Enabled) {
 auto* Ship=Cast<AVTShip>(GetPawn());auto* PS=GetPlayerState<AVTPlayerState>();
 if(!Ship||Ship->IsNPC||!PS||!PS->Profile.IsValid()||(Enabled&&(Ship->Docked||Ship->Disabled)))return;
 if(Ship->Autopilot==Enabled)return;Ship->Autopilot=Enabled;Ship->Intent=FVTPilotIntent();Ship->InputQueue.Reset();Ship->Brain.Action=-1;Ship->Brain.Shoulder=-1;Ship->Brain.Thumb=-1;Ship->Brain.AimLock=0;Ship->Brain.ThumbTravel=0;Ship->Brain.WarpPrime=0;
 Ship->Combat->WarpHeld=false;Ship->Combat->TorpedoHeld=false;Ship->Combat->EquipmentState.Locks.Reset();Ship->Combat->EquipmentState.LockElapsed=0;
 Ship->OnRep_Autopilot();Ship->ForceNetUpdate();
}

void AVTController::CancelFlight(const FInputActionValue& Value,int32 Index) {
 if(Index==3||Index==4){LocalIntent.Buttons&=~(Index==3?(VTButtons::AimPort|VTButtons::Port):(VTButtons::AimStarboard|VTButtons::Starboard));}
 else ReadFlight(Value,Index);
}

void VTNotifyHUD(UWorld* World) {
 if(!World||IsRunningCommandlet()) return;
 for(auto It=World->GetPlayerControllerIterator();It;++It) if(auto* PC=Cast<AVTController>(It->Get())) if(PC->IsLocalController()&&PC->UI) PC->UI->RequestRefresh();
}
void AVTShip::OnRep_UIState() {VTNotifyHUD(GetWorld());}
void AVTPlayerState::OnRep_UIState() {VTNotifyHUD(GetWorld());}
void AVTGameState::OnRep_UIState() {VTNotifyHUD(GetWorld());}
void AVTController::UpdateInputContexts() {
 if(!IsLocalController()||!GetLocalPlayer()) return;
 auto* Ship=Cast<AVTShip>(GetPawn()); int32 State=UI&&UI->MenuOpen ? 1 : Ship&&Ship->Docked ? 2 : 0;
 if(Ship&&AudioListenerRoot.Get()!=Ship->GetRootComponent()) {AudioListenerRoot=Ship->GetRootComponent(); SetAudioListenerOverride(Ship->GetRootComponent(),FVector::ZeroVector,FRotator::ZeroRotator);}
 if(State==InputContextState) return;
 InputContextState=State; LocalIntent=FVTPilotIntent(); GamepadAim=FVector2D::ZeroVector;
 if(auto* Sub=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer())) {
  for(auto* Mapping:{FlightMapping.Get(),MenuMapping.Get(),DockedMapping.Get()}) if(Mapping) Sub->RemoveMappingContext(Mapping);
  auto* Mapping=State==1 ? MenuMapping.Get() : State==2 ? DockedMapping.Get() : FlightMapping.Get();
  if(Mapping) Sub->AddMappingContext(Mapping,State==0 ? 0 : 10);
 }

}

bool AVTController::RemapControl(FName MappingName,FKey NewKey) {
 if(!GetLocalPlayer()||!NewKey.IsValid()) return false;
 auto* Sub=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());auto* Settings=Sub ? Sub->GetUserSettings() : nullptr;
 if(!Settings) return false;
 FMapPlayerKeyArgs Args;Args.MappingName=MappingName;Args.NewKey=NewKey;Args.Slot=EPlayerMappableKeySlot::First;
 FGameplayTagContainer Failures;Settings->MapPlayerKey(Args,Failures);if(!Failures.IsEmpty())return false;
 Settings->AsyncSaveSettings();Sub->RequestRebuildControlMappings();VTNotifyHUD(GetWorld());return true;
}

void AVTController::ToggleHUD(bool Chart){if(UI){if(Chart)UI->ToggleChart();else UI->ToggleControls();}}
