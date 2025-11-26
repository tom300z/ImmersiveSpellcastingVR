# Changelog

All notable changes to this project will be documented in this file. Dates use the ISO-8601 format (YYYY-MM-DD).

## [1.1.0] - 2025-11-26

### Summary (Players)
- You can finally shout while charging or holding a spell—the mod now interrupts the spell, refunds the Magicka, and lets the shout fire.
- Spell input feels more responsive and reliable thanks to the new event-driven input dispatcher.
- The optional HIGGS grip_touch patch now works on HIGGS 1.10.7

### Added
- You can now press the shout button while casting or charging; the mod interrupts active hand casters, refunds the Magicka cost, and immediately retries the shout so shout-centric loadouts (e.g. Spellsiphon) keep flowing smoothly.
- Introduced the `CasterStateTracker` event bus plus per-hand `HandInputDispatcher` workers, enabling other systems to react to precise spellcasting state transitions instead of polling.
- Added a reusable binary patcher and extended the optional HIGGS compatibility shim so grip-touch patches apply to both 1.10.6 and 1.10.7 without manual hex edits.
- Added an interactive setup flow that warns about conflicting grip bindings or outdated Haptic Skyrim VR installs right after a save is loaded.

### Changed
- Spell input redirection is now fully event-driven: controller events declare the desired caster state and the dispatcher only injects button presses when the player is in-game, holding a spell, and not already shouting, which reduces misfires and latency.
- Reworked spell charge tracking and haptics so each caster state (start, charging, holding, releasing) schedules purpose-built haptic envelopes and respects the live haptics enable toggle.
- Configuration moved to typed `Settings` definitions backed by SimpleIni persistence, ensuring defaults stay synchronized between the INI, Papyrus API, and the MCM while preserving descriptive comments on disk.
- The build pipeline was renamed to the `ISPVR` target, now performs incremental Papyrus/HKX compilation, copies DLLs together with PDBs, clears/repacks the mod folder before install, and can emit a ready-to-upload ZIP straight from `xmake releasedbg`.
- Mod assets, documentation, and packaging consistently use the Immersive Spellcasting VR branding (e.g., `ISPVR - Immersive Spellcasting VR` install folder and `ImmersiveSpellcastingVR.esp`), making the deliverable easier to locate in mod managers.

### Fixed
- Fixed spells occasionally refusing to recharge after shouting or after exiting menus by refreshing input state and, when desired, resuming casts automatically once menus close.
- Prevented casting state from getting stuck in left-handed mode or dual-cast scenarios by normalizing hand orientation and mirroring the left-hand caster during dual casts.
- Addressed multiple haptic regressions (missing pulses while charging, incorrect dual-cast haptics, and lingering vibration) by routing all haptic scheduling through the new state tracker.
- Cleaned up configuration edge cases where invalid values would silently persist or comment prefixes were dropped, so bad entries now fall back to defaults and comments remain intact.
- Updated the optional HIGGS grip-touch patch so it no longer fails on newer builds of HIGGS and can be toggled live from the INI/MCM.

## [1.0.0] - 2025-11-11

- Initial release.
