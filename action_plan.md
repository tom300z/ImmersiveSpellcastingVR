# Issues

- Make config system easier to use (auto generate papyrus side?)
- Spawn less threads

## Bugs

### Casting input config crash on bad data *(Priority 1)*
- Path: `src/InputInterceptor.cpp:41`
- Description: The code unconditionally dereferences `std::get_if<std::string>` on a variant value. If the INI provides the wrong type, the pointer is null and the mod crashes when reading the casting input method. Validate the type before use or provide a default.

### Spell release haptics assume equipped spell *(Priority 2)*
- Path: `src/SpellChargeTracker.cpp:115-118`
- Description: The code casts the equipped object to `RE::SpellItem*` without verifying it is non-null. If the hand holds something else mid-update, dereferencing causes a crash. Guard the cast.

### OpenVR button name returns dangling pointer *(Priority 2)*
- Path: `src/utils.cpp:121-124`
- Description: Returning `std::to_string(keyCode).c_str()` yields a pointer to a temporary string, producing undefined behaviour. Store the string in stable storage (e.g., static `thread_local` std::string) before returning.

## Performance

### Controller polling does redundant state work *(Priority 4)*
- Path: `src/InputInterceptor.cpp:81-109`
- Description: Each poll re-fetches the player, checks menu state, and retrieves equipped spells multiple times. Cache these results per call to reduce per-frame overhead.

## Nice To Haves