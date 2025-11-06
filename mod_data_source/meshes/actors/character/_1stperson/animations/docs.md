## What is changed in these files?
I've set the duration of animations that don't make sense in VR (using VRIK) to 0.0001.
For example the animation for opening your hand when casting Flames (mxh_preaimedcon) doesn't make much sense when VRIK displays your preal-time hysical hand position anyway. It just adds a frustrating delay to each spell cast.


## Animations and their purposes in game

### mlh_preaimedcon.xml
Left-hand concentration spell entering the aimed stance for precision casting.
Example spell: Flames (left hand, aimed while channeling)
State: Disabled

### mlh_precharge.xml
Spell charge animation (Hand slowly closing while charging). If disabled hand closes immediately
Generic left-hand spell starting a charge when you hold the cast button.
Example spell: Firebolt
State: Enabled (No effect on timing, smoother animation)

### mlh_preready.xml
Readies the left-hand spell from idle to the standard combat-ready pose.
Example spell: Ice Spike
State: Enabled (No effect on timing, smoother animation)

### mlh_preselfcon.xml
Left-hand concentration spell that targets the caster, settling into the self-channel pose.
Example spell: Healing
State: Disabled

### mlh_pretelekinesis.xml
Now i can fling stuff :)
Transition into the telekinesis casting stance before the looping grab animation.
Example spell: Telekinesis
State: Disabled

### mlh_prewardcharge.xml
Begins charging a ward spell, raising the hand into the defensive angle.
Example spell: Lesser Ward
State: Disabled (No visible change)

### mlh_prewardloop.xml
Runs before the ward starts. Disabling starts ward immediately
Example spell: Lesser Ward (sustain phase)
State: Disabled

### mlh_selfprecharge.xml
Starts charging a self-targeted spell from the left hand.
Example spell: Fast Healing
State: Disabled (No visible change)

### mrh_release.xml
Fire & forget spells have a release timer that starts once you release the casting button
Example spell: Ice Spike
State: Disabled

## TODO:
- Fix healing spell animation not matching being instant (Probably no universal way to do this...)