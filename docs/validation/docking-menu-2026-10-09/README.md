# Docking menu readability — 2026-10-09

- Native WBP_UI menu body/choice text increased from 10 to 18 points, headings to 22. Button vertical padding increased from 6 to 10 units, with larger combo padding.
- Loadout choices wrap at the menu width. Fit and faction summaries remain bounded; unavailable recovery no longer consumes a station action slot.
- Targeted `HUDStyle -MenuReadability` updates only WBP_UI, preserving HUD typography and other assets.
- Editor build and Development packaging passed. Native automation: 59 passed, zero failures. Asset validation passed; bootstrap preserved all 114 packages after the intentional menu update.
- Required busy armed 500-NPC check passed (p95 5.2731 ms; 8 ms gate). This concurrent-workload run is not a comparative performance measurement.
- Docked menu inspected at 1920x1080 and 1280x720. Shared frontend rendered and controller navigation passed.
- Shipping was not repackaged for this UI-only change.
