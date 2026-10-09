# Playability fixes — 8 October 2026

Addresses the reported slow ships, missing graphical HUD, reversed yaw,
small projectiles, untextured sun, absent plane grid and missing skybox.

- Native `DA_GameData.FlightSpeedMultiplier` scales authoritative and predicted
  thrust/top speed together, currently 2. Legacy per-hull tuning stays intact.
- Player keyboard/stick yaw is reflected once at the input boundary. AI intent,
  simulation coordinates and angular rules remain unchanged.
- The authored Widget Blueprint now has hull/battery/EMP/directional shield
  bars, equipment readiness/cooldowns/ammunition and interaction readouts.
  Menu/docking/controller flows and event-driven refresh remain.
- Projectiles use a 7-unit glowing sphere (the original visual radius), rather
  than the previous 1-unit appearance. Hit radii are unchanged.
- The sun uses an animated native surface material. Windows shader targets are
  explicit and rendered validation rejects missing/invalid cooked material maps.
- A local translucent material plane restores the 200-unit reference grid just
  below the flight plane at z=-9, matching the original layering convention.
- The transferred six-face starfield now uses a native TextureCube and unlit sky
  material, centered on the camera.

`Scripts/Playability.ps1` is an explicit, targeted authoring upgrade. It preserves
menu layout and definition assets while updating the named environment/HUD
assets. Normal bootstrap and native validation retain authored packages.

A new automation group verifies increased one-second travel distance and that
right/left player axes turn toward the correct reflected world side. Visual
verification covers the HUD/grid/projectiles/sky together, a dedicated sun view,
and a 1280x720 viewport. Compact final results are adjacent.

This intentionally changes flight pace at the user's request; it is not a claim
that legacy combat balance is unchanged. Ship collision and projectile hit radii,
fixed-step frequency, Actor ownership, and source repository remain unchanged.

The optional enforced 1,000-NPC stress runs did not retain the earlier 8 ms
result: first 8.3141 ms p95, isolated 15.1256 ms, presentation-cache-only 13.0468
ms, and after readiness guards 11.8046 ms. These runs are retained rather than
presented as a passed stress budget. The variability prevents attributing the
entire difference to one change. Projectile material loading/setup now stays out
of headless execution and native activation skips already active/cooling devices.
A persistent-intent regression verifies the same windup, reload and expiry-step
fire cadence. The required 500-NPC operating gate is validated separately.

The final pipeline's first host-departure attempt ended after 20 seconds before
the guest handshake completed (maximum one connected player). This fixture
failure is retained. The test host now stays for 40 seconds; the strict two-player
and guest return-to-menu/session-removal assertions remain. Passed preceding
gates are retained, and the interrupted pipeline resumes at departure.

Final validation: all 18 gates in `pipeline-final.json` passed. The resumed
run reused the 13 successful gates before the departure fixture interruption,
then passed departure, rendered arena/menu, Shipping and Shipping multiplayer.
All 27 automation groups passed (19 clean, eight with warnings; none failed
or skipped). Bootstrap preserved 97 authored packages. The final required
500-NPC busy/armed simulation p95 was 7.072799 ms, below 8 ms.

The final 1080p busy/armed packaged capture (`render-final-1080.json`) measured
8.504897 ms p95 frame and 164.307 mean FPS at quality 2. Final environment and
720p captures passed and were visually reviewed. The first final environment
capture timed out; its unchanged-binary rerun passed. Earlier `pipeline.json`
and `render-1080.json` are retained as the first pass, not the final evidence.
