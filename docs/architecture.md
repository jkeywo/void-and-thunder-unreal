# Architecture

Unreal Gameplay Framework owns lifecycle and network authority. GameMode creates
ships, GameState replicates shared summaries, PlayerState holds captain data,
and PlayerController/AIController supply FVTPilotIntent. AVTShip owns one movement
component and an AbilitySystemComponent. Native assets supply tuning.

UVTSimulation coordinates 64 Hz stepping, world population and simulation timing.
Coordinates stay in legacy units in FVTMotion; one unit maps to 100 Unreal cm,
with Y reflected to preserve the source's heading convention.
Systems occupy separate arenas in one UWorld. Per-system relevance determines
which ship actors reach each viewer; empty systems continue stepping.

UVTShipMovement predicts locally, retains unacknowledged inputs, applies server
acknowledgements and replays remaining input. GAS prediction is independent of
movement prediction.

The editor module imports the fully resolved Rust snapshot into native content
and provides scale benchmarks. Rust is not a runtime dependency. UI, mesh, sound
and effects assets remain editable in Unreal.

## Ownership
GameMode: authority, identity validation, spawning and respawn.
GameState: time and global system summaries.
PlayerState: credits, standings, heat, equipment and player ownership.
Ship components/GAS: physical state, equipment, interactions and ability state.
Save subsystem: consistent snapshots, durable IDs, guest records and backups.

See implementation-status.md for delivered coverage and validation limits.

## Equipment transfer
Native equipment definitions include optional EMP, boost, point defence, torpedoes, warp and mines. Device Gameplay Abilities own activation/cooldown; their replicated snapshot mirrors support UI and persistence. Battery costs and EMP stress use GAS attributes, while directional shields and launcher state belong to the combat component. Input includes a bounded cursor offset; authority derives targeting from its own ship pose. Mines and torpedoes are projectile kinds with durable target/source IDs.

Projectile contacts use a per-system 128-unit spatial broad phase rebuilt after ship contacts. Swept bounds include the largest hull radius and projectile radius; exact swept/3D tests retain authoritative damage. The index contains pointers, not duplicate gameplay state.

Hull damage, shared battery costs/recharge and EMP stress changes use instant GameplayEffects with an authoritative SetByCaller delta. GAS ability instances own wind-up and cooldown state, advanced by the 64 Hz coordinator and mirrored only for save/HUD serialization. Directional shield absorption precedes the single hull effect; commands never apply a second damage path.

Presentation is local-system scoped on listen hosts as well as clients. Authoritative planar poses and replication remain in movement state; background hulls/projectiles do not update cosmetic scene transforms each frame. Mesh visibility is local component state, never replicated Actor hidden state. Cosmic props and planar actors do not affect Unreal navigation; projectile spheres cast no shadows.


## World, sessions and persistence

AVTGameMode authorizes identity, spawning and recovery. AVTGameState exposes shared
time/populations; AVTPlayerState carries each captain's profile, credits, prizes,
reputation and heat. UVTSessionSubsystem (GameInstance) hosts/discovers/joins Null
LAN sessions. UVTSaveSubsystem captures world and captain records, version migrations,
CRC validation, backup recovery and atomic Windows replacement. Personal preferences,
local profile/tokens and solo career statistics are separate SaveGame objects.

The fixed-step phase order is scenario/intent/AI, equipment systems, motion,
contacts/landmarks/bounds, weapons, projectiles/point defence, destruction/recovery,
boarding and world interactions, summaries, boundary autosave. There is one game
thread owner; menus and disconnect callbacks cannot interleave with a fixed step.
Systems are coordinate-separated arenas in one persistent UWorld, not level travel.
Only a captain's ship changes system. Ship/projectile/landmark relevance uses that
captain's current system; GameState publishes separate global summaries.

## Native authoring

DA_GameData is a Primary Data Asset holding the resolved ship, equipment, scenario,
system and tuning catalogues. Designers edit native reflected structures. WBP_UI
assembles native interface widgets; cue Blueprints extend UVTShipCue; Niagara,
materials, meshes, engine sockets, Sound Waves and ST_UI remain native editable
assets. VTBootstrap/VTContent commandlets live only in the editor module and
regenerate baseline packages from the pinned export/source assets.

This split follows Epic's [C++/Blueprint foundation guidance](https://dev.epicgames.com/documentation/en-us/unreal-engine/coding-in-unreal-engine-blueprint-vs-cplusplus)
and [GAS ownership model](https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-the-unreal-engine-gameplay-ability-system).
