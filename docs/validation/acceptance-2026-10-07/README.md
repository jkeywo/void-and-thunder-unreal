# Acceptance follow-up — 7 October 2026

This evidence supplements the earlier transfer milestone. All 18 local native
pipeline gates passed, including Editor/assets/automation, 500 busy NPCs,
Development and Shipping packaging, four-process gameplay, reconnect, LAN
Create/Continue, host departure and verified 1080p rendering. AI expectations were
added afterward and the full native suite passed 17 groups (14 clean, three with
expected first-save warnings). The pinned Rust reference suite passed all 262 tests.

Gameplay fixtures arrange initial ships/resources, then use ordinary client intent
and station/refit/recovery RPCs. Fifteen host checks cover same-faction PvP,
individual crime, single contested prizes/bounties, boarding and death recovery,
station dwell/services/refit/undocking, charged independent travel and continued
movement after loss. Four packaged processes passed clean, 150 ms RTT/2% loss and
250 ms RTT/5% loss with a five-second complete packet blackout. Movement correction
metrics cover the 30-second final movement phase, including fixture placement and
the disruption; maxima are not a steady-state prediction bound.

The blackout originally failed: the fixed sequence window rejected all subsequent
inputs after more than 256 were lost. The elapsed-time recovery allowance preserves
validation and one command consumed per fixed step. Bounded visual correction
offsets smooth small corrections; large outages still produce snaps.

720p Performance and 1440p High menu checks used actual viewports, fresh screenshots
and Slate controller events. They prove controller focus/navigation routing, not
physical-device testing. Rendering and timing here co-ran with campaign soak (and
some resolution runs overlapped); the final isolated performance check is recorded
separately when available. Source/binary hashes identify each executable, including
the earlier isolated campaign soak binary whose persistence/world code is unchanged
by the subsequent movement recovery and UI changes.

Two-hour campaign soak results will be appended after completion. No completed
long-run result is claimed until the report exists.

See [parity coverage](../../parity-coverage.md) for tolerances and limits. Lower-spec
hardware, real WAN conditions, physical controllers and equivalence of every legacy
unit assertion remain unproven. The recorded 1,000-NPC stress case exceeds the
8 ms simulation budget; 500 NPCs remains the acceptance target. GitHub checks source
metadata; licensed native gates are the repeatable local pipeline.
