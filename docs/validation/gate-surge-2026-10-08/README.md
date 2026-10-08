# Gate surge, white flash and original torpedoes — 2026-10-08

The user requested restoration of torpedo visuals and a rapid gate acceleration/braking sequence with unchanged endpoints, plus a white flash over teleport. Torpedoes again use the original common projectile radius (7 source units) and material. The separate red asset/properties are removed; the correct deferred firing/load lifecycle is retained.

Reverse staging, charge duration and swept forward aperture validation remain. After alignment, departure uses native data tuning of 1200 source units/s squared. Teleport happens on reaching the source ring, placing the captain at the destination ring. A quadratic braking curve over 0.35 seconds ends at exactly the former arrival point (destination gate position times 0.85), at rest. Ordinary helm input resumes after braking. The native PlayerCameraManager starts an opaque white scene fade on the frame it detects the captain's system change and fades out over 0.3 seconds. Other captains do not flash.

Arrival state/time/target replicate. Prediction evaluates the same curve against authoritative motion time. Save schema 6 retains committed braking progress, while older Unreal schemas migrate without an arrival phase. World loading uses the snapshot clock; guest restoration rebases the saved elapsed braking time to the running host clock so disconnected time does not advance the removed ship.

- Final Editor compilation and all 43 automation groups passed: 22 clean, 21 with fixture warnings, zero failures or skips. Tests cover all captain hulls, reverse staging, forward aperture checks, departure under 0.5 seconds, unchanged teleport/endpoints, monotonic braking, rest within 0.5 seconds, serialized mid-braking restoration, reconnect after a 30-second clock advance and save migrations.
- Native asset validation passed; bootstrap preserved 107 authored packages after removing the separate torpedo material.
- Busy/armed 500-NPC benchmark passed at 2.438102 ms p95 against the 8 ms gate over 1,000 measured steps. The earlier 1,000-NPC stress budget is still outstanding; no new stress-budget claim.
- Packaged Development 1080p, quality 2: restored torpedo capture passed at 4.107203 ms p95 frame; actual equipment spawn uses the original radius/material. Mapped warp/exclusive-aim checks also passed.
- Packaged gate probe passed at 4.046001 ms p95 frame. It performs normal held-input gate entry, observes arrival braking, verifies the native camera's white fade above 90% opacity and checks the unchanged final point at rest. Both screenshots were inspected. Camera fade covers the scene; HUD remains readable.
- Four packaged Development processes passed gameplay acceptance at 150 ms RTT / 2% packet loss, including independent gate passage and completion of arrival braking, PvP, station actions, recovery, simultaneous boarding and a snapshot. This network run precedes only the final reconnect-clock refinement, covered by the final automation run. Final Development and Shipping packages compile that refinement.

The local run-unreal.bat launches the refreshed Development package. Existing release archives remain unchanged. The original Rust repository is unchanged.

![White gate flash](RenderGate.png)

![Restored original torpedo](RenderEnvironment.png)
