# Native validation — 7 October 2026

The subsequent [1,000-NPC stress optimisation](optimisation-2026-10-08/README.md)
records the new fixed-step result.

Current expanded evidence and the downloadable release are in
[acceptance-2026-10-07](acceptance-2026-10-07/README.md). The measurements below
are the earlier transfer milestone, retained for comparison.

Final compact evidence is in [transfer-2026-10-07](transfer-2026-10-07/summary.json).
Earlier flight-only reports elsewhere in this directory are historical milestones.
The final source behavior map is [../transfer-map.md](../transfer-map.md).

Reference Windows machine: Core Ultra 9 275HX, 64 GB RAM, Windows 11 25H2; rendering
uses the RTX 5090 Laptop GPU (the headless automation report's Intel display adapter
is not the rendering device). Unreal Engine 5.8.2, MSVC 14.44 and SDK 22621.

| Gate | Final result |
|---|---|
| Editor compile | Passed |
| Development build/cook/package | Passed |
| Shipping build/cook/package | Passed |
| Native asset validation | Passed; no reported asset errors/warnings |
| Automation | 13 successful, 0 failed; three include expected missing first-save file warnings |
| 500 NPCs / ten systems / concentrated mixed combat | 6.22 ms p95 per 64 Hz step; <8 ms gate passed |
| 1,000 NPCs / concentrated mixed combat stress | 19.84 ms p95; exceeds the 8 ms acceptance budget, outside the 500-NPC target |
| Packaged listen-host 1080p busy mixed combat | 7.00 ms p95 frame, 240 mean FPS; simulation 3.76 ms p95 |
| Packaged native menu | 2.35 ms p95 frame |
| Four Development processes, same system | Passed |
| Four Development processes, independent systems, 150 ms RTT / 2% loss | Passed, including live save/load and reconnect |
| Four Development processes, independent systems, 250 ms RTT / 5% loss | Passed |
| Packaged LAN Create / Continue | Passed; world ID and simulation time restored |
| Packaged host departure | Passed; guest returns to Menu and clears session |
| Shipping four-player ordinary startup/possession/autosave | Passed through normal host/join command-line flows |

The graphics preset is 1920x1080, screen percentage 100, all `sg.*` quality groups
set to 2, motion blur/auto exposure/Lumen disabled by project configuration, and
`t.MaxFPS 0`. `Scripts/Render.ps1 -Packaged -Busy -Armed` uses a listen-server URL,
warms for 20 seconds, captures the actual busy arena at 25 seconds and reports at
35 seconds. It contains 500 NPCs plus the observing player; half the NPCs are moved
to system zero and every fourth NPC receives alternate equipment/full utility AI.
The recorded simulation clock excludes map-start stalls and advances continuously;
solo hit-stop cannot slow this hosted run.

`Scripts/Benchmark.ps1 -Population 500 -Busy -Armed` runs 1,200 fixed steps, excludes
the first 200 samples and reports 1,000 observations. The 1,000 stress script reports
its failed budget comparison without failing the 500 acceptance gate. Fixtures are
invulnerable to retain measured population; normal hosted NPCs are mortal.

Multiplayer uses four actual packaged processes, initially joins during simulation,
checks possession/motion/server acknowledgement, observes all four captains, and
checks per-system NPC relevance. RTT emulation applies half the requested lag at each
endpoint; the scripts use Unreal packet lag/loss settings. Reconnect checks durable
profile/ship IDs, saved credits/prizes and position continuity within 60 source units.
Shipping uses distinct local `-UserDir` profiles and the normal `-VTHostWorld` / `-VTJoinAddress` flows (engine map overrides and Development probes are unavailable in Shipping). Its host autosave must contain four captain records and a valid checksum. The host is stopped only after that snapshot is verified.

Exact universal prediction-error bounds and cross-machine WAN behavior are not
claimed by these bounded probes. Extended soaks are recorded in the current
acceptance evidence linked above.

See Epic's [network emulation documentation](https://dev.epicgames.com/documentation/unreal-engine/using-network-emulation-in-unreal-engine)
for the packet simulation controls. Local scripts are the licensed native gates;
GitHub Actions checks project metadata and explicitly does not claim engine coverage.

Screenshots were reviewed from the packaged native renderer:

![Native menu](transfer-2026-10-07/RenderMenu.png)

![Busy listen-host arena](transfer-2026-10-07/Render.png)

PIE mode/ship/loadout selector validation: [2026-10-08](pie-toolbar-2026-10-08/README.md).

Physical gate travel, smooth local poses and equipment control validation: [2026-10-08](flight-gates-2026-10-08/README.md).

Reverse gate staging and actual fired torpedo visual fixes: [2026-10-08](gate-reverse-2026-10-08/README.md).
