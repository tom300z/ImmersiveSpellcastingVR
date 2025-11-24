# Complexity Reduction Opportunities

## 1. Decouple spell-state tracking from haptics (✅ implemented)
- Added `src/CasterStateTracker.{h,cpp}` to own the ActorMagicCaster hook, maintain `lastLeftHandState/lastRightHandState`, and broadcast `StateChangedEvent`s whenever either hand transitions.
- `src/SpellChargeTracker.cpp` now subscribes to those events purely for haptic scheduling and no longer gates the atomics behind the haptics toggle, so input reconciliation keeps working even when haptics are disabled.
- Existing call sites still use `SpellChargeTracker::ActualState` via inline aliases, preserving the public API while the internals run through the new tracker.

## 2. Streamline HandInputDispatcher without losing resilience
- `src/InputDispatcher.cpp:73-163` currently spins `Utils::TimedWorker` threads with several atomics just to reissue attack inputs.
- `src/InputInterceptor.cpp:88-139` is already the single place that knows whenever the casting button toggles, yet it only sets `DeclareCasterState` and a "refresh" flag, leaving the workers to poll every 20 ms until the states reconcile.
- Updated plan: keep a lightweight worker (or scheduled timer) that only retries when an event-driven attempt fails. Use caster-state events to trigger immediate reconciliation, but fall back to periodic re-presses if cooldowns or engine hiccups swallow the inputs. This preserves the necessary monitoring while reducing the complexity of the current always-on polling loop.

## 3. Introduce a shared hand-orientation helper (mind left vs. main hand)
- Every subsystem recomputes how “physical left/right controller” maps to Skyrim’s notion of main/off hand (`RE::BSOpenVRControllerDevice::IsLeftHandedMode()`) – see `src/InputDispatcher.cpp:53-70`, `src/InputDispatcher.cpp:110-163`, `src/InputInterceptor.cpp:107-139`, and `src/SpellChargeTracker.cpp:88-135`.
- Each copy has slightly different comments and variable names, which increases the cognitive load and risks inconsistencies (e.g., haptics swapping hands when dual-casting while other code doesn’t).
- Updated plan: add a helper struct that explicitly exposes both axes (`isPhysicalLeft`, `isMainHand`, `castingSource`, `slotType`). This preserves the semantic distinction between physical hand and main hand, but consolidates the branching logic so all callsites work with the same data.


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
