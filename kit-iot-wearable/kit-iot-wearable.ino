/****************************************************************
 ***********************KIT WEARABLE - CP ***********************
 ****************************************************************
 * Author:  Henrique Carvalho Silva *****************************
 * Email:   henrique@devtecnologia.com **************************
 * Company: DEV TEcnologia **************************************
 ****************************************************************
 ****************************************************************/

/****************************************************************
 * Libraries                                                    *
 ****************************************************************/
#include <SM.h>
#include <Wire.h>
#include <MMA7660.h>
#include <avr/pgmspace.h>
#include <EEPROMex.h>
#include <SPI.h>


/****************************************************************
 * Functionality enable flags                                   *
 ****************************************************************/
#define USE_GENERAL_DEFAULT_BLUETOOTH_PIN

#define DEBUG
#define DEBUG_DEVICE_SM
#define DEBUG_PROTOCOL_SM
#define ENABLE_BLUETOOTH_PROTOCOL


/****************************************************************
 * Constants and pins                                           *
 ****************************************************************/
const char BLUETOOTH_DEVICE_NAME[5] = "wV3_";
const long BLUETOOTH_DEFAULT_EDR_PIN = 1234;
const long BLUETOOTH_DEFAULT_BLE_PIN = 123456;

#define NUMBER_OF_DEVICES 11

/* Pins */
#define RED_RGB_LED_PIN                    5
#define GREEN_RGB_LED_PIN                 13
#define BLUE_RGB_LED_PIN                   6
#define ACCELEROMETER_INTERRUPT_PIN        7
#define LIGHT_SENSOR_PIN                 A11
#define BUZZER_PIN                        11
#define TEMPERATURE_SENSOR_PIN            A8
#define BUTTON1_PIN                        4
#define BUTTON2_PIN                       A1
#define BLUETOOTH_RESET_PIN               A0

/* EEPROM Addresses */
#define BLUETOOTH_EDR_PIN_EEPROM_ADDRESS   0
#define BLUETOOTH_BLE_PIN_EEPROM_ADDRESS   4
#define EEPROM_MAGIC_NUMBER_ADDRESS        8
#define EEPROM_MAGIC_NUMBER               93

const unsigned long PROTOCOL_TIMEOUT = 8000;

enum Device {
  ACCELEROMETER,
  RED_RGB_LED,
  GREEN_RGB_LED,
  BLUE_RGB_LED,
  LIGHT_SENSOR,
  TEMPERATURE_SENSOR,
  BUZZER,
  PLAY_MELODY,
  BLUETOOTH_EDR_PIN,
  BLUETOOTH_BLE_PIN
};

/* Device ID code for the bluetooth protocol */
#define PROTOCOL_REQUEST_LENGTH     8
#define PROTOCOL_DEVICE_CODE_LENGTH 2
#define PROTOCOL_RESPONSE_LENGTH    7

const char DEVICE_CODE[NUMBER_OF_DEVICES][PROTOCOL_DEVICE_CODE_LENGTH + 1] = {
  "AC", /* Accelerometer */
  "LR", /* Red RGB Led */
  "LG", /* Green RGB Led */
  "LB", /* Blue RGB Led */
  "LL", /* Off RGB Led */
  "LI", /* Light sensor */
  "TE", /* Temperature Sensor */
  "BZ", /* Buzzer */
  "PM", /* Play Mario Melody */
  "PE", /* Set Bluetooth EDR PIN */
  "PB"  /* Set Bluetooth BLE PIN */
};

Pstate DEVICE_STATE[NUMBER_OF_DEVICES + 3] = {
  accelerometerState,
  redRgbLedState,
  greenRgbLedState,
  blueRgbLedState,
  rgbLedOffState,
  lightSensorState,
  temperatureSensorState,
  buzzerState,
  playMelodyState,
  setPinState,
  waitForCommandState,  /* Must always come after device states */
  sendValueState,
  processButtonState,
};

const char BUTTON1_CODE[] = "B1"; /* Button 1 */
const char BUTTON2_CODE[] = "B2"; /* Button 2 */

/* Accelerometer axis */
#define X_AXIS   0
#define Y_AXIS   1
#define Z_AXIS   2
#define ALL_AXIS 3

/* RGB LED boundary values */
#define MIN_RGB_VALUE   0
#define MAX_RGB_VALUE 255

#define MELODY_MAX_SIZE          100

/* Melody codes */
#define MARIO_THEME_SONG         6789
#define CHRISTMAS_SONG_CODE      1234
#define IMPERIAL_MARCH_SONG_CODE 4567


/****************************************************************
 * Melodies                                                     *
 ****************************************************************/
PROGMEM const int MARIO_THEME_SONG_MELODY[] = {
  2637, 2637, 0, 2637, 0, 2093, 2637, 0,
  3136, 0, 0,  0, 1568, 0, 0, 0,
  2093, 0, 0, 1568, 0, 0, 1319, 0,
  0, 1760, 0, 1976, 0, 1865, 1760, 0,
  1568, 2637, 3136, 3520, 0, 2794, 3136,
  0, 2637, 0, 2093, 2349, 1976, 0, 0,
  2093, 0, 0, 1568, 0, 0, 1319, 0,
  0, 1760, 0, 1976, 0, 1865, 1760, 0,
  1568, 2637, 3136, 3520, 0, 2794, 3136,
  0, 2637, 0, 2093, 2349, 1976, 0, 0
};
int MARIO_THEME_SONG_SIZE = 78;
PROGMEM const int MARIO_THEME_SONG_TEMPO[] = {
  12, 12, 12, 12, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12,
  9, 9, 9, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12,
  9, 9, 9, 12, 12, 12, 12,
  12, 12, 12, 12, 12, 12, 12, 12
};


PROGMEM const int CHRISTMAS_THEME_SONG_MELODY[] = {
  147, 247, 220, 196,
  147, 147, 147, 147,
  247, 220, 196, 165,
  
  0, 165, 262, 247,
  220, 185, 0, 294,
  294, 262, 220, 247
};
int CHRISTMAS_THEME_SONG_SIZE = 24;
PROGMEM const int CHRISTMAS_THEME_SONG_TEMPO[] = {
  4, 4, 4, 4, 3, 8, 8, 4,
  4, 4, 4, 3, 2, 4, 4, 4,
  4, 3, 2, 4, 4, 4, 4, 1
};


PROGMEM const int IMPERIAL_MARCH_SONG_MELODY[] = {
  440, 440, 440, 349, 523, 440, 349, 523,
  440, 659, 659, 659, 698, 523, 440, 349,
  523, 440
};
int IMPERIAL_MARCH_SONG_SIZE = 18;
PROGMEM const int IMPERIAL_MARCH_SONG_TEMPO[] = { 
  4, 4, 4, 5, 16, 4, 5, 16,
  2, 4, 4, 4, 5, 16, 4, 5,
  16, 2
};


/****************************************************************
 * Function Prototypes                                          *
 ****************************************************************/
void playMelody(PROGMEM const int melody[], PROGMEM const int tempo[], int melodySize);
void getBluetoothLapAddress(char* lapAddress, bool ble);
void setBluetoothPin(long pinNumber, bool ble);


/****************************************************************
 * Global variables                                             *
 ****************************************************************/
SM deviceStateMachine(waitForCommandState);

char actionedDeviceCode[PROTOCOL_DEVICE_CODE_LENGTH + 1] = "";
long actionedDeviceArgument = 0;

int button1 = LOW;
int button2 = LOW;

#ifdef ENABLE_BLUETOOTH_PROTOCOL
  SM protocolStateMachine(sleepState);

  bool protocolCommandIsReady = false;
  bool readyToReceiveProtocolCommand = true;

  char protocolCommandString[2 * PROTOCOL_REQUEST_LENGTH];
  char* protocolCommand;
  char protocolResponseValue[PROTOCOL_RESPONSE_LENGTH + 1];

  bool sendMultipleAccelerometerValues = false;

  unsigned long protocolWatchdog = 0;
#endif


/****************************************************************
 * Arduino main functions                                       *
 ****************************************************************/
void setup() {
  #ifdef DEBUG
    delay(5000);
    Serial.begin(9600);
    Serial.println(F("Kit Wearable - V3 - Campus Party"));
  #endif

  /* Setup HC-13 bluetooth module */
  /* Note that HM-13 AT commands do not have \r\n terminating characters */
  Serial1.begin(115200);  /* HM-13 default */
  char atCommand[24];
  char pin[7];
  char bluetoothMacAddressLap[16];
  Serial1.print("AT"); /* Get control over the bluetooth module */
  delay(400);
  
  /* Retrieve MAC ADDRESS for unique ID and set name and PIN for EDR mode */
  getBluetoothLapAddress(bluetoothMacAddressLap, false);
  sprintf(atCommand, "AT+NAME%s%s", BLUETOOTH_DEVICE_NAME, bluetoothMacAddressLap);
  Serial1.print(atCommand);
  delay(400);
  
  if (EEPROM.read(EEPROM_MAGIC_NUMBER_ADDRESS) != EEPROM_MAGIC_NUMBER) {
    for (int i = 0; i < 5; i++) {
      bluetoothMacAddressLap[i] = bluetoothMacAddressLap[i + 4]; /* Shift MAC address */
    }
    
    setBluetoothPin(strtol(bluetoothMacAddressLap, NULL, 16), false);
    
    #ifdef USE_GENERAL_DEFAULT_BLUETOOTH_PIN
      setBluetoothPin(BLUETOOTH_DEFAULT_EDR_PIN, false);
    #endif
    
  } else {
    setBluetoothPin(EEPROM.readLong(BLUETOOTH_EDR_PIN_EEPROM_ADDRESS), false);
  }
  
  /* Retrieve MAC ADDRESS for unique ID and set name and PIN for BLE mode */
  getBluetoothLapAddress(bluetoothMacAddressLap, true);
  sprintf(atCommand, "AT+NAMB%s%s", BLUETOOTH_DEVICE_NAME, bluetoothMacAddressLap);
  Serial1.print(atCommand);
  delay(400);
  
  if (EEPROM.read(EEPROM_MAGIC_NUMBER_ADDRESS) != EEPROM_MAGIC_NUMBER) {
    for (int i = 0; i < 7; i++) {
      bluetoothMacAddressLap[i] = bluetoothMacAddressLap[i + 2]; /* Shift MAC address */
    }
    
    setBluetoothPin(strtol(bluetoothMacAddressLap, NULL, 16), true);
    
    #ifdef USE_GENERAL_DEFAULT_BLUETOOTH_PIN
      setBluetoothPin(BLUETOOTH_DEFAULT_BLE_PIN, true);
    #endif
    
  } else {
    setBluetoothPin(EEPROM.readLong(BLUETOOTH_BLE_PIN_EEPROM_ADDRESS), true);
  }
  
  /* Allow only one connection at a time */
  Serial1.print(F("AT+DUAL1"));
  delay(400);
  
  /* Require authentication when connecting */
  Serial1.print(F("AT+AUTH1"));
  delay(400);
  
    /* Enable connect notifications */
  Serial1.print(F("AT+NOTI1"));
  delay(400);
  
  /* Enable address notifications */
  Serial1.print(F("AT+NOTP1"));
  delay(400);
  
  /* Get firmware version */
  Serial1.print(F("AT+VERS?"));
  delay(400);
  
  /* Reset device so that the changes will have effect */
  Serial1.print(F("AT+RESET"));
  delay(400);

  /* Setup accelerometer */
  MMA7660.init();

  pinMode(RED_RGB_LED_PIN, OUTPUT);
  pinMode(GREEN_RGB_LED_PIN, OUTPUT);
  pinMode(BLUE_RGB_LED_PIN, OUTPUT);
  
  /* RGB is initialized as off */
  turnOffLeds();

  pinMode(LIGHT_SENSOR_PIN, INPUT);
  pinMode(TEMPERATURE_SENSOR_PIN, INPUT);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  
  pinMode(BLUETOOTH_RESET_PIN, OUTPUT);
  digitalWrite(BLUETOOTH_RESET_PIN, HIGH);
}

void loop() {
  EXEC(deviceStateMachine);
  
  #ifdef ENABLE_BLUETOOTH_PROTOCOL
    EXEC(protocolStateMachine);
  #endif
}


/****************************************************************
 * Device state machine functions                               *
 ****************************************************************/
State waitForCommandState() {
  deviceStateMachine.Set(processButtonState);

  #ifdef ENABLE_BLUETOOTH_PROTOCOL
  if (protocolCommandIsReady) {
    /* Parse device code */
    char deviceCode[PROTOCOL_DEVICE_CODE_LENGTH + 1];
    
    for (int i = 0; i < PROTOCOL_DEVICE_CODE_LENGTH; i++) {
      deviceCode[i] = protocolCommand[i];
    }
    
    deviceCode[PROTOCOL_DEVICE_CODE_LENGTH] = '\0';

    /* Only copy the received value to actionedDeviceCode if it is valid */
    for (int i = 0; i < NUMBER_OF_DEVICES; i++) {
      if (strcmp(deviceCode, DEVICE_CODE[i]) == 0) {
        for (int j = 0; j < PROTOCOL_DEVICE_CODE_LENGTH + 1; j++) {
          actionedDeviceCode[j] = deviceCode[j];
        }
      }
    }

    #ifdef DEBUG
      Serial.print(F("Debug: Parsed Command Device Code \""));
      Serial.print(actionedDeviceCode);
      Serial.println("\"");
    #endif

    /* Parse command argument */
    protocolCommand += PROTOCOL_DEVICE_CODE_LENGTH;

    /* Search for null argument or invalid characters */
    bool validArgument = true;
    for (int i = 0; i < strlen(protocolCommand); i++) {
      if (protocolCommand[i] < '0' || protocolCommand[i] > '9') {
        validArgument = false;
      }
    }
    
    if (strlen(protocolCommand) == 0) {
      validArgument = false;
    }

    if (validArgument) {
      actionedDeviceArgument = atol(protocolCommand);
      
    } else {
      actionedDeviceCode[0] = '\0';
    }
    
    readyToReceiveProtocolCommand = true;

    #ifdef DEBUG
      Serial.print(F("Debug: Parsed Command Argument \""));
      Serial.print(actionedDeviceArgument);
      Serial.println("\"");
    #endif
  }
  #endif

  /* Based on the device code, define next state */
  for (int i = 0; i < NUMBER_OF_DEVICES; i++) {
    if (strcmp(actionedDeviceCode, DEVICE_CODE[i]) == 0) {
      deviceStateMachine.Set(DEVICE_STATE[i]);
    }
  }

  return;
}

/*
  State Send Value
*/
State sendValueState() {
  #ifdef DEBUG_DEVICE_SM
    Serial.println(F("Debug Device State Machine: Send Value State"));
  #endif
  
  #ifdef DEBUG
    Serial.print(F("Debug: Sending value "));
    Serial.print(protocolResponseValue);
  #endif

  #ifdef ENABLE_BLUETOOTH_PROTOCOL
    Serial1.println(protocolResponseValue);
  #endif

  deviceStateMachine.Set(processButtonState);

  /* Deal with sending three accelerometer readings in a row */
  if (sendMultipleAccelerometerValues) {
    actionedDeviceArgument++;
    deviceStateMachine.Set(accelerometerState);
  }
}


/*
  State Process Buttons
*/
State processButtonState() {
  if (button1 != digitalRead(BUTTON1_PIN)) {
    button1 = digitalRead(BUTTON1_PIN);
    deviceStateMachine.Set(sendValueState);
    
    #ifdef ENABLE_BLUETOOTH_PROTOCOL
      sprintf(protocolResponseValue, "#%s%2d\r\n", BUTTON1_CODE, !button1);
    #endif
    
  } else if (button2 != digitalRead(BUTTON2_PIN)) {
    button2 = digitalRead(BUTTON2_PIN);
    deviceStateMachine.Set(sendValueState);
    
    #ifdef ENABLE_BLUETOOTH_PROTOCOL
      sprintf(protocolResponseValue, "#%s%2d\r\n", BUTTON2_CODE, !button2);
    #endif
    
  } else {
    actionedDeviceCode[0] = '\0'; /* Reset to prevent endless looping of a device state */
    deviceStateMachine.Set(waitForCommandState);
  }
}


/*
  State Accelerometer
*/
State accelerometerState() {
  #ifdef DEBUG_DEVICE_SM
    Serial.println(F("Debug Device State Machine: Accelerometer State"));
  #endif

  int x, y, z;
  int accelerometerReading;

  delay(100); /* Time for new values to come */

  MMA7660.getValues(&x, &y, &z);

  switch (actionedDeviceArgument) {
    case X_AXIS:
      accelerometerReading = x;
      break;

    case Y_AXIS:
      accelerometerReading = y;
      break;

    case Z_AXIS:
      accelerometerReading = z;
      break;

    case ALL_AXIS:
      /* Creates a loop in order to send three values in a row */
      accelerometerReading = x;
      actionedDeviceArgument = 0;
      sendMultipleAccelerometerValues = true;
      break;

    default:
      deviceStateMachine.Set(waitForCommandState);
      return;
  }

  #ifdef ENABLE_BLUETOOTH_PROTOCOL
    sprintf(protocolResponseValue, "#%s%4d\r\n", DEVICE_CODE[0], accelerometerReading);
    
    if (sendMultipleAccelerometerValues) {
      protocolResponseValue[2] = (char) ('X' + actionedDeviceArgument);
    }
    
    if (sendMultipleAccelerometerValues && (actionedDeviceArgument > 1)) {
      sendMultipleAccelerometerValues = false;
    }
  #endif

  deviceStateMachine.Set(sendValueState);
}


/*
  State Red LED
*/
State redRgbLedState() {
  #ifdef DEBUG_DEVICE_SM
    Serial.println(F("Debug Device State Machine: Red RGB LED State"));
  #endif

  if ((actionedDeviceArgument >= MIN_RGB_VALUE) && (actionedDeviceArgument <= MAX_RGB_VALUE)) {
    actionedDeviceArgument = MAX_RGB_VALUE - actionedDeviceArgument;
    analogWrite(RED_RGB_LED_PIN, actionedDeviceArgument);
  }

  deviceStateMachine.Set(processButtonState);
}


/*
  State Green Led
*/
State greenRgbLedState() {
  #ifdef DEBUG_DEVICE_SM
    Serial.println(F("Debug Device State Machine: Green RGB LED State"));
  #endif


  if ((actionedDeviceArgument >= MIN_RGB_VALUE) && (actionedDeviceArgument <= MAX_RGB_VALUE)) {
    actionedDeviceArgument = MAX_RGB_VALUE - actionedDeviceArgument;
    analogWrite(GREEN_RGB_LED_PIN, actionedDeviceArgument);
  }

  deviceStateMachine.Set(processButtonState);
}


/*
  State Blue Led
*/
State blueRgbLedState() {
  #ifdef DEBUG_DEVICE_SM
    Serial.println(F("Debug Device State Machine: Blue RGB LED State"));
  #endif

  if ((actionedDeviceArgument >= MIN_RGB_VALUE) && (actionedDeviceArgument <= MAX_RGB_VALUE)) {
    actionedDeviceArgument = MAX_RGB_VALUE - actionedDeviceArgument;
    analogWrite(BLUE_RGB_LED_PIN, actionedDeviceArgument);
  }

  deviceStateMachine.Set(processButtonState);
}


/*
  State Turn OFF Leds
*/
State rgbLedOffState() {
  #ifdef DEBUG_DEVICE_SM
    Serial.println(F("Debug Device State Machine: RGB LED OFF State"));
  #endif

  turnOffLeds();

  deviceStateMachine.Set(processButtonState);
}


/*
  State Light sensor
*/
State lightSensorState() {
  #ifdef DEBUG_DEVICE_SM
    Serial.println(F("Debug Device State Machine: Light Sensor State"));
  #endif

  int lightSensorValue = analogRead(LIGHT_SENSOR_PIN);
  deviceStateMachine.Set(sendValueState);
  
  #ifdef ENABLE_BLUETOOTH_PROTOCOL
    sprintf(protocolResponseValue, "#%s%4d\r\n", DEVICE_CODE[5], lightSensorValue);
  #endif
}


/*
  State Temperature sensor
*/
State temperatureSensorState() {
  #ifdef DEBUG_DEVICE_SM
    Serial.println(F("Debug Device State Machine: Temperature Sensor State"));
  #endif

  int temperatureSensorValue = (int) (10.0 * (0.1704388801 * analogRead(TEMPERATURE_SENSOR_PIN) - 20.5128205128));
  deviceStateMachine.Set(sendValueState);
  
  #ifdef ENABLE_BLUETOOTH_PROTOCOL
    sprintf(protocolResponseValue, "#%s%2d.%1d\r\n", DEVICE_CODE[6], temperatureSensorValue / 10, temperatureSensorValue % 10);
  #endif
}


/*
  State Buzzer
*/
State buzzerState() {
  #ifdef DEBUG_DEVICE_SM
    Serial.println(F("Debug Device State Machine: Buzzer State"));
  #endif

  if (actionedDeviceArgument == 0) noTone(BUZZER_PIN);
  else tone(BUZZER_PIN, actionedDeviceArgument);

  deviceStateMachine.Set(processButtonState);
}


/*
  State Play Melody
*/
State playMelodyState() {
  #ifdef DEBUG_DEVICE_SM
    Serial.println(F("Debug Device State Machine: Play Melody State"));
  #endif

  deviceStateMachine.Set(processButtonState);

  if (actionedDeviceArgument == MARIO_THEME_SONG) {
    playMelody(MARIO_THEME_SONG_MELODY, MARIO_THEME_SONG_TEMPO, MARIO_THEME_SONG_SIZE);
    
  } else if (actionedDeviceArgument == CHRISTMAS_SONG_CODE) {
    playMelody(CHRISTMAS_THEME_SONG_MELODY, CHRISTMAS_THEME_SONG_TEMPO, CHRISTMAS_THEME_SONG_SIZE);
    playMelody(CHRISTMAS_THEME_SONG_MELODY, CHRISTMAS_THEME_SONG_TEMPO, CHRISTMAS_THEME_SONG_SIZE);
    
  } else if (actionedDeviceArgument == IMPERIAL_MARCH_SONG_CODE) {
    playMelody(IMPERIAL_MARCH_SONG_MELODY, IMPERIAL_MARCH_SONG_TEMPO, IMPERIAL_MARCH_SONG_SIZE);
    
  } else {
    return;
  }
}


/*
  State Set Pin
*/
State setPinState() {
  char pin[7];

  #ifdef DEBUG_DEVICE_SM
    Serial.println(F("Debug Device State Machine: Set PIN State"));
  #endif

  Serial1.print("AT");
  delay(400);

  if (strcmp(actionedDeviceCode, DEVICE_CODE[8]) == 0) {
    setBluetoothPin(actionedDeviceArgument, false);
    EEPROM.writeLong(BLUETOOTH_EDR_PIN_EEPROM_ADDRESS, actionedDeviceArgument);
    
  } else {
    setBluetoothPin(actionedDeviceArgument, true);
    EEPROM.writeLong(BLUETOOTH_BLE_PIN_EEPROM_ADDRESS, actionedDeviceArgument);
  }

  EEPROM.write(EEPROM_MAGIC_NUMBER_ADDRESS, EEPROM_MAGIC_NUMBER);

  Serial1.print(F("AT+RESET"));
  delay(400);

  /* No use to send a confirmation message since bluetooth connection will be lost due to reset */
  deviceStateMachine.Set(processButtonState);
}


/****************************************************************
 * Protocol state machine functions                             *
 ****************************************************************/
#ifdef ENABLE_BLUETOOTH_PROTOCOL
State sleepState() {
  if (readyToReceiveProtocolCommand) {
    protocolStateMachine.Set(waitForStartState);
    protocolCommandIsReady = false;
    
  } else {
    protocolStateMachine.Set(sleepState);
  }
}

State waitForStartState() {
  if (!Serial1.available()) {
    protocolStateMachine.Set(waitForStartState);
    
  } else {
    char receivedChar = Serial1.read();
    
    if (receivedChar == '#') {
      /* Start of protocol command detetcted */
      #ifdef DEBUG_PROTOCOL_SM
        Serial.println(F("Debug Protocol State Machine: Wait For Start State => '#'"));
      #endif

      /* Reset protocol command string */
      sprintf(protocolCommandString, "");

      protocolCommand = protocolCommandString;
      protocolWatchdog = millis();
      protocolStateMachine.Set(receiveCharState);
      
    } else {
      protocolStateMachine.Set(waitForStartState);
    }
  }
}

State receiveCharState() {
  if (!Serial1.available()) {
    protocolStateMachine.Set(receiveCharState);
    
    if (millis() - protocolWatchdog > PROTOCOL_TIMEOUT) {
      /* Give up trying to receive this command if no character is received for too long */
      protocolStateMachine.Set(waitForStartState);
      
      #ifdef DEBUG_PROTOCOL_SM
        Serial.println(F("Debug Protocol State Machine: Receive Char State => TIMEOUT!"));
      #endif
    }
    
  } else {
    char receivedChar = Serial1.read();
    protocolWatchdog = millis();

    #ifdef DEBUG_PROTOCOL_SM
      Serial.print(F("Debug Protocol State Machine: Receive Char State => '"));
      Serial.print(receivedChar);
      Serial.println("'");
      Serial.print(F("Debug Protocol State Machine: Current length of protocol command string is "));
      Serial.println(strlen(protocolCommandString));
    #endif

    /* Received command string is too long */
    if (strlen(protocolCommandString) > PROTOCOL_REQUEST_LENGTH) {
      protocolStateMachine.Set(sleepState);

      #ifdef DEBUG
        Serial.println(F("Debug: Failed to receive command (command string is too long)"));
      #endif
      
      return;
    }

    if (receivedChar == '\n' || receivedChar == '\r') {
      /* End of protocol command detected */
      *protocolCommand = '\0';
      protocolCommandIsReady = true;
      readyToReceiveProtocolCommand = false;
      protocolCommand = protocolCommandString;
      protocolStateMachine.Set(sleepState);

      #ifdef DEBUG
        Serial.print(F("Debug: Received command \"#"));
        Serial.print(protocolCommand);
        Serial.println("\"");
      #endif
      
    } else {
      /* Keep receiving chars unitl the end is detected */
      *protocolCommand = receivedChar;
      *(protocolCommand + 1) = '\0';
      protocolCommand++;
      protocolStateMachine.Set(receiveCharState);
    }
  }
}
#endif


/****************************************************************
 * Function Implementations                                     *
 ****************************************************************/
void playMelody(PROGMEM const int melody[], PROGMEM const int tempo[], int melodySize) {
  /* Send the signal that start to play melody */
  Serial1.print("#PM0001\r\n");
  
  for (int thisNote = 0; thisNote < melodySize; thisNote++) {
    /* To calculate the note duration, take one second divided by the note type
       e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc. */
    unsigned int noteDuration = 1000 / pgm_read_word_near(tempo + thisNote);
    unsigned int playedNote = pgm_read_word_near(melody + thisNote);

    tone(BUZZER_PIN, playedNote, noteDuration);

    /* To distinguish the notes, set a minimum time between them
       The note's duration + 30% seems to work well */
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);

    /* Stop the tone playing */
    noTone(BUZZER_PIN);
  }
  
  /* Send the signal that ended melody */
  Serial1.print("#PM0000\r\n");
}


void getBluetoothLapAddress(char* lapAddress, bool ble) {
  bool foundAddressStart = false;
  char readByte;
  int byteCounter = 0;

  if (!ble) {
    Serial1.print(F("AT+ADDE?"));
    
  } else {
    Serial1.print(F("AT+ADDB?"));
  }
  
  delay(400);

  /* Wait until a get response arrives */
  while (Serial1.available()) {
    if (Serial1.read() == 'G') break;
  }

  /* Get MAC address */
  while (Serial1.available()) {
    readByte = Serial1.read();
    
    if (foundAddressStart) {
      lapAddress[byteCounter] = readByte;
      byteCounter++;
      
      if (byteCounter > 11) {
        lapAddress[12] = '\0';
        break;
      }
    }

    if (readByte == ':') { 
      foundAddressStart = true;
    }
  }

  /* Exclude the first 4 digits */
  for (int i = 0; i < 8; i++) {
    lapAddress[i] = lapAddress[i + 4];
  }
  
  lapAddress[8] = '\0';
}


void setBluetoothPin(long pinNumber, bool ble) {
  char atCommand[16];
  char debugString[26];
  char pin[7];

  if (!ble) {
    sprintf(pin, "%04ld", pinNumber - 10000 * (pinNumber / 10000));
    sprintf(atCommand, "AT+PINE%s", pin);
    sprintf(debugString, "New EDR PIN set: %s", pin);
    
  } else {
    sprintf(pin, "%06ld", pinNumber - 1000000 * (pinNumber / 1000000));
    sprintf(atCommand, "AT+PINB%s", pin);
    sprintf(debugString, "New BLE PIN set: %s", pin);
  }

  Serial1.print(atCommand);
  delay(400);
  
  #ifdef DEBUG
    Serial.println(debugString);
  #endif
}


void turnOffLeds() {
  analogWrite(RED_RGB_LED_PIN, 255);
  analogWrite(GREEN_RGB_LED_PIN, 255);
  analogWrite(BLUE_RGB_LED_PIN, 255);
}
