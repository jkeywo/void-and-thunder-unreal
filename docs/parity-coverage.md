# Behavior evidence and parity limits

The baseline is source commit c138f2c9caab77ed8288ddcb46d1622e471c2b15.
`Scripts/ExportParity.ps1` archives that commit into a temporary directory and
runs the original public Rust rule functions. It exports `Migration/golden-rules.json`;
the playable Unreal game and packages do not load Rust.

The original simulation suite has 262 unit tests. Running it validates the pinned
reference; it does not establish that all 262 assertions have been translated.
Native tests aggregate related scenarios, and the integration probes establish
multiplayer behavior through ordinary client intent and action RPCs.

| Behavior | Independent or native evidence |
|---|---|
| Flight | 40 original ten-second trajectories, eight checkpoints each, all five hulls; position tolerance 0.05 source units, velocity 0.02 units/s, heading/rotation 0.001 radians |
| Broadside direction | 210 original bearings, both banks and three arcs; direction tolerance 1e-4 |
| Shield bank choice | 105 original bearings across one-, two- and four-bank fits; exact bank selection |
| Input | Native manual broadside hold/release/cancellation and authoritative AI-pilot transitions with retained player identity; network enable/disable and resumed acknowledgements |
| Combat | Native authoritative volley, swept contacts, equipment lifecycles, shields, EMP, torpedoes, mines and interception |
| AI | 48 original cold-start utility action choices across range, hull damage, ammunition and battery states; native legacy behavior expectations for beam combat, EMP priority, empty fields, nearby prizes, civilian flight and crew control ownership; utility scores are not exhaustively cross-engine compared |
| Sandbox | Four separate packaged processes execute same-faction PvP, crime attribution, contested boarding, recovery, docking, repairs, heat payment, refit and charged independent travel |
| Authority | Invalid intent, replayed/oversized input, invalid identity/hull/token, simultaneous duplicate identity, remote station/refit/recovery rejection |
| Persistence | Atomic snapshot, failed writes, backup fallback, durable references, held-input cancellation, disconnect/reconnect and live save/load; extended campaign soak is recorded separately |
| Solo | Native authored wave progression and outcomes, Test Range target behavior and solo time-effect policy |

There is no claim of binary determinism across engines or full equivalence of every
legacy utility score, renderer effect or original unit assertion. Additional golden
corpora can extend this table without introducing a runtime dependency.
