# MIDI Drawbars Commander

## Pedals:
### Expression pedal
Any "standard" expression pedal compatible with the Roland EV5 will do.

### Switch pedal (leslie speed)
You should use a Normally Open non latching (momentary) pedal to toggle the Leslie's speed, otherwise the "switch impulse" is sent on pedal release instead of pedal push (see issue #24)

## Factory restore
You can restore the presets factory defaults with the following procedure
- **turn off** the MIDI Drawbars Comamnder (i.e. detach from the USB cable)
- **press and hold the *ALT* button while turning on the Commander** (i.e. attach the USB cable). This will boot the Commander in *Reset mode*. The "*Leslie fast*" button's led will start blinking
- **while holding the *ALT* button, press the blinking "*Leslie fast*" button** to wipe the memory and reload the factory presets. The led will stop blink.
- **TODO: how to know when the restore finish?**
- Now **release the *ALT* button**: the Commander will exit from the *Reset mode* and it's ready to perform using the default parameters.

## Changelog:
- v. 1.3.6 Fix issue with the Vibrato selector (isssue #70)

- v. 1.3.5 Fix some issues with control assign in FA preset, and a small bug in the Pedal switch control

- v. 1.3.4 Blinks preset's LED on MIDI in activity

- v. 1.3.3 Resolve some issues when triggering Analog Sync (press ALT + VIB ON); Implements "All sound OFF" (press ALT + FAST); Update the CC assigns in presets to reflects the final panels labelling

- v. 1.3.1 / v1.3.2 Let you specify that the Switch pedal has to turn on/off a "linked" button's LED: Set the Switch pedal's PARAM to the button index, and set to 0 the MIN, MAX, CHAN.

- v. 1.3 Reduce the MIDI thru latency.

- v. 1.2.2 implement the new Debug library

- v. 1.2 / v1.2.1 fix issues with Presets and button's led

- v. 1.1 fix issue with non-toggle buttons

- v. 1.0 first stable release. Not finished but very usable
