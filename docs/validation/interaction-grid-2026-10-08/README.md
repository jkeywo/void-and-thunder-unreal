# Contextual interactions and radial grid — 8 October 2026

Nearby eligible ship prizes now display an amber object-anchored hold prompt, live keyboard/controller mappings and boarding progress. Jump prompts name the linked destination and show jump progress. Stations explain automatic hold-position docking with progress instead of inventing a button requirement. HUD hints read the replicated authoritative boarding target/travel state, check range/system/eligibility, and disappear when the prize is claimed, removed or no longer reachable. The old unconditional BOARD labels were removed. The HUD does not award loot or change interaction rules.

The reference material now draws anti-aliased concentric rings and radial spokes about a dynamic StarCentre parameter. The local grid actor chooses the nearest authored star in the captain's current system and applies that system's arena translation. Non-stars are excluded; a system with no star hides the grid. RingSpacing (Unreal centimetres, default 20000) and SpokeCount (default 24) remain native editable material parameters. Scripts/RadialGrid.ps1 invokes a targeted GridOnly authoring path that updates just M_ReferenceGrid; normal bootstrap remains protective. An existing unity-build local/global Freebooters name collision was repaired by renaming the local constant, without changing faction rules.

Validation passed:

- Editor build, grid authoring, protected bootstrap (106 packages), asset validation and all 31 automation groups: 21 clean, ten with warnings, zero failed/skipped.
- ContextInteractionHints verifies host-selected same-faction player looting, actual progress, claimed/out-of-system/out-of-range suppression, disabled-captain suppression, and jump destination/progress.
- RadialGridNearestStar verifies selection, exclusion of non-stars, changing nearest star, system translation and absence handling.
- Scripts/Render.ps1 -Environment -Interaction routes the mapped interaction key through Slate/Enhanced Input, observes the loot hint and held progress, then verifies exactly one award and removal of the hint. Editor 1080p and packaged Development 1080p/720p runs passed. Captures were inspected for prompt legibility and star-centred rings/spokes.
- Busy/armed 500 active NPCs across ten systems: simulation p95 2.384499 ms against 8 ms. Packaged busy 1080p, quality 2, 500 NPCs plus captain: frame p95 8.062501 ms, mean 171.035 FPS on the recorded reference machine.
- Development and Shipping packages, four-player gameplay acceptance, and Shipping possession/autosave smoke passed.

The local run-unreal.bat launches the refreshed Development package. Original Rust source and the existing release archive are unchanged. Adjacent JSON/captures retain this revision's evidence; older latency/loss/lifecycle results are retained in the previous validation folders.
