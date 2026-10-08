# Five deep gameplay modules — 2026-10-08

Fit selection, projectile creation, phase-scoped ship queries, captain standings and gate passage now own their complete rules behind domain interfaces. This follows the approved plan and preserves Actor/GAS authority, 64 Hz ordering, native tuning, campaign schema 6 and identity tokens. The original Rust repository remains unchanged.

## Interfaces and ownership

- `FVTFitEditor` accepts valid edits and retains the previous selection on rejection. UMG and PIE use the same operations, mount constraints and crew derivation. Checkbox editing converts explicit legacy primary choices before toggling, preserves explicit empty optional sets, and reports catalogue-order pruning on smaller hulls. The preview shows inherited defaults or mount counts, resolved devices and crew counts. Start/Join/Refit persist the accepted frontend selection; PIE retains its own settings.
- `FVTProjectileSpawnSpec` initializes every firing/restoration path before deferred spawning completes. Kind, durable IDs, source attribution, pose history and torpedo motion are available when BeginPlay registers/presents the Actor. Restored attribution survives a missing source Actor.
- `FVTShipQueries` owns ordered membership, durable lookup, spatial cells and boarding candidates. Membership invalidates on spawn, destruction, travel and replicated state changes. Spatial/boarding views open and close at their simulation phases; stale use asserts in development. Actor state and live eligibility remain authoritative; pairwise contacts and tie ordering remain unchanged.
- `FVTCaptainStandings` resolves exactly one owner for each profile, preferring connected PlayerState over the disconnected record. Crime, rescue, decay and station heat payment share the rules. No adapter pointer survives an operation, and unknown profiles do not create campaign records.
- `UVTGatePassage` owns one replicated phase/charge/destination/arrival state. Movement and prediction sample the same geometry and clock; only the host commits system transfer. Snapshot adapters preserve existing tagged schema-6 fields and rebase elapsed arrival time on reconnect. Loading cancels held actions while retaining unrelated docking progress. Blueprint presentation has read-only status access. Reverse staging, held charge, forward aperture crossing, 1200 acceleration, 0.35-second braking, unchanged endpoint and traveller-only white flash remain.

## Delivery

The five isolated implementation commits are `1b56841` (fits), `504dda3` (projectiles), `7c15539` (queries), `e595423` (standings) and `4a774a9` (gates). The integration commit contains compatibility corrections, remaining presentation adapters, additional regression coverage and this validation record.

## Benchmarks

Unchanged busy/armed fixtures, ten systems, 500/1,000 active NPC Actors, 64 Hz. Each process measures 1,000 steps after 200 warm-up steps. Reference CPU: Intel Core Ultra 9 275HX; render GPU: NVIDIA GeForce RTX 5090 Laptop GPU. Values below are p95 milliseconds printed by the commandlet, rounded to three decimals.

| Population | Before runs | After runs | Before median | After median | Change |
|---|---|---|---:|---:|---:|
| 500 | 3.147, 2.594, 2.519 | 2.261, 2.417, 2.354 | 2.594 | 2.354 | -9.3% |
| 1,000 | 7.487, 6.894, 8.957 | 7.497, 6.773, 6.773 | 7.487 | 6.773 | -9.5% |

Both median regression gates pass; 500 remains below 8 ms. The earlier 1,000-NPC budget failure is historical: all three new runs are below 8 ms, although the pre-change measurements show machine/run variability. This measured stress result does not extend the rendered/four-player population promise beyond 500. Logs are retained locally under `Saved/Validation/DeepModules/`.

## Acceptance

Final acceptance results are recorded in `results.json`. Automation includes defaults/empty fits, real PIE/runtime edit parity, legacy checkbox removal, deferred projectile state and absent-source attribution, negative cells/large hulls/docking/destruction, broad-phase completeness against brute force, connected/disconnected standings ownership, heat payment, physical forward/reverse gates and save restoration after 30 seconds of host advancement. Existing numerical parity, authority, save/migration and multiplayer expectations remain in the suite.

The native menu and white-flash captures below were inspected. Menu focus/controller navigation and gate braking/endpoint checks use the shipped paths, rather than an isolated render-only projectile or gate fixture.

![Shared checked fit editor](RenderMenu.png)

![Local gate flash](RenderGate.png)

Final Editor compilation and all 50 automation cases passed (24 clean, 26 with fixture warnings, zero failures). Development and Shipping packages compile the final source. Four-process gameplay passed clean connections and 150 ms RTT/2% loss; final packaged latency and separate-system reconnect tests cover explicit query-phase closure. Shipping four-player possession/autosave passed. The inspected menu/gate captures and clean gameplay precede only that diagnostic query-phase closure; it does not change gameplay outcomes.

Busy/armed packaged listen-host rendering at 1920x1080, quality 2, 500 NPCs plus the captain: 8.833 ms p95 frame, 154.233 mean FPS, 3.464 ms p95 simulation. This rendered measurement precedes the diagnostic phase closure; final native gameplay/Shipping validation includes it. Bootstrap preserved all 107 authored packages and native asset validation passed.
