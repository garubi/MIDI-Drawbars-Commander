# MIDI Drawbars Commander

## Notes:
You should use a Normally Open pedal to switch from the Leslie's speed, otherwise the "switch impulse" is sent on pedal release instead of pedal push (see issue #24)

## Changelog:
- v. 1.3.5 Fix some issues with control assign in FA preset, and a small bug in the Pedal switch control

- v. 1.3.4 Blinks preset's LED on MIDI in activity

- v. 1.3.3 Resolve some issues when triggering Analog Sync (press ALT + VIB ON); Implements "All sound OFF" (press ALT + FAST); Update the CC assigns in presets to reflects the final panels labelling

- v. 1.3.1 / v1.3.2 Let you specify that the Switch pedal has to turn on/off a "linked" button's LED: Set the Switch pedal's PARAM to the button index, and set to 0 the MIN, MAX, CHAN.

- v. 1.3 Reduce the MIDI thru latency.

- v. 1.2.2 implement the new Debug library

- v. 1.2 / v1.2.1 fix issues with Presets and button's led

- v. 1.1 fix issue with non-toggle buttons

- v. 1.0 first stable release. Not finished but very usable
