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

## SysEX implementation
Since v. 2.0 the device has a full set of custom System Exclusive messages that you can use to can configure what kind of commands/codes every button and drawbar sends.

An editor is already available to configure the device. You can find it here: https://github.com/garubi/Web-editor-for-MIDI-Drawbars-Commander

Format for the requests:

F0 X_MANID1 X_MANID2 X_PRODID X_REQ COOMAND vv F7

Format for the reply

F0 X_MANID1 X_MANID2 X_PRODID X_REP OBJECT (Reply COdes) vv F7

const uint8_t X_MANID1 = 0x37; // Manufacturer ID 1
const uint8_t X_MANID2 = 0x72; // Manufacturer ID 2
const uint8_t X_PRODID = 0x09; // Product ID

/*
 * Where ACTION is
 */
const uint8_t X_REQ = 0x00; // Request
const uint8_t X_REP = 0x01; // Replay

 /*
 * Where COMMAND is:
 */
const uint8_t X_FW_VER 			= 0x01; // Firmware version. Replay vv is VERSION_MAJOR VERSION_MINOR VERSION_PATCH.
const uint8_t X_ACTIVE_PRESET 	= 0x02; // The active preset. Replay vv is byte) Active preset id [0-3].
const uint8_t X_CTRL_INFO 		= 0x03; // Reply with info about the controls: 1) Number of presets slots (PRESETS_COUNT), 2) Buttons number (BTN_COUNT), 3) Drawbars number (DRWB_COUNT)

const uint8_t X_REQ_CTRL_PARAMS = 0x10; // Current settings for a control: PRESET_ID CTRL_ID. Reply vv is: PRESET_ID CTRL_ID UPP_Type UPP_Prm UPP_Min UPP_Max UPP_Ch UPP_behavior LOW_Type LOW_Prm LOW_Min LOW_Max LOW_Ch LOW_behavior ALT_Type ALT_Prm ALT_Min ALT_Max ALT_Ch ALT_behavior
const uint8_t X_SET_CTRL_PARAMS = 0x11; // Send the settings for a control (but doesn't save it): PRESET_ID CTRL_ID UPP_Type UPP_Prm UPP_Min UPP_Max UPP_Ch UPP_behavior LOW_Type LOW_Prm LOW_Min LOW_Max LOW_Ch LOW_behavior ALT_Type ALT_Prm ALT_Min ALT_Max ALT_Ch ALT_behavior. Reply vv is 0 if is all right, an Error code if something went wrog
const uint8_t X_SET_PARAM 		= 0x12; //Send a setting for a single parameter (but doesn't save it): PRESET_ID CTRL_ID PARAM_ID (0-18) param value;
const uint8_t X_CMD_SAVE_PRESET= 0x7F; // Save the Preset to the non volative memory: vv is PRESET_ID. Reply vv is 0 if all went ok, an error code if someting wen wrong

/*
 * REPLY CODES
 */
 const uint8_t X_OK = 0x00;
 const uint8_t X_ERROR = 0x7F; // Something went wrong

/*
 * ERROR CODES
 */
  const uint8_t X_ERROR_UNKNOWN = 0x7F; //127
  const uint8_t X_ERROR_PRESET  = 0x10; //16
  const uint8_t X_ERROR_CONTROL = 0x20; //32
  const uint8_t X_ERROR_PARAM   = 0x30; //48
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
