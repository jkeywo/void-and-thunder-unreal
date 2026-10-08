# Narrative intro validation - 2026-10-09

Unreal Engine 5.8.2; Windows 11; Intel Core Ultra 9 275HX; 64 GB RAM;
NVIDIA GeForce RTX 5090 Laptop GPU. The original Rust repository was not changed.

## Delivered behavior

New sandbox captains wake in a private wreck field with the engineer. The silent
protagonist learns the four-notch helm, clears debris using broadsides, then builds
a fit by choosing boost versus EMP and torpedoes versus microwarp. Each repaired
system must actually be used before proceeding. The attacker challenges the player;
disabling and boarding or destroying the attacker unlocks a physical gate exit.
Engineer/enemy portraits, mapped control hints and objectives are native UMG assets.

The chosen hull is retained; optional fit selection is hidden before a sandbox
intro. PIE has a persisted Skip intro checkbox beside its mode/hull/loadout controls.
Skipping PIE preserves advance fit selection. Runtime Skip keeps repairs already
chosen. Skirmish, Test Range and existing campaign captains bypass the intro.

Each captain has an independent authoritative checkpoint. Normal sandbox simulation
continues. Loading/reconnecting reconstructs disposable encounter fixtures, cancels
held input and retains the chosen equipment. Defeat retries the encounter without
charging credits. Intro boarding pays no farmable reward. Schema 6 remains readable;
absent tutorial state defaults to Complete.

## Gates

- Editor build passed, including the final editor-only seed improvement.
- Intro asset authoring passed with zero errors/warnings. Native asset validation
  passed with zero errors/warnings. Bootstrap preserved all 114 authored packages.
- All 57 Unreal automation tests passed (25 without warnings, 32 with warnings).
  Coverage includes both actual-use repair paths, invalid/duplicate repair requests,
  damaged-system restrictions, broadside debris destruction, boarding, physical
  gate travel, four independent arenas, defeat retry, save/load and reconnect.
- Windows Development and Shipping BuildCookRun packages passed.
- Shipping four-player possession and autosave passed; the saved envelope checksum
  and all four captain records were verified.
- Packaged four-player intro checks passed on clean connections and 150 ms RTT /
  2% loss: owner-scoped arenas, independent skip, a captain remaining in Wake while
  others progress, network repair-choice RPCs, boost/warp trials and reconnect with
  both checkpoint and fit retained. Native automation also exercises EMP/torpedoes.
- Existing packaged four-player sandbox combat/travel/boarding/recovery acceptance
  passed at 150 ms RTT / 2% loss.
- The 500-NPC busy armed benchmark passed: **2.272 ms p95**, 1,000 measured steps
  after 200 warmup steps, 8 ms budget. The report counts 14 system definitions:
  ten sandbox systems plus four empty reserved intro arenas. Population seeding
  excludes the intro arenas; the existing busy fixture concentrates half in system 0.
- Packaged opening render passed at 1920 x 1080, quality preset 2, 100% screen
  percentage: **3.697 ms p95 frame**, 321.8 mean FPS. Opening and repair-choice
  screenshots were visually inspected for text, portrait, HUD overlap and buttons.
- Source metadata, script syntax, golden baseline attribution and LFS checks passed.

The network probe places remote captains at the repair checkpoint after observing
networked helm movement; it then uses normal client RPCs and actual device inputs.
It does not claim a fully hands-off playthrough of the complete narrative. The
native story test places the ship at the helm objective after checking real movement,
and uses normal damage to prepare the disabled enemy before real boarding and travel.

## Assets and development

DA_Intro owns dialogue and repair options. WBP_Intro owns the communications panel.
ArtSource/Intro retains both original generated portraits and their prompts.
IntroAssets seeds only missing packages; its explicit RepairChoices/RefreshVisuals
upgrades target the new intro content. First-time widget seeding includes choices.
The Shipping snapshot inspector accepts both legacy Profile-first records and new
Intro-first records; checksums and bounded record counts are still validated.

See the JSON reports and screenshots beside this document. Build/test process logs
remain in Saved/Validation/Intro*.log. No 1,000-NPC performance claim is made here.
