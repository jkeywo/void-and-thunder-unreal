# Source behavior transfer map

Pinned source: `c138f2c9caab77ed8288ddcb46d1622e471c2b15`. The complete source module/test inventory is in `Migration/source-inventory.json`; typed resolved values are in `Migration/resolved-baseline.json`.

| Source simulation module | Native implementation |
|---|---|
| `ai.rs` | VTAI.cpp / VTEquipment.cpp |
| `avenge.rs` | VTWorld.cpp / VTGameData.cpp / VTGameplay.cpp |
| `civilian.rs` | VTAI.cpp / VTEquipment.cpp |
| `collide.rs` | VTContacts.cpp / VTWorld.cpp / VTWorldAnchor.cpp |
| `combat.rs` | VTCombat.cpp / VTEquipment.cpp / VTProjectiles.cpp |
| `components.rs` | VTGameplay.cpp / VTTypes.cpp / VTTests.cpp |
| `distress.rs` | VTWorld.cpp / VTGameData.cpp / VTGameplay.cpp |
| `drive.rs` | VTGameplay.cpp / VTTypes.cpp / VTTests.cpp |
| `emp.rs` | VTCombat.cpp / VTEquipment.cpp / VTProjectiles.cpp |
| `events.rs` | VTCombat.cpp / VTEquipment.cpp / VTProjectiles.cpp |
| `harness.rs` | VTGameplay.cpp / VTTypes.cpp / VTTests.cpp |
| `heat.rs` | VTWorld.cpp / VTGameData.cpp / VTGameplay.cpp |
| `interact.rs` | VTContacts.cpp / VTWorld.cpp / VTWorldAnchor.cpp |
| `jump.rs` | VTContacts.cpp / VTWorld.cpp / VTWorldAnchor.cpp |
| `lib.rs` | VTGameplay.cpp / VTTypes.cpp / VTTests.cpp |
| `mines.rs` | VTCombat.cpp / VTEquipment.cpp / VTProjectiles.cpp |
| `patrol.rs` | VTAI.cpp / VTEquipment.cpp |
| `pilot.rs` | VTAI.cpp / VTEquipment.cpp |
| `piracy.rs` | VTContacts.cpp / VTWorld.cpp / VTWorldAnchor.cpp |
| `plugin.rs` | VTGameplay.cpp / VTTypes.cpp / VTTests.cpp |
| `point_defense.rs` | VTCombat.cpp / VTEquipment.cpp / VTProjectiles.cpp |
| `relations.rs` | VTWorld.cpp / VTGameData.cpp / VTGameplay.cpp |
| `reputation.rs` | VTWorld.cpp / VTGameData.cpp / VTGameplay.cpp |
| `shield.rs` | VTCombat.cpp / VTEquipment.cpp / VTProjectiles.cpp |
| `ship.rs` | VTGameplay.cpp / VTTypes.cpp / VTTests.cpp |
| `spawn.rs` | VTGameplay.cpp / VTTypes.cpp / VTTests.cpp |
| `starbase.rs` | VTContacts.cpp / VTWorld.cpp / VTWorldAnchor.cpp |
| `torpedo.rs` | VTCombat.cpp / VTEquipment.cpp / VTProjectiles.cpp |
| `tuning.rs` | VTTypes.h / DA_GameData / VTBootstrapCommandlet.cpp |
| `util.rs` | VTGameplay.cpp / VTTypes.cpp / VTTests.cpp |
| `world.rs` | VTWorld.cpp / VTGameData.cpp / VTGameplay.cpp |

The original client renderer, browser/native HTML HUD, RON editor and input/audio adapters are replaced by Unreal-native authoring and presentation. See `docs/architecture.md` for the editable packages. Skirmish and Test Range are imported into native scenario definitions and directed by `VTScenarios.cpp`. Exported default/feel values preserve sparse legacy RON resolution; the game reads native assets at runtime.

## Numerical and scenario tolerances

Fixtures compare forward/reverse thrust and broadside direction to source expectations within 1e-4; exponential drag, speed bounds and world contacts within 1e-3 source units; and warp range within 0.1 source unit. Source units map to 100 Unreal centimetres with Y reflected. Discrete counts, ammunition, attribution, identities and award ownership are exact. Timed actions allow one 1/64-second boundary where the original schedule/event order differed. UE double vectors and Rust f32 arithmetic need not produce identical replay bytes.

Automation groups source assertions by rule/lifecycle rather than preserving every Rust test as a separate Unreal test. The runtime fixtures cover actual GAS effects and spawned actors. Four-process packaged checks add movement acknowledgement, local-system actor relevance, travel, reconnect identity/economy and snapshot continuity. They are bounded local-process tests, not WAN soak tests.

## Recorded migration corrections

The source station centre-distance docking threshold was unreachable outside the solid station body for a normal hull. Native docking measures the authored distance from the station surface; the three-second dwell and standing refusal remain. Source models retain +X bow, Z-up, their ~44-unit length and credited faction textures after glTF import basis conversion. Native sockets retain the rig-sidecar engine anchors. Source mine feedback cadence is 0.25 seconds while its damage remains at 64 Hz. These and other agent-origin choices retain `[ai]` provenance in `docs/decisions.md`.

The expanded independent golden corpus and current integration evidence are mapped in [parity coverage](parity-coverage.md) and [acceptance](validation/acceptance-2026-10-07/README.md).
