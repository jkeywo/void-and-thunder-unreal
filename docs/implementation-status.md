# Implementation status

The full requested port is in progress. No full-parity claim is made.

## Milestone 1
Project and native module structure created. Original license and setting copied.
Pinned source baseline exporter prepared. Typed export of all five classes, ten systems, both scenarios, loadouts and tuning
completed. Native data/input assets and Sandbox map generated successfully.

## Milestone 2
Flight math, sequenced input, authoritative stepping, prediction/reconciliation,
per-system ship relevance, Enhanced Input authoring, all-system population and
scale benchmark implemented. Editor and Development game targets compile. Three initial automation tests pass.
Four separate game processes pass predicted motion, server acknowledgement,
independent travel and exactly 50 relevant NPCs per client under 150 ms RTT/2% loss.
The flight-only 500-NPC gate measured p95 0.071701 ms; 1,000 measured 0.297997 ms.
Combat and rendering are not covered by these performance measurements.

## Combat and persistence implemented so far
GAS broadsides own wind-up and independent reload clocks. Native directional
shield banks, regeneration suppression, brace reduction, swept projectile contacts,
system-scoped hull contacts, crippling, per-captain boarding claims and station
recovery are implemented. Automation verifies authored volley damage and PvP through
same-faction protection. The utility combat AI and remaining weapons are still pending.

Campaign snapshots capture all currently implemented ship, shield, broadside cooldown,
boarding, player and projectile state. Checksums reject corrupted payloads; Windows
atomic replacement preserves the old primary on failed writes. Backup recovery and
persistent source-reference restoration have automation coverage. Guest profile/token
handshakes reject duplicate identities and invalid reconnect tokens. Guest capture
uses PawnLeavingGame before Unreal destroys the pawn. Packaged Development validation passes four players under 150 ms RTT and 2% loss,
including live host save/load and guest reconnect with the same profile/ship IDs,
position continuity, credits and boarding totals. Schema 3 migrates schema 2 pose clocks.

## Remaining requested work
EMP, torpedoes, mines, point defence, boost, microwarp, native loadout catalogue and
crewed mount selection; utility combat AI; solo scenarios/outcomes/time effects;
native menus/HUD, LAN discovery and Create/Continue flows; faction/world responses,
station interactions and charged travel; schema migrations and further multiplayer
failure/contested-interaction tests; native art/audio/effects import, render profiling
and further packaged multiplayer/Shipping gameplay validation.

The repository is published. It is a partial port, not a finished game. Scale fixtures
are synthetic and invulnerable to keep the population stable; render performance and
full combat simulation performance have not yet been accepted. Motion probes verify
movement, acknowledgement and relevance; they do not yet prove a reconciliation error
budget under every network condition.

## Current build evidence
Eight automation tests pass. Unreal native asset validation reports zero errors or
warnings. Editor, Development and Shipping compile, and both Windows packages build.
Packaged Development multiplayer/reconnection passes. The latest flight/contact
fixtures measure p95 0.608999 ms for 500 NPCs and 1.350697 ms for 1,000 NPCs.
These are not full-combat or rendering acceptance measurements. See docs/validation/.
