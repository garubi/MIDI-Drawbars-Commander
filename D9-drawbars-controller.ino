/*
  D9 programmable drawbars controller

  ver 1.2.2

  Created 2018
  By Stefano Garuti stefano@garuti.it

  Project home:
  https://github.com/garubi/D9-drawbars-controller

  LICENSE:
  https://github.com/garubi/D9-drawbars-controller/blob/master/LICENSE

*/

/* TODO
 *  reduce code repetition when controlling for IS_GLOBAL and IS_ALL in both analog and digital input
 *  */

#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Bounce2.h> // https://github.com/thomasfredericks/Bounce2/wiki
#include <MIDI.h>
#include <ResponsiveAnalogRead.h>

#define PRINTSTREAM_FALLBACK
#define DEBUG_OUT Serial
#include "Debug.hpp" // https://github.com/tttapa/Arduino-Debugging

/* *************************************************************************
 *  Pins assign
 */
// Drawbars <-> Analog PIN corrispondence
const byte DWB16    = A8;
const byte DWB5_13  = A7;
const byte DWB8     = A6;
const byte DWB4     = A10;
const byte DWB2_23  = A11;
const byte DWB2     = A3;
const byte DWB1_35  = A2;
const byte DWB1_13  = A1;
const byte DWB1     = A0;
const byte PED_EXP  = A12;

// Buttons <-> Digital PIN corrispondence
const byte PED_SW = 12;
const byte BTN_ALT = 9;
const byte CHOVIB_ON = 8;
const byte PERC_ON = 7;
const byte PERC_SOFT = 6;
const byte PERC_FAST = 5;
const byte PERC_3RD = 4;
const byte LSL_STOP = 3;
const byte LSL_FAST = 2;
const byte LED_ALT = 0;

const byte BTN_COUNT = 8; // configurable digital input number (less the Alternate button, counted a part) include the pedal input
const byte DRWB_COUNT = 10; // configurable number of drawbars used (add the exp pedal too)
const byte CONTROLS_NUM = BTN_COUNT + DRWB_COUNT;
const byte BTN_LED_COUNT = 7; // number of digital inputs that have leds (less the Alternate button, counted a part)
const byte VIBCHO_LED_COUNT = 6; // number of leds used to show the Vibrato/Chorus selected.
const byte TOTAL_LED_COUNT = BTN_LED_COUNT + VIBCHO_LED_COUNT + 1; // total number of leds (buttons + vib/cho + ALT)

/* *************************************************************************
 *  The Controls STATUSES:
 *  We have 3 statuses: UPPER, LOWER and the ALT
 */
byte STATUS;
byte OLD_STATUS;

const byte ST_ALT  = 0; // Controller status: ALTERNATE
const byte ST_UP   = 1; // Controller status: UPPER
const byte ST_LOW  = 2; // Controller status: LOWER
const byte STATUS_IDX[] =  // the column number in the PRESETS array where the parameters starts for each status
 {
  12, 0, 6
 };
const byte STATUSES_COUNT = sizeof(STATUS_IDX) / sizeof(STATUS_IDX[0]);

/* *************************************************************************
 *  presets
 */

const byte BTN_IDX_START = DRWB_COUNT; // at wich row of the presets array does the buttons rows starts?

/*
   The multidimensional Array byte PRESETS will contains in each row:
   1) the type of midi message to send out:*/
		const byte TP_NO   = 0; // Disabled
		const byte TP_ON   = 1; // Note on
		const byte TP_CC   = 2; // COntrol Change
		const byte TP_PC   = 3; // Program CHange
		const byte TP_SX   = 4; // System Exclusive
/*
   2) the command parameter (CC number, or Note number, or SySEx parameter etc...)
   3) the min value to send out
   4) the max value to sed out
   5) the button (and associated control command) behaviour:
*/
		const byte IS_TOGGLE = 1; // is a pushbutton (momentary) or is toggle?
		const byte IS_GLOBAL = 2; // if the control sends always the same value both in Upper that in Lower state (sends what's set in the Upper one)
		const byte SEND_BOTH = 4; // send the value set in this STATUS to both the Upper and Lower channels
/*
	We can set the Pedal Switch as an alias of another button
	 (i.e. when we press the pedal, the script acts as we pressed the associated button: sends out the buttons values and turn on/off the respective LED )
	To set the Pedals as an alias: put in the Pedal UP Status the MIN, MAX, CHAN to 0 and put in PARAM the number of the aliased button.
	ES:   {TP_ON,  5, 0, 0, 0, 0,    TP_NO,  0, 0, 0, 0, 0 ,   TP_NO,  0, 0, 0, 0, 0 },
	*
	NOTE that the switch pedal don't do anything when we are in ST_ALT
*/
/*****************************
 * PRESETS array
 * 0: Factory preset for Roland FA 06/07/07
 * 1: Factory preset for GSi Gemini expander
 */
const byte PRESETS[][CONTROLS_NUM][18]=
{//                 UPPER                                    LOWER                                     ALTERNATE
{//PIN             Type Prm Min Max Ch Behaviour                  Type Prm Min Max Ch Behaviour                Type Prm Min Max Ch Behaviour
/*DWB1*/        {TP_SX, 0x2A, 0, 8, 1, 0,                      TP_SX, 0x2A, 0, 8, 2, 0,                       TP_NO, 0x00, 0, 8, 1, 0},
/*DWB1_13*/     {TP_SX, 0x29, 0, 8, 1, 0,                      TP_SX, 0x29, 0, 8, 2, 0,                       TP_NO, 0x00, 0, 8, 1, 0},
/*DWB1_35*/     {TP_SX, 0x28, 0, 8, 1, 0,                      TP_SX, 0x28, 0, 8, 2, 0,                       TP_NO, 0x00, 0, 8, 1, 0},
/*DWB2*/        {TP_SX, 0x27, 0, 8, 1, 0,                      TP_SX, 0x27, 0, 8, 2, 0,                       TP_NO, 0x00, 0, 8, 1, 0},
/*DWB2_23*/     {TP_SX, 0x26, 0, 8, 1, 0,                      TP_SX, 0x26, 0, 8, 2, 0,                       TP_NO, 0x00, 0, 8, 1, 0},
/*DWB4*/        {TP_SX, 0x25, 0, 8, 1, 0,                      TP_SX, 0x25, 0, 8, 2, 0,                       TP_NO, 0x00, 0, 8, 1, 0},
/*DWB8*/        {TP_SX, 0x24, 0, 8, 1, 0,                      TP_SX, 0x24, 0, 8, 2, 0,                       TP_NO, 0x00, 0, 8, 1, 0},
/*DWB5_13*/     {TP_SX, 0x23, 0, 8, 1, 0,                      TP_SX, 0x23, 0, 8, 2, 0,                       TP_NO, 0x00, 0, 8, 1, 0},
/*DWB16*/       {TP_SX, 0x22, 0, 8, 1, 0,                      TP_SX, 0x22, 0, 8, 2, 0,                       TP_NO, 0x00, 0, 8, 1, 0},
/*PED_EXP */    {TP_CC, 11, 0, 127, 1, SEND_BOTH,              TP_CC, 11, 0, 127, 2, SEND_BOTH,               TP_CC, 11, 0, 127, 0, SEND_BOTH}, //volume
/*CHOVIB_ON*/   {TP_NO, 0x00, 0, 0, 0, 0,                      TP_NO, 0x00, 0, 0, 0, 0,                       TP_NO, 0x00, 0, 0, 0, 0},
/*PERC_ON*/     {TP_SX, 0x2B, 0, 1, 1, IS_TOGGLE + IS_GLOBAL,  TP_SX, 0x2B, 0, 1, 2, IS_TOGGLE + IS_GLOBAL,   TP_PC, 0,    0, 0, 0, 0}, // reserved to preset
/*PERC_SOFT*/   {TP_SX, 0x36, 0, 1, 1, IS_TOGGLE + IS_GLOBAL,  TP_SX, 0x36, 0, 1, 2, IS_TOGGLE + IS_GLOBAL,   TP_PC, 0,    0, 0, 0, 0}, // reserved to preset
/*PERC_FAST*/   {TP_SX, 0x2D, 0, 1, 1, IS_TOGGLE + IS_GLOBAL,  TP_SX, 0x2D, 0, 1, 2, IS_TOGGLE + IS_GLOBAL,   TP_PC, 0,    0, 0, 0, 0}, // reserved to preset
/*PERC_3RD*/    {TP_SX, 0x2C, 0, 1, 1, IS_TOGGLE + IS_GLOBAL,  TP_SX, 0x2C, 0, 1, 2, IS_TOGGLE + IS_GLOBAL,   TP_PC, 0,    0, 0, 0, 0}, // reserved to preset
/*LSL_STOP*/    {TP_CC, 80, 0, 127, 1, IS_TOGGLE + SEND_BOTH,  TP_CC, 80, 0, 127, 2, IS_TOGGLE + SEND_BOTH,   TP_CC, 80, 0, 127, 0, IS_TOGGLE + SEND_BOTH}, //leslie OFF
/*LSL_FAST*/    {TP_CC, 81, 0, 127, 1, IS_TOGGLE + SEND_BOTH,  TP_CC, 81, 0, 127, 2, IS_TOGGLE + SEND_BOTH,   TP_NO, 0,  0, 127, 0, 0},
/*PED_SWITCH*/  {TP_ON,  5, 0, 0, 0, IS_TOGGLE + SEND_BOTH,    TP_ON,  5, 0, 127, 2, IS_TOGGLE + SEND_BOTH,   TP_ON,  5, 0,   0, 0, 0},
},//                 UPPER                                        LOWER                                    ALTERNATE
{//PIN            Type Prm Min Max Ch Behaviour                 Type Prm Min Max Ch Behaviour                  Type Prm Min Max Ch Behaviour
/*DWB1*/        {TP_CC, 20, 0, 127, 1, 0,                      TP_CC, 29, 0, 127, 1, 0,                       TP_CC, 84, 0, 127, 1, 0}, // REV LEVEL
/*DWB1_13*/     {TP_CC, 19, 0, 127, 1, 0,                      TP_CC, 28, 0, 127, 1, 0,                       TP_CC, 83, 0, 127, 1, 0}, // REV SIZE
/*DWB1_35*/     {TP_CC, 18, 0, 127, 1, 0,                      TP_CC, 27, 0, 127, 1, 0,                       TP_CC, 76, 0, 127, 1, 0}, // DRIVE
/*DWB2*/        {TP_CC, 17, 0, 127, 1, 0,                      TP_CC, 26, 0, 127, 1, 0,                       TP_CC, 75, 0, 127, 1, 0}, // KEY CLICK
/*DWB2_23*/     {TP_CC, 16, 0, 127, 1, 0,                      TP_CC, 25, 0, 127, 1, 0,                       TP_NO,  0, 0, 127, 1, 0},
/*DWB4*/        {TP_CC, 15, 0, 127, 1, 0,                      TP_CC, 24, 0, 127, 1, 0,                       TP_NO,  0, 0, 127, 1, 0},
/*DWB8*/        {TP_CC, 14, 0, 127, 1, 0,                      TP_CC, 23, 0, 127, 1, 0,                       TP_CC, 73, 0, 127, 1, 0}, // VIB TYPE
/*DWB5_13*/     {TP_CC, 13, 0, 127, 1, 0,                      TP_CC, 22, 0, 127, 1, 0,                       TP_CC, 35, 0, 127, 1, 0}, // PEDAL 8
/*DWB16*/       {TP_CC, 12, 0, 127, 1, 0,                      TP_CC, 21, 0, 127, 1, 0,                       TP_CC, 33, 0, 127, 1, 0}, // PEDAL 16
/*PED_EXP */    {TP_CC,  7, 0, 127, 1, IS_GLOBAL,              TP_CC,  7, 0, 127, 1, IS_GLOBAL,               TP_CC, 7,  0, 127, 1, IS_GLOBAL}, // Volume
/*CHOVIB_ON*/   {TP_CC, 31, 0, 127, 1, IS_TOGGLE,              TP_CC, 30, 0, 127, 1, IS_TOGGLE,               TP_CC, 55, 0, 127, 1, IS_TOGGLE}, // PEDAL TO LOWER
/*PERC_ON*/     {TP_CC, 66, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,  TP_CC, 66, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,   TP_PC, 0,  0,   0, 0, 0}, // reserved to preset
/*PERC_SOFT*/   {TP_CC, 70, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,  TP_CC, 70, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,   TP_PC, 0,  0,   0, 0, 0}, // reserved to preset
/*PERC_FAST*/   {TP_CC, 71, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,  TP_CC, 71, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,   TP_PC, 0,  0,   0, 0, 0}, // reserved to preset
/*PERC_3RD*/    {TP_CC, 72, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,  TP_CC, 72, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,   TP_PC, 0,  0,   0, 0, 0}, // reserved to preset
/*LSL_STOP*/    {TP_CC, 87, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,  TP_CC, 87, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,   TP_CC, 85, 0, 127, 1, IS_TOGGLE}, // LESLIE OFF
/*LSL_FAST*/    {TP_CC, 86, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,  TP_CC, 86, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,   TP_CC, 51, 0, 127, 1, IS_TOGGLE}, // REV OFF
/*PED_SWITCH*/  {TP_ON,  5, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,  TP_ON,  5, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,   TP_ON,  5,  0, 127, 1, 0},
}
};

const byte PRESETS_COUNT = sizeof(PRESETS) / sizeof(PRESETS[0]);

/* Array index position labels */
const byte TYPE = 0;
const byte PARAM = 1;
const byte MIN = 2;
const byte MAX = 3;
const byte CHAN = 4;
const byte BEHAV = 5;

byte curr_preset; // the currennt selected preset.

// declares the default status of buttons when we switch to a new preset
byte btn_default[BTN_LED_COUNT+1][STATUSES_COUNT] = {
                          //ALT UP LOW
    /*PEDAL TO LOWER */ 	{1, 1, 0},  /*CHOVIB_ON*/
    /*preset */           	{0, 1, 0},  /*PERC_ON*/
    /*preset */           	{1, 0, 0},  /*PERC_SOFT*/
    /*preset */            	{0, 0, 0},  /*PERC_FAST*/
    /*preset */            	{0, 0, 1},  /*PERC_3RD*/
    /* leslie off */      	{1, 0, 0},  /*LSL_STOP*/
    /* rev off */         	{0, 1, 0},  /*LSL_FAST*/
	/* vib/cho sel. value*/	{127,0,0} 	// we start with C3
};


/* *************************************************************************
 *  Drawbars initialization
 */

// a data array and a lagged copy to tell when Midi changes are required
byte analogData[DRWB_COUNT];
byte analogDataLag[DRWB_COUNT]; // when lag and new are not the same then update Midi CC value

// initialize the ReponsiveAnalogRead objects
ResponsiveAnalogRead drwb[] {
  {DWB1, true},
  {DWB1_13, true},
  {DWB1_35, true},
  {DWB2, true},
  {DWB2_23, true},
  {DWB4, true},
  {DWB8, true},
  {DWB5_13, true},
  {DWB16, true},
  {PED_EXP, true},
};

/* declares which is the analog input that controls Vib/cho type */
const byte VIBCHO_SEL_STATUS = ST_ALT;
const byte VIBCHO_SEL_DRWB = 6; // which Drawbar is used to select the vib/cho type when in ALT mode?


/* *************************************************************************
 *  Buttons initialization
 */
// Creates an array and fills it with Bounce objects. see https://forum.arduino.cc/index.php?topic=266132.msg2071306#msg2071306
byte btn_state[STATUSES_COUNT][BTN_COUNT] = {};
Bounce * btn = new Bounce[BTN_COUNT] ;
Bounce btn_alt = Bounce() ;

byte btnAlt_pushed;
byte btnAlt_released;
static unsigned long BTN_LONG_PRESS_MILLIS = 1300;

/* declare which are the Presetes buttons */
const byte BTN_PRST_STATUS = ST_ALT; // the status that contains the preset buttons
const byte BTN_PRST_START = 1; // the btn at wich the preset selectors starts
const byte BTN_PRST_COUNT = 4; // the number of presets selectors (even if inactive!!)
const byte BTN_PED= 6;

/* *************************************************************************
 *  LEDs initialization
 */
// Controls LEDs attacched to MCP23017
Adafruit_MCP23017 led;

// An array that store the state of the buttons leds (including the Alt btn/led).
byte ledState[STATUSES_COUNT] = {};
byte ledState_old[STATUSES_COUNT] = {}; // the previous leds state, to check if we have to update the leds register
long led_alt_on_time;
byte vibchoLedState; // stores the Vibrato/chorus leds state
byte vibchoLedState_old; // the previous leds state, to check if we have to update the leds register
byte old_preset_led; // the previous selected preset's led
byte vibcho_led_on_old;
/* *************************************************************************
 *  MIDI initialization
 */

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
const byte SEND_NOTE_OFF = 0; // don't send also the note off control when sending note on = 0;

void setup()
{
  Serial.begin(38400);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // set ALT button as input Pullup and attach debouncer
  pinMode(BTN_ALT, INPUT_PULLUP);
  btn_alt.attach(BTN_ALT);

  // set all "standard" buttons as input Pullup and attach debouncer
  pinMode(PED_SW, INPUT_PULLUP);
  pinMode(CHOVIB_ON, INPUT_PULLUP);
  pinMode(PERC_ON, INPUT_PULLUP);
  pinMode(PERC_SOFT , INPUT_PULLUP);
  pinMode(PERC_FAST , INPUT_PULLUP);
  pinMode(PERC_3RD , INPUT_PULLUP);
  pinMode(LSL_STOP, INPUT_PULLUP);
  pinMode(LSL_FAST , INPUT_PULLUP);
  btn[0].attach(CHOVIB_ON);
  btn[1].attach(PERC_ON);
  btn[2].attach(PERC_SOFT);
  btn[3].attach(PERC_FAST);
  btn[4].attach(PERC_3RD);
  btn[5].attach(LSL_STOP);
  btn[6].attach(LSL_FAST);
  btn[7].attach(PED_SW);

  led.begin();      // use default address 0
  for (int a = 0; a < TOTAL_LED_COUNT; a++) {
    // from 0 to 8 are the pushbutton leds, from 9 to 14 are the chorus/vibrato status leds
    led.pinMode(a, OUTPUT);
  }

// initialize at the startup
  STATUS = ST_UP;
  OLD_STATUS = ST_LOW;
  btnAlt_released = 1;

  // load the default preset
  for (byte btn_scanned = BTN_PRST_START; btn_scanned < (BTN_PRST_COUNT + BTN_PRST_START); btn_scanned++) {
      if( 0 != btn_default[btn_scanned][BTN_PRST_STATUS] ){
        changePreset(  btn_scanned  );
		break;
      }
    }

  syncAnalogData();
}

void loop() {
  // Scan ALT Button
  getAltBtn();

  // Scan Drawbars
  getAnalogData();
  MidiMerge();

  // Scan Buttons
  getDigitalData();
  MidiMerge();

  // Set Leds
  setLeds();

  /* MIDI merge and thru */
  MidiMerge();

}

bool isPresetButton( byte btn_scanned, byte the_status ) {
	if( the_status == BTN_PRST_STATUS && (btn_scanned >= BTN_PRST_START && btn_scanned <= (BTN_PRST_COUNT + BTN_PRST_START -1) )  ){
		return true;
	}
	return false;
}

void changePreset( byte btn_scanned ){
  	DEBUGFN(NAMEDVALUE(btn_scanned));
	// set it only if is defined in the preset array
	if( btn_scanned - BTN_PRST_START <= PRESETS_COUNT - 1){
    	DEBUGFN("CHANGING preset");
		setBtnLedState(BTN_PRST_STATUS, old_preset_led, 0);
		setBtnLedState(BTN_PRST_STATUS, btn_scanned,  !btn_state[BTN_PRST_STATUS][btn_scanned]);

		// set the new preset value
		curr_preset = btn_scanned - BTN_PRST_START;

		// reset all data
		resetToDefaultData();

		// Check if the pedal is aliased
		if( PRESETS[curr_preset][BTN_PED][STATUS_IDX[ST_UP] + MIX ] == 0 && PRESETS[curr_preset][BTN_PED][STATUS_IDX[ST_UP] + MAX ] == 0 && PRESETS[curr_preset][BTN_PED][STATUS_IDX[ST_UP] + CHAN ] == 0){
			isPedalAliased = true;
		}
		else {
			isPedalAliased = false;
		}

    	DEBUGFN( NAMEDVALUE(curr_preset) );
		old_preset_led = btn_scanned;
	}
	else{
    	DEBUGFN("CAN'T CHANGE preset: preset location empty");
	}

}

void resetToDefaultData(){
  DEBUGFN("Reset to default data");
	// SET al buttons to 0
	for (byte st = 0; st < STATUSES_COUNT; st++){
	  for (byte btn_scanned = 0; btn_scanned < BTN_LED_COUNT; btn_scanned++) {
	    if ( !isPresetButton(btn_scanned, st) ){ // check that's not a Preset button
	      updateBtn( btn_scanned, 0, st );
	    }
	  }
	}

	//Set the default value for all buttons, only if it's not 0 (since we have already set all of them to 0)
	for (byte st = 0; st < STATUSES_COUNT; st++){
	  for (byte btn_scanned = 0; btn_scanned < BTN_LED_COUNT; btn_scanned++) {
	    if ( !isPresetButton(btn_scanned, st) ){ // check that's not a Preset button
	  	  if (btn_default[btn_scanned][st] != 0 ){
	  		  updateBtn( btn_scanned, btn_default[btn_scanned][st], st );
	  	 }
	    }
	  }
	}

  setVibchoType( btn_default[7][ST_ALT] );
}

void getAltBtn(){
  static unsigned long btnAlt_DownTime;

  // scansioniamo il pulsante "ALT"
  if (btn_alt.update()){
    if (btn_alt.fell()) {
       btnAlt_pushed = 1;
       btnAlt_DownTime = millis();
       DEBUGFN( "ALT BTN PUSHED" );
       DEBUGFN( NAMEDVALUE(STATUS) );
    }
    else{
      if (btnAlt_pushed == 1){
        if ( STATUS != ST_ALT){
            if ( STATUS == ST_UP ){
              OLD_STATUS = STATUS;
              STATUS = ST_LOW;
              setAltLedState( STATUS, 1);
            }
            else{
              OLD_STATUS = STATUS;
              STATUS = ST_UP;
              setAltLedState( STATUS, 0);
            }
            bitWrite(vibchoLedState, 7, 1 );
            DEBUGFN( NAMEDVALUE(STATUS) );
        }
      }
      btnAlt_released = 1;
      btnAlt_pushed = 0;
      DEBUGFN( "RELEASED: " );
      DEBUGFN( NAMEDVALUE(btnAlt_released) );
    }
  } // ALT BTN not changed
  else{
      // se il suo stato è "premuto" ( cioè 0)... allora verifichiamo da quanto tempo è permuto
      if( btn_alt.read() == 0 ){
          // se è premuto da abbastanza tempo allora passiamo allo Status ALT o ne usciamo
          if ((millis() - btnAlt_DownTime) > BTN_LONG_PRESS_MILLIS && btnAlt_released == 1 ) {
            DEBUGFN( "ALT BTN LONG PRESS!!:" );
            btnAlt_pushed = 0;
            btnAlt_released = 0;
            OLD_STATUS = STATUS;
            if ( STATUS == ST_ALT ){ // Siamo già in ALT quindi ne usciamo e torniamo al UP
              STATUS = ST_UP;
              setAltLedState( STATUS, 0 );
              DEBUGFN("from ALT to STATUS: ");
              DEBUGFN( NAMEDVALUE(STATUS) );
            }
            else { //entriamo nello stato ALT
              STATUS = ST_ALT;
              led_alt_on_time = millis();
              setAltLedState( STATUS, 1);
              DEBUGFN( NAMEDVALUE(STATUS) );
            }
            bitWrite(vibchoLedState, 7, 1 );
         }
      }
  }
}
void setBtnLedState( byte status, byte btn, byte value ){
	byte btnled = btn + 1;
	setLedState( status, btnled, value );
}
void setAltLedState( byte status, byte value ){
	setLedState( status, LED_ALT, value );
}

void setLedState( byte status, byte btn, byte value ){
  if ( btn <= BTN_LED_COUNT ){ // escludiamo di impostare lo stato per input che non hanno il led (tipo il pedale)
    bitWrite(ledState[status], btn, value);
  }
}

void setLeds(){

    if ( STATUS == ST_ALT){
       //blink LED_ALT
      if( millis()-led_alt_on_time > 500 ){
		    bitWrite(ledState[STATUS], LED_ALT, !bitRead(ledState[STATUS], LED_ALT));
        led_alt_on_time = millis();
      }
    }

	if( word(vibchoLedState,ledState[STATUS]) != word(vibchoLedState_old,ledState_old[STATUS]) ){
		DEBUGFN("change LEDS");
		led.writeGPIOAB(word(vibchoLedState,ledState[STATUS]));
		ledState_old[STATUS] = ledState[STATUS];
    	bitWrite(vibchoLedState, 7, 0 );  // Reset the Flag
		vibchoLedState_old = vibchoLedState;
	}
}

void setVibchoType( byte CCvalue ){
	// calculate which Led turn on based on the Drawbar value
	byte vibcho_led_on =  map(CCvalue, 0, 127, 0, 5);

	// only send if the new VibCho type is different from the old one
	if ( vibcho_led_on != vibcho_led_on_old ){
    	vibcho_led_on_old = vibcho_led_on;
	  	vibchoLedState = 0; // reset the leds
	  	bitWrite(vibchoLedState, vibcho_led_on, 1 );
	  	sendMidi( PRESETS[curr_preset][VIBCHO_SEL_DRWB][STATUS_IDX[VIBCHO_SEL_STATUS] +TYPE], PRESETS[curr_preset][VIBCHO_SEL_DRWB][STATUS_IDX[VIBCHO_SEL_STATUS] +PARAM], CCvalue, VIBCHO_SEL_DRWB, PRESETS[curr_preset][VIBCHO_SEL_DRWB][STATUS_IDX[VIBCHO_SEL_STATUS] +CHAN] );
	}
}

void getAnalogData() {
  for (int drwb_scanned = 0; drwb_scanned < DRWB_COUNT; drwb_scanned++) {
    // update the ResponsiveAnalogRead object every loop
    drwb[drwb_scanned].update();
    if( PRESETS[curr_preset][drwb_scanned][STATUS_IDX[STATUS] +TYPE] != TP_NO ){
      // if the repsonsive value has changed, go
      if (drwb[drwb_scanned].hasChanged()) {
        analogData[drwb_scanned] = drwb[drwb_scanned].getValue() >> 3;
        if (analogData[drwb_scanned] != analogDataLag[drwb_scanned]) {
          analogDataLag[drwb_scanned] = analogData[drwb_scanned];
          DEBUGFN( "DWB changed: " );
          DEBUGVAL(drwb_scanned,analogData[drwb_scanned]);
          // check if this drawbar is dedicated to the VIB/CHO control
          if ( STATUS == VIBCHO_SEL_STATUS && drwb_scanned == VIBCHO_SEL_DRWB ){
            DEBUGFN("This DWB controls VIBCHO selection");
			setVibchoType( analogData[drwb_scanned] );
          }
          else{
                sendAnalogMidi( analogData[drwb_scanned], drwb_scanned, STATUS );
        	}
        }
      }
    }
  }
}

void syncAnalogData() {
  DEBUGFN("SYNC DWB");
  for (int drwb_scanned = 0; drwb_scanned < DRWB_COUNT; drwb_scanned++) {
    // update the ResponsiveAnalogRead object every loop
    drwb[drwb_scanned].update();
    if( PRESETS[curr_preset][drwb_scanned][STATUS_IDX[STATUS] +TYPE] != TP_NO ){
      analogData[drwb_scanned] = drwb[drwb_scanned].getValue() >> 3;
      analogDataLag[drwb_scanned] = analogData[drwb_scanned];
      DEBUGFN( "DWB synced: " );
      DEBUGVAL(drwb_scanned,analogData[drwb_scanned]);
      sendAnalogMidi( analogData[drwb_scanned], drwb_scanned, STATUS );
    }
  }
  // show a led animation to give a feedback about the correct update
  ledCarousel();
}

void sendAnalogMidi ( byte value, byte control, byte curr_status ){
  if ( ( PRESETS[curr_preset][control][STATUS_IDX[curr_status] +BEHAV] & SEND_BOTH ) == SEND_BOTH  ){
        sendMidi( PRESETS[curr_preset][control][STATUS_IDX[curr_status]  +TYPE], PRESETS[curr_preset][control][STATUS_IDX[curr_status]  +PARAM], value, control, PRESETS[curr_preset][control][STATUS_IDX[ST_UP]  +CHAN] );
        sendMidi( PRESETS[curr_preset][control][STATUS_IDX[curr_status] +TYPE], PRESETS[curr_preset][control][STATUS_IDX[curr_status] +PARAM], value, control, PRESETS[curr_preset][control][STATUS_IDX[ST_LOW] +CHAN] );
  }
  else if ( ( PRESETS[curr_preset][control][STATUS_IDX[curr_status] +BEHAV] & IS_GLOBAL ) == IS_GLOBAL ) {
        sendMidi( PRESETS[curr_preset][control][STATUS_IDX[ST_UP] +TYPE], PRESETS[curr_preset][control][STATUS_IDX[ST_UP] +PARAM], value, control, PRESETS[curr_preset][control][STATUS_IDX[ST_UP] +CHAN] );
  }
  else {
    sendMidi( PRESETS[curr_preset][control][STATUS_IDX[curr_status] +TYPE], PRESETS[curr_preset][control][STATUS_IDX[curr_status] +PARAM], value, control, PRESETS[curr_preset][control][STATUS_IDX[curr_status] +CHAN] );
  }
}

void updateBtn( byte btn_scanned, byte btn_val, byte curr_status ){
    byte btn_index = btn_scanned + BTN_IDX_START;
	if ( ( PRESETS[curr_preset][btn_index][STATUS_IDX[curr_status] +BEHAV] & SEND_BOTH ) == SEND_BOTH  ){
          sendMidi( PRESETS[curr_preset][btn_index][STATUS_IDX[curr_status] +TYPE], PRESETS[curr_preset][btn_index][STATUS_IDX[curr_status] +PARAM], btn_val * 127, btn_index, PRESETS[curr_preset][btn_index][STATUS_IDX[ST_UP] +CHAN] );
          sendMidi( PRESETS[curr_preset][btn_index][STATUS_IDX[curr_status] +TYPE], PRESETS[curr_preset][btn_index][STATUS_IDX[curr_status] +PARAM], btn_val * 127, btn_index, PRESETS[curr_preset][btn_index][STATUS_IDX[ST_LOW] +CHAN] );
		  if( curr_status == ST_ALT ){
			  setBtnLedState(curr_status, btn_scanned,  btn_val);
			  btn_state[curr_status][btn_scanned] = btn_val;
		  }
		  else{
			  setBtnLedState(ST_UP, btn_scanned,  btn_val);
			  setBtnLedState(ST_LOW, btn_scanned, btn_val);
			  btn_state[ST_UP][btn_scanned] = btn_val;
			  btn_state[ST_LOW][btn_scanned] = btn_val;
		  }

    }
    else if ( ( PRESETS[curr_preset][btn_index][STATUS_IDX[curr_status] +BEHAV] & IS_GLOBAL ) == IS_GLOBAL ) {
          sendMidi( PRESETS[curr_preset][btn_index][STATUS_IDX[ST_UP] +TYPE], PRESETS[curr_preset][btn_index][STATUS_IDX[ST_UP] +PARAM], btn_val * 127, btn_index, PRESETS[curr_preset][btn_index][STATUS_IDX[ST_UP] +CHAN] );
          setBtnLedState(ST_UP, btn_scanned, btn_val);
          setBtnLedState(ST_LOW, btn_scanned, btn_val);
          btn_state[ST_UP][btn_scanned] = btn_val;
          btn_state[ST_LOW][btn_scanned] = btn_val;
    }
    else {
      sendMidi( PRESETS[curr_preset][btn_index][STATUS_IDX[curr_status] +TYPE], PRESETS[curr_preset][btn_index][STATUS_IDX[curr_status] +PARAM], btn_val * 127, btn_index, PRESETS[curr_preset][btn_index][STATUS_IDX[curr_status] +CHAN] );
      setBtnLedState(curr_status, btn_scanned, btn_val);
	    btn_state[curr_status][btn_scanned] = btn_val;
    }
    DEBUGVAL(btn_scanned,btn_val,curr_status);
}

void getDigitalData() {

  // scansioniamo i pulsanti (tranne ALT che va da solo)
  for (byte btn_scanned = 0; btn_scanned < BTN_COUNT; btn_scanned++) {
    byte btn_val = 0;
    byte btn_index = btn_scanned + BTN_IDX_START;

	// don't do anything we ar in the AlT STATUS and the pedal switch is pressed
	if( STATUS == ST_ALT && btn_scanned = PED_BTN ){ continue; }

    if (PRESETS[curr_preset][btn_index][STATUS_IDX[STATUS] + TYPE] != TP_NO && btn[btn_scanned].update()) {
      btn_val = btn[btn_scanned].read();

      // Pulsnte PREMUTO
      if (btn[btn_scanned].fell()) {
        //se il pulsante è un preset...
		if(isPresetButton( btn_scanned, STATUS )) {
        	changePreset(  btn_scanned  );
        }
        else{
          if( btnAlt_pushed == 0){
            // Caso "normale" il pulsante è premuto da solo
              DEBUGFN("BTN pressed / value: ");
              DEBUGVAL(btn_scanned, btn_val);

			  // if the Pedal is aliased, we use the settings of the relative button
			  if( isPedalAliased == true ){
				  btn_scanned = PRESETS[curr_preset][btn_index][STATUS_IDX[STATUS] + PARAM];
				  btn_index = btn_scanned + BTN_IDX_START;
			  }

              if ( (PRESETS[curr_preset][btn_index][STATUS_IDX[STATUS] +BEHAV] & IS_TOGGLE )== IS_TOGGLE){
                DEBUGFN("toggle...");
                // il pulsante è TOGGLE
                btn_val = !btn_state[STATUS][btn_scanned];
              }
              else{
  	              //il pulsante non è TOGGLE
                  DEBUGFN("non toggle... non preset");
  	             // btn_state[STATUS][btn_scanned] = !btn_val;
  	              btn_val = !btn_val;
              }
              updateBtn( btn_scanned, btn_val, STATUS);
          }
          else{
            // Pulsante premuto in contemporanea all'ALT ...
            btnAlt_pushed = 0;
            DEBUGFN("ALT + BTN: ");
            DEBUGVAL(btn_scanned);
            //sync analog data
            if( btn_scanned == 0 ){
              syncAnalogData();
            }
          }
        }
    }
      // Pulsante rilasciato
      else {
        // reagisce solo se questo pulsante non è TOGGLE e non è PRESET
        if ( !isPresetButton( btn_scanned, STATUS ) ){
		  // if the Pedal is aliased, we use the settings of the relative button
		  if( isPedalAliased == true ){
			  btn_scanned = PRESETS[curr_preset][btn_index][STATUS_IDX[STATUS] + PARAM];
			  btn_index = btn_scanned + BTN_IDX_START;
		  }

			if ( (PRESETS[curr_preset][btn_index][STATUS_IDX[STATUS] +BEHAV] & IS_TOGGLE) != IS_TOGGLE){
	           DEBUGFN("Btn released - No TOOGLE & No PRESET...");
	           DEBUGVAL(btn_val);
	           DEBUGVAL(!btn_val);
			   //if is_pedal && pedalAlias > 0 --> btn_scanned = pedalAlias, btn_index = btn_scanned + BTN_IDX_START
	           updateBtn( btn_scanned, !btn_val, STATUS);
	   		}
        }
      }

    } // fine btn scanned.updated
  }
}


void sendMidi( int type, byte parameter, byte value, byte control, byte channel)
{
  DEBUGFN("Send midi");
  DEBUGVAL(type,parameter,value,control);
  int SysexLenght = 0;
    switch (type) {
      case TP_ON: // Note On
        usbMIDI.sendNoteOn(parameter, value, channel);
        MIDI.sendNoteOn(parameter, value, channel);
        if(SEND_NOTE_OFF){
            usbMIDI.sendNoteOff(parameter, value, channel);
            MIDI.sendNoteOff(parameter, value, channel);
        }
        break;
      case TP_CC: // Control Change
        MIDI.sendControlChange(parameter, value, channel);
        usbMIDI.sendControlChange(parameter, value, channel);
        break;
      case TP_PC: // Program Change
        MIDI.sendProgramChange(value, channel);
        usbMIDI.sendProgramChange(value, channel);
        break;
      case TP_SX: // SysEx
        if (curr_preset == 0) {
          /**
           * è il preset per Roland FA 06/07/08
           */
              //https://forum.arduino.cc/index.php?topic=228570.0
              //https://bitbucket.org/loveaurell/roland-alpha-juno-mks-50-midi-controller/src/62d01604594b5afc56ecee6e9dadc6dc4fb12242/src/mks50_controller/mks50_controller.ino?at=default&fileviewer=file-view-default
              SysexLenght = 14;
              uint8_t data[SysexLenght] = {0xF0, 0x41, 0x10, 0x00, 0x00, 0x77, 0x12, 0x19, 0x02, 0x00, 0x00, 0x00, 0x00, 0xF7 };
              byte partsA[16] = {0x19, 0x19, 0x19, 0x19, 0x1A, 0x1A, 0x1A, 0x1A, 0x1B, 0x1B, 0x1B, 0x1B, 0x1C, 0x1C, 0x1C, 0x1C };
              byte partsB[16] =  {0x02, 0x22, 0x42, 0x62, 0x02, 0x22, 0x42, 0x62, 0x02, 0x22, 0x42, 0x62, 0x02, 0x22, 0x42, 0x62 };
              data[7] = partsA[channel - 1];
              data[8] = partsB[channel - 1];
              data[10] = parameter;
              data[11] = value;
              if ( PRESETS[curr_preset][control][STATUS_IDX[STATUS] +MAX] != 127 || PRESETS[curr_preset][control][STATUS_IDX[STATUS] +MIN] != 0 ) {
                data[11] = map(value, 0, 127, PRESETS[curr_preset][control][STATUS_IDX[STATUS] +MIN], PRESETS[curr_preset][control][STATUS_IDX[STATUS] +MAX]);
              }

              /*
               * Roland CheckSum
               * */
              int address = data[7] + data[8] + data[9] + data[10] + data[11] ;
              int remainder = address % 128;
              data[12] = 128 - remainder;

              MIDI.sendSysEx(SysexLenght, data);
              usbMIDI.sendSysEx(SysexLenght, data);
          }
          else{
            DEBUGFN("altro preset");
            }

        break;
    }
}

void ledCarousel(){
      //Led feedback
    for (byte ledto = 1; ledto < 8; ledto++) {
      //turn off all leds
      led.digitalWrite(ledto, 0);
    }

    for (byte ledto = 1; ledto < 8; ledto++) {
      led.digitalWrite(ledto, 1);
      delay(100);
      led.digitalWrite(ledto, 0);
    }
}

void MidiMerge(){
    // MIDI IN to MIDI OUT should be already handled by the Midi library soft thru.
  // here we handle MIDI to usbMidi and usbMIDI to MIDI

  // code heavily derived form the Interface_3X3 example in Teensyduino
  //bool activity = false;

  if (MIDI.read()) {
    // get a MIDI IN1 (Serial) message
    byte type = MIDI.getType();
    byte channel = MIDI.getChannel();
    byte data1 = MIDI.getData1();
    byte data2 = MIDI.getData2();

    // forward the message to USB MIDI virtual cable #0
    if (type != midi::SystemExclusive) {
      // Normal messages, simply give the data to the usbMIDI.send()
      usbMIDI.send(type, data1, data2, channel, 0);
    } else {
      // SysEx messages are special.  The message length is given in data1 & data2
      unsigned int SysExLength = data1 + data2 * 256;
      usbMIDI.sendSysEx(SysExLength, MIDI.getSysExArray(), true, 0);
    }
    //activity = true;
  }

  if (usbMIDI.read()) {
    // get the USB MIDI message, defined by these 5 numbers (except SysEX)
    byte type = usbMIDI.getType();
    byte channel = usbMIDI.getChannel();
    byte data1 = usbMIDI.getData1();
    byte data2 = usbMIDI.getData2();
   // byte cable = usbMIDI.getCable();

    // forward this message to 1 of the 3 Serial MIDI OUT ports
    if (type != usbMIDI.SystemExclusive) {
      // Normal messages, first we must convert usbMIDI's type (an ordinary
      // byte) to the MIDI library's special MidiType.
      midi::MidiType mtype = (midi::MidiType)type;
      MIDI.send(mtype, data1, data2, channel);
    } else {
      // SysEx messages are special.  The message length is given in data1 & data2
      unsigned int SysExLength = data1 + data2 * 256;
      MIDI.sendSysEx(SysExLength, usbMIDI.getSysExArray(), true);
    }
    //activity = true;
  }

  // TODO: blink the LED when any activity has happened
  /*
  if (activity) {
    digitalWriteFast(13, HIGH); // LED on
    ledOnMillis = 0;
  }
  if (ledOnMillis > 15) {
    digitalWriteFast(13, LOW);  // LED off
  }
  */

}
