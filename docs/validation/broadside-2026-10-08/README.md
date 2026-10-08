# Broadside aiming — 8 October 2026

The first mouse-down previously went into viewport capture instead of the broadside action. Flight now uses GameOnly capture including the initial mouse-down. The port also plane-picked the pointer while the camera rotated toward that same aim, creating feedback. Broadside aiming now matches the original: hold LMB/RMB, move the mouse horizontally to accumulate an arc offset (right stick selects an absolute offset), release to fire. The released direction survives pointer movement on that frame. Raw mouse deltas avoid applying Unreal's default axis sensitivity on top of the authored sensitivity.

Targeting overlays convert viewport-relative pixels to widget units rather than treating them as desktop coordinates. Each gun draws its own beam using the same muzzle layout and momentum-inheriting velocity as the authoritative volley. Torpedo/boarding/warp markers receive the same coordinate correction. Rules and balance remain authoritative and unchanged.

The BroadsideHoldRelease automation regression covers press/hold/release, preserved release direction, fresh-bank reset, mouse/controller steering, cancellation without firing, and actual projectile/preview agreement while moving. All 29 automation groups pass (20 clean, nine with warnings, no failures or skipped groups).

`Scripts/Render.ps1 -Environment -Broadside` routes a single mouse-down/up through Slate and Enhanced Input, checks capture policy, held aim without firing, and release initiating the bank's charge/reload. It holds the bank again for the screenshot. Editor and packaged Development runs pass at 1920x1080 and 1280x720; adjacent JSON records `broadside_first_press_release: true`. Captures were inspected for beam direction and origins.

The full 18-gate pipeline passes: protected bootstrap, asset validation, automation, 500 active NPCs, both packages, four-player clean/latency/loss/blackout gameplay, reconnect, create/continue/host-departure flows, rendered flight/menu, and Shipping multiplayer. That pipeline began before the final raw-delta correction and input probe. Final Editor/automation, 500-NPC benchmark, Development packaging, both-resolution rendered input checks, and clean four-player gameplay were rerun after those changes; Shipping compiled the final code. `pipeline.json` retains the full suite and the other reports retain final-code checks.

Final busy/armed 500-NPC simulation p95 is 4.612200 ms against the 8 ms budget. The local `run-unreal.bat` launches the refreshed Development package. The original Rust repository is unchanged.
