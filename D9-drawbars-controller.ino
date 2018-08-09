/*
  D9 programmable drawbars controller

  ver 1.0

  Created 2018
  By Stefano Garuti stefano@garuti.it

  Project home:
  https://github.com/garubi/D9-drawbars-controller

  LICENSE:
  https://github.com/garubi/D9-drawbars-controller/blob/master/LICENSE

*/

/* TODO
 *  get rid of btn_scanned +1 moving the +1 in the setLeds() function. Also consider the opportunity of using directly BTN_IND_START
 *  create the setButton(btn_index) function. inside set the button subtracting BTN_IND_START
 *  reduce code repetition when controlling for IS_GLOBAL and IS_ALL in both analog and digital input
 *  */
 
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Bounce2.h> // https://github.com/thomasfredericks/Bounce2/wiki
#include <MIDI.h>
#include <ResponsiveAnalogRead.h>

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
const byte PRESET_CONTROLS_NUM = BTN_COUNT + DRWB_COUNT;
const byte BTN_LED_COUNT = 7; // number of digital inputs that have leds (less the Alternate button, counted a part)
const byte VIBCHO_LED_IDX_START = BTN_LED_COUNT + 1;
const byte VIBCHO_LED_COUNT = 6; // number of leds used to show the Vibrato/Chorus selected. 
const byte TOTAL_LED_COUNT = BTN_LED_COUNT + VIBCHO_LED_COUNT;
const byte TOTAL_LED_ALT_COUNT = BTN_LED_COUNT + VIBCHO_LED_COUNT + 1;

/* *************************************************************************
 *  presets
 */

const byte BTN_IDX_START = DRWB_COUNT; // at wich row of the presets array start the buttons rows?
byte curr_preset; // the currennt selected preset.

// Controls behaviour "labels"
const byte IS_TOGGLE = 1; // is a pushbutton (momentary) or is toggle?
const byte IS_VIBCHO = 2; // the drawbar with this constant sets controls the Vibrato / Chorus type
const byte IS_GLOBAL = 4; // if the control sends always the same value both in Upper that in Lower state (sends what's set in the Upper one)
const byte SEND_ALL  = 8; // if we have to send both the Lower and the Upper values at the same time both in Upper taht in Lower state

/*
   The multidimensional Array byte PRESETS will contains in each row:
   1) the pin to which the drawbar/button is attached to
   2) the type of midi message to send out:*/
      const byte TP_NO   = 0; // Disabled
      const byte TP_ON   = 2; // Note on
      const byte TP_CC   = 3; // COntrol Change
      const byte TP_PC   = 4; // Program CHange
      const byte TP_SX   = 5; // System Exclusive
      const byte TP_PR   = 6; // Controller presets (set the preset value in MAX column)
/*      
   3) the command parameter (CC number, or Note number, or SySEx parameter etc...)
   4) the min value to send out
   5) the max value to sed out
   6) does the button have to behave as a toggle one (IS_TOGGLE) or is used to change the Vibrato/choorus type (IS_VIBCHO)?
   7) Upper/lower behaviour.
*/

/* Array index position labels */
const byte TYPE = 0;
const byte PARAM = 1;
const byte MIN = 2;
const byte MAX = 3;
const byte CHAN = 4;
const byte BEHAV = 5;

/*****************************
 * PRESETS array
 * 0: Factory preset for Roland FA 06/07/07
 * 1: Factory preset for GSi Gemini expander
 */
const byte PRESETS[2][PRESET_CONTROLS_NUM][18]=
{//                 UPPER                                    LOWER                                     ALTERNATE
{//PIN             Type Prm Min Max Ch Behaviour                  Type Prm Min Max Ch Behaviour                Type Prm Min Max Ch Behaviour
/*DWB1*/        {TP_SX, 0x2A, 0, 8, 1, 0,                      TP_SX, 0x2A, 0, 8, 2, 0,                       TP_SX, 0x00, 0, 8, 1, 0},
/*DWB1_13*/     {TP_SX, 0x29, 0, 8, 1, 0,                      TP_SX, 0x29, 0, 8, 2, 0,                       TP_SX, 0x00, 0, 8, 1, 0},
/*DWB1_35*/     {TP_SX, 0x28, 0, 8, 1, 0,                      TP_SX, 0x28, 0, 8, 2, 0,                       TP_SX, 0x00, 0, 8, 1, 0},
/*DWB2*/        {TP_SX, 0x27, 0, 8, 1, 0,                      TP_SX, 0x27, 0, 8, 2, 0,                       TP_SX, 0x00, 0, 8, 1, 0},
/*DWB2_23*/     {TP_SX, 0x26, 0, 8, 1, 0,                      TP_SX, 0x26, 0, 8, 2, 0,                       TP_SX, 0x00, 0, 8, 1, 0},
/*DWB4*/        {TP_SX, 0x25, 0, 8, 1, 0,                      TP_SX, 0x25, 0, 8, 2, 0,                       TP_SX, 0x00, 0, 8, 1, 0},
/*DWB8*/        {TP_SX, 0x24, 0, 8, 1, 0,                      TP_SX, 0x24, 0, 8, 2, 0,                       TP_SX, 0x00, 0, 8, 1, 0},
/*DWB5_13*/     {TP_SX, 0x23, 0, 8, 1, 0,                      TP_SX, 0x23, 0, 8, 2, 0,                       TP_SX, 0x00, 0, 8, 1, 0},
/*DWB16*/       {TP_SX, 0x22, 0, 8, 1, 0,                      TP_SX, 0x22, 0, 8, 2, 0,                       TP_SX, 0x00, 0, 8, 1, 0},
/*PED_EXP */    {TP_CC, 11, 0, 127, 1, SEND_ALL,               TP_CC, 11, 0, 127, 2, SEND_ALL,                TP_NO, 0x00, 0, 8, 1, 0},
/*CHOVIB_ON*/   {TP_NO, 0x00, 0, 0, 0, 0,                      TP_NO, 0x00, 0, 0, 0, 0,                       TP_NO, 0x00, 0, 0, 0, 0},
/*PERC_ON*/     {TP_SX, 0x2B, 0, 1, 1, IS_TOGGLE + IS_GLOBAL,  TP_SX, 0x2B, 0, 1, 1, IS_TOGGLE + IS_GLOBAL,   TP_PR, 0,    0, 0, 1, 0},
/*PERC_SOFT*/   {TP_SX, 0x36, 0, 1, 1, IS_TOGGLE + IS_GLOBAL,  TP_SX, 0x36, 0, 1, 1, IS_TOGGLE + IS_GLOBAL,   TP_PR, 0,    0, 1, 1, 0},
/*PERC_FAST*/   {TP_SX, 0x2D, 0, 1, 1, IS_TOGGLE + IS_GLOBAL,  TP_SX, 0x2D, 0, 1, 1, IS_TOGGLE + IS_GLOBAL,   TP_NO, 0,    0, 1, 1, 0},
/*PERC_3RD*/    {TP_SX, 0x2C, 0, 1, 1, IS_TOGGLE + IS_GLOBAL,  TP_SX, 0x2C, 0, 1, 1, IS_TOGGLE + IS_GLOBAL,   TP_NO, 0,    0, 1, 1, 0},
/*LSL_STOP*/    {TP_CC, 80, 0, 127, 1, IS_TOGGLE + SEND_ALL,   TP_CC, 80, 0, 127, 1, IS_TOGGLE + SEND_ALL,    TP_CC, 80, 0, 127, 1, IS_TOGGLE}, //leslie OFF
/*LSL_FAST*/    {TP_CC, 81, 0, 127, 1, IS_TOGGLE + SEND_ALL,   TP_CC, 81, 0, 127, 1, IS_TOGGLE + SEND_ALL,    TP_NO, 0,  0, 127, 1, 0},
/*PED_SWITCH*/  {TP_CC, 81, 0, 127, 1, IS_TOGGLE + SEND_ALL,   TP_CC, 81, 0, 127, 1, IS_TOGGLE + SEND_ALL,    TP_NO, 0,  0, 127, 1, 0},
},//                 UPPER                                        LOWER                                    ALTERNATE
{//PIN            Type Prm Min Max Ch Behaviour                 Type Prm Min Max Ch Behaviour                  Type Prm Min Max Ch Behaviour
/*DWB1*/        {TP_CC, 20, 0, 127, 1, 0,                      TP_CC, 29, 0, 127, 1, 0,                       TP_CC, 84, 0, 127, 1, 0}, // REV LEVEL
/*DWB1_13*/     {TP_CC, 19, 0, 127, 1, 0,                      TP_CC, 28, 0, 127, 1, 0,                       TP_CC, 76, 0, 127, 1, 0}, // DRIVE
/*DWB1_35*/     {TP_CC, 18, 0, 127, 1, 0,                      TP_CC, 27, 0, 127, 1, 0,                       TP_CC, 75, 0, 127, 1, 0}, // KEY CLICK
/*DWB2*/        {TP_CC, 17, 0, 127, 1, 0,                      TP_CC, 26, 0, 127, 1, 0,                       TP_NO,  0, 0, 127, 1, 0},
/*DWB2_23*/     {TP_CC, 16, 0, 127, 1, 0,                      TP_CC, 25, 0, 127, 1, 0,                       TP_NO,  0, 0, 127, 1, 0},
/*DWB4*/        {TP_CC, 15, 0, 127, 1, 0,                      TP_CC, 24, 0, 127, 1, 0,                       TP_NO,  0, 0, 127, 1, 0},
/*DWB8*/        {TP_CC, 14, 0, 127, 1, 0,                      TP_CC, 23, 0, 127, 1, 0,                       TP_CC, 73, 0, 127, 1, IS_VIBCHO}, // VIB TYPE
/*DWB5_13*/     {TP_CC, 13, 0, 127, 1, 0,                      TP_CC, 22, 0, 127, 1, 0,                       TP_CC, 35, 0, 127, 1, 0}, // PEDAL 8
/*DWB16*/       {TP_CC, 12, 0, 127, 1, 0,                      TP_CC, 21, 0, 127, 1, 0,                       TP_CC, 33, 0, 127, 1, 0}, // PEDAL 16
/*PED_EXP */    {TP_CC, 11, 0, 127, 1, IS_GLOBAL,              TP_CC, 11, 0, 127, 1, IS_GLOBAL,               TP_NO, 0x00, 0, 8, 1, 0},
/*CHOVIB_ON*/   {TP_CC, 31, 0, 127, 1, IS_TOGGLE,              TP_CC, 30, 0, 127, 1, IS_TOGGLE,               TP_CC, 55, 0, 127, 1, IS_TOGGLE}, // PEDAL TO LOWER
/*PERC_ON*/     {TP_CC, 66, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,  TP_CC, 66, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,   TP_PR, 0,  0,   0, 1, 0}, //unused
/*PERC_SOFT*/   {TP_CC, 70, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,  TP_CC, 70, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,   TP_PR, 0,  0,   0, 1, 0}, //unused
/*PERC_FAST*/   {TP_CC, 71, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,  TP_CC, 71, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,   TP_NO, 0,  0, 127, 1, 0},
/*PERC_3RD*/    {TP_CC, 72, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,  TP_CC, 72, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,   TP_NO, 0,  0, 127, 1, 0},
/*LSL_STOP*/    {TP_CC, 87, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,  TP_CC, 87, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,   TP_CC, 85, 0, 127, 1, IS_TOGGLE}, // LESLIE OFF
/*LSL_FAST*/    {TP_CC, 86, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,  TP_CC, 86, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,   TP_CC, 51, 0, 127, 1, IS_TOGGLE}, // REV OFF
/*PED_SWITCH*/  {TP_CC, 86, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,  TP_CC, 86, 0, 127, 1, IS_TOGGLE + IS_GLOBAL,   TP_NO, 0,  0, 127, 1, 0},
}
};


/* *************************************************************************
 *  Momentary status
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


/* *************************************************************************
 *  Buttons initialization
 */
// Creates an array and fills it with Bounce objects. see https://forum.arduino.cc/index.php?topic=266132.msg2071306#msg2071306
byte btn_state[3][BTN_COUNT] = {};
Bounce * btn = new Bounce[BTN_COUNT] ;
Bounce btn_alt = Bounce() ;

byte btnAlt_pushed;
byte btnAlt_released;

/* *************************************************************************
 *  LEDs initialization
 */
// Controls LEDs attacched to MCP23017
Adafruit_MCP23017 led;

// An array that store the state of the buttons leds (including the Alt btn/led).
byte ledState[3][BTN_LED_COUNT+1] = {};
long led_alt_on_time;
byte led_alt_blink_status;
byte vibcho_control[2] = {};
byte vibcho_lag; // the previous selected vibrato type's led
byte old_preset_led; // the previous selected preset's led

/* *************************************************************************
 *  MIDI initialization
 */

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
const byte SEND_NOTE_OFF = 0; // if send also the note off control when sending note on = 0;

byte btn_default[BTN_LED_COUNT][3] = {
                         //ALT UP LOW
    /*PEDAL TO LOWER */   {1, 0, 1},  /*CHOVIB_ON*/
    /*preset */           {0, 1, 0},  /*PERC_ON*/
    /*preset */           {1, 0, 0},  /*PERC_SOFT*/
                          {0, 0, 0},  /*PERC_FAST*/
                          {0, 0, 0},  /*PERC_3RD*/
    /* leslie off */      {0, 0, 0},  /*LSL_STOP*/
    /* rev off */         {0, 1, 0},  /*LSL_FAST*/
};

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
  for (int a = 0; a < TOTAL_LED_ALT_COUNT; a++) {
    // from 0 to 8 are the pushbutton leds, from 9 to 14 are the chorus/vibrato status leds
    led.pinMode(a, OUTPUT);
  }

// initialize at the startup
  STATUS = ST_UP;
  OLD_STATUS = ST_LOW;
  btnAlt_released = 1;
  //curr_preset = 1;
  //old_preset_led = 3;

  // turn off all the 6 vib/cho status leds
  for (byte ledto = VIBCHO_LED_IDX_START; ledto < TOTAL_LED_ALT_COUNT; ledto++) {
    led.digitalWrite(ledto, 0);
 }
 
  // load the default preset
  for (byte st = 0; st < 3; st++){
      for (byte btn_scanned = 0; btn_scanned < BTN_COUNT; btn_scanned++) {
        byte btn_index = btn_scanned + BTN_IDX_START;        
        if( PRESETS[0][btn_index][STATUS_IDX[st] + TYPE] == TP_PR ){
          if( 0 != btn_default[btn_scanned][st] ){
            changePreset(  btn_scanned, st  ); 
          }          
        }
      }
  }

 
  syncAnalogData();
}


void loop() {
  // Scan ALT Button
  getAltBtn();

  // Scan Drawbars
  getAnalogData();

  // Scan Buttons
  getDigitalData();

  // Set Leds
  setLeds();

  /* MIDI merge and thru */

  // MIDI IN to MIDI OUT should be already handled by the Midi library soft thru.
  // here we handle MIDI to usbMidi and usbMIDI to MIDI

  // code heavily derived form the Interface_3X3 example in Teensyduino
  bool activity = false;

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
    activity = true;
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
    activity = true;
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

void setLedState( byte status, byte btn, byte value ){
  if ( btn <= BTN_LED_COUNT ){ // escludiamo di impostare lo stato per input che non hanno il led (tipo il pedale)
    ledState[status][btn] = value;
  }
}

void SetAltLedState( byte status, byte value ){
  ledState[status][LED_ALT] = value;
  }
  
void changePreset( byte btn_scanned, byte curr_status ){
   byte btn_index = btn_scanned + BTN_IDX_START;
   Serial.println (String("CHANGING preset") + curr_status );
   setLedState(curr_status, old_preset_led, 0);
   setLedState(curr_status, btn_scanned +1,  !btn_state[curr_status][btn_scanned]);

   // set the new preset value
   curr_preset = PRESETS[0][btn_index][STATUS_IDX[curr_status] +MAX];
   
   // reset all data
   resetToDefaultData();
   
   Serial.println (String("New preset is: ") + curr_preset );  
   
   old_preset_led = btn_scanned +1;
}

void resetToDefaultData(){
  Serial.println (String("RESET ALL BUTTONS to 0 - except for the preset buttons")); 
  for (byte st = 0; st < 3; st++){
      for (byte btn_scanned = 0; btn_scanned < BTN_COUNT; btn_scanned++) {
        byte btn_index = btn_scanned + BTN_IDX_START;
        if( PRESETS[curr_preset][btn_index][STATUS_IDX[st] + TYPE] != TP_PR ){
            updateBtn( btn_scanned, 0, st );
        }
      }
  }
  
  Serial.println (String("SET DEFAULT DATA"));
  for (byte st = 0; st < 3; st++){
     Serial.println (String("For STATUS: ") + st + String(" IDX: ") + STATUS_IDX[st]);

      for (byte btn_scanned = 0; btn_scanned < BTN_COUNT; btn_scanned++) {
        byte btn_index = btn_scanned + BTN_IDX_START;
        if( PRESETS[curr_preset][btn_index][STATUS_IDX[st] + TYPE] != TP_PR && PRESETS[curr_preset][btn_index][STATUS_IDX[st] + TYPE] != TP_NO ){
            Serial.println (String("Button: ") + btn_scanned + String(" value set: ") + btn_default[btn_scanned][st]);
            updateBtn( btn_scanned, btn_default[btn_scanned][st], st );
        }
      }
      for (byte analog = 0; analog < DRWB_COUNT; analog++) {
        if ( (PRESETS[curr_preset][analog][STATUS_IDX[st] +BEHAV] & IS_VIBCHO )== IS_VIBCHO ){
            Serial.println (String("SET VIB/CHO to C3 (127)"));
            byte vibcho_led_on =  5;
            setVibchoLeds( vibcho_led_on );
            vibcho_lag = vibcho_led_on;
            sendMidi( PRESETS[curr_preset][analog][STATUS_IDX[st] +TYPE], PRESETS[curr_preset][analog][STATUS_IDX[st] +PARAM], 127, analog, PRESETS[curr_preset][analog][STATUS_IDX[st] +CHAN] );
        }
      }  
  }
}

void getAltBtn(){
  static unsigned long btnAlt_DownTime;
  static unsigned long holdTime = 2000;
  // scansioniamo il pulsante "ALT"
  if (btn_alt.update()){
    if (btn_alt.fell()) {
       btnAlt_pushed = 1;
       btnAlt_DownTime = millis();
       Serial.println (String("ALT BTN PUSHED. STATUS: ") + STATUS);
    }
    else{
      if (btnAlt_pushed == 1){
        if ( STATUS != ST_ALT){
            if ( STATUS == ST_UP && OLD_STATUS != ST_ALT){
              OLD_STATUS = STATUS;
              STATUS = ST_LOW;
              SetAltLedState( STATUS, 1);
            }
            else{
              OLD_STATUS = STATUS;
              STATUS = ST_UP;
              SetAltLedState( STATUS, 0);
            }
            Serial.println (String("STATUS: ") + STATUS);
        }
      }

      btnAlt_released = 1;
      btnAlt_pushed = 0;
      Serial.println (String("RELEASED: ") + btnAlt_released);
    }
  } // ALT BTN not changed
  else{
      // se il suo stato è "premuto" ( cioè 0)... allora verifichiamo da quanto tempo è permuto
      if( btn_alt.read() == 0 ){
          // se è premuto da abbastanza tempo allora passiamo a allo Status ALT
          if ((millis() - btnAlt_DownTime) > holdTime && btnAlt_released == 1 ) {
            Serial.println(String("ALT BTN LONG PRESS!!:") );
            btnAlt_pushed = 0;
            btnAlt_released = 0;
            OLD_STATUS = STATUS;
            if ( STATUS == ST_ALT ){
              STATUS = ST_UP;
              SetAltLedState( STATUS, 0 );
              Serial.println (String("from ALT to STATUS: ") + STATUS);
            }
            else {
              STATUS = ST_ALT;
              led_alt_on_time = millis();
              SetAltLedState( STATUS, 1);
              led_alt_blink_status = ledState[STATUS][LED_ALT];
              Serial.println (String("STATUS: ") + STATUS);
            }
         }
      }
  }
}


void setVibchoLeds( byte ledon ){
    // turn off the old led
    led.digitalWrite(vibcho_lag + VIBCHO_LED_IDX_START, 0);

    // turn on the Led
    Serial.println (String("VIB/CHO led: ") + ledon);
    led.digitalWrite(ledon + VIBCHO_LED_IDX_START, 1);
  }

void setLeds(){

    if ( STATUS == ST_ALT){
       //blink LED_ALT
      if( millis()-led_alt_on_time < 500 ){
        led.digitalWrite(LED_ALT, led_alt_blink_status);
      }
      else{
        led_alt_blink_status = !led_alt_blink_status;
        led_alt_on_time = millis();
      }
    }
    else{
      led.digitalWrite(LED_ALT, ledState[STATUS][LED_ALT]);
    }

    for (byte ledto = 1; ledto <= BTN_LED_COUNT; ledto++) {
      // Serial.println (String("Leds for status: ") + STATUS + String(" led : ") + ledto + ledState[STATUS][ledto] );
      led.digitalWrite(ledto, ledState[STATUS][ledto]);
    }

    if ( STATUS != ST_ALT ){
      //blink FAST LED_LESLIE_SP
    }
    else{
      //blink SLOW LED_LESLIE_SP

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
          Serial.println (String("DWB changed: ") + drwb_scanned + String(" value: ") + analogData[drwb_scanned] );
  
          // check if this drawbar is dedicated to the VIB/CHO control
          if ( (PRESETS[curr_preset][drwb_scanned][STATUS_IDX[STATUS] +BEHAV] & IS_VIBCHO )== IS_VIBCHO ){
             Serial.println (String("DWB controls VIBCHO") );
  
            // calculate which Led turn on based on the Drawbar value
            byte vibcho_led_on =  map(analogData[drwb_scanned], 0, 127, 0, 5);
            if (vibcho_led_on != vibcho_lag){
              setVibchoLeds( vibcho_led_on );
              vibcho_lag = vibcho_led_on;
              sendAnalogMidi( analogData[drwb_scanned], drwb_scanned );
            }
          }
          else{
                sendAnalogMidi( analogData[drwb_scanned], drwb_scanned );
            }
        }
      }
    }
  }
}

void sendAnalogMidi ( byte value, byte control ){
  // sendMidi( PRESETS[curr_preset][drwb_scanned][STATUS_IDX[STATUS] +TYPE], PRESETS[curr_preset][drwb_scanned][STATUS_IDX[STATUS] +PARAM], analogData[drwb_scanned], drwb_scanned, PRESETS[curr_preset][drwb_scanned][STATUS_IDX[STATUS] +CHAN] );

  if ( ( PRESETS[curr_preset][control][STATUS_IDX[STATUS] +BEHAV] & SEND_ALL ) == SEND_ALL  && STATUS != ST_ALT ){
        sendMidi( PRESETS[curr_preset][control][STATUS_IDX[ST_UP]  +TYPE], PRESETS[curr_preset][control][STATUS_IDX[ST_UP]  +PARAM], value, control, PRESETS[curr_preset][control][STATUS_IDX[ST_UP]  +CHAN] );
        sendMidi( PRESETS[curr_preset][control][STATUS_IDX[ST_LOW] +TYPE], PRESETS[curr_preset][control][STATUS_IDX[ST_LOW] +PARAM], value, control, PRESETS[curr_preset][control][STATUS_IDX[ST_LOW] +CHAN] );
  }
  else if ( ( PRESETS[curr_preset][control][STATUS_IDX[STATUS] +BEHAV] & IS_GLOBAL ) == IS_GLOBAL && STATUS != ST_ALT) {
        sendMidi( PRESETS[curr_preset][control][STATUS_IDX[ST_UP] +TYPE], PRESETS[curr_preset][control][STATUS_IDX[ST_UP] +PARAM], value, control, PRESETS[curr_preset][control][STATUS_IDX[ST_UP] +CHAN] );
  }
  else {
    sendMidi( PRESETS[curr_preset][control][STATUS_IDX[STATUS] +TYPE], PRESETS[curr_preset][control][STATUS_IDX[STATUS] +PARAM], value, control, PRESETS[curr_preset][control][STATUS_IDX[STATUS] +CHAN] );
  }
}

void syncAnalogData() {
  Serial.println (String("SYNC DWB") );
  for (int drwb_scanned = 0; drwb_scanned < DRWB_COUNT; drwb_scanned++) {
    // update the ResponsiveAnalogRead object every loop
    drwb[drwb_scanned].update();
    if( PRESETS[curr_preset][drwb_scanned][STATUS_IDX[STATUS] +TYPE] != TP_NO ){
      analogData[drwb_scanned] = drwb[drwb_scanned].getValue() >> 3;
      analogDataLag[drwb_scanned] = analogData[drwb_scanned];
      Serial.println (String("DWB synced: ") + drwb_scanned + String(" value: ") + analogData[drwb_scanned] );
      //sendMidi( PRESETS[curr_preset][drwb_scanned][STATUS_IDX[STATUS] +TYPE], PRESETS[curr_preset][drwb_scanned][STATUS_IDX[STATUS] +PARAM], analogData[drwb_scanned], drwb_scanned, PRESETS[curr_preset][drwb_scanned][STATUS_IDX[STATUS] +CHAN] );
      sendAnalogMidi( analogData[drwb_scanned], drwb_scanned );
    }
  }

  ledCarousel();
}

void updateBtn( byte btn_scanned, byte btn_val, byte curr_status ){
      byte btn_index = btn_scanned + BTN_IDX_START;
            if ( ( PRESETS[0][btn_index][STATUS_IDX[curr_status] +TYPE] ) == TP_PR ){
                // If this button is dedicated to switch the presets...
                changePreset( btn_scanned, curr_status );
            }
            else if ( ( PRESETS[curr_preset][btn_index][STATUS_IDX[curr_status] +BEHAV] & SEND_ALL ) == SEND_ALL  && curr_status != ST_ALT ){
                  sendMidi( PRESETS[curr_preset][btn_index][STATUS_IDX[ST_UP] +TYPE], PRESETS[curr_preset][btn_index][STATUS_IDX[ST_UP] +PARAM], btn_val * 127, btn_index, PRESETS[curr_preset][btn_index][STATUS_IDX[ST_UP] +CHAN] );
                  sendMidi( PRESETS[curr_preset][btn_index][STATUS_IDX[ST_LOW] +TYPE], PRESETS[curr_preset][btn_index][STATUS_IDX[ST_LOW] +PARAM], btn_val * 127, btn_index, PRESETS[curr_preset][btn_index][STATUS_IDX[ST_LOW] +CHAN] );
                  setLedState(ST_UP, btn_scanned +1,  btn_val);
                  setLedState(ST_LOW, btn_scanned +1, btn_val);
                  btn_state[ST_UP][btn_scanned] = btn_val;
                  btn_state[ST_LOW][btn_scanned] = btn_val;
            }
            else if ( ( PRESETS[curr_preset][btn_index][STATUS_IDX[curr_status] +BEHAV] & IS_GLOBAL ) == IS_GLOBAL && curr_status != ST_ALT) {
                  sendMidi( PRESETS[curr_preset][btn_index][STATUS_IDX[ST_UP] +TYPE], PRESETS[curr_preset][btn_index][STATUS_IDX[ST_UP] +PARAM], btn_val * 127, btn_index, PRESETS[curr_preset][btn_index][STATUS_IDX[ST_UP] +CHAN] );
                  setLedState(ST_UP, btn_scanned +1, btn_val);
                  setLedState(ST_LOW, btn_scanned +1, btn_val);
                  btn_state[ST_UP][btn_scanned] = btn_val;
                  btn_state[ST_LOW][btn_scanned] = btn_val;
            }
            else {
              sendMidi( PRESETS[curr_preset][btn_index][STATUS_IDX[curr_status] +TYPE], PRESETS[curr_preset][btn_index][STATUS_IDX[curr_status] +PARAM], btn_val * 127, btn_index, PRESETS[curr_preset][btn_index][STATUS_IDX[curr_status] +CHAN] );
              setLedState(curr_status, btn_scanned +1, btn_val);
            }
            Serial.println(String("new btn_val: ") + btn_val + String(" Status: ") + curr_status);

}

void getDigitalData() {

  // scansioniamo i pulsanti (tranne ALT che va da solo)
  for (byte btn_scanned = 0; btn_scanned < BTN_COUNT; btn_scanned++) {
    byte btn_val = 0;
    byte btn_index = btn_scanned + BTN_IDX_START;
    
    if (PRESETS[curr_preset][btn_index][STATUS_IDX[STATUS] + TYPE] != TP_NO && btn[btn_scanned].update()) {
      btn_val = btn[btn_scanned].read();

      // Pulsnte PREMUTO
      if (btn[btn_scanned].fell()) {

        if( btnAlt_pushed == 0){
          // Caso "normale" il pulsante è premuto da solo
           Serial.println(String("BTN pressed: ") + btn_scanned + String(" value: ") + btn_val );


            if ( (PRESETS[curr_preset][btn_index][STATUS_IDX[STATUS] +BEHAV] & IS_TOGGLE )== IS_TOGGLE){
              if ( btn_state[STATUS][btn_scanned] == 1){
                btn_state[STATUS][btn_scanned] = 0;
              }
              else{
                btn_state[STATUS][btn_scanned] = 1;
              }
              btn_val = btn_state[STATUS][btn_scanned];
            }

            updateBtn( btn_scanned, btn_val, STATUS);
        }
        else{
          // Pulsante premuto in contemporanea all'ALT ...
          btnAlt_pushed = 0;
          Serial.println (String("ALT + BTN: ") + btn_scanned);
          //sync analog data
          if( btn_scanned == 0 ){
            syncAnalogData();
            //resetToDefaultData();
          }
        }

      }
      // Pulsante rilasciato
      else {
          if (PRESETS[curr_preset][btn_index][STATUS_IDX[STATUS] +BEHAV] == 0 && ( PRESETS[curr_preset][btn_index][STATUS_IDX[STATUS] +TYPE] ) != TP_PR){
           Serial.println(String("Btn released - No TOOGLE - new btn_val: ") + !btn_val );
           setLedState(STATUS, btn_scanned +1, !btn_val);
           sendMidi( PRESETS[curr_preset][btn_index][STATUS_IDX[STATUS] +TYPE], PRESETS[curr_preset][btn_index][STATUS_IDX[STATUS] +PARAM], 0, btn_index, PRESETS[curr_preset][btn_index][STATUS_IDX[STATUS] +CHAN] );
          }
      }

    } // fine btn scanned.updated
  }
}


void sendMidi( int type, byte parameter, byte value, byte control, byte channel)
{
  Serial.println(String("Send midi - Type: ") + type + String(" Par: ") + parameter + String(" value: ") + value + String(" control: ") + control );

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
            Serial.println("altro preset");
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

