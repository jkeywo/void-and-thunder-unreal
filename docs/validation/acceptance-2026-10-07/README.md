# Acceptance follow-up — 7 October 2026

The final control build passed all 18 local native pipeline gates: Editor, assets,
20 automation groups (17 clean, three with expected first-save warnings), 500 busy
NPCs, Development/Shipping packaging, four-process gameplay, reconnect, LAN
Create/Continue, host departure and verified 1080p rendering. The pinned Rust
reference suite passed all 262 tests. Release source is
`2852ad421b699c4d07400c4fb33cd026d8a09a23`.

Gameplay fixtures arrange initial ships/resources, then use ordinary client intent
and station/refit/recovery RPCs. Seventeen host checks cover same-faction PvP,
individual crime, single contested prizes/bounties, boarding and death recovery,
station dwell/services/refit/undocking, charged independent travel, movement after
loss, and AI pilot enable/disable while retaining the captain. Four packaged
processes passed clean, 150 ms RTT/2% loss and 250 ms RTT/5% loss with a five-second
complete packet blackout. Correction metrics cover the final movement/disruption
and AI/manual transitions, including fixture placement; maxima are not a
steady-state prediction bound.

Independent source expectations exposed captain/crew utility ownership, EMP
recharge stance and initial commitment differences. These are corrected; 48
source-exported action choices now match. Manual broadsides now hold to aim and
fire on release, with cancellation producing no shot. The player can toggle the
same authoritative utility AI without relinquishing possession or identity.

The blackout originally failed: the fixed sequence window rejected all subsequent
inputs after more than 256 were lost. The elapsed-time recovery allowance preserves
validation and one command consumed per fixed step. Bounded visual correction
offsets smooth small corrections; large outages still produce snaps.

Final-build 720p and 1440p menu checks used actual viewports, fresh screenshots and
Slate controller events. They prove focus/navigation routing, not physical-device
testing. Command-line quality overrides are recorded in the reports; the menu's
Graphics label reflects the separately saved user preference.

The downloadable Shipping ZIP was extracted into a fresh folder and passed normal
four-player hosting/joining and checksum-validated autosave. It includes the x64
runtime installer and attribution, excludes debug symbols, and records its source
commit and binary hash. `release-SHA256SUMS.txt` identifies the exact archive.

## Campaign durability

A 30-minute run started with 500 mortal NPCs, four captains and 150 ms RTT/2% loss.
It passed five live save/load cycles and fourteen guest reconnects, advanced all ten
systems and simulated 1799.98 seconds in 1800.02 wall seconds. Mortal attrition
reduced the minute-sample ship count from 429 to 398. Host memory was 414.04 MB at
the first minute, 426.28 MB at the last and peaked at 432.43 MB. This is not a
claim of 500 ships remaining alive throughout the run.

The two-hour authored-population run passed: 53 initial NPCs, 23 live save/load
cycles, 23 guest reconnects and all ten systems advanced. Simulation advanced
7199.83 seconds in 7200.00 wall seconds. The last four-captain minute sample had
54 ships and 228.27 MB host memory; sampled memory peaked at 375.81 MB. This earlier
binary predates movement recovery, UI and AI ownership fixes.

The final control build also passed ten minutes with 500 initial mortal NPCs, four
reconnects, two live save/load cycles and all ten systems advancing. It simulated
600.00 seconds in 600.01 wall seconds; the last four-captain minute sample had
401 ships and 418.56 MB host memory. Sampled memory peaked at 442.18 MB. Captains
are invulnerable in durability fixtures, NPCs are mortal, and guests leave shortly
before host shutdown. Neither soak proves a stable population of 500 survivors.

Binary hashes distinguish the two-hour and 30-minute isolated builds from the
final control build. The final build has the complete native pipeline and the
additional ten-minute soak; it is not labelled as having run the full two hours.
Persistence/world code is unchanged by the later player input and UI fixes.

## Isolated final performance

After all campaign soak processes exited, the final build measured 3.37 ms p95
for 500 active NPCs across ten systems with concentrated mixed combat (1,000
measured 64 Hz steps). This passes the 8 ms gate. The 1,000-NPC stress run measured
9.46 ms p95 and failed that budget.

The packaged listen-host render measured 8.54 ms p95 frame time and 187.58 mean FPS
at actual 1920x1080, with 500 NPCs plus the player; simulation was 4.32 ms p95.
All scalability groups were set to 2, screen percentage 100 and frame cap disabled.
This passes the 60 FPS target on the recorded Core Ultra 9 / RTX 5090 Laptop GPU
machine. These are separate isolated headless and rendered fixtures, not timings
from the mortal durability runs. Their complete reports and screenshot are adjacent.

## Scope of evidence

See [parity coverage](../../parity-coverage.md) for numerical tolerances and limits.
Lower-spec hardware, real WAN conditions, physical controllers and equivalence of
every legacy unit assertion remain unproven. The 1,000-NPC stress case exceeds the
8 ms simulation budget; 500 NPCs remains the acceptance target. GitHub checks source
metadata; licensed native gates are the repeatable local pipeline.
