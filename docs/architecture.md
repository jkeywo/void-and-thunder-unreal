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

## Required final ownership
GameMode: authority, identity validation, spawning and respawn.
GameState: time and global system summaries.
PlayerState: credits, standings, heat, equipment and player ownership.
Ship components/GAS: physical state, equipment, interactions and ability state.
Save subsystem: consistent snapshots, durable IDs, guest records and backups.

See implementation-status.md for the current milestone and outstanding work.
