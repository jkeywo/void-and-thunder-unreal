# Implementation status

The implemented source gameplay and content have been transferred to the native
Unreal project, with the shared sandbox additions. The port retains the pinned
baseline in Migration and SourceAssets; neither runtime gameplay nor packaging
requires Rust, Bevy, the legacy browser HUD or PASM.

## Delivery coverage

| Milestone | Delivered |
|---|---|
| Repository and baseline | Runtime/editor modules, pinned resolved export and behavior inventory, MIT/credits/read-only setting, LFS, scripts, native agent guide and attributed plain decisions |
| Flight and scale | Planar 64 Hz coordinator, sequenced validated inputs, prediction/reconciliation, replica interpolation, possession, player AI pilot handover, camera/controller input, ten continuous arenas and system relevance |
| Combat and solo | GAS resources/effects/activation/cooldowns, directional shields, hold/release broadsides and aim beams, EMP, torpedoes, mines, point defence, boost, warp, brace, contacts/landmarks, crippling/boarding, crewed mounts, utility AI, loadouts, Skirmish, Test Range, outcomes/HUD and solo time effects |
| Shared world | Civilian/patrol populations, faction relations, scanning/distress, individual heat/reputation/credits, stations/refit, charged independent jumps, PvP, contested prizes, free station recovery, join in progress, disconnect/reconnect identities, host saves/backups/migrations |
| Content and packaging | Five native hull meshes and textures with corrected faction art/basis/socket anchors, authored tuning/feel, UMG Widget Blueprint/String Tables, native sky/stellar materials, Niagara/cue Blueprints, offline-rendered source audio, Development/Shipping Windows packages and published source and downloadable Shipping preview |

## Acceptance and limits

See [current acceptance evidence](validation/acceptance-2026-10-07/README.md) for measured results and commands. All 18 native gates and 20 automation groups passed; the extracted Shipping download passed ordinary four-player hosting and autosave.
Four processes are tested together and in separate systems, including latency/loss,
live host save/load, reconnect and native LAN Create/Continue/host-departure flows.
Meaningful automation covers numerical rules, equipment lifecycles, authority,
contested boarding, persistent references, failed writes, corrupt backups,
individual world attribution, background distress, scenarios and station recovery.

The 500-NPC acceptance fixture exercises concentrated mixed combat and preserves
population through invulnerability. Ordinary Shared sandbox ships are mortal and
retain the source behavior proportions; their population can decline through combat.
The latest optimisation passed all 18 native gates and 21 automation groups. The optimised 1,000-NPC fixed-step case measures 6.33 ms p95 and now passes the 8 ms stress budget on the reference machine; it is not an extension of the 500-NPC rendered/network promise. See [stress optimisation](validation/optimisation-2026-10-08/README.md).

Native materials, Niagara and UMG rebuild the source presentation rather than
reproducing Rust shaders/web layout pixel-for-pixel. Source feel is imported, but
Niagara trail/burst appearance remains an Unreal interpretation. Numerical behavior
uses documented tolerances, not cross-engine binary determinism. Network probes
establish possession, motion, acknowledgement, relevance and continuity; they do
not establish a universal reconciliation error bound on every network.

Browser delivery, dedicated servers, online accounts/invites, new multiplayer
missions, host migration, legacy-save compatibility and Mass conversion remain
outside the approved scope. GitHub CI checks metadata, not licensed engine gates.
