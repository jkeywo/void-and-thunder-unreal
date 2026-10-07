# 1,000-NPC fixed-step optimisation — 8 October 2026

The concentrated mixed-combat fixture still simulates 1,000 Actor/component ships
across ten systems at 64 Hz. Half occupy the busy system and every fourth ship has
full utility equipment/AI. Synthetic ships remain invulnerable to hold population.
No tuning, simulation frequency, population or gameplay ownership was changed.

The fresh pre-change benchmark measured 12.52 ms p95 per step, compared with the
previous release's historical 9.46 ms measurement. The first optimised run measured
6.47 ms p95, below the 8 ms budget. Each run has 1,200 steps, discards the first 200
for its step percentile and measures 1,000 samples. Machine load/clock variability
means the fresh paired result and historical release result are recorded separately.

Phase means from all 1,200 steps:

| Phase | Before ms | First optimised ms |
|---|---:|---:|
| AI / system bucketing | 4.236 | 2.274 |
| Systems / GAS | 0.741 | 0.458 |
| Movement | 0.094 | 0.066 |
| Contacts / landmarks | 0.652 | 0.571 |
| Bounds / weapons | 2.079 | 1.446 |
| Projectiles | 0.760 | 0.581 |
| Death / crippling | 0.013 | 0.010 |
| Boarding / world / summaries | 1.822 | 0.025 |

Simple pilots now select the nearest hostile in a linear pass; exact nearest ties
retain the previous sort fallback. Utility pilots retain complete target ordering,
using cached squared distances for sorting. Threat sums retain system iteration
order. Small AI lists use inline storage. Boarding builds a phase-local list of
eligible disabled Actors and rechecks live validity/claim/range conditions for
contested awards. EMP range rejection precedes expensive angle calculations.

`Scripts/Benchmark.ps1 -Population 1000 -Busy -Armed -RequireBudget` enforces the
8 ms budget with a failing exit code. Without `-RequireBudget`, populations above
500 retain the historical non-blocking stress-report behavior.

The native target-priority regression checks simple and utility pilots against
reordered contacts, including a nearer friendly ship. Existing golden AI, combat,
boarding, persistence and four-process gameplay tests remain the parity gates.
All 18 final native pipeline gates passed, including Editor/assets, 21 automation
groups (three expected first-save warning groups, no failures), Development and
Shipping packaging, clean/latency/blackout four-player gameplay, reconnect,
Create/Continue, host departure, and rendered menu/arena checks.

The final enforced 1,000-NPC run measured **6.3347 ms p95**, passing 8 ms. The
500-NPC gate measured **2.2328 ms p95**. Packaged 1080p Balanced busy rendering
measured **6.4729 ms p95 frame**, 237.665 mean FPS and 2.8333 ms p95 simulation,
with 500 NPCs plus the player. The first and final stress runs both passed;
reference-machine and measurement scope are unchanged.

Normal Shipping four-player startup/possession/checksummed autosave passed.
The v0.3.1 Windows preview includes this code. Its exact ZIP was extracted into a
fresh folder and passed ordinary four-player Shipping hosting and autosave. Archive
SHA-256 and build metadata are adjacent; publication is recorded separately.

This is a fixed-step simulation result on the Core Ultra 9 275HX / 64 GB reference
machine. It does not claim 60 FPS or four-client network scalability at 1,000 NPCs,
or a new default campaign population. The existing 500-NPC acceptance gate remains.
