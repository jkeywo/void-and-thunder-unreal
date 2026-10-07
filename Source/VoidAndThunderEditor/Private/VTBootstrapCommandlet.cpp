#include "VTBootstrapCommandlet.h"
#include "VTGameData.h"
#include "VTGameplay.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/Engine.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "VTUI.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"

static bool SaveAsset(UObject* Asset, const FString& Name) {
 Asset->SetFlags(RF_Public|RF_Standalone);
 UPackage* Package=Asset->GetOutermost(); Package->MarkAsFullyLoaded(); Package->MarkPackageDirty();
 FAssetRegistryModule::AssetCreated(Asset);
 const FString File=FPackageName::LongPackageNameToFilename(Name,Asset->IsA<UWorld>() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension());
 IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);
 FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_None;
 return UPackage::SavePackage(Package,Asset,*File,Args);
}
static float Number(const TSharedPtr<FJsonObject>& O,const TCHAR* Key,float Default) {
 double Value; return O->TryGetNumberField(Key,Value) ? float(Value) : Default;
}
static FVTEquipmentDefinition Equipment(const TSharedPtr<FJsonObject>& Loadout) {
 FVTEquipmentDefinition E;
 auto Read=[&](const TCHAR* Key,auto Fn) {const TSharedPtr<FJsonObject>* O=nullptr; if(Loadout->TryGetObjectField(Key,O)&&O&&O->IsValid()) Fn(*O);};
 Read(TEXT("emp"),[&](auto O){E.EMP=true; E.EMPCooldown=Number(O,TEXT("cooldown"),E.EMPCooldown); E.EMPDrain=Number(O,TEXT("drain_per_sec"),E.EMPDrain); E.EMPRange=Number(O,TEXT("range"),E.EMPRange); E.EMPSwivel=Number(O,TEXT("swivel_rate"),E.EMPSwivel); E.EMPArc=Number(O,TEXT("arc"),E.EMPArc); E.EMPSpeed=Number(O,TEXT("bolt_speed"),E.EMPSpeed); E.EMPFraction=Number(O,TEXT("bolt_damage_frac"),E.EMPFraction);});
 Read(TEXT("boost"),[&](auto O){E.Boost=true; E.BoostMultiplier=Number(O,TEXT("multiplier"),E.BoostMultiplier); E.BoostDrain=Number(O,TEXT("drain_per_sec"),E.BoostDrain);});
 Read(TEXT("point_defense"),[&](auto O){E.PointDefense=true; E.PDRadius=Number(O,TEXT("radius"),E.PDRadius); E.PDRate=Number(O,TEXT("rate"),E.PDRate); E.PDDrain=Number(O,TEXT("drain_per_sec"),E.PDDrain);});
 Read(TEXT("torpedoes"),[&](auto O){E.Torpedoes=true; E.TorpedoDamage=Number(O,TEXT("damage"),E.TorpedoDamage); E.LockInterval=Number(O,TEXT("lock_interval"),E.LockInterval); E.LockRadius=Number(O,TEXT("lock_radius"),E.LockRadius); E.TorpedoMagazine=int32(Number(O,TEXT("magazine_max"),E.TorpedoMagazine)); E.Tubes=int32(Number(O,TEXT("tubes_max"),E.Tubes)); E.TubeReload=Number(O,TEXT("reload_per_tube"),E.TubeReload); E.TorpedoRange=Number(O,TEXT("range"),E.TorpedoRange); E.TorpedoSpeed=Number(O,TEXT("speed"),E.TorpedoSpeed); E.TorpedoTurn=Number(O,TEXT("turn_rate"),E.TorpedoTurn); E.TorpedoResupplyMin=int32(Number(O,TEXT("resupply_min"),E.TorpedoResupplyMin)); E.TorpedoResupplyMax=int32(Number(O,TEXT("resupply_max"),E.TorpedoResupplyMax));});
 Read(TEXT("microwarp"),[&](auto O){E.Warp=true; E.WarpRange=Number(O,TEXT("range"),E.WarpRange); E.WarpCooldown=Number(O,TEXT("cooldown"),E.WarpCooldown);});
 Read(TEXT("mines"),[&](auto O){E.Mines=true; E.MineCooldown=Number(O,TEXT("cooldown"),E.MineCooldown); E.MineDamage=Number(O,TEXT("damage_per_sec"),E.MineDamage); E.MineMagazine=int32(Number(O,TEXT("magazine_max"),E.MineMagazine)); E.MineRadius=Number(O,TEXT("radius"),E.MineRadius); E.MineTTL=Number(O,TEXT("ttl"),E.MineTTL); E.MineResupplyMin=int32(Number(O,TEXT("resupply_min"),E.MineResupplyMin)); E.MineResupplyMax=int32(Number(O,TEXT("resupply_max"),E.MineResupplyMax));});
 return E;
}
UVTBootstrapCommandlet::UVTBootstrapCommandlet() { IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true; }
int32 UVTBootstrapCommandlet::Main(const FString& Params) {
 FString Json;
 const FString Input=FPaths::ProjectDir()/TEXT("Migration/resolved-baseline.json");
 if(!FFileHelper::LoadFileToString(Json,*Input)) { UE_LOG(LogTemp,Error,TEXT("Missing typed migration baseline: %s"),*Input); return 1; }
 TSharedPtr<FJsonObject> Root;
 if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root)) return 2;
 const FString DataPath=TEXT("/Game/Data/DA_GameData");
 TMap<FName,TSoftObjectPtr<UStaticMesh>> ExistingMeshes,ExistingFactionMeshes;
 if(auto* Existing=LoadObject<UVTGameData>(nullptr,TEXT("/Game/Data/DA_GameData.DA_GameData"))) {ExistingFactionMeshes=Existing->FactionMeshes; for(const auto& Ship:Existing->Ships) ExistingMeshes.Add(Ship.Id,Ship.Mesh);}
 UPackage* Package=CreatePackage(*DataPath);
 UVTGameData* Data=NewObject<UVTGameData>(Package,TEXT("DA_GameData"),RF_Public|RF_Standalone);
 Data->FactionMeshes=ExistingFactionMeshes;
 const auto ShipTable=Root->GetObjectField(TEXT("ships"));
 for(const auto& Item:ShipTable->GetArrayField(TEXT("classes"))) {
  auto Entry=Item->AsObject(); auto Class=Entry->GetObjectField(TEXT("class"));
  FVTShipDefinition Def; Def.Id=FName(Entry->GetStringField(TEXT("name"))); Def.Mesh=ExistingMeshes.FindRef(Def.Id);
  auto Stats=Class->GetObjectField(TEXT("stats"));
  Def.Stats.Thrust=Number(Stats,TEXT("thrust"),Def.Stats.Thrust);
  Def.Stats.TurnRate=Number(Stats,TEXT("turn_rate"),Def.Stats.TurnRate);
  Def.Stats.MaxSpeed=Number(Stats,TEXT("max_speed"),Def.Stats.MaxSpeed);
  Def.Stats.ForwardDrag=Number(Stats,TEXT("forward_drag"),Def.Stats.ForwardDrag);
  Def.Stats.LateralDrag=Number(Stats,TEXT("lateral_drag"),Def.Stats.LateralDrag);
  Def.Stats.TurnRateSlow=Number(Stats,TEXT("turn_rate_slow"),Def.Stats.TurnRateSlow);
  Def.Stats.TurnRateFast=Number(Stats,TEXT("turn_rate_fast"),Def.Stats.TurnRateFast);
  Def.Stats.TurnAccel=Number(Stats,TEXT("turn_accel"),Def.Stats.TurnAccel);
  Def.Hull=Number(Class,TEXT("hull"),Def.Hull);
  Def.Radius=Number(Class->GetObjectField(TEXT("collider")),TEXT("radius"),Def.Radius);
  auto Broadside=Class->GetObjectField(TEXT("loadout"))->GetObjectField(TEXT("broadside"));
  Def.Damage=Number(Broadside,TEXT("damage"),Def.Damage);
  Def.Reload=Number(Broadside,TEXT("cooldown"),Def.Reload);
  Def.MuzzleSpeed=Number(Broadside,TEXT("muzzle_speed"),Def.MuzzleSpeed);
  Def.Arc=Number(Broadside,TEXT("arc"),Def.Arc);
  Def.ChargeTime=Number(Broadside,TEXT("charge_time"),Def.ChargeTime);
  Def.Guns=int32(Number(Broadside,TEXT("guns"),Def.Guns));
  Def.Mesh=TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")));
  auto Loadout=Class->GetObjectField(TEXT("loadout"));
  Def.Equipment=Equipment(Loadout);
  auto Defense=Class->GetObjectField(TEXT("emp_defense")); Def.EMPResist=Number(Defense,TEXT("resist"),100); Def.EMPRecovery=Number(Defense,TEXT("recovery_per_sec"),14);
  auto AI=Class->GetObjectField(TEXT("ai")); Def.AIEngageRange=Number(AI,TEXT("engage_range"),300); Def.AIFireArc=Number(AI,TEXT("fire_arc"),0.35f); Def.AIFleeFraction=Number(AI,TEXT("flee_hull_frac"),0.25f); Def.AIAim=AI->GetBoolField(TEXT("aim_at_target")); Def.AIAbilities=AI->GetBoolField(TEXT("use_abilities"));
  const TSharedPtr<FJsonObject>* Playable=nullptr; if(Class->TryGetObjectField(TEXT("playable"),Playable)&&Playable&&Playable->IsValid()) {Def.Mounts=int32(Number(*Playable,TEXT("mounts"),2)); Def.Crewed=int32(Number(*Playable,TEXT("crewed"),2));}
  auto Battery=Loadout->GetObjectField(TEXT("battery"));
  Def.BatteryMax=Number(Battery,TEXT("max"),3); Def.BatteryRecharge=Number(Battery,TEXT("recharge_per_sec"),0.6f);
  auto Shield=Loadout->GetObjectField(TEXT("shield"));
  Def.ShieldArcs=int32(Number(Shield,TEXT("arcs"),2));
  Def.ShieldMax=FVTShieldBanks(Number(Shield,TEXT("fore_max"),0),Number(Shield,TEXT("aft_max"),0),Number(Shield,TEXT("port_max"),0),Number(Shield,TEXT("starboard_max"),0));
  Def.ShieldRegen=Number(Shield,TEXT("regen_per_sec"),7); Def.ShieldDelay=Number(Shield,TEXT("regen_delay"),2.5f);
  UE_LOG(LogTemp,Display,TEXT("Import %s shields %.3f %.3f"),*Def.Id.ToString(),Def.ShieldMax.X,Def.ShieldMax.Y);
  Data->Ships.Add(Def);
 }
 auto Tuning=Root->GetObjectField(TEXT("tuning"));
  Data->Feel.camera.aim_dist=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("aim_dist"),Data->Feel.camera.aim_dist);
 Data->Feel.camera.aim_lerp=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("aim_lerp"),Data->Feel.camera.aim_lerp);
 Data->Feel.camera.aim_pitch=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("aim_pitch"),Data->Feel.camera.aim_pitch);
 Data->Feel.camera.base_fov=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("base_fov"),Data->Feel.camera.base_fov);
 Data->Feel.camera.boost_fov_gain=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("boost_fov_gain"),Data->Feel.camera.boost_fov_gain);
 Data->Feel.camera.dist_lerp=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("dist_lerp"),Data->Feel.camera.dist_lerp);
 Data->Feel.camera.distance=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("distance"),Data->Feel.camera.distance);
 Data->Feel.camera.focus_lerp=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("focus_lerp"),Data->Feel.camera.focus_lerp);
 Data->Feel.camera.fov_lerp=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("fov_lerp"),Data->Feel.camera.fov_lerp);
 Data->Feel.camera.height=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("height"),Data->Feel.camera.height);
 Data->Feel.camera.kick_decay=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("kick_decay"),Data->Feel.camera.kick_decay);
 Data->Feel.camera.kick_max=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("kick_max"),Data->Feel.camera.kick_max);
 Data->Feel.camera.lead_max=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("lead_max"),Data->Feel.camera.lead_max);
 Data->Feel.camera.lead_secs=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("lead_secs"),Data->Feel.camera.lead_secs);
 Data->Feel.camera.look_pitch_rate=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("look_pitch_rate"),Data->Feel.camera.look_pitch_rate);
 Data->Feel.camera.look_yaw_rate=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("look_yaw_rate"),Data->Feel.camera.look_yaw_rate);
 Data->Feel.camera.menu_orbit_rate=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("menu_orbit_rate"),Data->Feel.camera.menu_orbit_rate);
 Data->Feel.camera.pitch_base=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("pitch_base"),Data->Feel.camera.pitch_base);
 Data->Feel.camera.pitch_max=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("pitch_max"),Data->Feel.camera.pitch_max);
 Data->Feel.camera.pitch_min=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("pitch_min"),Data->Feel.camera.pitch_min);
 Data->Feel.camera.recenter_delay=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("recenter_delay"),Data->Feel.camera.recenter_delay);
 Data->Feel.camera.recenter_delay_pad=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("recenter_delay_pad"),Data->Feel.camera.recenter_delay_pad);
 Data->Feel.camera.recenter_lerp=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("recenter_lerp"),Data->Feel.camera.recenter_lerp);
 Data->Feel.camera.shake_freq=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("shake_freq"),Data->Feel.camera.shake_freq);
 Data->Feel.camera.shake_magnitude=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("shake_magnitude"),Data->Feel.camera.shake_magnitude);
 Data->Feel.camera.topdown_margin=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("topdown_margin"),Data->Feel.camera.topdown_margin);
 Data->Feel.camera.topdown_pitch=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("topdown_pitch"),Data->Feel.camera.topdown_pitch);
 Data->Feel.camera.trauma_decay=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("trauma_decay"),Data->Feel.camera.trauma_decay);
 Data->Feel.camera.yaw_lerp=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("camera")),TEXT("yaw_lerp"),Data->Feel.camera.yaw_lerp);
 Data->Feel.controls.aim_cursor_max=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("controls")),TEXT("aim_cursor_max"),Data->Feel.controls.aim_cursor_max);
 Data->Feel.controls.aim_cursor_rate=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("controls")),TEXT("aim_cursor_rate"),Data->Feel.controls.aim_cursor_rate);
 Data->Feel.controls.deadzone=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("controls")),TEXT("deadzone"),Data->Feel.controls.deadzone);
 Data->Feel.controls.mouse_aim_sens=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("controls")),TEXT("mouse_aim_sens"),Data->Feel.controls.mouse_aim_sens);
 Data->Feel.controls.saturation=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("controls")),TEXT("saturation"),Data->Feel.controls.saturation);
 Data->Feel.impact.debris_count=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("debris_count"),Data->Feel.impact.debris_count);
 Data->Feel.impact.debris_life=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("debris_life"),Data->Feel.impact.debris_life);
 Data->Feel.impact.debris_speed_max=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("debris_speed_max"),Data->Feel.impact.debris_speed_max);
 Data->Feel.impact.debris_speed_min=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("debris_speed_min"),Data->Feel.impact.debris_speed_min);
 Data->Feel.impact.flash_time=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("flash_time"),Data->Feel.impact.flash_time);
 Data->Feel.impact.kill_hitstop=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("kill_hitstop"),Data->Feel.impact.kill_hitstop);
 Data->Feel.impact.nearby_death_trauma=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("nearby_death_trauma"),Data->Feel.impact.nearby_death_trauma);
 Data->Feel.impact.nearby_hit_trauma=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("nearby_hit_trauma"),Data->Feel.impact.nearby_hit_trauma);
 Data->Feel.impact.own_death_trauma=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("own_death_trauma"),Data->Feel.impact.own_death_trauma);
 Data->Feel.impact.own_hit_hitstop=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("own_hit_hitstop"),Data->Feel.impact.own_hit_hitstop);
 Data->Feel.impact.own_hit_kick=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("own_hit_kick"),Data->Feel.impact.own_hit_kick);
 Data->Feel.impact.own_hit_trauma=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("own_hit_trauma"),Data->Feel.impact.own_hit_trauma);
 Data->Feel.impact.own_hit_trauma_per_damage=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("own_hit_trauma_per_damage"),Data->Feel.impact.own_hit_trauma_per_damage);
 Data->Feel.impact.rumble_death_secs=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("rumble_death_secs"),Data->Feel.impact.rumble_death_secs);
 Data->Feel.impact.rumble_hit_secs=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("rumble_hit_secs"),Data->Feel.impact.rumble_hit_secs);
 Data->Feel.impact.rumble_scale=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("rumble_scale"),Data->Feel.impact.rumble_scale);
 Data->Feel.impact.shake_range=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("shake_range"),Data->Feel.impact.shake_range);
 Data->Feel.impact.spark_cone=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("spark_cone"),Data->Feel.impact.spark_cone);
 Data->Feel.impact.spark_count=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("spark_count"),Data->Feel.impact.spark_count);
 Data->Feel.impact.spark_life=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("spark_life"),Data->Feel.impact.spark_life);
 Data->Feel.impact.spark_speed_max=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("spark_speed_max"),Data->Feel.impact.spark_speed_max);
 Data->Feel.impact.spark_speed_min=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("spark_speed_min"),Data->Feel.impact.spark_speed_min);
 Data->Feel.impact.wreck_drift=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("wreck_drift"),Data->Feel.impact.wreck_drift);
 Data->Feel.impact.wreck_life=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("wreck_life"),Data->Feel.impact.wreck_life);
 Data->Feel.impact.wreck_spin=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("impact")),TEXT("wreck_spin"),Data->Feel.impact.wreck_spin);
 Data->Feel.rings.drop=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("rings")),TEXT("drop"),Data->Feel.rings.drop);
 Data->Feel.rings.opacity=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("rings")),TEXT("opacity"),Data->Feel.rings.opacity);
 Data->Feel.time.aim_timescale=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("time")),TEXT("aim_timescale"),Data->Feel.time.aim_timescale);
 Data->Feel.time.battery_drain_per_sec=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("time")),TEXT("battery_drain_per_sec"),Data->Feel.time.battery_drain_per_sec);
 Data->Feel.time.battery_max=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("time")),TEXT("battery_max"),Data->Feel.time.battery_max);
 Data->Feel.time.battery_recharge_per_sec=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("time")),TEXT("battery_recharge_per_sec"),Data->Feel.time.battery_recharge_per_sec);
 Data->Feel.time.hitstop_timescale=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("time")),TEXT("hitstop_timescale"),Data->Feel.time.hitstop_timescale);
 Data->Feel.trails.boost_width=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("trails")),TEXT("boost_width"),Data->Feel.trails.boost_width);
 Data->Feel.trails.engine.lifetime=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("trails"))->GetObjectField(TEXT("engine")),TEXT("lifetime"),Data->Feel.trails.engine.lifetime);
 Data->Feel.trails.engine.max_crumbs=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("trails"))->GetObjectField(TEXT("engine")),TEXT("max_crumbs"),Data->Feel.trails.engine.max_crumbs);
 Data->Feel.trails.engine.min_step=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("trails"))->GetObjectField(TEXT("engine")),TEXT("min_step"),Data->Feel.trails.engine.min_step);
 Data->Feel.trails.engine.width=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("trails"))->GetObjectField(TEXT("engine")),TEXT("width"),Data->Feel.trails.engine.width);
 Data->Feel.trails.nacelle_y=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("trails")),TEXT("nacelle_y"),Data->Feel.trails.nacelle_y);
 Data->Feel.trails.stern_x=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("trails")),TEXT("stern_x"),Data->Feel.trails.stern_x);
 Data->Feel.trails.throttle_deadzone=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("trails")),TEXT("throttle_deadzone"),Data->Feel.trails.throttle_deadzone);
 Data->Feel.trails.torpedo.lifetime=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("trails"))->GetObjectField(TEXT("torpedo")),TEXT("lifetime"),Data->Feel.trails.torpedo.lifetime);
 Data->Feel.trails.torpedo.max_crumbs=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("trails"))->GetObjectField(TEXT("torpedo")),TEXT("max_crumbs"),Data->Feel.trails.torpedo.max_crumbs);
 Data->Feel.trails.torpedo.min_step=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("trails"))->GetObjectField(TEXT("torpedo")),TEXT("min_step"),Data->Feel.trails.torpedo.min_step);
 Data->Feel.trails.torpedo.width=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("trails"))->GetObjectField(TEXT("torpedo")),TEXT("width"),Data->Feel.trails.torpedo.width);
 Data->Feel.trails.torpedo_tail_z=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("trails")),TEXT("torpedo_tail_z"),Data->Feel.trails.torpedo_tail_z);
 Data->Feel.trails.width_falloff=Number(Root->GetObjectField(TEXT("feel"))->GetObjectField(TEXT("trails")),TEXT("width_falloff"),Data->Feel.trails.width_falloff);
 auto AITuning=Tuning->GetObjectField(TEXT("ai")); auto Pilot=AITuning->GetObjectField(TEXT("pilot")); auto WorldTuning=Tuning->GetObjectField(TEXT("open_world"));
 Data->AI.station_lead_secs=Number(AITuning,TEXT("station_lead_secs"),Data->AI.station_lead_secs);
 Data->AI.station_throttle=Number(AITuning,TEXT("station_throttle"),Data->AI.station_throttle);
 Data->AI.surround_count=Number(AITuning,TEXT("surround_count"),Data->AI.surround_count);
 Data->AI.surround_radius=Number(AITuning,TEXT("surround_radius"),Data->AI.surround_radius);
 Data->AI.torpedo_min_volley=Number(AITuning,TEXT("torpedo_min_volley"),Data->AI.torpedo_min_volley);
 Data->AI.turn_ease=Number(AITuning,TEXT("turn_ease"),Data->AI.turn_ease);
 Data->AI.turn_gain=Number(AITuning,TEXT("turn_gain"),Data->AI.turn_gain);
 Data->AI.warp_prime=Number(AITuning,TEXT("warp_prime"),Data->AI.warp_prime);
 Data->AI.pilot.aim_lock=Number(Pilot,TEXT("aim_lock"),Data->AI.pilot.aim_lock);
 Data->AI.pilot.board_base=Number(Pilot,TEXT("board_base"),Data->AI.pilot.board_base);
 Data->AI.pilot.board_repair_pull=Number(Pilot,TEXT("board_repair_pull"),Data->AI.pilot.board_repair_pull);
 Data->AI.pilot.board_resupply_pull=Number(Pilot,TEXT("board_resupply_pull"),Data->AI.pilot.board_resupply_pull);
 Data->AI.pilot.board_safe_threat=Number(Pilot,TEXT("board_safe_threat"),Data->AI.pilot.board_safe_threat);
 Data->AI.pilot.brace_range_frac=Number(Pilot,TEXT("brace_range_frac"),Data->AI.pilot.brace_range_frac);
 Data->AI.pilot.commit_bonus=Number(Pilot,TEXT("commit_bonus"),Data->AI.pilot.commit_bonus);
 Data->AI.pilot.emp_busy_interest=Number(Pilot,TEXT("emp_busy_interest"),Data->AI.pilot.emp_busy_interest);
 Data->AI.pilot.emp_throttle=Number(Pilot,TEXT("emp_throttle"),Data->AI.pilot.emp_throttle);
 Data->AI.pilot.mine_lead=Number(Pilot,TEXT("mine_lead"),Data->AI.pilot.mine_lead);
 Data->AI.pilot.mine_reserve=Number(Pilot,TEXT("mine_reserve"),Data->AI.pilot.mine_reserve);
 Data->AI.pilot.presentation_floor=Number(Pilot,TEXT("presentation_floor"),Data->AI.pilot.presentation_floor);
 Data->AI.pilot.ram_hull_floor=Number(Pilot,TEXT("ram_hull_floor"),Data->AI.pilot.ram_hull_floor);
 Data->AI.pilot.reloading_interest=Number(Pilot,TEXT("reloading_interest"),Data->AI.pilot.reloading_interest);
 Data->AI.pilot.scarcity_floor=Number(Pilot,TEXT("scarcity_floor"),Data->AI.pilot.scarcity_floor);
 Data->AI.pilot.screen_base=Number(Pilot,TEXT("screen_base"),Data->AI.pilot.screen_base);
 Data->AI.pilot.screen_danger=Number(Pilot,TEXT("screen_danger"),Data->AI.pilot.screen_danger);
 Data->AI.pilot.screen_raise=Number(Pilot,TEXT("screen_raise"),Data->AI.pilot.screen_raise);
 Data->AI.pilot.shield_bias=Number(Pilot,TEXT("shield_bias"),Data->AI.pilot.shield_bias);
 Data->AI.pilot.shield_worth=Number(Pilot,TEXT("shield_worth"),Data->AI.pilot.shield_worth);
 Data->AI.pilot.thumb_travel=Number(Pilot,TEXT("thumb_travel"),Data->AI.pilot.thumb_travel);
 Data->AI.pilot.torpedo_aim_lock=Number(Pilot,TEXT("torpedo_aim_lock"),Data->AI.pilot.torpedo_aim_lock);
 Data->AI.pilot.torpedo_standoff=Number(Pilot,TEXT("torpedo_standoff"),Data->AI.pilot.torpedo_standoff);
 Data->AI.pilot.w_board=Number(Pilot,TEXT("w_board"),Data->AI.pilot.w_board);
 Data->AI.pilot.w_broadside=Number(Pilot,TEXT("w_broadside"),Data->AI.pilot.w_broadside);
 Data->AI.pilot.w_disengage=Number(Pilot,TEXT("w_disengage"),Data->AI.pilot.w_disengage);
 Data->AI.pilot.w_emp=Number(Pilot,TEXT("w_emp"),Data->AI.pilot.w_emp);
 Data->AI.pilot.w_microwarp=Number(Pilot,TEXT("w_microwarp"),Data->AI.pilot.w_microwarp);
 Data->AI.pilot.w_ram=Number(Pilot,TEXT("w_ram"),Data->AI.pilot.w_ram);
 Data->AI.pilot.w_torpedo=Number(Pilot,TEXT("w_torpedo"),Data->AI.pilot.w_torpedo);
 Data->AI.pilot.warp_escape_threat=Number(Pilot,TEXT("warp_escape_threat"),Data->AI.pilot.warp_escape_threat);
 Data->AI.pilot.warp_reposition_interest=Number(Pilot,TEXT("warp_reposition_interest"),Data->AI.pilot.warp_reposition_interest);
 Data->World.alert_ttl=Number(WorldTuning,TEXT("alert_ttl"),Data->World.alert_ttl);
 Data->World.avenge_reputation_bonus=Number(WorldTuning,TEXT("avenge_reputation_bonus"),Data->World.avenge_reputation_bonus);
 Data->World.dock_refusal_threshold=Number(WorldTuning,TEXT("dock_refusal_threshold"),Data->World.dock_refusal_threshold);
 Data->World.heat_decay_per_sec=Number(WorldTuning,TEXT("heat_decay_per_sec"),Data->World.heat_decay_per_sec);
 Data->World.heat_engage_threshold=Number(WorldTuning,TEXT("heat_engage_threshold"),Data->World.heat_engage_threshold);
 Data->World.heat_per_damage=Number(WorldTuning,TEXT("heat_per_damage"),Data->World.heat_per_damage);
 Data->World.heat_to_reputation_rate=Number(WorldTuning,TEXT("heat_to_reputation_rate"),Data->World.heat_to_reputation_rate);
 Data->World.hostile_threshold=Number(WorldTuning,TEXT("hostile_threshold"),Data->World.hostile_threshold);
 Data->World.recent_attack_memory=Number(WorldTuning,TEXT("recent_attack_memory"),Data->World.recent_attack_memory);
 Data->World.scan_decay_rate=Number(WorldTuning,TEXT("scan_decay_rate"),Data->World.scan_decay_rate);
 Data->World.scan_rate=Number(WorldTuning,TEXT("scan_rate"),Data->World.scan_rate);
 auto Catalogue=Root->GetObjectField(TEXT("loadouts"));
 const TCHAR* Categories[]={TEXT("broadsides"),TEXT("batteries"),TEXT("specials")};
 for(int I=0;I<3;++I) for(const auto& Item:Catalogue->GetArrayField(Categories[I])) {
  auto O=Item->AsObject(); FVTLoadoutOption Option; Option.Id=FName(O->GetStringField(TEXT("id"))); Option.Slot=EVTLoadoutSlot(I); auto& Def=Option.Definition; Def.Equipment=Equipment(O);
  if(I==0) {auto Gun=O->GetObjectField(TEXT("broadside")); Def.Damage=Number(Gun,TEXT("damage"),12); Def.Reload=Number(Gun,TEXT("cooldown"),10); Def.MuzzleSpeed=Number(Gun,TEXT("muzzle_speed"),325); Def.Arc=Number(Gun,TEXT("arc"),1.1780972f); Def.ChargeTime=Number(Gun,TEXT("charge_time"),0); Def.Guns=int32(Number(Gun,TEXT("guns"),3));}
  if(I==1) {auto Battery=O->GetObjectField(TEXT("battery")); Def.BatteryMax=Number(Battery,TEXT("max"),3); Def.BatteryRecharge=Number(Battery,TEXT("recharge_per_sec"),0.6f);}
  Data->Loadouts.Add(Option);
 }
 FString FactionJson; TSharedPtr<FJsonObject> Factions;
 if(!FFileHelper::LoadFileToString(FactionJson,*(FPaths::ProjectDir()/TEXT("Migration/factions.json")))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(FactionJson),Factions)) return 7;
 for(const auto& Name:Factions->GetArrayField(TEXT("tracked"))) Data->TrackedFactions.Add(FName(Name->AsString()));
 for(const auto& Value:Factions->GetArrayField(TEXT("initial"))) Data->InitialReputation.Add(float(Value->AsNumber()));
 for(const auto& Value:Factions->GetArrayField(TEXT("relations"))) {auto O=Value->AsObject(); FVTFactionRelation R; R.A=FName(O->GetStringField(TEXT("a"))); R.B=FName(O->GetStringField(TEXT("b"))); R.Standing=Number(O,TEXT("standing"),0); Data->Relations.Add(R);}
 auto Map=Root->GetObjectField(TEXT("world"));
 Data->StartSystem=FName(Map->GetStringField(TEXT("start")));
 for(const auto& Item:Map->GetArrayField(TEXT("systems"))) {
  auto O=Item->AsObject(); FVTSystemDefinition Def;
  Def.Id=FName(O->GetStringField(TEXT("id"))); Def.DisplayName=FText::FromString(O->GetStringField(TEXT("name")));
  FString Owner; if(O->TryGetStringField(TEXT("owner"),Owner)) Def.Owner=FName(Owner);
  FString Security=O->GetStringField(TEXT("security"));
  Def.Security=Security=="High" ? 2 : Security=="Medium" ? 1 : 0;
  Def.Danger=Number(O,TEXT("danger"),0); Def.Radius=Number(O,TEXT("bounds_radius"),1400); Def.HasStation=O->GetBoolField(TEXT("has_starbase"));
  Def.ChartPosition=FVector2D(Number(O,TEXT("x"),0),Number(O,TEXT("y"),0));
  for(const auto& Link:O->GetArrayField(TEXT("links"))) Def.Links.Add(FName(Link->AsString()));
  Data->Systems.Add(Def);
 }
 auto Rules=Root->GetObjectField(TEXT("tuning"));
 Data->Rules.RamDamagePerSpeed=Number(Rules,TEXT("ram_damage_per_speed"),0.22f);
 Data->Rules.RamThreshold=Number(Rules,TEXT("ram_damage_threshold"),45);
 Data->Rules.RamRestitution=Number(Rules,TEXT("ram_restitution"),0.35f);
 Data->Rules.RamSeparation=Number(Rules,TEXT("ram_separation"),0.6f);
 Data->Rules.BoardRepairFraction=Number(Rules,TEXT("board_repair_frac"),0.1f);
 Data->Rules.EngagementRange=Number(Rules,TEXT("engagement_range"),506);
 Data->Rules.HullLength=Number(Rules,TEXT("hull_length"),40);
 Data->Rules.MuzzleStandoff=Number(Rules,TEXT("muzzle_standoff"),22);
 Data->Rules.ReverseThrottle=Number(Rules,TEXT("reverse_throttle"),0.25);
 Data->Rules.BoundsSpring=Number(Rules,TEXT("bounds_spring"),3);
 Data->Rules.BraceDamageFactor=Number(Rules,TEXT("brace_damage_factor"),0.35f);
 Data->Rules.ProjectileTTL=Number(Rules,TEXT("projectile_ttl"),2.5);
 Data->Rules.ProjectileRadius=Number(Rules,TEXT("projectile_radius"),5);
 Data->Rules.BoardRange=Number(Rules,TEXT("board_range"),95);
 Data->Rules.BoardDwell=Number(Rules,TEXT("board_dwell"),3);
 Data->Rules.CrippleThreshold=Number(Rules,TEXT("cripple_threshold"),0.25);
 Data->Rules.JumpRange=Number(Rules,TEXT("jump_range"),120);
 Data->Rules.JumpDwell=Number(Rules,TEXT("jump_charge_time"),6);
 auto SpawnDefinition=[](const TSharedPtr<FJsonObject>& O) {FVTScenarioSpawn D; D.ClassId=FName(O->GetStringField(TEXT("class"))); D.Faction=FName(O->GetStringField(TEXT("faction"))); auto Pos=O->GetArrayField(TEXT("pos")); D.Position=FVector2D(Pos[0]->AsNumber(),Pos[1]->AsNumber()); D.Heading=Number(O,TEXT("heading"),0); auto Flags=O->GetObjectField(TEXT("flags")); D.Anchored=Flags->GetBoolField(TEXT("anchored")); D.Inert=Flags->GetBoolField(TEXT("inert")); D.Invulnerable=Flags->GetBoolField(TEXT("invulnerable")); return D;};
 for(const auto* Id:{TEXT("skirmish"),TEXT("test_range")}) {
  auto O=Root->GetObjectField(Id); FVTScenarioDefinition D; D.Id=FName(FString(Id)==TEXT("test_range") ? TEXT("range") : Id); D.Player=SpawnDefinition(O->GetObjectField(TEXT("player"))); D.Radius=Number(O,TEXT("bounds_radius"),1400);
  for(const auto& Enemy:O->GetArrayField(TEXT("enemies"))) D.Enemies.Add(SpawnDefinition(Enemy->AsObject()));
  const TSharedPtr<FJsonObject>* Director=nullptr; if(O->TryGetObjectField(TEXT("director"),Director)&&Director&&Director->IsValid()) {D.Director=true; D.BaseCount=int32(Number(*Director,TEXT("base_count"),2)); D.MaxWaves=int32(Number(*Director,TEXT("max_waves"),3)); D.BaseHull=Number(*Director,TEXT("base_hull"),100); D.HullPerWave=Number(*Director,TEXT("hull_per_wave"),25); D.EnemyClass=FName((*Director)->GetStringField(TEXT("enemy_class"))); D.EnemyFaction=FName((*Director)->GetStringField(TEXT("faction"))); D.FinaleClass=FName((*Director)->GetStringField(TEXT("finale_class"))); D.FinaleCount=int32(Number(*Director,TEXT("finale_count"),1));}
  Data->Scenarios.Add(D);
 }
 if(!SaveAsset(Data,DataPath)) return 3;
 UPackage* InputPackage=CreatePackage(TEXT("/Game/Input/IMC_Flight"));
 auto* Mapping=NewObject<UInputMappingContext>(InputPackage,TEXT("IMC_Flight"),RF_Public|RF_Standalone);
 const TCHAR* Names[]={TEXT("Throttle"),TEXT("Turn"),TEXT("Aim"),TEXT("Port"),TEXT("Starboard"),TEXT("EMP"),TEXT("Torpedo"),TEXT("Warp"),TEXT("Boost"),TEXT("Brace"),TEXT("Interact"),TEXT("Mine"),TEXT("PointDefense")};
 const FKey Keys[]={EKeys::W,EKeys::D,EKeys::Gamepad_Right2D,EKeys::LeftMouseButton,EKeys::RightMouseButton,EKeys::Q,EKeys::LeftControl,EKeys::LeftShift,EKeys::SpaceBar,EKeys::C,EKeys::B,EKeys::M,EKeys::X};
 for(int32 I=0;I<UE_ARRAY_COUNT(Names);++I) {
  const FString Path=FString::Printf(TEXT("/Game/Input/IA_%s"),Names[I]);
  auto* A=NewObject<UInputAction>(CreatePackage(*Path),FName(FString("IA_")+Names[I]),RF_Public|RF_Standalone);
  A->ValueType=I<2 ? EInputActionValueType::Axis1D : I==2 ? EInputActionValueType::Axis2D : EInputActionValueType::Boolean;
  Mapping->MapKey(A,Keys[I]);
  if(I<2) {
   auto& Negative=Mapping->MapKey(A,I==0 ? EKeys::S : EKeys::A);
   Negative.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
   Mapping->MapKey(A,I==0 ? EKeys::Gamepad_LeftY : EKeys::Gamepad_LeftX);
  }
  if(I>=3) {
   const FKey Pad[]={EKeys::Gamepad_LeftTrigger,EKeys::Gamepad_RightTrigger,EKeys::Gamepad_FaceButton_Left,EKeys::Gamepad_LeftShoulder,EKeys::Gamepad_RightShoulder,EKeys::Gamepad_FaceButton_Bottom,EKeys::Gamepad_FaceButton_Top,EKeys::Gamepad_FaceButton_Right,EKeys::Gamepad_DPad_Left,EKeys::Gamepad_DPad_Right};
   Mapping->MapKey(A,Pad[I-3]);
  }
  if(!SaveAsset(A,Path)) return 4;
 }
 if(!SaveAsset(Mapping,TEXT("/Game/Input/IMC_Flight"))) return 5;
 UPackage* MapPackage=CreatePackage(TEXT("/Game/Maps/Sandbox"));
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false,FName("Sandbox"),MapPackage);
 auto* Light=World->SpawnActor<ADirectionalLight>(); Light->SetActorRotation(FRotator(-50,-30,0)); Light->GetLightComponent()->SetIntensity(5);
 World->SpawnActor<ASkyLight>();
 const bool Saved=SaveAsset(World,TEXT("/Game/Maps/Sandbox"));
 World->DestroyWorld(false);
 UPackage* MenuPackage=CreatePackage(TEXT("/Game/Maps/Menu")); UWorld* MenuWorld=UWorld::CreateWorld(EWorldType::Editor,false,FName("Menu"),MenuPackage); if(!SaveAsset(MenuWorld,TEXT("/Game/Maps/Menu"))) return 8; MenuWorld->DestroyWorld(false);
 auto* BP=Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UVTUI::StaticClass(),CreatePackage(TEXT("/Game/UI/WBP_UI")),TEXT("WBP_UI"),BPTYPE_Normal,UWidgetBlueprint::StaticClass(),UWidgetBlueprintGeneratedClass::StaticClass()));
 auto* Tree=BP->WidgetTree.Get(); auto* Overlay=Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(),TEXT("Root")); Tree->RootWidget=Overlay;
 auto* MenuBorder=Tree->ConstructWidget<UBorder>(); MenuBorder->SetBrushColor(FLinearColor(0.018f,0.025f,0.05f,0.96f)); MenuBorder->SetPadding(FMargin(28)); auto* MenuScale=Tree->ConstructWidget<UScaleBox>();MenuScale->SetStretch(EStretch::ScaleToFit);auto* MenuBounds=Tree->ConstructWidget<USizeBox>();MenuBounds->SetWidthOverride(1100);MenuScale->SetContent(MenuBounds);MenuBounds->SetContent(MenuBorder);auto* MenuSlot=CastChecked<UOverlaySlot>(Overlay->AddChild(MenuScale));MenuSlot->SetHorizontalAlignment(HAlign_Center);MenuSlot->SetVerticalAlignment(VAlign_Center);
 auto* Menu=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("MenuPanel")); MenuBorder->SetContent(Menu);
 auto Text=[&](UPanelWidget* Parent,const TCHAR* Name,const TCHAR* Label,int Size) {auto* T=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),FName(Name)); T->SetText(FText::FromString(Label)); FSlateFontInfo Font=T->GetFont(); Font.Size=Size; T->SetFont(Font); T->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f,0.9f,1))); auto* Slot=Parent->AddChild(T);if(auto* V=Cast<UVerticalBoxSlot>(Slot))V->SetPadding(FMargin(0,8,0,4)); return T;};
 Text(Menu,TEXT("Title"),TEXT("VOID & THUNDER"),32); Text(Menu,TEXT("Subtitle"),TEXT("Space piracy in the Settled Dark"),16);
 auto Row=[&]() {auto* R=Tree->ConstructWidget<UHorizontalBox>(); auto* Slot=CastChecked<UVerticalBoxSlot>(Menu->AddChild(R));Slot->SetPadding(FMargin(0,4)); return R;};
 auto Button=[&](UPanelWidget* Parent,const TCHAR* Name,const TCHAR* Label) {auto* B=Tree->ConstructWidget<UButton>(UButton::StaticClass(),FName(Name)); auto* Slot=Parent->AddChild(B);if(auto* H=Cast<UHorizontalBoxSlot>(Slot))H->SetPadding(FMargin(0,0,8,0));FButtonStyle Style=B->GetStyle();Style.SetNormal(FSlateColorBrush(FLinearColor(0.055f,0.12f,0.18f)));Style.SetHovered(FSlateColorBrush(FLinearColor(0.08f,0.28f,0.35f)));Style.SetPressed(FSlateColorBrush(FLinearColor(0.025f,0.18f,0.22f)));Style.SetDisabled(FSlateColorBrush(FLinearColor(0.03f,0.045f,0.065f)));Style.SetNormalPadding(FMargin(14,9));Style.SetPressedPadding(FMargin(14,10,14,8));B->SetStyle(Style);Text(B,*FString::Printf(TEXT("%sLabel"),Name),Label,16);};
 Text(Menu,TEXT("WorldHeading"),TEXT("Shared world"),18);auto* WorldRow=Row(); auto* WorldName=Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(),TEXT("WorldName")); WorldName->SetText(FText::FromString(TEXT("Campaign"))); WorldRow->AddChild(WorldName); WorldRow->AddChild(Tree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(),TEXT("PopulationChoice"))); Button(WorldRow,TEXT("Create"),TEXT("Create World")); Button(WorldRow,TEXT("Continue"),TEXT("Continue World"));
 auto* JoinRow=Row(); auto* Address=Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(),TEXT("Address")); Address->SetText(FText::FromString(TEXT("127.0.0.1:7777"))); JoinRow->AddChild(Address); Button(JoinRow,TEXT("Join"),TEXT("Join address")); Button(JoinRow,TEXT("Discover"),TEXT("Find LAN worlds")); auto* LAN=Tree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(),TEXT("LANChoice")); JoinRow->AddChild(LAN); Button(JoinRow,TEXT("JoinLAN"),TEXT("Join world"));
 Text(Menu,TEXT("SoloHeading"),TEXT("Solo flight"),18);auto* SoloRow=Row(); Button(SoloRow,TEXT("Skirmish"),TEXT("Solo Skirmish")); Button(SoloRow,TEXT("Range"),TEXT("Test Range"));
 Text(Menu,TEXT("FitHeading"),TEXT("Ship and equipment"),20); auto* FitRow=Row();FitRow->Rename(TEXT("FitRow"),Tree);
 const TCHAR* FitNames[]={TEXT("HullChoice"),TEXT("GunChoice"),TEXT("BatteryChoice"),TEXT("SpecialChoice")};const TCHAR* FitLabels[]={TEXT("Hull"),TEXT("Broadsides"),TEXT("Battery"),TEXT("Utility")};for(int I=0;I<4;++I){auto* Column=Tree->ConstructWidget<UVerticalBox>();auto* Slot=CastChecked<UHorizontalBoxSlot>(FitRow->AddChild(Column));Slot->SetPadding(FMargin(0,0,12,0));Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Text(Column,*FString::Printf(TEXT("FitLabel%d"),I),FitLabels[I],12);Column->AddChild(Tree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(),FName(FitNames[I])));}
 Text(Menu,TEXT("MountHeading"),TEXT("Additional mounts (limited by hull; crew works surplus mounts)"),14); auto* MountRow=Row();MountRow->Rename(TEXT("MountRow"),Tree); for(const TCHAR* Name:{TEXT("BatterySecond"),TEXT("BatteryThird"),TEXT("SpecialSecond"),TEXT("SpecialThird")}) MountRow->AddChild(Tree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(),FName(Name)));
 Text(Menu,TEXT("StationHeading"),TEXT("Station services"),18);auto* StationRow=Row(); Button(StationRow,TEXT("Recover"),TEXT("Recover disabled ship")); Button(StationRow,TEXT("Repair"),TEXT("Repair hull")); Button(StationRow,TEXT("PayHeat"),TEXT("Pay off local heat")); Button(StationRow,TEXT("Refit"),TEXT("Refit at station")); Button(StationRow,TEXT("Undock"),TEXT("Undock"));
 auto* GraphicsRow=Row();Text(GraphicsRow,TEXT("GraphicsStatus"),TEXT("Graphics"),14);Button(GraphicsRow,TEXT("GraphicsLow"),TEXT("Performance"));Button(GraphicsRow,TEXT("GraphicsBalanced"),TEXT("Balanced"));Button(GraphicsRow,TEXT("GraphicsHigh"),TEXT("High"));
 auto* SessionRow=Row(); Button(SessionRow,TEXT("Resume"),TEXT("Resume")); Button(SessionRow,TEXT("Save"),TEXT("Save world")); Button(SessionRow,TEXT("Leave"),TEXT("Main menu")); Button(SessionRow,TEXT("Quit"),TEXT("Quit")); Text(Menu,TEXT("Status"),TEXT(""),16);
 auto* HUDBorder=Tree->ConstructWidget<UBorder>();HUDBorder->SetBrushColor(FLinearColor(0.015f,0.025f,0.04f,0.8f));HUDBorder->SetPadding(FMargin(14,10));auto* HUDSlot=CastChecked<UOverlaySlot>(Overlay->AddChild(HUDBorder));HUDSlot->SetHorizontalAlignment(HAlign_Left);HUDSlot->SetVerticalAlignment(VAlign_Top);HUDSlot->SetPadding(FMargin(18));auto* HUD=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("HUDPanel"));HUDBorder->SetContent(HUD);Text(HUD,TEXT("Flight"),TEXT(""),13);
 TArray<UWidget*> AuthoredWidgets; Tree->GetAllWidgets(AuthoredWidgets); for(auto* Widget:AuthoredWidgets) Widget->bIsVariable=false;
 FKismetEditorUtilities::CompileBlueprint(BP); if(!SaveAsset(BP,TEXT("/Game/UI/WBP_UI"))) return 9;
 UE_LOG(LogTemp,Display,TEXT("Imported %d ship classes and %d systems"),Data->Ships.Num(),Data->Systems.Num());
 return Saved ? 0 : 6;
}
UVTBenchmarkCommandlet::UVTBenchmarkCommandlet() { IsClient=false; IsServer=true; IsEditor=true; LogToConsole=true; }
int32 UVTBenchmarkCommandlet::Main(const FString& Params) {
 int32 Count=500; FParse::Value(*Params,TEXT("Population="),Count);
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,FName("VTBenchmark"));
 FWorldContext& Context=GEngine->CreateNewWorldContext(EWorldType::Game); Context.SetCurrentWorld(World);
 World->SetGameInstance(NewObject<UVTGameInstance>(GEngine));
 auto* Sim=World->GetSubsystem<UVTSimulation>(); if(!Sim->Data) return 2;
 World->SetGameMode(FURL());
 World->InitializeActorsForPlay(FURL());
 World->BeginPlay();
 Sim->Bootstrap(Count);
 if(Sim->Ships.Num()!=Count) { UE_LOG(LogTemp,Error,TEXT("Benchmark spawned %d instead of %d"),Sim->Ships.Num(),Count); return 3; }
 bool Busy=FParse::Param(*Params,TEXT("Busy")); bool Armed=FParse::Param(*Params,TEXT("Armed"));
 Sim->ConfigurePopulationFixture(Count,Busy,Armed);
 for(int32 I=0;I<1200;++I) Sim->FixedStep();
 for(int P=0;P<8;++P) UE_LOG(LogTemp,Display,TEXT("Phase %d mean %.3f ms"),P,Sim->PhaseTotals[P]/1200);
 auto Samples=Sim->StepMilliseconds; Samples.RemoveAt(0,200); Samples.Sort();
 const double P95=Samples[FMath::FloorToInt(Samples.Num()*0.95)];
 FString Report=FString::Printf(TEXT("{\"population\":%d,\"systems\":%d,\"samples\":%d,\"p95_ms\":%.6f,\"budget_ms\":8,\"cpu\":\"%s\",\"passed\":%s}"),Sim->Ships.Num(),Sim->Data->Systems.Num(),Samples.Num(),P95,FPlatformMisc::GetCPUBrand().GetCharArray().GetData(),P95<8 ? TEXT("true") : TEXT("false"));
 const FString Path=FPaths::ProjectSavedDir()/FString::Printf(TEXT("Validation/scale-%d%s%s.json"),Count,Busy ? TEXT("-busy") : TEXT(""),Armed ? TEXT("-armed") : TEXT(""));
 IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true); FFileHelper::SaveStringToFile(Report,*Path);
 UE_LOG(LogTemp,Display,TEXT("Population %d simulation p95 %.3f ms"),Sim->Ships.Num(),P95);
 World->BeginTearingDown(); World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World);
 return Count>500||P95<8 ? 0 : 1;
}
