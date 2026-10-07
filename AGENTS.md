# Void & Thunder — Unreal agent guide

This repository is the Unreal 5.8.2 port of the game at source commit
c138f2c9caab77ed8288ddcb46d1622e471c2b15. The original repository is a read-only
reference. The user explicitly chose Unreal-native development and plain
architecture/decision documents in place of PASM for this new repository.

- Gameplay authority lives on the host. Clients submit intent, never results.
- All ships use AVTShip and the same components; controllers supply pilot intent.
- Movement/combat calculations advance at 64 Hz; presentation interpolates.
- GAS attributes are authoritative: do not add a parallel health or battery store.
- Gameplay tuning belongs in native data assets. Migration/default fallbacks must
  be checked against Migration/resolved-baseline.json.
- Systems remain simulated without players. Replication is scoped to system.
- design/setting is a read-only upstream snapshot; retain SOURCE.md and credits.
- Record architecture before implementation in docs/architecture.md and docs/decisions.md.
  Mark agent-origin decisions with [ai]; do not erase their provenance.
- Preserve the 500-NPC/four-player scale gate before expanding the port.
- Run Scripts/Build.ps1, Scripts/Bootstrap.ps1, Scripts/Test.ps1 and
  Scripts/Benchmark.ps1 for native changes; package and run multiplayer validation
  before claiming a completed milestone.
- Never claim full parity or publication when a remaining gate is unverified.
