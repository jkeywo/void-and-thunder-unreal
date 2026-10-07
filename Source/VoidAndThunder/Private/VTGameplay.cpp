#include "VTGameplay.h"
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
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "VTSaveSubsystem.h"
#include "GameFramework/GameSession.h"

FGameplayAttribute UVTAttributes::HullAttribute() { return FGameplayAttribute(FindFProperty<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UVTAttributes, Hull))); }
FGameplayAttribute UVTAttributes::BatteryAttribute() { return FGameplayAttribute(FindFProperty<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UVTAttributes, Battery))); }
void UVTAttributes::OnRep_Hull(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UVTAttributes, Hull, Old); }
void UVTAttributes::OnRep_Battery(const FGameplayAttributeData& Old) { GAMEPLAYATTRIBUTE_REPNOTIFY(UVTAttributes, Battery, Old); }
void UVTAttributes::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);
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
 if (S->Docked || S->Anchored) return;
 auto* Sim = GetWorld()->GetSubsystem<UVTSimulation>();
 const float Reverse = Sim->Data ? Sim->Data->Rules.ReverseThrottle : 0.25f;
 Previous = Motion;
 FVTPilotIntent Effective=S->Disabled ? FVTPilotIntent() : Value;
 VT::HelmStep(Motion, S->Definition.Stats, Effective, Reverse, VT::Step);
 if (Predict && Sim->Data && Sim->Data->Systems.IsValidIndex(S->SystemIndex)) {
  float Radius = Sim->Data->Systems[S->SystemIndex].Radius;
  const float Length = float(Motion.Position.Size());
  if (Length > Radius) Motion.Velocity -= Motion.Position.GetSafeNormal() * ((Length - Radius) * Sim->Data->Rules.BoundsSpring * VT::Step);
 }
 Motion.Ack = Value.Sequence;
 Motion.SimulationTime=Predict ? Motion.SimulationTime+VT::Step : Sim->SimulationTime;
 if (Predict) {
  Pending.Add({Value});
  if (Pending.Num() > 256) Pending.RemoveAt(0, Pending.Num() - 256);
 } else Authority = Motion;
}
void UVTShipMovement::OnRep_Authority() {
 AVTShip* S = CastChecked<AVTShip>(GetOwner());
 if(ReplicaSystem!=S->SystemIndex) {ReplicaFrames.Reset(); ReplicaSystem=S->SystemIndex;}
 LastAuthorityReceived=FPlatformTime::Seconds(); ReplicaFrames.Add(Authority);
 if(ReplicaFrames.Num()>8) ReplicaFrames.RemoveAt(0);
 if (!S->IsLocallyControlled()) { Previous = Motion; Motion = Authority; return; }
 Pending.RemoveAll([this](const FPending& P) { return int32(P.Intent.Sequence - Authority.Ack) <= 0; });
 Motion = Authority;
 auto Replay = Pending;
 Pending.Reset();
 for (const auto& P : Replay) Step(P.Intent, true);
}
void UVTShipMovement::ApplyPose() {
 AVTShip* S = CastChecked<AVTShip>(GetOwner());
 const double Alpha = FMath::Clamp(GetWorld()->GetSubsystem<UVTSimulation>()->Accumulator / VT::Step, 0., 1.);
 FVTMotion Render = Motion;
 if (!S->IsLocallyControlled()) {
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
}
void UVTShipMovement::TickComponent(float Dt, ELevelTick Tick, FActorComponentTickFunction* Fn) { Super::TickComponent(Dt, Tick, Fn); ApplyPose(); }
void UVTShipMovement::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const { Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(UVTShipMovement, Authority); }

AVTShip::AVTShip() {
 bReplicates = true; SetReplicateMovement(false); SetNetUpdateFrequency(20); SetMinNetUpdateFrequency(10);
 Mesh = CreateDefaultSubobject<UStaticMeshComponent>("HullMesh"); RootComponent = Mesh;
 Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
 UStaticMesh* HullMesh = Definition.Mesh.LoadSynchronous();
 if (!HullMesh) {
  HullMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
  Mesh->SetRelativeScale3D(FVector(40,18,8));
 }
 if(HullMesh&&HullMesh->GetPathName()==TEXT("/Engine/BasicShapes/Cube.Cube")) Mesh->SetRelativeScale3D(FVector(40,18,8));
 Mesh->SetStaticMesh(HullMesh);
 Combat->Initialize();
}
void AVTShip::EndPlay(const EEndPlayReason::Type Reason) {
 if (GetWorld()) if (auto* Sim = GetWorld()->GetSubsystem<UVTSimulation>()) Sim->Ships.Remove(this);
 Super::EndPlay(Reason);
}
void AVTShip::ServerIntent_Implementation(FVTPilotIntent Value) {
 if (!VT::ValidIntent(Value) || int32(Value.Sequence - LastReceived) <= 0 || Value.Sequence - LastReceived > 256) return;
 LastReceived = Value.Sequence; Intent = Value; LastInputTime = GetWorld()->GetRealTimeSeconds();
}
bool AVTShip::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const {
 const APlayerController* PC = Cast<APlayerController>(RealViewer);
 const AVTShip* Viewer = PC ? Cast<AVTShip>(PC->GetPawn()) : Cast<AVTShip>(ViewTarget);
 return Viewer && Viewer->SystemIndex == SystemIndex;
}
void AVTShip::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 DOREPLIFETIME(AVTShip, IsNPC); DOREPLIFETIME(AVTShip, SystemIndex); DOREPLIFETIME(AVTShip, ClassId); DOREPLIFETIME(AVTShip, Faction);
 DOREPLIFETIME(AVTShip, PersistentId); DOREPLIFETIME(AVTShip, Docked); DOREPLIFETIME(AVTShip, Disabled);
 DOREPLIFETIME(AVTShip, PortReload); DOREPLIFETIME(AVTShip, StarboardReload);
}

AVTShipAI::AVTShipAI() { PrimaryActorTick.bCanEverTick = false; }
void AVTShipAI::Decide(float Dt) {
 AVTShip* Ship = Cast<AVTShip>(GetPawn()); if (!Ship || Ship->Docked || Ship->Disabled) return;
 auto* Sim = GetWorld()->GetSubsystem<UVTSimulation>();
 const FVector2D P = Ship->Movement->Motion.Position;
 const float Time = float(Sim->SimulationTime);
 const FVector2D Target = FVector2D(FMath::Cos(Time * 0.03f + Ship->SystemIndex), FMath::Sin(Time * 0.03f + Ship->SystemIndex)) * 650;
 const FVector2D Direction = (Target - P).GetSafeNormal();
 const float Error = FMath::UnwindRadians(FMath::Atan2(Direction.Y, Direction.X) - Ship->Movement->Motion.Heading);
 Ship->Intent.Turn = FMath::Clamp(Error * 2.5f, -1.f, 1.f);
 Ship->Intent.Throttle = FMath::Abs(Error) > 1.2f ? 0.3f : 0.7f;
 Ship->Intent.Aim = Direction;
}

void AVTPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 DOREPLIFETIME_CONDITION(AVTPlayerState, Credits, COND_OwnerOnly); DOREPLIFETIME_CONDITION(AVTPlayerState, Boarded, COND_OwnerOnly);
 DOREPLIFETIME(AVTPlayerState, Profile); DOREPLIFETIME_CONDITION(AVTPlayerState, Reputation, COND_OwnerOnly); DOREPLIFETIME_CONDITION(AVTPlayerState, Heat, COND_OwnerOnly);
}
void AVTGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
 Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AVTGameState, SimulationTime); DOREPLIFETIME(AVTGameState, Populations);
}
AVTController::AVTController() { bShowMouseCursor = true; }
void AVTController::BeginPlay() {
 Super::BeginPlay();
 if (IsLocalController()) {
  FlightMapping = LoadObject<UInputMappingContext>(nullptr,TEXT("/Game/Input/IMC_Flight.IMC_Flight"));
  if (FlightMapping) if (auto* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer())) Sub->AddMappingContext(FlightMapping, 0);
 }
}
void AVTController::ReadFlight(const FInputActionValue& Value, int32 Index) {
 if (Index == 0) LocalIntent.Throttle = Value.Get<float>();
 else if (Index == 1) LocalIntent.Turn = Value.Get<float>();
 else if (Index == 2) {
  FVector2D V = Value.Get<FVector2D>(); if (V.SizeSquared() > 0.04) {LocalIntent.Aim = V.GetSafeNormal(); UsingGamepadAim=true;}
 } else {
  const uint16 Bit = uint16(1 << (Index - 3));
  if (Value.Get<bool>()) LocalIntent.Buttons |= Bit; else LocalIntent.Buttons &= ~Bit;
 }
}
void AVTController::SetupInputComponent() {
 Super::SetupInputComponent();
 auto* Input = Cast<UEnhancedInputComponent>(InputComponent);
 if (!Input) return;
 const TCHAR* Names[] = {TEXT("Throttle"),TEXT("Turn"),TEXT("Aim"),TEXT("Port"),TEXT("Starboard"),TEXT("EMP"),TEXT("Torpedo"),TEXT("Warp"),TEXT("Boost"),TEXT("Brace"),TEXT("Interact"),TEXT("Mine"),TEXT("PointDefense")};
 for (int32 I=0; I<UE_ARRAY_COUNT(Names); ++I) {
  const FString Path = FString::Printf(TEXT("/Game/Input/IA_%s.IA_%s"), Names[I], Names[I]);
  auto* Action = LoadObject<UInputAction>(nullptr,*Path); Actions.Add(Action);
  if (Action) {
   Input->BindAction(Action,ETriggerEvent::Triggered,this,&AVTController::ReadFlight,I);
   Input->BindAction(Action,ETriggerEvent::Completed,this,&AVTController::ReadFlight,I);
   Input->BindAction(Action,ETriggerEvent::Canceled,this,&AVTController::ReadFlight,I);
  }
 }
}
void AVTController::PlayerTick(float Dt) {
 Super::PlayerTick(Dt);
 ValidationInput(Dt);
 AVTShip* Ship = Cast<AVTShip>(GetPawn()); if (!IsLocalController() || !Ship) return;
 float MouseX=0,MouseY=0; GetInputMouseDelta(MouseX,MouseY); if(FMath::Abs(MouseX)+FMath::Abs(MouseY)>0.1f) UsingGamepadAim=false;
 FVector Origin, Direction;
 if (!UsingGamepadAim && DeprojectMousePositionToWorld(Origin, Direction) && FMath::Abs(Direction.Z) > 0.0001) {
  const FVector Hit = Origin + Direction * (-Origin.Z / Direction.Z);
  const FVector Delta = Hit - Ship->GetActorLocation();
  if (Delta.SizeSquared2D() > 1) LocalIntent.Aim = FVector2D(Delta.X, -Delta.Y).GetSafeNormal();
 }
 SendAccumulator += Dt;
 while (SendAccumulator >= VT::Step) {
  SendAccumulator -= VT::Step;
  LocalIntent.Sequence = ++NextSequence;
  if (Ship->HasAuthority()) { Ship->Intent = LocalIntent; Ship->LastInputTime = GetWorld()->GetRealTimeSeconds(); }
  else { Ship->Movement->Step(LocalIntent, true); Ship->ServerIntent(LocalIntent); }
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
 Data = LoadObject<UVTGameData>(nullptr,TEXT("/Game/Data/DA_GameData.DA_GameData"));
}
AVTShip* UVTSimulation::SpawnShip(FName Id, int32 System, const FVTMotion& Motion, bool NPC, FName Faction) {
 AVTShip* Ship = GetWorld()->SpawnActorDeferred<AVTShip>(AVTShip::StaticClass(),FTransform(VT::ToWorld(Motion.Position,System)),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
 Ship->Faction = Faction; Ship->IsNPC = NPC; Ship->InitializeShip(Id,System,Motion);
 UGameplayStatics::FinishSpawningActor(Ship,FTransform(VT::ToWorld(Motion.Position,System)));
 if (NPC) Ship->SpawnDefaultController();
 return Ship;
}
void UVTSimulation::Bootstrap(int32 Population) {
 if (!Data || GetWorld()->GetNetMode() == NM_Client) return;
 if (Bootstrapped && Population < 0) return;
 auto Existing = Ships;
 for (AVTShip* S : Existing) if (IsValid(S) && S->IsNPC) { if(S->Controller) S->Controller->Destroy(); S->Destroy(); }
 Bootstrapped = true;
 uint32 Random=uint32(WorldSeed);
 for (int32 I=0; I<Data->Systems.Num(); ++I) {
  const auto& Def = Data->Systems[I];
  const int32 Civilians = Def.Security == 2 ? 4 : Def.Security == 1 ? 2 : 1;
  const int32 Patrols = Def.Owner.IsNone() ? 0 : Def.Security == 2 ? 3 : Def.Security == 1 ? 1 : 0;
  const int32 Danger = FMath::RoundToInt(Def.Danger * 4);
  const int32 Count = Population >= 0 ? Population / Data->Systems.Num() + (I < Population % Data->Systems.Num() ? 1 : 0) : Civilians + Patrols + Danger;
  for (int32 N=0; N<Count; ++N) {
   const float Angle = VT::LcgNext(Random)*2*PI;
   const float Radius = 200+VT::LcgNext(Random)*(Def.Radius*0.7f-200);
   FVTMotion Motion; Motion.Position = FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*Radius; Motion.Heading = Angle;
   auto* NPC=SpawnShip("house_patrol",I,Motion,true,N<Civilians ? FName("Guild") : N<Civilians+Patrols ? Def.Owner : FName("Freebooters"));
   NPC->Invulnerable=Population>=0;
  }
 }
}
void UVTSimulation::Tick(float Dt) {
 if (GetWorld()->GetNetMode() == NM_Client) { Accumulator = FMath::Fmod(Accumulator + Dt,double(VT::Step)); ValidationTick(); return; }
 if (!GetWorld()->HasBegunPlay()) return;
 Accumulator += Dt;
 int32 Steps = 0;
 while (Accumulator >= VT::Step && Steps++ < 16) { Accumulator -= VT::Step; FixedStep(); }
 ValidationTick();
}
void UVTSimulation::FixedStep() {
 const double Start = FPlatformTime::Seconds();
 SimulationTime += VT::Step;
 SystemShips.SetNum(Data->Systems.Num());
 for(auto& Bucket:SystemShips) Bucket.Reset();
 for(AVTShip* S:Ships) if(IsValid(S)&&SystemShips.IsValidIndex(S->SystemIndex)) SystemShips[S->SystemIndex].Add(S);
 for (AVTShip* S : Ships) if (IsValid(S)) {
  if (auto* Brain = Cast<AVTShipAI>(S->Controller)) Brain->Decide(VT::Step);
  else if (GetWorld()->GetRealTimeSeconds() - S->LastInputTime > 0.25) { S->Intent.Throttle=0; S->Intent.Turn=0; S->Intent.Buttons=0; }
 }
 for (AVTShip* S : Ships) if (IsValid(S)) S->Combat->SystemsStep();
 for (AVTShip* S : Ships) if (IsValid(S)) S->Movement->Step(S->Intent, false);
 ContactStep();
 // Bounds follows contacts, matching the legacy simulation ordering.
 for(AVTShip* S:Ships) if(IsValid(S)&&!S->Docked&&!S->Anchored) {
  float Length=float(S->Movement->Motion.Position.Size()), Radius=Data->Systems[S->SystemIndex].Radius;
  if(Length>Radius) S->Movement->Motion.Velocity-=S->Movement->Motion.Position.GetSafeNormal()*((Length-Radius)*Data->Rules.BoundsSpring*VT::Step);
 }
 for (AVTShip* S : Ships) if (IsValid(S)) S->Movement->Authority=S->Movement->Motion;
 for (AVTShip* S : Ships) if (IsValid(S)) S->Combat->WeaponsStep();
 // Queries remain inside each system. Swept contacts avoid fast shots tunnelling through hulls.
 auto Shots=Projectiles;
 for(AVTProjectile* Shot:Shots) if(IsValid(Shot)) {
  Shot->Previous=Shot->Position; Shot->Position+=Shot->Velocity*VT::Step; Shot->Remaining-=VT::Step;
  AVTShip* Hit=nullptr; double Best=DBL_MAX;
  for(AVTShip* S:SystemShips[Shot->SystemIndex]) if(IsValid(S)&&S!=Shot->Source&&!S->Docked) {
   if(Shot->Source&&Shot->Source->IsNPC&&S->IsNPC&&S->Faction==Shot->Source->Faction) continue;
   float R=S->Definition.Radius+Shot->Radius;
   if(VTCombat::SegmentDistanceSquared(Shot->Previous,Shot->Position,S->Movement->Motion.Position)<=R*R) {
    double Along=(S->Movement->Motion.Position-Shot->Previous).SizeSquared(); if(Along<Best) {Hit=S; Best=Along;}
   }
  }
  if(Hit) {Hit->Combat->Damage(Shot->Damage,Shot->Previous,Shot->Source); Shot->Destroy();}
  else if(Shot->Remaining<=0) Shot->Destroy();
 }
 auto Survivors=Ships;
 for(AVTShip* S:Survivors) if(IsValid(S)) {
  if(S->Attributes->Hull.GetCurrentValue()<=0) {
   if(S->IsNPC) {if(S->Controller) S->Controller->Destroy(); S->Destroy();}
   else if(auto* Mode=GetWorld()->GetAuthGameMode<AVTGameMode>()) Mode->RecoverShip(S);
  }
  else if(!S->Invulnerable&&S->Attributes->Hull.GetCurrentValue()<=S->Definition.Hull*Data->Rules.CrippleThreshold) {S->Disabled=true; S->Intent=FVTPilotIntent();}
 }
 PiracyStep();
 if (auto* State = GetWorld()->GetGameState<AVTGameState>()) {
  State->SimulationTime = SimulationTime;
  if (Data) {
   State->Populations.SetNumZeroed(Data->Systems.Num());
   for (AVTShip* S : Ships) if (IsValid(S) && State->Populations.IsValidIndex(S->SystemIndex)) ++State->Populations[S->SystemIndex];
  }
 }
 const double Ms = (FPlatformTime::Seconds()-Start)*1000;
 if (StepMilliseconds.Num() < 20000) StepMilliseconds.Add(Ms);
 if (auto* Save = GetWorld()->GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()) Save->Advance(VT::Step);
}

AVTGameMode::AVTGameMode() {
 DefaultPawnClass = AVTShip::StaticClass(); PlayerControllerClass = AVTController::StaticClass();
 PlayerStateClass = AVTPlayerState::StaticClass(); GameStateClass = AVTGameState::StaticClass();
}
void AVTGameMode::BeginPlay() { Super::BeginPlay(); GetWorld()->GetSubsystem<UVTSimulation>()->Bootstrap(); }
void AVTGameMode::PostLogin(APlayerController* NewPlayer) {
 Super::PostLogin(NewPlayer);
 if(auto* PC=Cast<AVTController>(NewPlayer)) PC->ClientIdentify(GetGameInstance()->GetSubsystem<UVTSaveSubsystem>()->WorldId);
}
void AVTGameMode::RestartPlayer(AController* Player) {
 auto* Sim = GetWorld()->GetSubsystem<UVTSimulation>(); Sim->Bootstrap();
 if (!Sim->Data || Sim->Data->Systems.IsEmpty()) return;
 FVTMotion Motion; Motion.Position = FVector2D(0,-200 - GetNumPlayers()*70);
 const int32 System = FMath::Max(0,Sim->Data->FindSystem(Sim->Data->StartSystem));
 AVTShip* Ship = Sim->SpawnShip("corsair_cruiser",System,Motion,false,"Corsairs");
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
 FVTMotion M; M.Position=FVector2D(0,-200);
 AVTShip* Replacement=Sim->SpawnShip(Ship->ClassId,Station,M,false,Ship->Faction);
 Replacement->LastReceived=Ship->LastReceived; Replacement->Movement->Motion.Ack=Ship->Movement->Motion.Ack; Replacement->Movement->Authority=Replacement->Movement->Motion;
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
void UVTGameInstance::Host() { UGameplayStatics::OpenLevel(this,"/Game/Maps/Sandbox",true,"listen"); }
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
 ServerIdentify(Save->Personal->Profile,Record ? Record->Token : FGuid());
}
void AVTController::ServerIdentify_Implementation(FGuid Profile,FGuid Token) {
 auto* PS=GetPlayerState<AVTPlayerState>(); if(!PS||PS->Profile.IsValid()) return;
 auto* Save=GetGameInstance()->GetSubsystem<UVTSaveSubsystem>();
 bool Valid=Profile.IsValid();
 for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It) if(It->Get()!=this) if(auto* Other=It->Get()->GetPlayerState<AVTPlayerState>()) if(Other->Profile==Profile) Valid=false;
 const auto* Record=Save->PlayerRecords.FindByPredicate([Profile](const FVTSavedPlayer& R){return R.Profile==Profile;});
 if(Record&&Record->Token!=Token) Valid=false;
 if(!Valid) {if(auto* Mode=GetWorld()->GetAuthGameMode<AVTGameMode>()) Mode->GameSession->KickPlayer(this,FText::FromString(TEXT("Profile already connected or reconnect token invalid."))); return;}
 PS->Profile=Profile;
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
 if(auto* Ship=Cast<AVTShip>(GetPawn())) if(Ship->Disabled) if(auto* Mode=GetWorld()->GetAuthGameMode<AVTGameMode>()) Mode->RecoverShip(Ship);
}
void UVTGameInstance::Shutdown() {
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
