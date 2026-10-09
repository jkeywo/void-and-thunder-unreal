# Unreal native integration — 8 October 2026

Implements audit items 1–7 against base commit
`e054cc2ec1630f6f18523820a33501e4047dd0a6`, using Unreal Engine 5.8.2.
The original Rust repository and vendored setting remain unchanged.

| Item | Delivered behavior |
|---|---|
| 1. Native authoring | Default bootstrap preserves authored packages; missing catalogue with existing content is refused. Additive native upgrade seeds missing assets. Ship/UI Blueprint classes are configurable. Regeneration requires an explicit flag. |
| 2. Autosaves | Boundary capture and serialization remain on the game thread; immutable bytes write on the thread pool. Pending writes flush before load/manual saves/teardown. CRC, backup recovery and atomic replacement remain. |
| 3. UMG | Typed widget bindings, cached lookups and coalesced session/replication/GAS notifications replace per-frame view rebuilding. Stable selection IDs remain separate from localized FText labels. Blueprint option presentation inherits authored styling. |
| 4. Asset Manager | Separate ship/equipment/system/scenario Primary Data Assets override pinned migration defaults. Asset Manager owns definition loading and presentation preloading at world initialization. |
| 5. GAS | Ability commit/blocking/cooldown tags use native GAS semantics. Coordinator-driven Ability Tasks retain 64 Hz duration ownership and save restoration. Server-only security rejects direct client activation/termination RPCs. |
| 6. Enhanced Input | Menu, autopilot and recovery join native Input Actions. Common/flight/menu/docked contexts cancel held flight actions during transitions. Enhanced Input User Settings persist remapping exposed to Blueprint. |
| 7. Presentation budgets | Editable native attenuation and sound concurrency; Niagara Effect Types bound burst/trail counts and distance. Audio listener follows the ship. |

All **18 final native pipeline gates passed**: source checks, Editor build,
authoring protection, asset validation, automation, 500-NPC simulation,
Development packaging, four-player clean/150 ms RTT with 2% loss/250 ms RTT with
5% loss and blackout gameplay, reconnect, Create/Continue, host departure,
rendered arena/menu, Shipping packaging and Shipping multiplayer.

All **25 automation groups passed**: 19 succeeded and six succeeded with
warnings; zero failures or unrun groups. New regressions cover fixed-step GAS
cooldown restore, client GAS RPC rejection, asynchronous save/corruption/failed
replacement recovery, and localized stable selection identity. Warnings are
retained in the engine report rather than presented as a warning-free run.

Default bootstrap preserved all **93 authored packages byte-for-byte**.
Removing the catalogue temporarily caused the expected refusal (exit 11), did
not reseed it, and the original catalogue was restored with its hash unchanged.

On the reference Windows 11 machine (Core Ultra 9 275HX, 64 GB RAM, RTX 5090
Laptop GPU), concentrated mixed-combat fixtures across ten systems measured:

| Fixture | Result |
|---|---:|
| 500 active NPCs, busy/armed | 2.7712 ms p95 fixed step |
| 1,000 active NPCs, busy/armed, enforced 8 ms gate | 7.2722 ms p95 fixed step |
| Packaged 1080p Balanced, 500 NPCs plus captain | 6.6107 ms p95 frame; 227.625 mean FPS |
| Same rendered scene simulation | 3.0724 ms p95 fixed step |
| Packaged menu | 2.9546 ms p95 frame; initial focus and controller navigation passed |

Simulation percentiles use 1,000 measured samples after 200 warmup steps.
Synthetic benchmark ships are invulnerable to retain population. These results
are local reference-machine observations, not a 1,000-NPC network/render guarantee.
The final menu capture was visually inspected for readable authored option styling.

Asset loads are completed at the world-loading boundary; this change does not
implement a seamless asynchronous loading-screen flow. Snapshot serialization
still costs game-thread time. CommonUI remains optional pending compatibility
assessment; Enhanced Input migration is complete without adding it. The remapping
API is available to Blueprint; a new keybinding settings screen is outside this
change. No gameplay tuning, AI cadence, offline advancement or Actor/entity
architecture was changed.

See the root README's native authoring workflow and the recorded `[ai]` decisions.
Compact adjacent JSON files retain the final native evidence; full logs and
packaged builds remain under `Saved/Validation` and `Artifacts` respectively.

The initial 180-second soak host and all emitted guest reports passed, but the
script failed its requested reconnect count: only one rejoin fit after engine
startup overhead. Its host observations are retained in `soak-short-host.json`;
this is not recorded as a passed aggregate soak. A longer rerun is recorded below.
Gameplay logs also contain native GAS cosmetic-cue RPC throttle warnings during
bursts; the engine may suppress excess audiovisual cues per update. Authoritative
combat checks passed. Presentation budgets do not change that RPC transport limit.

The **240-second packaged rerun passed** with 500 initial mortal NPCs, 150 ms RTT
and 2% packet loss. Two guest rejoins retained the same profile and ship identity;
all ten systems advanced and four captains were observed together. Population
fell through normal combat (430/413/407/398 ships at minute samples), so this is
not a constant-500 stress benchmark. Captains are invulnerable in this fixture.
The run did not reach the longer soak's save/load-cycle interval (zero cycles);
automation and the packaged Continue/reconnect gates provide load coverage.
This short regression run does not replace earlier long-duration acceptance.
