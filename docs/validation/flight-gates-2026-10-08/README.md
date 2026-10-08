# Physical gates, flight presentation and equipment controls — 2026-10-08

Unreal Engine 5.8.2 on Windows, Intel Core Ultra 9 275HX, 64 GB RAM. This batch fixes the reported gate, torpedo, warp, motion, aim exclusivity and loadout issues.

- Jump anchors use native ring meshes. Holding the mapped interact key (B by default) steers and brakes to the approach point, charges, aligns, then drives through the aperture. Authority requires a forward swept crossing; dwell alone cannot travel. Automation exercises all three captain hulls and invalid crossings.
- Torpedoes use a separate 2.5-unit red visual, compared with 7-unit cannon visuals; collision and damage remain unchanged.
- Microwarp shows its mapped binding (Left Shift / controller right shoulder). Hold to aim and release to activate. The packaged probe sends actual mapped Slate input and verifies movement, cooldown and no broadside discharge while aiming.
- Local predicted and authoritative ships interpolate fixed-step poses; the camera follows the same pose. Automation checks fractional motion and shortest-angle interpolation. This does not claim a quantified perceptual smoothness threshold.
- Warp/torpedo aim blocks broadside input, activation and unfinished windup on authority as well as the local controller.
- Initialization and replicated class/fit notifications resolve the selected loadout. Frontend selection preserves explicit empty slots and module arrays. A gun-only PIE checkbox fit no longer inherits unchecked optional equipment. Actual PIE runs exercise all three modes.

Editor compilation, native asset validation and protected bootstrap passed (108 native packages retained). All 41 automation groups passed: 22 clean, 19 with expected fixture warnings, zero failures or skips. Warnings include missing fresh fixture saves, not ignored test failures.

The busy armed 500-NPC fixture across ten systems passed its 8 ms simulation gate at 3.695801 ms p95 over 1,000 measured steps. The earlier 1,000-NPC stress result remains above budget; this batch does not claim it is resolved.

Four packaged Development processes passed gameplay acceptance at 150 ms round-trip latency and 2% packet loss, including physical independent travel, simultaneous boarding, PvP attribution, station actions, recovery and a post-gameplay snapshot. Four Shipping processes passed ordinary possession/autosave. Those network runs precede the final material color, prompt-width and editor-only checkbox refinements; authoritative gameplay is unchanged afterward.

Automation.json, authoring-protection.json, scale-500-busy-armed.json, GameplayHost/Client*.json and Shipping.json record those checks. Packaged renderer evidence and final package results follow below.

Final packaged Development captures passed at 1920x1080, quality 2, 100% screen percentage, uncapped FPS on the RTX 5090 Laptop GPU. Environment capture: 4.001599 ms p95 frame / 302.871 mean FPS; the mapped warp/exclusive-aim check passed. The screenshot was inspected: the ring has a visible aperture, the torpedo is smaller and red, the complete gate prompt fits its panel, and the fitted warp binding is visible with no EMP fitted.

Busy armed capture: 500 NPCs plus the player, 10.6383 ms p95 frame / 140.452 mean FPS; hosted simulation 4.920598 ms p95. This passes the documented 60 FPS and 8 ms gates. Render.json and RenderEnvironment.json retain actual dimensions and graphics preset.

![Ring, red torpedo and warp controls](RenderEnvironment.png)

![Busy armed sandbox](Render.png)

Final Editor, Development and Shipping builds and native asset validation passed. The local run-unreal.bat launches the refreshed Development package; no existing release archive was replaced. The original repository remains unchanged.
