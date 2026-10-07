# Decisions

The user approved the following port choices: Unreal 5.8.2, C++ foundations with
Blueprint presentation/content, public GitHub repository, four-player listen-server
sandbox, independent travel, persistent host worlds, continuously simulated empty
systems, immediate saving/removal on guest disconnect, station respawn without a
new monetary penalty, and slow motion only in standalone play.

The user selected plain architecture documents in this new repository rather
than PASM. This choice does not alter the original game or vellum.

- [ai] Use one Actor/component ship implementation for 500 active NPCs. Prove the
  scale target early; do not introduce a second Mass simulation without a revised
  architecture decision and measured justification.
- [ai] Use simulation coordinates in legacy units and a single conversion boundary:
  Unreal centimetres = (X, -Y, Z) * 100. Yaw is the negative source heading.
- [ai] Keep export tooling isolated in a temporary pinned source snapshot. Export
  through original typed RON deserialization so omitted fields inherit real defaults.
- [ai] Separate the runtime and editor modules. Import/build tooling does not ship.

- [ai] GAS broadside instances own wind-up and reload time; the fixed-step coordinator
  advances their clocks. Replicated bank indicators are presentation snapshots.
- [ai] Directional shield banks use an explicit reflected structure; hull and battery
  remain GAS attributes and only one combat damage path modifies hull.
- [ai] Native migration regenerates complete packages and marks them fully loaded
  before saving. Loading the old package after constructing new data overwrites it;
  the authoritative-volley fixture caught this importer lifecycle bug.
- [ai] Guest state is captured in PawnLeavingGame, before base Unreal pawn destruction.
  Logout performs the campaign write after removal. Reconnect uses a persistent local
  profile GUID and a host-issued per-world GUID token, with simultaneous identity rejection.
- [ai] Campaign files use a checksummed SaveGame payload, a last-good backup and Windows
  atomic replacement. Personal profiles/tokens are separate from campaign records.
- [ai] Synthetic scale fixtures are invulnerable so contacts do not reduce the measured
  population. They do not establish full combat or renderer acceptance.

- [ai] Restored the legacy wrapping LCG instead of the prototype FRandomStream,
  following the source agent guide's requirement to preserve cosmetic spawn jitter.
- [ai] Snapshot schema 3 migrates schema 2 pose clocks. Unknown future schemas fall
  back to a validated backup. The identity-free initial prototype slot format and
  legacy Rust saves are unsupported.
- [ai] The uncharged development jump helper is disabled in Shipping; charged
  gameplay travel now uses the native charged, proximity-validated interaction.

- [ai] Equipment configuration is imported in full into reflected native structs. GAS owns EMP stress and each firing cooldown; the combat component owns locks, launcher queues and ammunition. All clocks advance only at the coordinated 64 Hz boundary. Projectile kind and 3D torpedo motion share the durable projectile actor, with stationary mines using the same system relevance and save lifecycle.

- [ai] Movement RPCs retransmit the eight latest unacknowledged sequenced commands in bounded unreliable batches. Authority consumes one accepted command per fixed step. Replicated acknowledgement refers to a consumed command, rather than the last packet received. This corrects latest-intent sampling under packet loss.
- [ai] Multi-mount battery and special fits follow legacy catalogue order and preserve the hull battery floor. Crew assignment is derived from Playable.crewed (pilot-operated mount count), rather than accepted as a client-selected authority override.

- [ai] Busy mixed-combat scale validation exposed redundant GAS attribute writes and headless cosmetic cue loads. Retain Actors and the fixed-step ordering, suppress commandlet cosmetics, and avoid attribute mutation when the value is unchanged. Re-run the unchanged 500-ship gate before accepting this optimization.

- [ai] The busy 500-NPC fixture remained above budget after removing redundant GAS work (8.29 ms). Revisit queries inside the Actor architecture: use a per-system spatial broad phase for projectile collisions, retaining swept and 3D narrow-phase tests and all 64 Hz updates.

- [ai] New hosted worlds default to the native Shared sandbox population profile (500 mortal NPC Actors). Authored preserves the legacy population counts and balance. Invulnerability remains exclusive to explicit synthetic scale fixtures. Recovery now places captains alongside the selected station; crime records use pre-shield damage as in the source.

- [ai] Import resolved presentation tuning as reflected native Feel structs. PlayerCameraManager owns local camera trauma and velocity lead; controller rumble and cues are cosmetic. Solo dilation uses native time tuning while hosted worlds remain real-time.

- [ai] Preserve the source career semantics: record completed solo runs, wins, deepest wave and boarding totals once at outcome. Store career independently from personal preferred hull/fit and host campaign snapshots; quit mid-run adds no completed run.

- [ai] Transfer the source star (radius 120) and two planet landmarks, solid hull separation, projectile absorption and patrol line of sight. Define docking proximity from the station surface: the source centre-distance 95 gate was unreachable outside its 90-radius solid body plus hull radius. Keep the authored 95-unit interaction distance and three-second dwell.

- [ai] Rebuild the source six-face sky atlas through an Unreal material sampled by local sky Actors, and animate stellar noise with native material expressions. Engine trail assets are Niagara systems attached to authored mesh sockets; no Rust shader or audio runtime remains.

- [ai] Frozen docked/anchored ships must still sequence and acknowledge input. Otherwise long docking sessions exceed the movement sequence window and prevent steering after undocking. Freeze integration while retaining prediction history and acknowledgement.

- [ai] Visual verification corrected two migration details: preserve Corsairs=Executioner/green and Freebooters=Challenger/purple as credited by the source; fully load the String Table when constructing menu choices so asynchronous missing labels cannot be cached as option IDs. Render reports use the RHI adapter, not the desktop display adapter.

- [ai] The first concentrated-combat render check failed (29.4 ms p95). Scope cosmetic scene updates to the local player system even on a listen host, retain all-system authoritative simulation, disable unused navigation influence and planar mesh/projectile shadows, and re-run the graphics gate from within the busy arena. Mesh visibility is local component state so this optimization cannot hide another captain's system through Actor replication.

- [ai] Busy rendering exposed a source-parity error: native mines emitted hit feedback every 64 Hz step, while the source reports it every 0.25 seconds. Restore that cadence without changing burn damage, save its report clock, and limit burst lifetime to native impact tuning. Restore camera-relative gamepad cursor integration so close torpedo targets can be locked rather than forcing the cursor to maximum range.

- [ai] Render acceptance must run as a listen host so standalone hit-stop cannot reduce the real-time simulation workload. Include its simulation clock and p95 step time in the report. Preserve civilian no-cripple behavior, scanner confirmation before patrol engagement and solo celestial landmarks. A failed Continue must disable saving before returning to the menu so it cannot overwrite the rejected campaign.

- [ai] Projectile attribution carries the firing faction and profile after its source pawn disappears. Apply heat and rescue reputation to disconnected host records as well as connected PlayerStates; the same records are restored on reconnect. Host connection loss clears the local session and returns through Unreal disconnect handling to Menu.

- [ai] A crippled sandbox captain can request free station recovery through the native menu, R or controller D-pad down; Start opens the controller menu. Authority rejects recovery in solo scenarios so the Skirmish outcome cannot be bypassed.

- [ai] Packaged Continue exposed a teardown overwrite: player/clock checks passed while NPCs vanished because Shutdown saved after NPC actors had ended play. Finalize saving at OnWorldBeginTearDown and disallow further saves from that world. Defer restore to the first world simulation boundary, defer identity until the saved world ID is loaded, and require a nonempty NPC population in session continuation probes. Skip unidentified controllers during snapshot restoration. Shipping ignores engine command-line map overrides, so expose normal VTHostWorld/VTJoinAddress frontend flows in GameInstance.OnStart instead of enabling debug map overrides.

- [ai] New captain identity requests carry that client's selected hull and fit, validated by the host. Store initial selection per controller rather than using the host GameInstance selection for every guest. Returning captains restore their saved fit regardless of the requested initial choice. Joining through either menu flow stores the local selection before travel.

- [ai] Starting solo play saves and closes any hosted session, then resets transient world identity and captain records. This prevents a prior campaign fit from replacing the solo selection and prevents solo identity tokens from overwriting campaign reconnect tokens. Ignore captain capture from worlds already finalized for teardown.

- [ai] Complete the authored camera rig in PlayerCameraManager: real-time orbit/free-look, broadside lock, overhead torpedo/warp framing by vertical FOV and engagement range, idle/reverse recenter, menu orbit, eased focus/distance/FOV and impact kick. Native camera presentation owns no gameplay state.

- [ai] Restrict player crippling/boarding to sandbox play. Solo captains retain the source ability to fight at low hull until destruction; civilians remain excluded from crippling.

- [ai] Close acceptance gaps with a Development-only replicated gameplay fixture. It arranges initial poses/resources, then clients use the shipping intent, station and recovery interfaces; observations come from authoritative results and replicated client state. Keep scenario fixtures out of Shipping and record setup separately from actions under test. Measure reconciliation corrections and run a two-hour mortal-world soak with repeated guest reconnects and boundary save/load cycles.

- [ai] A real five-second packet blackout reproduced permanent input rejection after the 256-sequence window. Permit only the additional sequence advance justified by elapsed 64 Hz input time, with a bounded recovery allowance; continue rejecting invalid, duplicate and arbitrary future input. Smooth small reconciliation differences only in cosmetic transforms, snapping system changes/large teleports; authoritative motion and damage remain untouched.

- [ai] Validate AI through legacy behavioral expectations (beam combat, EMP priorities, quiet-field boarding, civilians, and crew authority), alongside numerical golden corpora. Aggregate tests are not evidence that every original assertion or utility score is identical.

- [ai] Restore legacy utility ownership: mask crewed EMP/torpedo/screen/mine stations from the captain scoring and hand budget, while crew reads the full fit. EMP stance may remain selected at empty battery while resource rules wait for recharge; readiness for gun-dependent scoring includes range. Independent legacy expectations exposed and now guard both regressions.
- [ai] Initialize pilot commitment to Hold (-1), matching the source brain. Starting committed to Broadside silently grants a score bonus before any action is selected and prevented the empty-battery EMP expectation even after its resource gate was corrected.
- [ai] Make soak population explicit (default 500 mortal NPCs; -1 uses authored counts). A direct Sandbox URL otherwise uses the authored 53-NPC profile rather than the menu Shared sandbox profile. Retain that two-hour campaign evidence at its actual population and add a separate 500-NPC run; never label it as a 500-NPC soak.

- [ai] Restore the source T AI-pilot toggle as a host-owned control mode. Keep player possession, profile and camera while reusing the AIController decision path; clients submit mode intent and never AI outcomes. Disable local flight prediction in AI mode and clear held locks/warp/queued manual commands on transitions. Mode defaults to manual on restored/new pawns.

- [ai] Preserve manual broadside hold-to-aim/release-to-fire in the input adapter. Distinguish aim flags from authoritative fire pulses; AI still supplies fire intent directly. Fire pulses are consumed once per simulation step, and cancelled input does not release a shot. Camera/solo aim effects and native beam previews read aim flags.

- [ai] Optimise the 1,000-NPC stress case by removing unused simple-pilot sorts, caching utility sort distances, narrowing boarding candidates at its phase boundary, and ordering EMP range rejection before trigonometry. Preserve Actor/component ownership, simulation cadence, target ordering for utility weapons, and live contested-prize validation; do not lower AI frequency or change population/balance to pass the gate.
