# Void & Thunder — Unreal

Unreal Engine 5.8.2 port of Void & Thunder, a planar ship-combat game in the
Settled Dark. C++ gameplay, Blueprint/native assets, four-player player-hosted
sandbox, independently visited systems and a persistent host world.

**The full port is in progress.** See [implementation status](docs/implementation-status.md)
and the [architecture](docs/architecture.md).

## Development
Requires Unreal 5.8.2, Visual Studio C++ tools, Windows SDK and Git LFS.
Run `git lfs install` and `git lfs pull` after cloning.
The default engine location is C:/Program Files/Epic Games/UE_5.8; scripts accept
-EngineRoot for another installation.

1. Run Scripts/Build.ps1.
2. Run Scripts/Bootstrap.ps1 to generate native baseline assets and the Sandbox map.
3. Open VoidAndThunder.uproject, then Play.
4. Run Scripts/Test.ps1 and Scripts/Benchmark.ps1.

Flight: W/S throttle, A/D steer, mouse/right stick aim. Device mappings preserve
the original broadside/EMP/torpedo/warp/boost/brace/interaction keys; implementation
status identifies which gameplay devices have been ported.

Development console: VTHost, VTJoin 127.0.0.1, VTScale 500, VTJump meridian_gate,
VTSave and VTLoad. VTJump is a development travel helper, not the final charged
jump interaction.

Original game baseline: c138f2c9caab77ed8288ddcb46d1622e471c2b15.
The vendored setting remains read-only and retains its source attribution.

Current playable prototype: planar flight, port/starboard broadsides, directional
shields, brace, boarding and station recovery. Other device mappings are reserved
for the remaining port. `VTRecover` requests recovery when your hull is disabled.

Validate native assets with `Scripts/ValidateAssets.ps1`. Package with
`Scripts/Package.ps1` (add `-Configuration Shipping` for Shipping). Local builds
are written to `Artifacts/<configuration>/Windows/` and are not committed.
`Scripts/Multiplayer.ps1 -Packaged -Emulate -Reconnect` validates the packaged
Development build, including a live host save/load and guest reconnection.
