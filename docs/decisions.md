# Decisions

The user approved the following port choices: Unreal 5.8.2, C++ foundations with
Blueprint presentation/content, public GitHub repository, four-player listen-server
sandbox, independent travel, persistent host worlds, continuously simulated empty
systems, immediate saving/removal on guest disconnect, station respawn without a
new monetary penalty, and slow motion only in standalone play.

The user selected plain architecture documents in this new repository rather
than PASM. This choice does not alter the original game or vellum.

- [ai] Use one Actor/component ship implementation for 500 active NPCs. Prove the
  scale target early; do not introduce a second Mass simulation without a revised
  architecture decision and measured justification.
- [ai] Use simulation coordinates in legacy units and a single conversion boundary:
  Unreal centimetres = (X, -Y, Z) * 100. Yaw is the negative source heading.
- [ai] Keep export tooling isolated in a temporary pinned source snapshot. Export
  through original typed RON deserialization so omitted fields inherit real defaults.
- [ai] Separate the runtime and editor modules. Import/build tooling does not ship.

- [ai] GAS broadside instances own wind-up and reload time; the fixed-step coordinator
  advances their clocks. Replicated bank indicators are presentation snapshots.
- [ai] Directional shield banks use an explicit reflected structure; hull and battery
  remain GAS attributes and only one combat damage path modifies hull.
- [ai] Native migration regenerates complete packages and marks them fully loaded
  before saving. Loading the old package after constructing new data overwrites it;
  the authoritative-volley fixture caught this importer lifecycle bug.
- [ai] Guest state is captured in PawnLeavingGame, before base Unreal pawn destruction.
  Logout performs the campaign write after removal. Reconnect uses a persistent local
  profile GUID and a host-issued per-world GUID token, with simultaneous identity rejection.
- [ai] Campaign files use a checksummed SaveGame payload, a last-good backup and Windows
  atomic replacement. Personal profiles/tokens are separate from campaign records.
- [ai] Synthetic scale fixtures are invulnerable so contacts do not reduce the measured
  population. They do not establish full combat or renderer acceptance.

- [ai] Restored the legacy wrapping LCG instead of the prototype FRandomStream,
  following the source agent guide's requirement to preserve cosmetic spawn jitter.
- [ai] Snapshot schema 3 migrates schema 2 pose clocks. Unknown future schemas fall
  back to a validated backup. The identity-free initial prototype slot format and
  legacy Rust saves are unsupported.
- [ai] The uncharged development jump helper is disabled in Shipping; charged
  gameplay travel remains outstanding.
