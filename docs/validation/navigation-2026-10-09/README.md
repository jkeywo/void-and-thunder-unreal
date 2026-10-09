# Navigation, relationships and world scale — 2026-10-09

UE 5.8.2, Windows 11, Intel Core Ultra 9 275HX, 64 GB RAM.

- Editor Development compiled successfully. The final compile used `-NoUBA -MaxParallelActions=1` because another workload was consuming memory.
- Full native automation: 59 tests passed, zero failures. Includes boundary endpoints/interpolation and prediction, unused equipment input, routes, personal reputation/heat colours, distant landmark relevancy and idempotent schema-6 position migration.
- Bootstrap preserved all 114 authored native packages. Asset validation returned zero errors or warnings.
- Busy armed 500-NPC simulation: 1,000 measured steps, p95 2.4032 ms against the 8 ms gate. This run was not an isolated-machine before/after benchmark. Four private tutorial definitions remain alongside the ten sandbox systems.
- Editor four-player gameplay acceptance passed at 150 ms RTT / 2% packet loss, including combat, contested boarding, recovery, station refits and independent travel.

## Controls and scope

G opens the full-screen map; F opens the compact chart; M remains mines. Click a system to select a gate route. Closing the chart displays the next gate waypoint, recalculated after each jump. Full-screen selection captures flight controls while the world continues; compact chart permits piloting.

Ship outer rings are green for friendly, amber for neutral, red for hostile reputation or heat, and blue for the local ship. Unfitted equipment cannot enter aim/camera mode.

System positions/radii and celestial sizes are scaled fivefold; inter-system arena spacing is tenfold. Distant systems expose only their stars. The nearest star's system boundary begins a 100 m linear band from 1x to 0.01x thrust and maximum speed. Host and predicted movement use the same calculation. Committed gate motion retains its authored curve.

Ships, station sizes, gate apertures, weapon ranges and close tutorial training distances retain their existing values. The tutorial field moves clear of the enlarged star. Native authoring packages stay unchanged: scaling occurs on a world-owned catalogue copy. Existing schema-6 saves gain an optional layout scale marker; spatial state migrates once, preserving dock offsets, identities, fits and standings.

## Packaged checks

- Development packaging passed. Packaged four-player gameplay passed at 150 ms RTT / 2% loss; see gameplay reports.
- Packaged four-player independent travel and reconnect passed at 150 ms RTT / 2% loss; the guest retained durable identity, credits and boarding progress.
- Packaged full-screen map and waypoint rendered at 1920x1080, quality 2. Click selection was exercised through the UMG pointer handler; distant anchor meshes were asserted visible only for non-private stars. Both captures were visually inspected. See map.png and waypoint.png.
- Shipping packaging and four-player possession/autosave smoke test passed; snapshot checksum validated and four captain records were recovered.
