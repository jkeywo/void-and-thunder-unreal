#pragma once
#include "CoreMinimal.h"
#include "VTTypes.generated.h"

USTRUCT(BlueprintType)
struct FVTLandmarkDefinition {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVector2D Position=FVector2D::ZeroVector;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float Radius=120;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Kind=0;
 FVTLandmarkDefinition()=default;
 FVTLandmarkDefinition(FVector2D P,float R,int32 K):Position(P),Radius(R),Kind(K){}
};
USTRUCT(BlueprintType)
struct FVTFeelCamera {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float aim_dist=0.620000005f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float aim_lerp=9.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float aim_pitch=0.280000001f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float base_fov=0.785398185f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float boost_fov_gain=0.140000001f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float dist_lerp=6.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float distance=480.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float focus_lerp=10.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float fov_lerp=5.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float height=110.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float kick_decay=9.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float kick_max=60.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float lead_max=120.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float lead_secs=0.25f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float look_pitch_rate=1.79999995f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float look_yaw_rate=2.4000001f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float menu_orbit_rate=0.150000006f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float pitch_base=0.550000012f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float pitch_max=1.20000005f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float pitch_min=0.219999999f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float recenter_delay=3.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float recenter_delay_pad=0.100000001f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float recenter_lerp=1.60000002f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float shake_freq=34.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float shake_magnitude=26.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float topdown_margin=1.12f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float topdown_pitch=1.5f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float trauma_decay=1.39999998f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float yaw_lerp=2.5f;
};

USTRUCT(BlueprintType)
struct FVTFeelControls {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float aim_cursor_max=1300.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float aim_cursor_rate=780.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float deadzone=0.0500000007f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float mouse_aim_sens=0.00319999992f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float saturation=0.899999976f;
};

USTRUCT(BlueprintType)
struct FVTFeelImpact {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float debris_count=8.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float debris_life=0.349999994f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float debris_speed_max=220.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float debris_speed_min=90.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float flash_time=0.0799999982f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float kill_hitstop=0.0599999987f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float nearby_death_trauma=0.449999988f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float nearby_hit_trauma=0.100000001f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float own_death_trauma=0.899999976f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float own_hit_hitstop=0.0399999991f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float own_hit_kick=22.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float own_hit_trauma=0.180000007f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float own_hit_trauma_per_damage=0.00999999978f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float rumble_death_secs=0.550000012f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float rumble_hit_secs=0.159999996f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float rumble_scale=1.60000002f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float shake_range=900.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float spark_cone=1.20000005f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float spark_count=5.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float spark_life=0.180000007f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float spark_speed_max=180.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float spark_speed_min=70.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float wreck_drift=0.550000012f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float wreck_life=1.10000002f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float wreck_spin=3.0f;
};

USTRUCT(BlueprintType)
struct FVTFeelRings {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float drop=8.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float opacity=0.5f;
};

USTRUCT(BlueprintType)
struct FVTFeelTime {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float aim_timescale=0.100000001f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float battery_drain_per_sec=1.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float battery_max=5.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float battery_recharge_per_sec=1.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float hitstop_timescale=0.0500000007f;
};

USTRUCT(BlueprintType)
struct FVTFeelTrailsEngine {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<float> cool={0.119999997f,0.319999993f,0.899999976f};
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<float> hot={0.779999971f,0.930000007f,1.0f};
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float lifetime=0.899999976f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float max_crumbs=64.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float min_step=5.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float width=5.0f;
};

USTRUCT(BlueprintType)
struct FVTFeelTrailsTorpedo {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<float> cool={0.899999976f,0.280000001f,0.0500000007f};
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<float> hot={1.0f,0.860000014f,0.550000012f};
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float lifetime=0.449999988f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float max_crumbs=48.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float min_step=4.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float width=3.5f;
};

USTRUCT(BlueprintType)
struct FVTFeelTrails {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float boost_width=1.70000005f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTFeelTrailsEngine engine;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float nacelle_y=6.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float stern_x=-20.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float throttle_deadzone=0.0500000007f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTFeelTrailsTorpedo torpedo;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float torpedo_tail_z=-13.0f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float width_falloff=0.550000012f;
};

USTRUCT(BlueprintType)
struct FVTFeel {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTFeelCamera camera;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTFeelControls controls;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTFeelImpact impact;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTFeelRings rings;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTFeelTime time;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTFeelTrails trails;
};
USTRUCT(BlueprintType)
struct FVTShieldBanks {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float X = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float Y = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float Z = 0;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float W = 0;
 FVTShieldBanks() = default;
 FVTShieldBanks(float Fore,float Aft,float Port,float Starboard):X(Fore),Y(Aft),Z(Port),W(Starboard) {}
 float& operator[](int32 I) {check(I>=0&&I<4); return I==0 ? X : I==1 ? Y : I==2 ? Z : W;}
 float operator[](int32 I) const {check(I>=0&&I<4); return I==0 ? X : I==1 ? Y : I==2 ? Z : W;}
};
USTRUCT(BlueprintType)
struct FVTShipStats {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float Thrust = 115;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float TurnRate = 0.6875f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxSpeed = 127.5f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float ForwardDrag = 0.9f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float LateralDrag = 4.5f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float TurnRateSlow = 1.55f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float TurnRateFast = 0.55f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite) float TurnAccel = 4;
};
USTRUCT(BlueprintType)
struct FVTPilotIntent {
 GENERATED_BODY()
 UPROPERTY(BlueprintReadWrite) float Throttle = 0;
 UPROPERTY(BlueprintReadWrite) float Turn = 0;
 UPROPERTY(BlueprintReadWrite) FVector2D Aim = FVector2D(0, 1);
 UPROPERTY(BlueprintReadWrite) FVector2D CursorOffset=FVector2D::ZeroVector;
 UPROPERTY() uint16 Buttons = 0;
 UPROPERTY() uint32 Sequence = 0;
};
UENUM(BlueprintType)
enum class EVTDevice : uint8 { Port, Starboard, EMP, Torpedo, Microwarp, Boost, Brace, Board, Mine, PointDefense };
namespace VTButtons {
 constexpr uint16 Port=1, Starboard=2, EMP=4, Torpedo=8, Warp=16, Boost=32, Brace=64, Interact=128, Mine=256, PointDefense=512;
}
USTRUCT(BlueprintType)
struct FVTMotion {
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) FVector2D Position = FVector2D::ZeroVector;
 UPROPERTY(BlueprintReadOnly) FVector2D Velocity = FVector2D::ZeroVector;
 UPROPERTY(BlueprintReadOnly) float Heading = 0;
 UPROPERTY(BlueprintReadOnly) float Omega = 0;
 UPROPERTY() uint32 Ack = 0;
 UPROPERTY() double SimulationTime = 0;
};
USTRUCT(BlueprintType)
struct FVTPilotTuning {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float aim_lock=1;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float board_base=0.349999994f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float board_repair_pull=0.449999988f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float board_resupply_pull=0.400000006f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float board_safe_threat=1;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float brace_range_frac=0.600000024f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float commit_bonus=0.150000006f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float emp_busy_interest=0.349999994f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float emp_throttle=0.349999994f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float mine_lead=3;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float mine_reserve=0.25f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float presentation_floor=0.400000006f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float ram_hull_floor=0.5f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float reloading_interest=0.550000012f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float scarcity_floor=0.349999994f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float screen_base=0.550000012f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float screen_danger=1;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float screen_raise=0.349999994f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float shield_bias=0.349999994f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float shield_worth=0.699999988f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float thumb_travel=0.200000003f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float torpedo_aim_lock=2;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float torpedo_standoff=0.800000012f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float w_board=1.10000002f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float w_broadside=1;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float w_disengage=1.79999995f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float w_emp=0.699999988f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float w_microwarp=1.39999998f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float w_ram=0.899999976f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float w_torpedo=0.949999988f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float warp_escape_threat=2;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float warp_reposition_interest=0.600000024f;
};
USTRUCT(BlueprintType)
struct FVTAITuning {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTPilotTuning pilot;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float station_lead_secs=0.75f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float station_throttle=0.300000012f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float surround_count=2;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float surround_radius=400;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float torpedo_min_volley=3;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float turn_ease=0.75f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float turn_gain=2.5f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float warp_prime=0.400000006f;
};
USTRUCT(BlueprintType)
struct FVTWorldTuning {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float alert_ttl=20;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float avenge_reputation_bonus=8;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float dock_refusal_threshold=-40;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float heat_decay_per_sec=1.5f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float heat_engage_threshold=50;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float heat_per_damage=0.400000006f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float heat_to_reputation_rate=0.150000006f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float hostile_threshold=-20;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float recent_attack_memory=15;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float scan_decay_rate=0.800000012f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float scan_rate=0.400000006f;
};
USTRUCT(BlueprintType)
struct FVTEquipmentDefinition {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool EMP=false;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float EMPCooldown=0.4f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float EMPDrain=0.7f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float EMPRange=620;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float EMPSwivel=0.34906584f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float EMPArc=1.57079637f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float EMPSpeed=360;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float EMPFraction=0.25f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool Boost=false;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float BoostMultiplier=1.6f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float BoostDrain=1;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool PointDefense=false;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float PDRadius=190;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float PDRate=4;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float PDDrain=0.8f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool Torpedoes=false;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float TorpedoDamage=22;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float LockInterval=0.5f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float LockRadius=112.5f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 TorpedoMagazine=20;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Tubes=6;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float TubeReload=1.5f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float TorpedoRange=506;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float TorpedoSpeed=260;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float TorpedoTurn=3.375f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 TorpedoResupplyMin=0;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 TorpedoResupplyMax=2;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool Warp=false;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float WarpCooldown=20;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float WarpRange=506;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool Mines=false;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float MineCooldown=1.2f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float MineDamage=14;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MineMagazine=8;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float MineRadius=52;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float MineTTL=6;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MineResupplyMin=1;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MineResupplyMax=3;
};
USTRUCT(BlueprintType)
struct FVTEquipmentState {
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) float Loaded=0;
 UPROPERTY(BlueprintReadOnly) int32 TorpedoMagazine=0;
 UPROPERTY(BlueprintReadOnly) int32 MineMagazine=0;
 UPROPERTY(BlueprintReadOnly) float EMPAim=0;
 UPROPERTY(BlueprintReadOnly) float LockElapsed=0;
 UPROPERTY(BlueprintReadOnly) float LaunchTimer=0;
 UPROPERTY(BlueprintReadOnly) float EMPCooldown=0;
 UPROPERTY(BlueprintReadOnly) float MineCooldown=0;
 UPROPERTY(BlueprintReadOnly) float WarpCooldown=0;
 UPROPERTY(BlueprintReadOnly) float PDCooldown=0;
 UPROPERTY(BlueprintReadOnly) bool LaunchFlip=false;
 UPROPERTY(BlueprintReadOnly) TArray<FGuid> Locks;
 UPROPERTY(BlueprintReadOnly) TArray<FGuid> LaunchQueue;
};
UENUM()
enum class EVTProjectileKind : uint8 { Cannon, EMP, Torpedo, Mine };
USTRUCT(BlueprintType)
struct FVTFactionRelation {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName A;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName B;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float Standing=0;
};
USTRUCT()
struct FVTBrainState {
 GENERATED_BODY()
 UPROPERTY() int32 Action=-1;
 UPROPERTY() int32 Shoulder=-1;
 UPROPERTY() int32 Thumb=-1;
 UPROPERTY() float AimLock=0;
 UPROPERTY() float ThumbTravel=0;
 UPROPERTY() float WarpPrime=0;
 UPROPERTY() FGuid ScanTarget;
 UPROPERTY() float ScanProgress=0;
 UPROPERTY() FVector2D Alert=FVector2D::ZeroVector;
 UPROPERTY() float AlertTTL=0;
 UPROPERTY() float DistressTimer=-1;
 UPROPERTY() bool DistressSent=false;
 UPROPERTY() FGuid LastAttacker;
 UPROPERTY() FGuid LastAttackerProfile;
 UPROPERTY() FName LastAttackerFaction;
 UPROPERTY() double AttackTime=-100;
};
USTRUCT(BlueprintType)
struct FVTShipDefinition {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FVTShipStats Stats;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTEquipmentDefinition Equipment;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float EMPResist=100;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float EMPRecovery=14;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Mounts=2;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Crewed=2;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float AIEngageRange=300;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float AIFireArc=0.35f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float AIFleeFraction=0.25f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool AIAim=false;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool AIAbilities=false;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float Hull = 50;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float Radius = 26;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float Damage = 12;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float Reload = 10;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float MuzzleSpeed = 325;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float Arc = 1.1780972f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float ChargeTime = 0;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Guns = 3;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float BatteryMax = 3;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float BatteryRecharge = 0.6f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ShieldArcs = 2;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FVTShieldBanks ShieldMax = FVTShieldBanks(0,0,0,0);
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float ShieldRegen = 7;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float ShieldDelay = 2.5f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UStaticMesh> Mesh;
};
UENUM(BlueprintType)
enum class EVTLoadoutSlot : uint8 { Broadside, Battery, Special };
USTRUCT(BlueprintType)
struct FVTLoadoutSelection {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FName Broadside;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FName Battery;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FName Special;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FName> Batteries;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FName> Specials;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<EVTDevice> CrewedDevices;
};
USTRUCT(BlueprintType)
struct FVTLoadoutOption {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Id;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) EVTLoadoutSlot Slot=EVTLoadoutSlot::Broadside;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTShipDefinition Definition;
};
USTRUCT(BlueprintType)
struct FVTScenarioSpawn {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName ClassId;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Faction;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVector2D Position=FVector2D::ZeroVector;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float Heading=0;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool Anchored=false;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool Inert=false;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool Invulnerable=false;
};
USTRUCT(BlueprintType)
struct FVTScenarioDefinition {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Id;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVTScenarioSpawn Player;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FVTScenarioSpawn> Enemies;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool Director=false;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 BaseCount=2;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MaxWaves=3;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float BaseHull=100;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float HullPerWave=25;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName EnemyClass=TEXT("house_patrol");
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName EnemyFaction=TEXT("Houses");
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName FinaleClass=TEXT("house_bastion");
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 FinaleCount=1;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float Radius=1400;
};
USTRUCT(BlueprintType)
struct FVTSystemDefinition {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Owner;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Security = 0;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float Danger = 0;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float Radius = 1400;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) bool HasStation = false;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> Links;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector2D ChartPosition = FVector2D::ZeroVector;
};
USTRUCT(BlueprintType)
struct FVTRules {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float EngagementRange=506;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float RamDamagePerSpeed = 0.22f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float RamThreshold = 45;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float RamRestitution = 0.35f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float RamSeparation = 0.6f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float BoardRepairFraction = 0.1f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float HullLength = 40;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float MuzzleStandoff = 22;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float ReverseThrottle = 0.25f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float BoundsSpring = 3;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float BraceDamageFactor = 0.35f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float ProjectileTTL = 2.5f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float ProjectileRadius = 5;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float BoardRange = 95;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float BoardDwell = 3;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float CrippleThreshold = 0.25f;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float JumpRange = 120;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float JumpDwell = 6;
 UPROPERTY(EditAnywhere, BlueprintReadOnly) float AutosaveSeconds = 60;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 BoardingBounty=50;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 CreditsPerHeat=4;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float StationRadius=90;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVector2D StationPosition=FVector2D(300,300);
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float JumpEdgeFraction=0.95f;
};
namespace VT {
 constexpr float Step = 1.0f / 64.0f;
 VOIDANDTHUNDER_API void HelmStep(FVTMotion& Motion, const FVTShipStats& Stats, const FVTPilotIntent& Intent, float Reverse, float Dt);
 VOIDANDTHUNDER_API float LcgNext(uint32& Seed);
 VOIDANDTHUNDER_API bool SequenceAdvanceAllowed(uint32 Next,uint32 Last,double SecondsSinceInput);
 VOIDANDTHUNDER_API bool ValidIntent(const FVTPilotIntent& Intent);
 VOIDANDTHUNDER_API FVector ArenaOrigin(int32 System);
 VOIDANDTHUNDER_API FVector ToWorld(const FVector2D& Position, int32 System);
}
