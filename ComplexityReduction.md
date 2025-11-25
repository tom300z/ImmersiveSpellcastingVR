# Complexity Reduction Opportunities

## 1. Decouple spell-state tracking from haptics (✅ implemented)
- Added `src/CasterStateTracker.{h,cpp}` to own the ActorMagicCaster hook, maintain `lastLeftHandState/lastRightHandState`, and broadcast `StateChangedEvent`s whenever either hand transitions.
- `src/SpellChargeTracker.cpp` now subscribes to those events purely for haptic scheduling and no longer gates the atomics behind the haptics toggle, so input reconciliation keeps working even when haptics are disabled.
- Existing call sites still use `SpellChargeTracker::ActualState` via inline aliases, preserving the public API while the internals run through the new tracker.

## 2. Streamline HandInputDispatcher without losing resilience (✅ implemented)
- `src/InputDispatcher.cpp:12-190` now treats `HandInputDispatcher` as an event-driven worker: `CasterStateTracker` events and input declarations call `RequestWork()`, which wakes the `TimedWorker` immediately. When no retries are pending, the worker sleeps indefinitely (`minInterval = 0`), cutting idle CPU usage.
- The reconciliation loop was rewritten to check for scheduled runs vs. timed retries, pause when the player isn’t casting, and restart the grace-period timer only when the desired state changes. Once the states match, the worker returns to sleep; otherwise it keeps the 20 ms retry cadence for cooldown hiccups.

## 3. Introduce a shared hand-orientation helper (✅ implemented)
- Added `src/HandOrientation.h` with a small `Info` struct and helper functions to convert between physical controllers and in-game casting sources/slot indices.
- `CasterStateTracker`, `InputDispatcher`, and `InputInterceptor` now use this helper instead of re-deriving `IsLeftHandedMode()` logic, keeping the physical-vs-main-hand distinction clear and eliminating duplicate code paths.


## 4. Rebuild the configuration layer with rex_ini (✅ implemented)
- Enabled the `rex_ini` option (pulling in SimpleIni) and rewired `ConfigManager` to load/save via `CSimpleIniA` rather than manually parsing and emitting INI files.
- The settings map is now the single source of truth; `LoadFromDisk` reads each section/key straight from SimpleIni, `SaveToDisk` emits entries with their descriptions, and the bulky stream/trim code was removed accordingly. Papyrus bindings continue to use the same API, but the underlying persistence layer is far simpler.

## 5. Share the ActorMagicCaster hook plumbing (✅ implemented)
- Added `hooks/ActorMagicCaster.{h,cpp}` to resolve the player’s caster vtable once and patch arbitrary slots with a templated helper.
- Both `AllowShoutWhileCasting` and `CasterStateTracker` now call `Hooks::ActorMagicCaster::WriteHook`, eliminating duplicated logic for resolving the player caster, validating the vtable, and logging patched slots.

## 6. Break up `utils.cpp` into purpose-specific modules (✅ implemented)
- Replaced the monolithic `src/utils.cpp` with focused modules under `src/utils/` (Console, GameState, MessageBox, Input, Setup, Animation, LogUtils, EventHandler) and turned `utils.h` into a light aggregator.
- Each subsystem now includes only what it needs (e.g., Setup handles Config/HIGGS/Windows, Input pulls in OpenVR, etc.), shrinking compile-time dependencies and making future edits localized.
