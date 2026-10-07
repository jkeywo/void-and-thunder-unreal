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
