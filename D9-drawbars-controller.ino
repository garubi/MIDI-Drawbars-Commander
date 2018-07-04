#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Bounce2.h> // https://github.com/thomasfredericks/Bounce2/wiki
#include <MIDI.h>
#include <ResponsiveAnalogRead.h>


 byte STATUS;
 byte OLD_STATUS;
 byte btnAlt_released;

//const byte LED = 13;       // LED pin on board
 
 byte preset;
 byte old_preset_led;
 
 byte midi_channel = 1;
 
 const byte DWB16    = A8;
 const byte DWB5_13  = A7;
 const byte DWB8     = A6;
 const byte DWB4     = A10;
 const byte DWB2_23  = A11;
 const byte DWB2     = A3;
 const byte DWB1_35  = A2;
 const byte DWB1_13  = A1;
 const byte DWB1     = A0;

const byte BTN_ALT = 9;
const byte CHOVIB_ON = 8;
const byte PERC_ON = 7;
const byte PERC_SOFT = 6;
const byte PERC_FAST = 5;
const byte PERC_3RD = 4;
const byte LSL_STOP = 3;
const byte LSL_FAST = 2;

const byte LED_ALT = 0;

const byte BTN_COUNT = 7; // configurable buttons number (less the Alternate button, counted a part)
const byte DRWB_COUNT = 9; // configurable number of drawbars used
const byte BTN_IDX_START = DRWB_COUNT; // at wich row of the presets array start the drawbars rows?
const byte PRESET_CONTROLS_NUM = BTN_COUNT + DRWB_COUNT; 
const byte IS_TOGGLE   = 1;
const byte IS_VIBCHO = 2;
const byte IS_PRESET = 3;
const byte IS_GLOBAL = 1; // if the control sends always the same value both in Upper that in Lower state (sends what's set in the Upper one)
const byte SEND_ALL = 2; // if we have to send both the Lower and the Upper values at the same time both in Upper taht in Lower state

// a data array and a lagged copy to tell when Midi changes are required
byte data[DRWB_COUNT];
byte dataLag[DRWB_COUNT]; // when lag and new are not the same then update Midi CC value

byte vibcho_lag; // 

/*
   The multidimensional Array byte PRESETS will contains in each row:
   1) the pin to which the drawbar/button is attached to
   2) the type of midi message to send out:
      0 = Disabled
      1 = Note Off,
      2 = Note On,
     c 3 = Poliphonic Pressure,
      4 = Control Change,
      5 = Program Change,
      6 = After Touch,
      7 = Pitch Bend,
      8 = System Exclusive)
   3) the command parameter (CC number, or Note number, or SySEx parameter etc...)
   4) the min value to send out
   5) the max value to sed out
   6) does the button have to behave as a toggle one (IS_TOGGLE) or is used for switch presets (IS_PRESET, set the preset number in the MAX location) or is used to change the Vibrato/choorus type (IS_VIBCHO)?
   7) Upper/lower behaviour.
*/

/* Array index position labels */
const byte TYPE = 0;
const byte PARAM = 1;
const byte MIN = 2;
const byte MAX = 3;
const byte CHAN = 4;
const byte TOGGLE = 5;
const byte BEHAV = 6;

/*****************************
 * PRESETS
 * 0: Factory preset for Roland FA 06/07/07
 * 1: Factory preset for GSi Gemini expander 
 */
const byte PRESETS[2][PRESET_CONTROLS_NUM][21]=
{//                 UPPER                                    LOWER                                  ALTERNATE
{//PIN      Type Prm Min Max Ch Toggle    Behaviour  Type Prm Min Max Ch Toggle    Behaviour   Type Prm Min Max Ch Toggle
  {DWB1,      8, 0x2A, 0, 8, 1, 0,         0,          8, 0x2A, 0, 8, 2, 0,         0,           8, 0x00, 0, 8, 1, 0},
  {DWB1_13,   8, 0x29, 0, 8, 1, 0,         0,          8, 0x29, 0, 8, 2, 0,         0,           8, 0x00, 0, 8, 1, 0},
  {DWB1_35,   8, 0x28, 0, 8, 1, 0,         0,          8, 0x28, 0, 8, 2, 0,         0,           8, 0x00, 0, 8, 1, 0},
  {DWB2,      8, 0x27, 0, 8, 1, 0,         0,          8, 0x27, 0, 8, 2, 0,         0,           8, 0x00, 0, 8, 1, 0},
  {DWB2_23,   8, 0x26, 0, 8, 1, 0,         0,          8, 0x26, 0, 8, 2, 0,         0,           8, 0x00, 0, 8, 1, 0},
  {DWB4,      8, 0x25, 0, 8, 1, 0,         0,          8, 0x25, 0, 8, 2, 0,         0,           8, 0x00, 0, 8, 1, 0},
  {DWB8,      8, 0x24, 0, 8, 1, 0,         0,          8, 0x24, 0, 8, 2, 0,         0,           8, 0x00, 0, 8, 1, 0},
  {DWB5_13,   8, 0x23, 0, 8, 1, 0,         0,          8, 0x23, 0, 8, 2, 0,         0,           8, 0x00, 0, 8, 1, 0},
  {DWB16,     8, 0x22, 0, 8, 1, 0,         0,          8, 0x22, 0, 8, 2, 0,         0,           8, 0x00, 0, 8, 1, 0},
  {CHOVIB_ON, 0, 0x00, 0, 0, 0, 0,         0,          0, 0x00, 0, 0, 0, 0,         0,           0, 0x00, 0, 0, 0, 0}, 
  {PERC_ON,   8, 0x2B, 0, 1, 1, IS_TOGGLE, IS_GLOBAL,  8, 0x2B, 0, 1, 1, IS_TOGGLE, IS_GLOBAL,   0, 0,    0, 0, 1, IS_PRESET},
  {PERC_SOFT, 8, 0x36, 0, 1, 1, IS_TOGGLE, IS_GLOBAL,  8, 0x36, 0, 1, 1, IS_TOGGLE, IS_GLOBAL,   0, 0,    0, 1, 1, IS_PRESET},
  {PERC_FAST, 8, 0x2D, 0, 1, 1, IS_TOGGLE, IS_GLOBAL,  8, 0x2D, 0, 1, 1, IS_TOGGLE, IS_GLOBAL,   0, 0,    0, 1, 1, 0},
  {PERC_3RD,  8, 0x2C, 0, 1, 1, IS_TOGGLE, IS_GLOBAL,  8, 0x2C, 0, 1, 1, IS_TOGGLE, IS_GLOBAL,   0, 0,    0, 1, 1, 0},
  {LSL_STOP,  4, 80, 0, 127, 1, IS_TOGGLE, SEND_ALL,   4, 80, 0, 127, 1, IS_TOGGLE, SEND_ALL,    4, 80, 0, 127, 1, IS_TOGGLE}, //leslie OFF
  {LSL_FAST,  4, 81, 0, 127, 1, IS_TOGGLE, SEND_ALL,   4, 81, 0, 127, 1, IS_TOGGLE, SEND_ALL,    0, 0,  0, 127, 1, 0},  
},//                 UPPER                           |             LOWER                       |             ALTERNATE
{//PIN      Type Prm Min Max Ch Toggle    Behaviour  Type Prm Min Max Ch Toggle    Behaviour   Type Prm Min Max Ch Toggle   
  {DWB1,      4, 20, 0, 127, 1, 0,         0,          4, 29, 0, 127, 1, 0,         0,           4, 84, 0, 127, 1, 0}, // REV LEVEL
  {DWB1_13,   4, 19, 0, 127, 1, 0,         0,          4, 28, 0, 127, 1, 0,         0,           4, 76, 0, 127, 1, 0}, // DRIVE
  {DWB1_35,   4, 18, 0, 127, 1, 0,         0,          4, 27, 0, 127, 1, 0,         0,           4, 75, 0, 127, 1, 0}, // KEY CLICK
  {DWB2,      4, 17, 0, 127, 1, 0,         0,          4, 26, 0, 127, 1, 0,         0,           0,  0, 0, 127, 1, 0},
  {DWB2_23,   4, 16, 0, 127, 1, 0,         0,          4, 25, 0, 127, 1, 0,         0,           0,  0, 0, 127, 1, 0},
  {DWB4,      4, 15, 0, 127, 1, 0,         0,          4, 24, 0, 127, 1, 0,         0,           0,  0, 0, 127, 1, 0},
  {DWB8,      4, 14, 0, 127, 1, 0,         0,          4, 23, 0, 127, 1, 0,         0,           4, 73, 0, 127, 1, IS_VIBCHO}, // VIB TYPE
  {DWB5_13,   4, 13, 0, 127, 1, 0,         0,          4, 22, 0, 127, 1, 0,         0,           4, 35, 0, 127, 1, 0}, // PEDAL 8
  {DWB16,     4, 12, 0, 127, 1, 0,         0,          4, 21, 0, 127, 1, 0,         0,           4, 33, 0, 127, 1, 0}, // PEDAL 16
  {CHOVIB_ON, 4, 31, 0, 127, 1, IS_TOGGLE, 0,          4, 30, 0, 127, 1, IS_TOGGLE, 0,           4, 55, 0, 127, 1, IS_TOGGLE}, // PEDAL TO LOWER
  {PERC_ON,   4, 66, 0, 127, 1, IS_TOGGLE, IS_GLOBAL,  4, 66, 0, 127, 1, IS_TOGGLE, IS_GLOBAL,   0, 0,  0,   0, 1, IS_PRESET},
  {PERC_SOFT, 4, 70, 0, 127, 1, IS_TOGGLE, IS_GLOBAL,  4, 70, 0, 127, 1, IS_TOGGLE, IS_GLOBAL,   0, 0,  0,   1, 1, IS_PRESET},
  {PERC_FAST, 4, 71, 0, 127, 1, IS_TOGGLE, IS_GLOBAL,  4, 71, 0, 127, 1, IS_TOGGLE, IS_GLOBAL,   0, 0,  0, 127, 1, 0},
  {PERC_3RD,  4, 72, 0, 127, 1, IS_TOGGLE, IS_GLOBAL,  4, 72, 0, 127, 1, IS_TOGGLE, IS_GLOBAL,   0, 0,  0, 127, 1, 0},
  {LSL_STOP,  4, 87, 0, 127, 1, IS_TOGGLE, IS_GLOBAL,  4, 87, 0, 127, 1, IS_TOGGLE, IS_GLOBAL,   4, 85, 0, 127, 1, IS_TOGGLE}, // LESLIE OFF
  {LSL_FAST,  4, 86, 0,  127, 1,IS_TOGGLE, IS_GLOBAL,  4, 86, 0, 127, 1, IS_TOGGLE, IS_GLOBAL,   4, 51, 0, 127, 1, IS_TOGGLE}, // REV OFF
}
};

 const byte ST_ALT  = 0; // Controller status: ALTERNATE
 const byte ST_UP   = 1; // Controller status: UPPER
 const byte ST_LOW  = 2; // Controller status: LOWER
 const byte STATUS_IDX[] =  // the column number in the PRESETS array where starts parameters for each status
 {
  15, 1, 8
 };

// initialize the ReponsiveAnalogRead objects
ResponsiveAnalogRead drwb[] {
  {PRESETS[preset][0][0], true},
  {PRESETS[preset][1][0], true},
  {PRESETS[preset][2][0], true},
  {PRESETS[preset][3][0], true},
  {PRESETS[preset][4][0], true},
  {PRESETS[preset][5][0], true},
  {PRESETS[preset][6][0], true},
  {PRESETS[preset][7][0], true},
  {PRESETS[preset][8][0], true},
};

// Creates an array and fills it with Bounce objects. see https://forum.arduino.cc/index.php?topic=266132.msg2071306#msg2071306
byte btn_state[3][BTN_COUNT] = {};
Bounce * btn = new Bounce[BTN_COUNT] ; 
Bounce btn_alt = Bounce() ; 

// Controls LEDs attacched to MCP23017 
Adafruit_MCP23017 led;

// An array that store the state of the buttons leds.
byte ledState[3][8] = {};
long led_alt_on_time;
byte led_alt_blink_status;
  
void setup()
{

  //pinMode(LED, OUTPUT);

  
  Serial.begin(38400);
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // set ALT button as input Pullup and attach debouncer
  pinMode(BTN_ALT, INPUT_PULLUP);
  btn_alt.attach(BTN_ALT);
    
  // set all "standard" buttons as input Pullup and attach debouncer
  for (int b = 0; b < BTN_COUNT; b++) {
    pinMode(PRESETS[preset][b + BTN_IDX_START][0], INPUT_PULLUP);
    btn[b].attach(PRESETS[preset][b + BTN_IDX_START][0]);
  }

  led.begin();      // use default address 0
  for (int a = 0; a < 15; a++) {
    // from 0 to 8 are the pushbutton leds, from 9 to 14 are the chorus/vibrato status leds
    led.pinMode(a, OUTPUT);
  }

  STATUS = ST_UP;
  OLD_STATUS = ST_LOW;
  btnAlt_released = 1;
  preset = 1;
  old_preset_led = 3;
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
  
  // read and discard any incoming Midi messages
  while (usbMIDI.read()); 
}

void getAltBtn(){
  static unsigned long btnAlt_DownTime;
  static unsigned long holdTime = 2000;
  // scansioniamo il pulsante "ALT"
  if (btn_alt.update()){
    if (btn_alt.fell()) {
       btnAlt_DownTime = millis();
       Serial.println (String("ALT BTN PUSHED. STATUS: ") + STATUS);
    }
    else{
      if ( STATUS != ST_ALT){
          if ( STATUS == ST_UP && OLD_STATUS != ST_ALT){
            OLD_STATUS = STATUS;
            STATUS = ST_LOW;
            //led.digitalWrite(LED_ALT, 1);
            ledState[STATUS][LED_ALT] = 1;
            Serial.println (String("STATUS: ") + STATUS);
          }
          else{
            OLD_STATUS = STATUS;
            STATUS = ST_UP;
            //led.digitalWrite(LED_ALT, 0);
            ledState[STATUS][LED_ALT] = 0;
            Serial.println (String("STATUS: ") + STATUS);
          }          
      }
        btnAlt_released = 1;
        Serial.println (String("RELEASED: ") + btnAlt_released);
      
    }
  } // ALT BTN not changed
  else{
      // se il suo stato è "premuto" ( cioè 0)... allora verifichiamo da quanto tempo è permuto
      if( btn_alt.read() == 0 ){
          // se è premuto da abbastanza tempo allora passiamo a allo Status ALT
          if ((millis() - btnAlt_DownTime) > holdTime && btnAlt_released == 1 ) {

            Serial.println(String("ALT BTN LONG PRESS!!:") );
            OLD_STATUS = STATUS;
            if ( STATUS == ST_ALT ){
              btnAlt_released = 0;
              STATUS = ST_UP;
              //digitalWrite(LED, 0);
              ledState[STATUS][LED_ALT] = 0;
              //led.digitalWrite(LED_ALT, 0);
              Serial.println (String("from ALT to STATUS: ") + STATUS);
            }
            else {
              btnAlt_released = 0;
              STATUS = ST_ALT;
              led_alt_on_time = millis();
              //digitalWrite(LED, 1);
              ledState[STATUS][LED_ALT] = 1;
              led_alt_blink_status = ledState[STATUS][LED_ALT];
              //led.digitalWrite(LED_ALT, 0);
              Serial.println (String("STATUS: ") + STATUS);
            }
         }
      }    
  }  
}


void setVibchoLeds( byte ledon ){
  /*
    // tur off all the 6 vib/cho status leds
    for (byte ledto = 8; ledto < 14; ledto++) {
      led.digitalWrite(ledto, 0);
    }
    */
    // turn off the old led     
    led.digitalWrite(vibcho_lag + 8, 0);
    
    // turn on the Led
    Serial.println (String("VIB/CHO led: ") + ledon);
    led.digitalWrite(ledon + 8, 1);
  }
  
void setLeds(){
  
    if ( STATUS == ST_ALT){
      //digitalWrite(LED, 1);

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
     // digitalWrite(LED, 0);
      led.digitalWrite(LED_ALT, ledState[STATUS][LED_ALT]);
    }
    
    for (byte ledto = 1; ledto < 8; ledto++) {
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
    
    // if the repsonsive value has changed, go
    if (drwb[drwb_scanned].hasChanged()) {
      data[drwb_scanned] = drwb[drwb_scanned].getValue() >> 3;
      if (data[drwb_scanned] != dataLag[drwb_scanned]) {
        dataLag[drwb_scanned] = data[drwb_scanned];
        Serial.println (String("DWB changed: ") + drwb_scanned + String(" value: ") + data[drwb_scanned] );

        // check if this drawbar is dedicated to the VIB/CHO control
        if (PRESETS[preset][drwb_scanned][STATUS_IDX[STATUS] + TOGGLE] == 2){
           Serial.println (String("DWB controls VIBCHO") );
          
          // calculate which Led turn on based on the Drawbar value
          byte vibcho_led_on =  map(data[drwb_scanned], 0, 127, 0, 5);
          if (vibcho_led_on != vibcho_lag){
            setVibchoLeds( vibcho_led_on );
            vibcho_lag = vibcho_led_on;
            sendMidi( PRESETS[preset][drwb_scanned][STATUS_IDX[STATUS] +TYPE], PRESETS[preset][drwb_scanned][STATUS_IDX[STATUS] +PARAM], data[drwb_scanned], drwb_scanned, PRESETS[preset][drwb_scanned][STATUS_IDX[STATUS] +CHAN] );    
          }
        }
        else{
          sendMidi( PRESETS[preset][drwb_scanned][STATUS_IDX[STATUS] +TYPE], PRESETS[preset][drwb_scanned][STATUS_IDX[STATUS] +PARAM], data[drwb_scanned], drwb_scanned, PRESETS[preset][drwb_scanned][STATUS_IDX[STATUS] +CHAN] );    
          }
      }
    }
  }
}
void getDigitalData() {

  // scansioniamo i pulsanti "normali"
  for (int btn_scanned = 0; btn_scanned < BTN_COUNT; btn_scanned++) {
    int btn_index = btn_scanned + BTN_IDX_START;
    byte btn_val = 0;

    if (btn[btn_scanned].update()) {
      
      btn_val = btn[btn_scanned].read();


      // Pulsnte PREMUTO
      if (btn[btn_scanned].fell()) {

       Serial.println(String("BTN pressed: ") + btn_scanned + String(" value: ") + btn_val );
        
        if (PRESETS[preset][btn_index][STATUS_IDX[STATUS] +TOGGLE] == IS_TOGGLE){
          if ( btn_state[STATUS][btn_scanned] == 1){
            btn_state[STATUS][btn_scanned] = 0;
          }
          else{
            btn_state[STATUS][btn_scanned] = 1;
          }
          btn_val = btn_state[STATUS][btn_scanned];
        }

        if ( IS_PRESET == PRESETS[preset][btn_index][STATUS_IDX[STATUS] +TOGGLE] ){
            // If this button is dedicated to switch the presets...
               Serial.println (String("CHANGING preset") + STATUS );
               btn_val = !btn_state[STATUS][btn_scanned];
               ledState[STATUS][old_preset_led] = 0;
               ledState[STATUS][btn_scanned +1] = btn_val;  
               old_preset_led = btn_scanned +1;
               preset = PRESETS[preset][btn_index][STATUS_IDX[STATUS] +MAX];  
               Serial.println (String("New preset is: ") + preset );
        }
        else if ( SEND_ALL == PRESETS[preset][btn_index][STATUS_IDX[STATUS] +BEHAV] && STATUS != ST_ALT ){
              sendMidi( PRESETS[preset][btn_index][STATUS_IDX[ST_UP] +TYPE], PRESETS[preset][btn_index][STATUS_IDX[ST_UP] +PARAM], btn_val * 127, btn_index, PRESETS[preset][btn_index][STATUS_IDX[ST_UP] +CHAN] );
              sendMidi( PRESETS[preset][btn_index][STATUS_IDX[ST_LOW] +TYPE], PRESETS[preset][btn_index][STATUS_IDX[ST_LOW] +PARAM], btn_val * 127, btn_index, PRESETS[preset][btn_index][STATUS_IDX[ST_LOW] +CHAN] );
              ledState[ST_UP][btn_scanned +1] = btn_val;
              ledState[ST_LOW][btn_scanned +1] = btn_val;
              btn_state[ST_UP][btn_scanned] = btn_val;
              btn_state[ST_LOW][btn_scanned] = btn_val;
        }
        else if (IS_GLOBAL == PRESETS[preset][btn_index][STATUS_IDX[STATUS] +BEHAV] && STATUS != ST_ALT) {
              sendMidi( PRESETS[preset][btn_index][STATUS_IDX[ST_UP] +TYPE], PRESETS[preset][btn_index][STATUS_IDX[ST_UP] +PARAM], btn_val * 127, btn_index, PRESETS[preset][btn_index][STATUS_IDX[ST_UP] +CHAN] );         
              ledState[ST_UP][btn_scanned +1] = btn_val;
              ledState[ST_LOW][btn_scanned +1] = btn_val;     
              btn_state[ST_UP][btn_scanned] = btn_val;
              btn_state[ST_LOW][btn_scanned] = btn_val;               
        }
        else {
          sendMidi( PRESETS[preset][btn_index][STATUS_IDX[STATUS] +TYPE], PRESETS[preset][btn_index][STATUS_IDX[STATUS] +PARAM], btn_val * 127, btn_index, PRESETS[preset][btn_index][STATUS_IDX[STATUS] +CHAN] );
          ledState[STATUS][btn_scanned +1] = btn_val;        
        }
        Serial.println(String("new btn_val: ") + btn_val );    
      }
      // Pulsante rilasciato
      else {
          if (PRESETS[preset][btn_index][STATUS_IDX[1] +TOGGLE] == 0){
           Serial.println(String("Btn released - No TOOGLE - new btn_val: ") + !btn_val );
           ledState[STATUS][btn_scanned +1] = !btn_val;
           //led.digitalWrite(btn_scanned +1, !btn_val);
           sendMidi( PRESETS[preset][btn_index][STATUS_IDX[STATUS] +TYPE], PRESETS[preset][btn_index][STATUS_IDX[STATUS] +PARAM], 0, btn_index, PRESETS[preset][btn_index][STATUS_IDX[STATUS] +CHAN] );
          }
          
      }
    } // fine btn scanned.updated
  }
}


void sendMidi( int type, byte parameter, byte value, byte control, byte channel)
{
  Serial.println(String("Send midi - Type: ") + type);
 
  int SysexLenght = 0;
    switch (type) {
      case 1: // NoteOff
        usbMIDI.sendNoteOff(parameter, value, midi_channel);
        MIDI.sendNoteOff(parameter, value, midi_channel);
        break;
      case 2: // Note On
        usbMIDI.sendNoteOn(parameter, value, midi_channel);
        MIDI.sendNoteOn(parameter, value, midi_channel);
        break;
      case 3: // Poliphonic Pressure
        MIDI.sendPolyPressure(parameter, value, midi_channel);
        usbMIDI.sendPolyPressure(parameter, value, midi_channel);
        break;
      case 4: // Control Change
        MIDI.sendControlChange(parameter, value, midi_channel);
        usbMIDI.sendControlChange(parameter, value, midi_channel);
        break;
      case 5: // Program Change
        MIDI.sendProgramChange(value, midi_channel);
        usbMIDI.sendProgramChange(value, midi_channel);
        break;
      case 6: // AfterTouch
        MIDI.sendAfterTouch(value, midi_channel);
        usbMIDI.sendAfterTouch(value, midi_channel);
        break;
      case 7: // Pitch Bend
        MIDI.sendPitchBend(value, midi_channel);
        usbMIDI.sendPitchBend(value, midi_channel);
        break;
      case 8: // SysEx
        if (preset == 0) {
          /**
           * è il preset per Roland FA 06/07/08
           */
              //https://forum.arduino.cc/index.php?topic=228570.0
              //https://bitbucket.org/loveaurell/roland-alpha-juno-mks-50-midi-controller/src/62d01604594b5afc56ecee6e9dadc6dc4fb12242/src/mks50_controller/mks50_controller.ino?at=default&fileviewer=file-view-default    
              SysexLenght = 14;
              uint8_t data[SysexLenght] = {0xF0, 0x41, 0x10, 0x00, 0x00, 0x77, 0x12, 0x19, 0x02, 0x00, 0x00, 0x00, 0x00, 0xF7 };
              byte partsA[16] = {0x19, 0x19, 0x19, 0x19, 0x1A, 0x1A, 0x1A, 0x1A, 0x1B, 0x1B, 0x1B, 0x1B, 0x1C, 0x1C, 0x1C, 0x1C };
              byte partsB[16] =  {0x02, 0x22, 0x42, 0x62, 0x02, 0x22, 0x42, 0x62, 0x02, 0x22, 0x42, 0x62, 0x02, 0x22, 0x42, 0x62 };
              data[7] = partsA[midi_channel - 1];
              data[8] = partsB[midi_channel - 1];
              data[10] = parameter;
              data[11] = value;
              if ( PRESETS[preset][control][4] != 127 || PRESETS[preset][control][3] != 0 ) {
                data[11] = map(value, 0, 127, PRESETS[preset][control][3], PRESETS[preset][control][4]);
              }

              /*
               * Roland CheckSum
               * */
              int address = data[7] + data[8] + data[9] + data[10] + data[11] ;
              int remainder = address % 128;
              data[12] = 128 - remainder;
              /*
              Serial.print("SysEX data:");
              for(int i = 0; i < SysexLenght; i++)
              {
                Serial.println(data[i], HEX);
              }
              */
              MIDI.sendSysEx(SysexLenght, data);
              usbMIDI.sendSysEx(SysexLenght, data);
          }
          else{
            Serial.println("altro preset");
            }

        break;
    }
}
