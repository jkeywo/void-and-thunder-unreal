# Reverse gate approach and real torpedo visuals — 2026-10-08

Gate approach compares translation plus turn estimates using the selected hull's thrust, reverse multiplier, forward drag and slow-speed turn rate. Nearby staging behind a forward-facing hull can use reverse; sufficiently long approaches may instead turn and move forwards. The entry phase never chooses reverse, and the existing forward swept-aperture test remains authoritative. This is an approximate course estimate, not a globally optimal path planner.

The previous torpedo render fixture used deferred spawning but normal equipment firing assigned Kind after BeginPlay. Actual fired torpedoes therefore kept cannon visuals. Device firing and save restoration now initialize kind before FinishSpawning, and replicated kind notifications refresh client material/scale. The fixture now uses SpawnDeviceProjectile, the same helper as the torpedo salvo. The native data asset radius is 1.25, half the previous 2.5; M_Torpedo uses pure red (linear 0.4, 0, 0). Cannon visuals and gameplay collision/damage remain unchanged.

Unreal 5.8.2 Editor build, asset validation and protective bootstrap passed (108 existing packages preserved). All 42 automation groups passed: 22 clean, 20 with fixture warnings, none failed or skipped. Coverage includes all three hulls physically travelling through gates, backward entry rejection, reverse-vs-forward course selection, equipment projectile lifecycle and existing save/load round trips. Automation.json and authoring-protection.json retain the native reports.

The fresh busy/armed 500-NPC benchmark passed at 5.270500 ms p95 against 8 ms over 1,000 measured steps. Packaged Development render passed at 1920x1080, quality 2: 3.936801 ms p95 frame, 308.962 mean FPS. The report verifies the actual equipment-spawned torpedo has radius 1.25 and M_Torpedo material; the screenshot was inspected and shows a smaller red dot. Mapped warp input/exclusive aim also passed. This fixture freezes motion for a stable visual comparison; full salvo behavior remains covered by automation.

![Actual equipment-spawned red torpedo](RenderEnvironment.png)

Four packaged Development processes passed at 150 ms RTT / 2% loss. GameplayHost/Client*.json include normal charged independent travel, PvP attribution, simultaneous boarding, station actions, recovery, save snapshot and AI pilot handoff. SourceChecks passed; engine evidence is separate from GitHub's lightweight checks.

Final Development and Shipping packaging passed. The original Rust repository is unchanged. Restart Unreal Editor to load the new runtime/editor binaries or use run-unreal.bat for the refreshed Development package.
