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


## 4. Rebuild the configuration layer with rex_ini
- `src/ConfigManager.cpp:48-208` hand-parses INI files (manual trim helpers, string copies, error handling), and `SaveToDisk` repeats the process in reverse with custom sorting logic.
- The getters/setters span almost 400 lines for four value types, and the Papyrus bindings duplicate boilerplate for each primitive.
- Updated plan: enable the `rex_ini` option in `lib/commonlibsse-ng/xmake.lua`, switch `ConfigManager` over to SimpleIni from commonlibsse-ng, and define settings via a constexpr table that drives registration, serialization, and Papyrus binding generation. This should eliminate the bespoke parser while keeping behavior intact.

## 5. Share the ActorMagicCaster hook plumbing (consider folder/grouping)
- Both `src/SpellChargeTracker.cpp:155-201` and `src/AllowShoutWhileCasting.cpp:45-88` perform the same steps: wait for `PlayerCharacter`, fetch a caster, validate the vtable, and overwrite a slot. Only the index and hook differ.
- Revised plan: introduce a reusable helper (e.g., `CasterHook::Install(slotIndex, hook, logTag)`) that returns the original function and logs failures consistently. Place both hook implementations in a dedicated folder or combined file so related logic stays together.

## 6. Break up `utils.cpp` into purpose-specific modules (✅ go implement)
- `src/utils.cpp:14-368` currently holds unrelated features: console scripting, VR input helpers, UI dialogs, startup/setup flows, logging bootstrap, animation debugging, and developer logging tools. Any file that needs one helper has to include all dependencies (spdlog, Windows, OpenVR, ConfigManager, compatibility headers).
- Plan: move each concern into its own header/translation unit (`ConsoleCommands`, `MessageBox`, `VRInput`, `Setup`, `AnimationDebug`, etc.) and leave `utils.h` as a thin facade. This was explicitly requested, so treat it as an actionable task.

## 7. Consolidate setup-time warnings into data (low priority)
- `Utils::Setup::CheckForUnwantedBindings` (`src/utils.cpp:228-263`) repeatedly queries config values and formats long `std::format` strings inline for every controller hand. Adding new warnings currently means cloning another 30–40 lines of logic.
- Represent the warnings as simple data (e.g., vector of `{settingKey, messageBuilder, remediationAction}`) and drive the UI through that table. This is sensible but low priority; revisit after higher-impact refactors above.
