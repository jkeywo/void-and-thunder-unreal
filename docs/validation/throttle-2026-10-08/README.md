# Restored throttle ladder — 2026-10-08

Original reference: `crates/vt_client/src/input.rs`, ThrustState and W/S just_pressed input. Default half; ladder -1, 0, 0.5, 1; clamped endpoints. Stick input remains analogue, updates the nearest keyboard notch, and centres at halt.

The Unreal controller now binds separate up/down Enhanced Input Started events. Its local copy of IMC_Flight adapts the legacy keyboard throttle mappings and retains their per-key remap metadata. The authored context and packages remain unchanged. Key release and repeat do not change the notch. Menus suppress transmitted throttle without losing keyboard selection. Prediction and host authority still consume scalar pilot intent. HUD labels and controls instructions reflect the ladder.

Validation: Editor build, default Bootstrap, native asset validation and lightweight source checks passed. All 51 VT automation cases passed (27 carry fixture warnings). A rendered key-event probe passed in both Editor and packaged Development: half default, held S to halt, repeated S ignored, release retention, W to half/full and S through halt/reverse with endpoint clamping. Development and Shipping Windows packages built successfully. Four packaged players passed combat, boarding, station, recovery and travel checks at 150 ms round-trip latency / 2% packet loss. Busy armed 500-NPC benchmark p95 was 2.919100 ms against 8 ms. No new Shipping runtime smoke was run for this input fix.

Evidence is summarized in results.json; complete logs remain under Saved/Validation/throttle-*. The original Rust working tree is clean; no authored Unreal content package changed.
