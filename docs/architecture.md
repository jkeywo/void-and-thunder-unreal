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

## Acceptance harness
Development probes coordinate four processes through a replicated AInfo fixture. Initial placement and vulnerable hull setup are explicit fixtures; broadside damage, simultaneous boarding, station services, death recovery and charged jumps use normal gameplay paths. Client reports include observed phase completion and reconciliation distances. A separate campaign soak preserves normal mortal populations, records per-system advancement, snapshots/reconnects and bounded memory/performance samples. Shipping neither creates nor runs probe actors.

The optional player AI pilot is an authoritative control mode on the same pawn.
The PlayerController retains possession and camera ownership; AVTShipAI supplies
intent through its shared decision entry point. Remote clients interpolate that
authoritative motion while AI is active, and resume sequenced prediction on return
to manual control. Loading/reconnecting starts in manual mode and cancels held inputs.

## Simulation query optimisation

Simple beam/civilian/patrol pilots select their nearest hostile contact in one pass.
Utility pilots retain a full ordered target list, sorting cached squared distances
rather than repeatedly reading component poses. Threat sums retain system order.
Boarding uses a phase-local per-system list of disabled, non-invulnerable ships;
claim, validity, faction and range checks remain live for every captain. These
lists contain Actor pointers, not independent state, and are rebuilt at the phase
where they are consumed. EMP rejects out-of-range contacts before angular tests.
All systems and movement/combat still advance at 64 Hz.

## Native integration revision

Migration is an explicit editor operation, never a validation step. Seed mode preserves
existing authored packages; replacement requires an explicit regeneration switch.
C++ ship rules are assembled by configurable Blueprint pawn/UI classes. Primary
asset catalogues expose ship, equipment, system and scenario definitions, with
presentation bundles preloaded through Asset Manager before population spawning.

Autosaves capture and serialize consistent snapshots on the game thread, then use
a single immutable-byte background writer. Completion is observed on the game
thread; load, manual save and orderly teardown flush pending writes. CRC, atomic
replacement and last-good backups remain intact.

HUD changes originate from GAS delegates, replicated state notifications, session
events and authoritative simulation boundaries; updates are coalesced at 10 Hz.
Widgets cache typed bindings and selections retain stable IDs independent of labels.
Enhanced Input uses common/menu/flight/docked contexts. Ability tags gate activation,
and fixed-step Ability Tasks own deterministic cooldown clocks; continuous battery
costs retain their existing GAS attribute path. Audio attenuation/concurrency and
Niagara Effect Types provide native presentation budgets without changing gameplay.

The flight HUD uses authored UMG resource bars and ability readouts fed by the existing coalesced refresh path. A local native grid Actor follows the current system; sky remains camera-centered. Projectile visual radius and flight pace are independently editable on DA_GameData; collision radii remain simulation-owned.

[ai] Restore the original amber CRT HUD composition using native Slate drawing within the authored UMG widget: five original panel frames exported as UI textures, native live gauges/tubes/shield edges, and matching amber menu styling. Static artwork retains source attribution; no HTML runtime is added. The sandbox chart remains available as a collapsible overlay rather than occupying the coordinate cluster.

Broadside aim is a local arc offset while held, preserved through release and encoded as the existing authoritative aim intent. The tracking camera and preview consume that direction. Viewport capture forwards the first mouse press; painted tactical markers use viewport-relative pixel scaling into widget coordinates.

Broadside previews and authoritative firing share the muzzle/velocity geometry function, including inherited ship momentum. Each gun projects its own trajectory.

Contextual HUD interaction hints read replicated boarding targets and travel/docking state, recheck proximity, and resolve live Enhanced Input mappings. They display intent and progress without awarding loot or changing rules. The local reference-grid actor selects the nearest authored star in the current system and passes its world centre to a dynamic radial material; ring/spoke density remains editable in the native material.

The editor module extends the native Play toolbar with mode, hull and checked loadout menus. Editor-only World Settings asset user data declares supported modes, while per-user editor settings retain the chosen fit. OnPIEMapReady applies selections after profile initialization and before gameplay spawning. PIE profile/career/campaign slots are isolated by PIE instance. Explicit empty equipment sets are distinguished from inherited hull defaults in the existing fit contract.

Gate entry remains host-authoritative: a held interaction guides the shared ship helm to the inside staging point, aligns with the gate normal, then flies across the opening after charging. Travel requires a swept aperture crossing. Replicated entry phase supports client prediction and is cancelled on release/load. Native authored meshes/materials represent gates and torpedoes. Ships and camera share an interpolated presentation pose; replicated fits are resolved during initialization and class/fit notifications.

Gate staging compares forward and reverse travel/turn estimates using the hull thrust, drag and turn rate. Reverse is restricted to the approach phase; charged passage retains the outward heading and swept crossing. Device projectiles and restored projectiles finish deferred spawning only after their type/state is assigned, and replicated type changes refresh presentation.
