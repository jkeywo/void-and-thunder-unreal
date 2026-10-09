# PIE toolbar validation — 2026-10-08

Unreal 5.8.2, reference Windows machine: Intel Core Ultra 9 275HX, 64 GB RAM.

The editor Play toolbar now contains mode, hull and checked loadout selectors.
The loaded Sandbox level declares all three supported rulesets on World Settings
asset user data; Menu declares none. The editor-only metadata is stripped during
cooking. Preferences live in EditorPerProjectUserSettings. Choices are applied
through OnPIEMapReady after profile initialization and before gameplay starts.
Optional equipment has an explicit empty-fit contract; older tagged saves retain
hull defaults when the new override flags are absent. PIE personal/career saves
use instance-specific slots and campaign files use an instance-specific directory,
including after changing the world name.

- Editor Development build passed.
- All 35 native automation groups passed: 22 clean, 13 with warnings, zero failed
  or skipped. Three new PIE cases report the first-use missing career save warning.
- Actual PIE sessions started Sandbox, Skirmish and Test Range, possessed the
  selected frigate and applied an explicitly empty optional fit before spawning.
  Automation restores editor preferences after each session.
- Model tests check toolbar registration, supported-mode detection, hull mount
  limits, pruning excess modules, empty fits, native serialization and frontend
  exclusion. No visual toolbar screenshot was captured.
- Normal bootstrap preserved all 106 authored packages. Native asset validation
  passed after explicitly adding Sandbox's supported-mode metadata.
- Busy armed 500-NPC simulation across ten systems passed: p95 4.357401 ms against
  the 8 ms budget, 1,000 measured fixed steps. This is not a new 1,000-NPC result.
- Development and Shipping build/cook/package gates passed, including cooking
  Sandbox with editor-only mode metadata.

Automation, authoring protection and scale evidence are adjacent JSON files.

- Packaged Development four-player gameplay acceptance passed (PvP attribution,
  simultaneous boarding, recovery, docking/refit and independent travel).
- Shipping four-player possession and autosave smoke passed; source metadata,
  script syntax and LFS checks passed. The original repository remains clean.
