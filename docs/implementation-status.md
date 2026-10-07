# Implementation status

The implemented source gameplay and content have been transferred to the native
Unreal project, with the shared sandbox additions. The port retains the pinned
baseline in Migration and SourceAssets; neither runtime gameplay nor packaging
requires Rust, Bevy, the legacy browser HUD or PASM.

## Delivery coverage

| Milestone | Delivered |
|---|---|
| Repository and baseline | Runtime/editor modules, pinned resolved export and behavior inventory, MIT/credits/read-only setting, LFS, scripts, native agent guide and attributed plain decisions |
| Flight and scale | Planar 64 Hz coordinator, sequenced validated inputs, prediction/reconciliation, replica interpolation, possession, camera/controller input, ten continuous arenas and system relevance |
| Combat and solo | GAS resources/effects/activation/cooldowns, directional shields, broadsides, EMP, torpedoes, mines, point defence, boost, warp, brace, contacts/landmarks, crippling/boarding, crewed mounts, utility AI, loadouts, Skirmish, Test Range, outcomes/HUD and solo time effects |
| Shared world | Civilian/patrol populations, faction relations, scanning/distress, individual heat/reputation/credits, stations/refit, charged independent jumps, PvP, contested prizes, free station recovery, join in progress, disconnect/reconnect identities, host saves/backups/migrations |
| Content and packaging | Five native hull meshes and textures with corrected faction art/basis/socket anchors, authored tuning/feel, UMG Widget Blueprint/String Tables, native sky/stellar materials, Niagara/cue Blueprints, offline-rendered source audio, Development/Shipping Windows packages and published source |

## Acceptance and limits

See [validation evidence](validation/README.md) for measured results and commands.
Four processes are tested together and in separate systems, including latency/loss,
live host save/load, reconnect and native LAN Create/Continue/host-departure flows.
Meaningful automation covers numerical rules, equipment lifecycles, authority,
contested boarding, persistent references, failed writes, corrupt backups,
individual world attribution, background distress, scenarios and station recovery.

The 500-NPC acceptance fixture exercises concentrated mixed combat and preserves
population through invulnerability. Ordinary Shared sandbox ships are mortal and
retain the source behavior proportions; their population can decline through combat.
The 1,000-NPC case is a stress measurement, not an extension of the 500-NPC promise.

Native materials, Niagara and UMG rebuild the source presentation rather than
reproducing Rust shaders/web layout pixel-for-pixel. Source feel is imported, but
Niagara trail/burst appearance remains an Unreal interpretation. Numerical behavior
uses documented tolerances, not cross-engine binary determinism. Network probes
establish possession, motion, acknowledgement, relevance and continuity; they do
not establish a universal reconciliation error bound on every network.

Browser delivery, dedicated servers, online accounts/invites, new multiplayer
missions, host migration, legacy-save compatibility and Mass conversion remain
outside the approved scope. GitHub CI checks metadata, not licensed engine gates.
