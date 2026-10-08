# Void & Thunder — Unreal

Unreal Engine 5.8.2 port of Void & Thunder: planar ship combat in the Settled Dark,
solo Skirmish and Test Range, and a persistent four-player listen-hosted sandbox.
All ten systems simulate while the host runs. Captains can cooperate, fight each
other, board prizes, dock, refit and travel independently.

The source baseline is `c138f2c9caab77ed8288ddcb46d1622e471c2b15`.
The original repository remains unchanged. MIT code, asset credits and the
read-only setting snapshot retain their attribution.

See [implementation status](docs/implementation-status.md),
[architecture](docs/architecture.md), [hosting and controls](docs/playing.md),
and [validation](docs/validation/README.md).

## Windows download

Download the Shipping preview from [GitHub Releases](https://github.com/jkeywo/void-and-thunder-unreal/releases/tag/v0.3.1-sandbox-preview).
Extract the entire bundle and launch `VoidAndThunder.exe`. The bundle includes
controls, credits, a SHA-256 checksum and Unreal runtime prerequisites.

## Development

Install Unreal 5.8.2, Visual Studio C++ tools, the Windows SDK and Git LFS.
After cloning, run `git lfs install` and `git lfs pull`, then:

```powershell
.\Scripts\Build.ps1
```

Open `VoidAndThunder.uproject`. Play the Menu map for the frontend; the Sandbox
map is the editor's direct gameplay entry. Native assets are committed and editable
in Unreal. C++ owns authority, the simulation and explicit movement prediction;
Widget Blueprints, Niagara, materials, Sound Waves, String Tables and Primary Data
Assets supply presentation and authored content. No Rust runtime is required.

To regenerate the exported baseline, run `Scripts/ExportLegacy.ps1` against the
pinned source snapshot, followed by `Scripts/Bootstrap.ps1` and
`Scripts/ImportContent.ps1`. Regeneration overwrites generated native packages;
commit authored asset changes before using it.

## Validation and packaging

```powershell
.\Scripts\NativeValidation.ps1 # complete licensed engine pipeline
.\Scripts\NativeValidation.ps1 -Soak # includes two-hour campaign validation
.\Scripts\Test.ps1
.\Scripts\ValidateAssets.ps1
.\Scripts\Benchmark.ps1 -Population 500 -Busy -Armed
.\Scripts\Benchmark.ps1 -Population 1000 -Busy -Armed -RequireBudget
.\Scripts\Render.ps1 -Busy -Armed
.\Scripts\Package.ps1
.\Scripts\Package.ps1 -Configuration Shipping
.\Scripts\Multiplayer.ps1 -Packaged -SameSystem
.\Scripts\Multiplayer.ps1 -Packaged -Emulate -Reconnect
.\Scripts\SessionFlows.ps1 -Packaged
.\Scripts\SessionFlows.ps1 -Packaged -Continue
.\Scripts\SessionFlows.ps1 -Packaged -HostDeparture
.\Scripts\ShippingSmoke.ps1
.\Scripts\Gameplay.ps1 -Packaged -RoundTripMs 250 -Loss 5 -Blackout
.\Scripts\ExportParity.ps1 # regenerate independent Rust reference corpus
.\Scripts\ReleaseBundle.ps1
```

Engine-based scripts default to `C:/Program Files/Epic Games/UE_5.8` and accept `-EngineRoot`.
Windows packages go to `Artifacts/<configuration>/Windows/`. Start
`VoidAndThunder.exe` there. Packages and generated validation logs are local build
outputs; source and compact validation evidence are published in this repository.
GitHub checks validate project metadata; licensed Unreal build and gameplay gates
run locally and are reported separately.

### Native asset authoring

Normal validation never rebuilds authored content. `Scripts/Bootstrap.ps1` seeds a
fresh checkout and preserves existing packages. `-UpgradeNative` adds missing
primary definitions, class references, input contexts/remapping metadata and
presentation budgets without replacing layouts or existing definition/budget assets.
`-Regenerate` explicitly replaces the pinned migration baseline: commit authored
work before using it. `VTContent` also requires `-Regenerate` to replace an existing
presentation import.

Edit ship/equipment/system/scenario Primary Data Assets under `Content/Data/` for
gameplay definitions. The aggregate arrays retain the migration fallback; primary
definitions override matching IDs when a world loads. Configure the ship and UI
Blueprint classes on `DA_GameData`. Meshes are preloaded at the world-loading boundary
through Asset Manager, rather than synchronously loaded for every spawn.

Input Actions cover menu/autopilot/recovery as well as flight. Mapping contexts
remove flight controls while menus or docking are active and cancel held inputs.
Blueprint UI can call `RemapControl` with the mapping's stable name (for example
`Throttle_W`) and a new key; Enhanced Input User Settings persist the change.
CommonUI remains an optional later integration, not a required dependency.

World autosaves queue a background byte write; manual save, load and host teardown
flush pending writes. The game thread still captures/serializes the boundary
snapshot. Failures retain the previous committed file and report an error.
Native sound attenuation/concurrency and Niagara Effect Types under `Content/Audio`
and `Content/Effects` are editable presentation budgets, separate from gameplay.

Widget Blueprints can override `GenerateChoiceWidget` and read `GetChoiceItem` for the
stable definition ID, localized text and authored style. Presentation overrides
do not alter selection identity or authoritative gameplay.

Validation of audit items 1–7 is recorded in
[the native integration report](docs/validation/native-integration-2026-10-08/README.md).
