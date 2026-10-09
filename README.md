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
map is the editor's direct gameplay entry. The three dropdowns beside Play select
mode, captain's ship and checked loadout for the next PIE session. Sandbox supports
Skirmish, Test Range and Sandbox; Menu has no direct gameplay modes. Choices are
saved per user, and PIE campaigns/profiles are separate from normal play.
Optional modules respect hull mount limits; unticking all leaves optional mounts
empty. Use hull default equipment restores the authored fit. Native assets are committed and editable
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

The flight HUD includes resource/shield bars and an equipment strip. Flight pace
(`FlightSpeedMultiplier`, currently 2) and projectile visual radius (7 simulation
units) are editable on `DA_GameData`. Shot hit radii are unchanged. Player yaw
input is converted once at the reflected coordinate boundary. The native cube
sky, animated sun, emissive projectiles and plane grid are under `Content/Environment`.
After an explicit baseline regeneration, `Scripts/Playability.ps1` reapplies these
targeted authored upgrades; normal validation preserves their packages.

After packaging, double-click `run-unreal.bat` to start the current Development
build from this checkout. Published older preview ZIPs do not contain these fixes.

The original amber CRT GUI is restored in native UMG/Slate: five salvaged-metal panel textures, cooldown dials, segmented bars, torpedo tubes, shield edges and projected ship rings. Tab toggles controls; F toggles the sandbox chart (controller View/right-stick respectively). The centered title card keeps Cast off and Test range, with hosting/joining under Shared world. Run `Scripts/HUDStyle.ps1` for the explicit asset upgrade after building the Editor module. `Scripts/ExportLegacyHUD.cjs <source hud.html>` regenerates static artwork with Playwright; it is a development tool, not a game dependency. Normal bootstrap preserves authored UI.


New sandbox captains begin in a private wreck field with the engineer's narrative
intro. Learn the helm and broadsides, choose **boost or EMP**, then **torpedoes or
microwarp**, confront the attacker, board their disabled ship and escape through
the gate. You choose the hull beforehand; the engineer's repairs build the fit.
The captain remains silent. Dialogue uses engineer/enemy portraits and mapped
control hints. Choices and checkpoints survive saving and reconnecting.

In the Sandbox level, the **Skip intro** checkbox beside the PIE mode/hull/loadout
controls starts directly in the sandbox and enables advance loadout selection.
Skirmish and Test Range always bypass the intro. Existing campaign captains stay
in the sandbox. The in-game dialogue also provides Skip intro.

`Scripts/IntroAssets.ps1` seeds the intro's native data, portraits, wreck mesh and
UMG widget while preserving existing assets. `-RepairChoices` and `-RefreshVisuals`
are explicit targeted upgrades. Edit dialogue and encounter tuning in
`Content/Data/DA_Intro`, presentation in `Content/UI/WBP_Intro`.
`Scripts/IntroNetwork.ps1 -Packaged -RoundTripMs 150 -Loss 2` exercises independent
four-player introductions, repair choices, skipping and reconnecting.


Press **G** for the full-screen star chart, or **F** for its compact form. Click a
system to plot the shortest connected gate route; close the chart to see the next
gate waypoint. The waypoint advances after each jump. Click the current system to
clear guidance. The chart leaves the world running. The full-screen map suppresses flight input
while you select a destination; the compact chart allows piloting.

Outer ship rings show your captain's relationship: green friendly, amber neutral,
red hostile (including hostile heat), blue your own ship. Unfitted Ctrl/Shift
systems do nothing, including camera changes. System layout distances and celestial
radii are five times larger; arenas are ten times farther apart and only stars are
visible outside your current system. At the nearest star's system boundary, thrust
and maximum speed fade linearly from 1x to 0.01x over 100 m, identically on host and
predicted clients. Ships, weapons, stations and gate apertures retain their sizes.
Existing schema-6 campaigns migrate their spatial positions once when loaded.

Docking and frontend menus use larger native text and controls, with wrapping loadout choices. `Scripts/HUDStyle.ps1 -MenuReadability` reapplies only the menu readability settings to WBP_UI; ordinary bootstrap preserves authored packages. `Scripts/Render.ps1 -Docked` captures a docked menu for visual validation.
