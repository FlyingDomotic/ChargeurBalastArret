#define CODE_VERSION "V26.4.2-1"

#define VERSION_FRANCAISE

/*

This code controls a ballast loader into appropriate wagons.

It's based on a hopper driven by 2 pulse relays.

First opens hopper while second close it.

A rail has been equipped with 10 ILS regularly spaced, in order to detect a magnet placed under each wagon.

When a wagon goes over one of up to three opening ILS, a relay cuts power on rail, stopping train.After a settable time, a (settable) pulse is send to opening realy. After a (settable) time, hopper is closed, a power restored after a (settable) time.
    
This cycle can be repeated up to 3 times per wagon, for each wagon equipped with a magnet.

Note that circulation way of train has no impact if magnet is placed in middle of wagons.

It may happen that granulate block hopper exit. To balance this issue, you can add a vibrator that'll be activated when hopper opens until a (settable) time after hopper closing.

In addition, it's possible to add a supplemental ILS on unloading zone, that'll also activate vibrator during a settable time, to facilitate the descent of the ballast into the hopper.

On certain hopper models, it may happen that vibrations (re)open hopper. To counter balance this tendency, it's possible to define a re-closing delay, which will resend a closing pulse at regular interval as long as vibrations are active.

For more realistic experience, you can add a MP3 module that plays a sound during wagon loading, and another one during unloading. This module is not mandatory, don't add it if not useful. Still in the sale realistic spirit, sound can increase from 0 to its (settable) value over a (settable) time at loafing start, and decrease it from its maximum value to 0 at end of load.

Settings are send to Arduino through its serial (USB) link. This link is also used to send feedback and messages to user.

Settings are memorized in Arduino's EEPROM to be available after (re)start.

	
# Hardware Arduino Nano:
	- 11 ILS inputs (10 for wagon localization during load, 1 (optional) for unloading detection). Note that electronic magnetic captors can also be used.
	- 4 relays output:
        - to open hopper,
        - to close hopper,
        - for vibrator,
        - to cut power on rail.
    - 1 MP3 output:
        - to play loading and unloading sound, optional.

# Setting parameters:
    - find the proper pulse duration to open and close hopper, without burning coils, increasing or reducing `ID` parameter and using `O` (open) and `C` (close) hopper,
 	- find loading ILS number 1 manually pushing wagon, set it in `IL1` parameter,
    - repeat previous step up to 3 times, if needed,
    - load `WS` with delay to wait after stopping train, before loading wagon,
    - find wagon filling duration and set it into `FD`,
    - load `WF` with delay to wait after filling wagon, before starting train,
    - if vibrations are needed:
        - vibrator is activated when hopper opens, stopper after `FV` delay when closed,
        - vibrator is activated when unloading ILS is activated, for `UV` delay,
        - set hopper re-closing interval into `RC`,
    - if sound id needed:
        - set sound volume into `SV`, testing sounds with `TS`,
        - set sound increment into `SI`, if needed,
        - set loading sound index into `LS`, unload sound into `US`
    - start process with `R`,
    - send train over ILS.

# Commands:
    - I11-10 : ILS 1 (number)
    - I20-10 : ILS 2 (number)
    - I30-10 : ILS 3 (number)
    - ID1-999 : Relay Impulsion Duration (ms)
    - FD1-9999 : Wagon Filling DuraTion (ms)
    - FV0-9999 : Filling Vibrations (ms)
    - UV0-9999 : Unloading Vibrations (ms)
    - RC0-9999 : Repeat Close (ms)
    - WS0-9999 : Wait After Stop (ms)
    - WF0-9999 : Wait After Filling (ms)
    - LS0-99 : Loading sound
    - US0-99 : Unloading Sound
    - TS1-99 : Test sound
    - SV0-30 : Sound Volume
    - SI0-999 : Sound Increment (ms)
    - R : Run
    - S : Stop
    - E : Emergency Stop
    - IS : ILS State
    - O : Open Hopper
    - C : Close Hopper
    - ST : Stop Train
    - RT : Run Train
    - D : Toggle Debug
    - INIT : Global Initialization
    - DV : Display Variables

## Pushing <ESC> (sometimes wtriing Escape):
    - Emergency stop:
        - Stop process,
        - Close hopper,
        - Stop vibrations,
        - Stop train,
        - Stop sound.

Author : Flying Domotic, February 2025, for FabLab
License: GNU GENERAL PUBLIC LICENSE - Version 3, 29 June 2007

*/

// Includes
#include "Arduino.h"
#include "EEPROM.h"

// Command and text
#ifdef VERSION_FRANCAISE
    #define ILS_ACTIVATION1_COMMAND "I1"
    #define ILS_ACTIVATION2_COMMAND "I2"
    #define ILS_ACTIVATION3_COMMAND "I3"
    #define RELAY_PULSE_DURATION_COMMAND "DI"
    #define WAGON_FILL_DURATION_COMMAND "DR"
    #define LOAD_DELAY_COMMAND "VR"
    #define UNLOAD_DELAY_COMMAND "VV"
    #define RECLOSE_DELAY_COMMAND "RF"
    #define WAIT_AFTER_STOP_COMMAND "AA"
    #define WAIT_AFTER_FILL_COMMAND "AR"
    #define FILL_SOUND_COMMAND "SC"
    #define UNLOAD_SOUND_COMMAND "SD"
    #define TEST_SOUND_COMMAND "TS"
    #define SOUND_VOLUME_COMMAND "V"
    #define SOUND_INCREMENT_COMMAND "IS"
    #define START_COMMAND "M"
    #define STOP_COMMAND "A"
    #define EMERGENCY_COMMAND "U"
    #define ILS_STATE_COMMAND "E"
    #define OPEN_RELAY_COMMAND "O"
    #define CLOSE_RELAY_COMMAND "F"
    #define STOP_TRAIN_COMMAND "AT"
    #define START_TRAIN_COMMAND "DT"
    #define DEBUG_TOGGLE_COMMAND "D"
    #define INIT_COMMAND "INIT"
    #define DISPLAY_VARIABLES_COMMAND "AV"
#else
    #define ILS_ACTIVATION1_COMMAND "I1"
    #define ILS_ACTIVATION2_COMMAND "I2"
    #define ILS_ACTIVATION3_COMMAND "I3"
    #define RELAY_PULSE_DURATION_COMMAND "ID"
    #define WAGON_FILL_DURATION_COMMAND "FD"
    #define LOAD_DELAY_COMMAND "FV"
    #define UNLOAD_DELAY_COMMAND "UV"
    #define RECLOSE_DELAY_COMMAND "RC"
    #define WAIT_AFTER_STOP_COMMAND "WS"
    #define WAIT_AFTER_FILL_COMMAND "WF"
    #define FILL_SOUND_COMMAND "LS"
    #define UNLOAD_SOUND_COMMAND "US"
    #define TEST_SOUND_COMMAND "TS"
    #define SOUND_VOLUME_COMMAND "SV"
    #define SOUND_INCREMENT_COMMAND "SI"
    #define START_COMMAND "R"
    #define STOP_COMMAND "S"
    #define EMERGENCY_COMMAND "E"
    #define ILS_STATE_COMMAND "IS"
    #define OPEN_RELAY_COMMAND "O"
    #define CLOSE_RELAY_COMMAND "C"
    #define STOP_TRAIN_COMMAND "ST"
    #define START_TRAIN_COMMAND "RT"
    #define DEBUG_TOGGLE_COMMAND "D"
    #define INIT_COMMAND "INIT"
    #define DISPLAY_VARIABLES_COMMAND "DV"
#endif

//  Parameters

#define MAGIC_NUMBER 56                                             // EEPROM magic byte
#define EEPROM_VERSION 1                                            // EEPROM version
#define BUFFER_LENGHT 50                                            // Serial input buffer length
#define ILS_CLOSED LOW                                              // State read when ILS is closed
#define RELAY_CLOSED LOW                                            // State to write to close relay
#define RELAY_OPENED HIGH                                           // State to write to open relay
#define DISPLAY_ILS_TIME 100                                        // Display ILS state every xxx ms
#define OPEN_RELAY 0                                                // Index of open relay into relayPinMapping
#define CLOSE_RELAY 1                                               // Index of close relay into relayPinMapping
#define VIBRATION_RELAY 2                                           // Index of vibration relay into relayPinMapping (do not define in no vibration relay)
#define POWER_ISOLATION_RELAY 3                                     // Index of power isolation relay into relayPinMapping
#define DISPLAY_KEYBOARD_INPUT                                      // Display each character read on keyboard (do not define if not needed)
#define MP3_PIN 8                                                   // MP3 sound module pin (comment it if not)

#ifdef MP3_PIN
    #include <mp3_player_module_wire.h>								// MP3 player (one wire connection)
#endif

// EEPROM data (current version)
struct eepromData_s {
    uint8_t magicNumber;                                            // Magic number
    uint8_t version;                                                // Structure version
    uint8_t activationIls1;                                         // ILS number 1 to activate filling
    uint8_t activationIls2;                                         // ILS number 2 to activate filling
    uint8_t activationIls3;                                         // ILS number 3 to activate filling
    uint16_t fillingTime;                                           // Time (0.001s) to fill wagon
    uint16_t pulseTime;                                             // Time (0.001s) to send current to relay
    bool isActive;                                                  // When active flag is true, relays are triggered by ILS
    bool inDebug;                                                   // Print debug message when true
    uint16_t loadDelay;                                             // Duration (ms) to keep vibrations after load (here even if VIBRATION_RELAY not set)
    uint16_t unloadDelay;                                           // Duration (ms) to keep vibrations after unload (here even if VIBRATION_RELAY not set)
    uint16_t repeatCloseDelay;                                      // Duration (ms) to force close relay when vibration active and door closed
    uint16_t waitAfterStop;                                         // Duration (ms) to wait after train stop before filling
    uint16_t waitAfterFill;                                         // Duration (ms) to wait after filling to restart train
    uint8_t fillSound;                                              // Fill sound index
    uint8_t unloadSound;                                            // Unload sound index
    uint16_t soundIncrementDuration;                                // Sound increment
    uint8_t soundVolume;                                            // Sound volume
};

bool displayIls = false;                                            // When set, continously display ILS state
uint8_t ilsPinMapping[] = {4, 5, 6, 7, A5, A4, A3, A2, A1, A0, 2};  // Maps ILS number to Arduino PIN number
uint8_t relayPinMapping[] = {12, 11, 10, 9};                        // Maps relay number to Arduino PIN number
uint8_t relayState[] = {0, 0, 0, 0};                                // Relay current state

// Data

// State machine
enum stateMachineEnum {
    waitForWagon = 0,                                               // Waiting for wagon over ILS to stop train
    waitingAfterStop,                                               // Waiting after train stop before opening door
    waitFilled,                                                     // Waiting for wagon filled to close door
    waitAfterFilling                                                // Waiting after closing door before restarting train
};

stateMachineEnum stateMachine = waitForWagon;                       // State machine index
unsigned long waitAfterStopTimer = 0;                               // Timers for each state
unsigned long waitFilledTimer = 0;
unsigned long waitAfterFillingTimer = 0;
unsigned long waitForIlsClosedTimer = 0;

// Relay pulse
bool relayPulseActive = false;                                      // Relay pulse active flag
unsigned long relayPulseTimer = 0;                                  // Relay pulse start time

// Vibrations
#ifdef VIBRATION_RELAY
	bool fillingVibrationActive = false;                            // Vibrations during wagon filling flag
	bool unloadingVibrationActive = false;                          // Vibrations during wagon unloading flag
	unsigned long fillingVibrationTimer = 0;                        // Vibrations during filling timer
	unsigned long unloadingVibrationTimer = 0;                      // Vibrations during unloading timer

    // Repeat close during vibrations
    bool repeatCloseActive = false;                                 // Repeat close during vibrations flag
    unsigned long repeatCloseTimer = 0;                             // Repeat close during vibrations timer
#endif

// Sound
#ifdef MP3_PIN
    Mp3PlayerModuleWire mp3Player(MP3_PIN);					        // MP3 module one-line protocol
    unsigned long lastSoundChangeTime = 0;                          // Last time we changed sound volume
    unsigned long soundChangeDuration = 0;                          // Duration of one sound change
    int8_t soundIncrement = 0;                                      // Sound increment (negative to decrement)
    uint8_t soundVolume = 0;                                        // Current sound volume
#endif

// Debouncer
struct debouncer_s {                                                // Debouncer data for one pin
    uint8_t dataIndex;                                              // Pin index to scan
    bool isClosed;                                                  // ILS is currently closed
    bool lastWasClosed;                                             // ILS was closed at preivous loop
    unsigned long lastChangeTime;                                   // Last time pin state changed
};

debouncer_s debouncer[4];                                           // Debouncer for 3 filling ILS and 1 unloading ILS

// Other data
unsigned long lastIlsDisplay = 0;                                   // Last time we displayed ILS state
uint16_t commandValue;                                              // Value extracted from command
uint8_t bufferLen = 0;                                              // Used chars in input buffer
char inputBuffer[BUFFER_LENGHT];                                    // Serial input buffer
bool doorOpened = false;                                            // Is door opened?
eepromData_s data;                                                  // Data stored to/read from EEPROM

// Routines and functions

void loadIls(uint8_t index, uint8_t pin_index);                     // Load one debounce table ILS
void reloadIls(void);                                               // Reload ILS from data
void displayStatus(void);                                           // Display current status
void loadSettings(void);                                            // Load settings from EEPROM
void saveSettings(void);                                            // Save settings to EEPROM (only if modified)
void initSettings(void);                                            // Init settings to default values
void resetInputBuffer(void);                                        // Reset serial input buffer
void initIO(void);                                                  // Init IO pins
void emergencyStop(void);                                           // Emergency stop
void workWithSerial(void);                                          // Work with Serial input
bool isCommand(char* inputBuffer, char* commandToCheck);            // Check command without value
bool isCommandValue(char* inputBuffer, char* commandToCheck, uint16_t minValue, uint16_t maxValue); // Check command with value
void startFilling(void);                                            // Start filling
void stopFilling(void);                                             // Stop filling
void stopTrain(void);                                               // Stop train
void startTrain(void);                                              // Start train
#ifdef VIBRATION_RELAY
    void startUnloading(void);                                      // Start unloading process
    void startVibration(void);                                      // Start vibration relay
    void stopVibration(void);                                       // Stop vibration relay
#endif
#ifdef MP3_PIN
    void soundSetup();                                              // Setup sound chip
    void playSound(uint8_t index);                                  // Play sound index
    void stopSound(void);                                           // Stop playing sound
    void changeVolume();                                            // Change current volume
#endif
void displayIlsState(void);                                         // Display all ILS state
void printHelp(void);                                               // Print help message
void toggleDebug(void);                                             // Toggle  debug flag
void reinitAll(void);                                               // Reinitialize all settings
void executeCommand(void);                                          // Execute command read on serial input (a-z and 0-9)
void displayVariables(void);                                        // Display all variables (debug)
void setRelay(uint8_t pin, uint8_t state);                          // Change a relay state
void setup(void);                                                   // Setup
void loop(void);                                                    // Main loop

// Reload ILS debounce table from data
void reloadIls(void){
    loadIls(0, data.activationIls1);
    loadIls(1, data.activationIls2);
    loadIls(2, data.activationIls3);
    loadIls(3, 10+1);
}

// Load one debounce table ILS
void loadIls(uint8_t index, uint8_t pin_index) {
    if (debouncer[index].dataIndex != pin_index) {              // Do it only if changed
        debouncer[index].dataIndex = pin_index;                 // Load pin index
        debouncer[index].lastChangeTime = millis();             // Save change time
        if (debouncer[index].dataIndex) {                       // If pin index defined, read current value and save it
            debouncer[index].isClosed = digitalRead(ilsPinMapping[debouncer[index].dataIndex-1]) == ILS_CLOSED;
            debouncer[index].lastWasClosed = debouncer[0].isClosed;
        }
    }
}

// Display current status
void displayStatus(void) {
    Serial.print(F(ILS_ACTIVATION1_COMMAND));
    Serial.print(data.activationIls1);
    if (data.activationIls2) {
        Serial.print(F(" "));
        Serial.print(F(ILS_ACTIVATION2_COMMAND));
        Serial.print(data.activationIls2);
        Serial.print(F(" "));
    }
    if (data.activationIls3) {
        Serial.print(F(" "));
        Serial.print(F(ILS_ACTIVATION3_COMMAND));
        Serial.print(data.activationIls3);
    }
    Serial.print(F(" "));
    Serial.print(F(RELAY_PULSE_DURATION_COMMAND));
    Serial.print(data.pulseTime);
    Serial.print(F(" "));
    Serial.print(F(WAGON_FILL_DURATION_COMMAND));
    Serial.print(data.fillingTime);
    if (data.waitAfterStop) {
        Serial.print(F(" "));
        Serial.print(F(WAIT_AFTER_STOP_COMMAND));
        Serial.print(data.waitAfterStop);
    }
    if (data.waitAfterFill) {
        Serial.print(F(" "));
        Serial.print(F(WAIT_AFTER_FILL_COMMAND));
        Serial.print(data.waitAfterFill);
    }
    #ifdef VIBRATION_RELAY
        if (data.loadDelay) {
            Serial.print(F(" "));
            Serial.print(F(LOAD_DELAY_COMMAND));
            Serial.print(data.loadDelay);
        }
        if (data.unloadDelay) {
            Serial.print(F(" "));
            Serial.print(F(UNLOAD_DELAY_COMMAND));
            Serial.print(data.unloadDelay);
        }
        if (data.repeatCloseDelay) {
            Serial.print(F(" "));
            Serial.print(F(RECLOSE_DELAY_COMMAND));
            Serial.print(data.repeatCloseDelay);
        }
    #endif
    #ifdef MP3_PIN
        if (data.fillSound) {
            Serial.print(F(" "));
            Serial.print(F(FILL_SOUND_COMMAND));
            Serial.print(data.fillSound);
        }
        if (data.unloadSound) {
            Serial.print(F(" "));
            Serial.print(F(UNLOAD_SOUND_COMMAND));
            Serial.print(data.unloadSound);
        }
        if (data.soundVolume) {
            Serial.print(F(" "));
            Serial.print(F(SOUND_VOLUME_COMMAND));
            Serial.print(data.soundVolume);
        }
        if (data.soundIncrementDuration) {
            Serial.print(F(" "));
            Serial.print(F(SOUND_INCREMENT_COMMAND));
            Serial.print(data.soundIncrementDuration);
        }
    #endif
    #ifdef VERSION_FRANCAISE
        if (data.inDebug) Serial.print(F(", déverminage"));
        Serial.println(data.isActive ? F(", en marche") : F(", à l'arrêt"));
    #else
        if (data.inDebug) Serial.print(F(", debug"));
        Serial.println(data.isActive ? F(", running") : F(", stopped"));
    #endif
}

// Load settings from EEPROM
void loadSettings(void) {
    initSettings();                                                 // Init data structure
    if (EEPROM.read(0) != MAGIC_NUMBER) {                           // Is first byte equal to magic number?
        #ifdef VERSION_FRANCAISE
            Serial.print(F("Magic est "));
        #else
            Serial.print(F("Magic is "));
        #endif
        Serial.print(EEPROM.read(0));
        #ifdef VERSION_FRANCAISE
            Serial.print(F(", pas "));
        #else
            Serial.print(F(", not "));
        #endif
        Serial.print(MAGIC_NUMBER);
        Serial.println(F("!"));
        return;
    }

    uint8_t version = EEPROM.read(1);                               // Get version
    if (version == 1) {
        // EEPROM data (V1 version)
        struct eepromDataV1_s {
            uint8_t magicNumber;                                    // Magic number
            uint8_t version;                                        // Structure version
            uint8_t activationIls1;                                 // ILS number 1 to activate filling
            uint8_t activationIls2;                                 // ILS number 2 to activate filling
            uint8_t activationIls3;                                 // ILS number 3 to activate filling
            uint16_t fillingTime;                                   // Time (0.001s) to fill wagon
            uint16_t pulseTime;                                     // Time (0.001s) to send current to relay
            bool isActive;                                          // When active flag is true, relays are triggered by ILS
            bool inDebug;                                           // Print debug message when true
            uint16_t loadDelay;                                     // Duration (ms) to keep vibrations after load (here even if VIBRATION_RELAY not set)
            uint16_t unloadDelay;                                   // Duration (ms) to keep vibrations after unload (here even if VIBRATION_RELAY not set)
            uint16_t repeatCloseDelay;                              // Duration (ms) to force close relay when vibration active and door closed
            uint16_t waitAfterStop;                                 // Duration (ms) to wait after train stop before filling
            uint16_t waitAfterFill;                                 // Duration (ms) to wait after filling to restart train
        };
        eepromDataV1_s dataV1;
        EEPROM.get(0, dataV1);                                      // Load EEPROM V1 structure
        data.activationIls1 =  dataV1.activationIls1;               // Copy V1 data to V2
        data.activationIls2 =  dataV1.activationIls2;
        data.activationIls3 =  dataV1.activationIls3;
        data.fillingTime =  dataV1.fillingTime;
        data.pulseTime =  dataV1.pulseTime;
        data.isActive =  dataV1.isActive;
        data.inDebug =  dataV1.inDebug;
        data.loadDelay =  dataV1.loadDelay;
        data.unloadDelay =  dataV1.unloadDelay;
        data.repeatCloseDelay =  dataV1.repeatCloseDelay;
        data.waitAfterStop =  dataV1.waitAfterStop;
        data.waitAfterFill =  dataV1.waitAfterFill;
    } else if (version == 2) {
        EEPROM.get(0, data);                                        // Load EEPROM V2 structure
    } else {
        #ifdef VERSION_FRANCAISE
            Serial.print(F("Version est "));
        #else
            Serial.print(F("Version is "));
        #endif
        Serial.print(version);
        #ifdef VERSION_FRANCAISE
            Serial.print(F(", pas "));
        #else
            Serial.print(F(", not "));
        #endif
        Serial.print(EEPROM_VERSION);
        Serial.println(F("!"));
        return;
    }
}

// Save settings to EEPROM (only if modified)
void saveSettings(void) {
    data.magicNumber = MAGIC_NUMBER;                                // Force magic number
    data.version = EEPROM_VERSION;                                  // ... and version
    eepromData_s savedData;                                         // Current EEPROM data
    EEPROM.get(0, savedData);                                       // Get saved data
    if (memcmp(&data, &savedData, sizeof(data))) {                  // Compare full buffers
        EEPROM.put(0, data);                                        // Store structure
    }
}

// Init settings to default values
void initSettings(void) {
    data.activationIls1 = 0;                                        // ILS number 1 to activate filling
    data.activationIls2 = 0;                                        // ILS number 1 to activate filling
    data.activationIls3 = 0;                                        // ILS number 1 to activate filling
    data.fillingTime = 2000;                                        // Time to fill wagon
    data.pulseTime = 100;                                           // Time to send current to relay
    data.isActive = false;                                          // When active flag is true, relays are triggered by ILS
    data.inDebug = false;                                           // Print debug message when true
    data.loadDelay = 1000;                                          // Keep vibrations after load
    data.unloadDelay = 5000;                                        // Keep vibrations after unload
    data.repeatCloseDelay = 1000;                                   // Force close relay when vibration active and door closed
    data.waitAfterStop = 1000;                                      // Delay between stop and fill
    data.waitAfterFill = 1000;                                      // Delay between fill and start
    data.fillSound = 1;                                             // Fill sound index
    data.unloadSound = 2;                                           // Unload sound index
    data.soundVolume = 15;                                          // Sound volume
    data.soundIncrementDuration = 500UL;                                    // Sound increment
}

// Reset serial input buffer
void resetInputBuffer(void) {
    memset(inputBuffer, 0, sizeof(inputBuffer));
    bufferLen = 0;
}

// Init IO pins
void initIO(void) {
    // Set ILS PIN to input with pullup resistor
    for (uint8_t i = 0; i < 11; i++) {
        pinMode(ilsPinMapping[i], INPUT_PULLUP);
    }
    // Set relay PIN to output, init level = opened
    for (uint8_t i = 0; i < 4; i++) {
        setRelay(i, RELAY_OPENED);
        pinMode(relayPinMapping[i], OUTPUT);
    }
    // Force activating close relay for 100 ms (preferences not yet loaded, user set pulse length not defined)
    setRelay(CLOSE_RELAY, RELAY_CLOSED);
    delay(100);
    setRelay(CLOSE_RELAY, RELAY_OPENED);
}

// Emergency stop
void emergencyStop(void) {
    // Arrêt du relai d'ouverture de la trémie
    digitalWrite(relayPinMapping[OPEN_RELAY], RELAY_OPENED);

    // Arrêt du train
    digitalWrite(relayPinMapping[POWER_ISOLATION_RELAY], RELAY_CLOSED);

    // Arrêt des vibrations
    digitalWrite(relayPinMapping[VIBRATION_RELAY], RELAY_OPENED);

    // Armement du relai de fermeture de la trémie
    digitalWrite(relayPinMapping[CLOSE_RELAY], RELAY_CLOSED);
    delay(100);

    // Arrêt du relai de fermeture de la trémie
    digitalWrite(relayPinMapping[CLOSE_RELAY], RELAY_OPENED);

    // Arrête le son
    stopSound();

    // Reset variables and timers
    data.isActive = false;

    // State machine
    stateMachine = waitForWagon;                                    // Reset state machine
    waitAfterStopTimer = 0;                                         // Timers for each state
    waitFilledTimer = 0;
    waitAfterFillingTimer = 0;
    waitForIlsClosedTimer = 0;

    // Relay pulse
    relayPulseActive = false;                                       // Relay pulse active flag
    relayPulseTimer = 0;                                            // Relay pulse start time

    // Vibrations
    #ifdef VIBRATION_RELAY
        fillingVibrationActive = false;                             // Vibrations during wagon filling flag
        unloadingVibrationActive = false;                           // Vibrations during wagon unloading flag
        fillingVibrationTimer = 0;                                  // Vibrations during filling timer
        unloadingVibrationTimer = 0;                                // Vibrations during unloading timer

        // Repeat close during vibrations
        repeatCloseActive = false;                                  // Repeat close during vibrations flag
        repeatCloseTimer = 0;                                       // Repeat close during vibrations timer
    #endif

    doorOpened = false;                                             // Door is closed
}

// Work with Serial input
void workWithSerial(void) {
    // Reset display ILS flag
    if (displayIls) {
        Serial.println();
        displayIls = false;
    }
    // Read serial input
    while (Serial.available()) {
        // Read one character
        char c = Serial.read();
        if (c == 13) {                                              // Is this a return?
            Serial.print(c);
            executeCommand();
            resetInputBuffer();
        } else if (c == 27) {                                       // Is this an <ESC>?
            emergencyStop();
        } else if (c) {                                             // Is this not null?
            // Keep only "A" to "Z", "a" to "z" and "0" to "9"
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
                if (bufferLen >= BUFFER_LENGHT - 1) {
                    #ifdef VERSION_FRANCAISE
                        Serial.println(F("Buffer plein - Reset!"));
                    #else
                        Serial.println(F("Buffer full - Reset!"));
                    #endif
                    resetInputBuffer();
                }
                inputBuffer[bufferLen++] = c;
            } else if (c == 8) {                                    // Is this <backspace> character?
                if (bufferLen) {                                    // Is buffer not empty?
                    bufferLen--;                                    // Reduce buffer len
                    inputBuffer[bufferLen] = 0;                     // Remove charcater
                }
            }
        }
    }
    #ifdef DISPLAY_KEYBOARD_INPUT
        Serial.print(F("\r"));
        Serial.print(inputBuffer);
        Serial.print(F("  "));
        Serial.print(F("\r"));
        Serial.print(inputBuffer);
    #endif
}

// Check command without value
bool isCommand(char* inputBuffer, char* commandToCheck) {
    return !strcasecmp(inputBuffer, commandToCheck);
}

// Check command with value
bool isCommandValue(char* inputBuffer, char* commandToCheck, uint16_t minValue, uint16_t maxValue) {
    if (strncmp(inputBuffer, commandToCheck, strlen(commandToCheck))) {
        return false;
    }
    commandValue = atoi(&inputBuffer[strlen(commandToCheck)]);      // Convert given value
    if (commandValue < minValue || commandValue > maxValue) {       // Check limits
        Serial.print(commandValue);
        #ifdef VERSION_FRANCAISE
            Serial.print(F(" hors limites "));
        #else
            Serial.print(F(" out of limits "));
        #endif
        Serial.print(minValue);
        Serial.print(F("-"));
        Serial.print(maxValue);
        #ifdef VERSION_FRANCAISE
            Serial.print(F(" pour la commande "));
        #else
            Serial.print(F(" for command "));
        #endif
        Serial.println(commandToCheck);
        return false;
    }
    return true;
}

//  Stop train
void stopTrain(void) {
    if (data.inDebug) {
        #ifdef VERSION_FRANCAISE
            Serial.print(F("Arrêt du train à "));
        #else
            Serial.print(F("Train stop at "));
        #endif
        Serial.println(millis());
    }
    // Close isolation relay
    setRelay(POWER_ISOLATION_RELAY, RELAY_CLOSED);
}

// Start filling
void startFilling(void) {
    if (data.inDebug) {
        #ifdef VERSION_FRANCAISE
            Serial.print(F("Début chargement à "));
        #else
            Serial.print(F("Starting load at "));
        #endif
        Serial.println(millis());
    }
    setRelay(CLOSE_RELAY, RELAY_OPENED);
    setRelay(OPEN_RELAY, RELAY_CLOSED);
    #ifdef VIBRATION_RELAY
        startVibration();
    #endif
}

// Stop filling
void stopFilling(void) {
    if (data.inDebug) {
        #ifdef VERSION_FRANCAISE
            Serial.print(F("Fin chargement à "));
        #else
            Serial.print(F("Ending load at "));
        #endif
        Serial.print(millis());
        #ifdef VERSION_FRANCAISE
            Serial.print(F(", durée "));
        #else
            Serial.print(F(", duration "));
        #endif
        Serial.println(millis() - waitFilledTimer);
    }
    setRelay(OPEN_RELAY, RELAY_OPENED);
    setRelay(CLOSE_RELAY, RELAY_CLOSED);
    #ifdef VIBRATION_RELAY
        // Keep vibrations after load
        fillingVibrationActive = true;
        fillingVibrationTimer = millis();
    #endif

}

//  Start train
void startTrain(void) {
    if (data.inDebug) {
        #ifdef VERSION_FRANCAISE
            Serial.print(F("Départ du train à "));
        #else
            Serial.print(F("Starting train at "));
        #endif
        Serial.println(millis());
    }
    // Open isolation relay
    setRelay(POWER_ISOLATION_RELAY, RELAY_OPENED);
}

#ifdef VIBRATION_RELAY
    // Start unloading process
    void startUnloading(void) {
        if (!unloadingVibrationActive) {
            if (data.inDebug) {
                #ifdef VERSION_FRANCAISE
                    Serial.print(F("Début déchargement à "));
                #else
                    Serial.print(F("Starting unload at "));
                #endif
                Serial.println(millis());
            }
            startVibration();
        }
        unloadingVibrationActive = true;                            // Start unloading vibrations
        unloadingVibrationTimer = millis();                         // Set unload vibration timer
    }

    // Start vibration relay
    void startVibration(void) {
        if (data.inDebug) {
            #ifdef VERSION_FRANCAISE
                Serial.print(F("Début vibrations à "));
            #else
                Serial.print(F("Starting vibrations at "));
            #endif
            Serial.println(millis());
        }
        setRelay(VIBRATION_RELAY, RELAY_CLOSED);
    }

    // Stop vibration relay
    void stopVibration(void) {
        if (data.inDebug) {
            #ifdef VERSION_FRANCAISE
                Serial.print(F("Fin vibrations à "));
            #else
                Serial.print(F("Ending vibrations at "));
            #endif
            Serial.println(millis());
        }
        setRelay(VIBRATION_RELAY, RELAY_OPENED);
        repeatCloseActive = false;
        fillingVibrationActive = false;
        unloadingVibrationActive = false;
    }
#endif

// Display all ILS state
void displayIlsState(void) {
    displayIls = true;
}

// Print help message
void printHelp(void) {
    #ifdef VERSION_FRANCAISE
        Serial.print(F(ILS_ACTIVATION1_COMMAND)); Serial.print(F("1-10 : ILS 1 (number) => ")); Serial.println(data.activationIls1);
        Serial.print(F(ILS_ACTIVATION2_COMMAND)); Serial.print(F("0-10 : ILS 2 (number) => ")); Serial.println(data.activationIls2);
        Serial.print(F(ILS_ACTIVATION3_COMMAND)); Serial.print(F("0-10 : ILS 3 (number) => ")); Serial.println(data.activationIls3);
        Serial.print(F(RELAY_PULSE_DURATION_COMMAND)); Serial.print(F("1-999 : Relay Impulsion Duration (ms) => ")); Serial.println(data.pulseTime);
        #ifdef VIBRATION_RELAY
            Serial.print(F(WAGON_FILL_DURATION_COMMAND)); Serial.print(F("1-9999 : Durée Remplissage wagon (ms) => ")); Serial.println(data.fillingTime);
            Serial.print(F(LOAD_DELAY_COMMAND)); Serial.print(F("0-9999 : Vibrations Remplissage (ms) => ")); Serial.println(data.loadDelay);
            Serial.print(F(UNLOAD_DELAY_COMMAND)); Serial.print(F("0-9999 : Retard Vidage vibration (ms) => ")); Serial.println(data.unloadDelay);
            Serial.print(F(RECLOSE_DELAY_COMMAND)); Serial.print(F("0-9999 : Répétition fermeture (ms) => ")); Serial.println(data.repeatCloseDelay);
        #endif
        Serial.print(F(WAIT_AFTER_STOP_COMMAND)); Serial.print(F("0-9999 : Attente après Arrêt (ms) => ")); Serial.println(data.waitAfterStop);
        Serial.print(F(WAIT_AFTER_FILL_COMMAND)); Serial.print(F("0-9999 : Attente après remplissage (ms) => ")); Serial.println(data.waitAfterFill);
        #ifdef MP3_PIN
            Serial.print(F(FILL_SOUND_COMMAND)); Serial.print(F("0-99 : Son chargement => ")); Serial.println(data.fillSound);
            Serial.print(F(UNLOAD_SOUND_COMMAND)); Serial.print(F("0-99 : Son déchargement => ")); Serial.println(data.unloadSound);
            Serial.print(F(SOUND_VOLUME_COMMAND)); Serial.print(F("0-30 : Volume son => ")); Serial.println(data.soundVolume);
            Serial.print(F(SOUND_INCREMENT_COMMAND)); Serial.print(F("0-999 : Incrément son (ms) => ")); Serial.println(data.soundIncrementDuration);
            Serial.print(F(TEST_SOUND_COMMAND)); Serial.print(F("1-99 : Test son")); Serial.println();
        #endif
        Serial.print(F(START_COMMAND)); Serial.print(F(" : Marche")); Serial.println();
        Serial.print(F(STOP_COMMAND)); Serial.print(F(" : Arrêt")); Serial.println();
        Serial.print(F(EMERGENCY_COMMAND)); Serial.print(F(" : arrêt d'Urgence")); Serial.println();
        Serial.print(F(ILS_STATE_COMMAND)); Serial.print(F(" : Etat ILS")); Serial.println();
        Serial.print(F(OPEN_RELAY_COMMAND)); Serial.print(F(" : Ouverture trémie")); Serial.println();
        Serial.print(F(CLOSE_RELAY_COMMAND)); Serial.print(F(" : Fermeture trémie")); Serial.println();
        Serial.print(F(STOP_TRAIN_COMMAND)); Serial.print(F(" : Arrêt du train")); Serial.println();
        Serial.print(F(START_TRAIN_COMMAND)); Serial.print(F(" : Démarrage du train")); Serial.println();
        Serial.print(F(DEBUG_TOGGLE_COMMAND)); Serial.print(F(" : Bascule déverminage")); Serial.println();
        Serial.print(F(DISPLAY_VARIABLES_COMMAND)); Serial.print(F(" : Afficher variables")); Serial.println();
        Serial.print(F(INIT_COMMAND)); Serial.println(F(" : Initialisation globale"));
    #else
        Serial.print(F(ILS_ACTIVATION1_COMMAND)); Serial.print(F("1-10 : ILS 1 (numéro) => ")); Serial.println(data.activationIls1);
        Serial.print(F(ILS_ACTIVATION2_COMMAND)); Serial.print(F("0-10 : ILS 2 (numéro) => ")); Serial.println(data.activationIls2);
        Serial.print(F(ILS_ACTIVATION3_COMMAND)); Serial.print(F("0-10 : ILS 3 (numéro) => ")); Serial.println(data.activationIls3);
        Serial.print(F(RELAY_PULSE_DURATION_COMMAND)); Serial.print(F("1-999 : Durée Impulsion relai (ms) => ")); Serial.println(data.pulseTime);
        #ifdef VIBRATION_RELAY
            Serial.print(F(WAGON_FILL_DURATION_COMMAND)); Serial.print(F("1-9999 : Wagon filling duration (ms) => ")); Serial.println(data.fillingTime);
            Serial.print(F(LOAD_DELAY_COMMAND)); Serial.print(F("0-9999 : Filling Vibrations (ms) => ")); Serial.println(data.loadDelay);
            Serial.print(F(UNLOAD_DELAY_COMMAND)); Serial.print(F("0-9999 : Unloading Vibration (ms) => ")); Serial.println(data.unloadDelay);
            Serial.print(F(RECLOSE_DELAY_COMMAND)); Serial.print(F("0-9999 : Repeat Close (ms) => ")); Serial.println(data.repeatCloseDelay);
        #endif
        Serial.print(F(WAIT_AFTER_STOP_COMMAND)); Serial.print(F("0-9999 : Wait After Stop (ms) => ")); Serial.println(data.waitAfterStop);
        Serial.print(F(WAIT_AFTER_FILL_COMMAND)); Serial.print(F("0-9999 : Wait After Filling (ms) => ")); Serial.println(data.waitAfterFill);
        #ifdef MP3_PIN
            Serial.print(F(FILL_SOUND_COMMAND)); Serial.print(F("0-99 : Loading Sound => ")); Serial.println(data.fillSound);
            Serial.print(F(UNLOAD_SOUND_COMMAND)); Serial.print(F("0-99 : Unloading Sound => ")); Serial.println(data.unloadSound);
            Serial.print(F(SOUND_VOLUME_COMMAND)); Serial.print(F("0-30 : Sound Volume => ")); Serial.println(data.soundVolume);
            Serial.print(F(SOUND_INCREMENT_COMMAND)); Serial.print(F("0-999 : Sound Increment (ms) => ")); Serial.println(data.soundIncrementDuration);
            Serial.print(F(TEST_SOUND_COMMAND)); Serial.print(F("1-99 : Sound Test")); Serial.println();
        #endif
        Serial.print(F(START_COMMAND)); Serial.print(F(" : Run")); Serial.println();
        Serial.print(F(STOP_COMMAND)); Serial.print(F(" : Stop")); Serial.println();
        Serial.print(F(EMERGENCY_COMMAND)); Serial.print(F(" : Emergency Stop")); Serial.println();
        Serial.print(F(ILS_STATE_COMMAND)); Serial.print(F(" : ILS State")); Serial.println();
        Serial.print(F(OPEN_RELAY_COMMAND)); Serial.print(F(" : Open Hopper")); Serial.println();
        Serial.print(F(CLOSE_RELAY_COMMAND)); Serial.print(F(" : Close Hopper")); Serial.println();
        Serial.print(F(STOP_TRAIN_COMMAND)); Serial.print(F(" : Stop Train")); Serial.println();
        Serial.print(F(START_TRAIN_COMMAND)); Serial.print(F(" : Run Train")); Serial.println();
        Serial.print(F(DEBUG_TOGGLE_COMMAND)); Serial.print(F(" : Toggle Debug")); Serial.println();
        Serial.print(F(DISPLAY_VARIABLES_COMMAND)); Serial.print(F(" : Display Variables")); Serial.println();
        Serial.print(F(INIT_COMMAND)); Serial.print(F(" : Global Initialisztion")); Serial.println();
    #endif
}

// Toggle  debug flag
void toggleDebug(void){
    data.inDebug = !data.inDebug;
    saveSettings();
}

// Reinitialize all settings
void reinitAll(void) {
    stopFilling();
    initSettings();
    saveSettings();
}

// Execute command read on serial input (a-z and 0-9)
void executeCommand(void) {
    #ifndef DISPLAY_KEYBOARD_INPUT
        Serial.println(F(""));
        #ifdef VERSION_FRANCAISE
            Serial.print(F("Reçu : "));
        #else
            Serial.print(F("Received: "));
        #endif
    #endif
    Serial.println(inputBuffer);
    if (isCommand(inputBuffer, (char*) INIT_COMMAND)) {
        reinitAll();
    } else if (isCommand(inputBuffer, (char*) OPEN_RELAY_COMMAND)) {
        startFilling();
    } else if (isCommand(inputBuffer, (char*) CLOSE_RELAY_COMMAND)) {
        stopFilling();
    } else if (isCommand(inputBuffer, (char*) ILS_STATE_COMMAND)) {
        displayIlsState();
    } else if (isCommand(inputBuffer, (char*) START_COMMAND)) {
        data.isActive = true;
    } else if (isCommand(inputBuffer, (char*) STOP_COMMAND)) {
        data.isActive = false;
        stopFilling();
    } else if (isCommand(inputBuffer, (char*) EMERGENCY_COMMAND)) {
        emergencyStop();
    } else if (isCommand(inputBuffer, (char*) START_TRAIN_COMMAND)) {
        startTrain();
    } else if (isCommand(inputBuffer, (char*) STOP_TRAIN_COMMAND)) {
        stopTrain();
    } else if (isCommand(inputBuffer, (char*) DEBUG_TOGGLE_COMMAND)) {
        toggleDebug();
    } else if (isCommand(inputBuffer, (char*) DISPLAY_VARIABLES_COMMAND)) {
        displayVariables();
    } else if (isCommandValue(inputBuffer, (char*) ILS_ACTIVATION1_COMMAND, 1, 10)) { 
        data.activationIls1 = commandValue;
        saveSettings();
        reloadIls();
    } else if (isCommandValue(inputBuffer, (char*) ILS_ACTIVATION2_COMMAND, 0, 10)) { 
        data.activationIls2 = commandValue;
        saveSettings();
        reloadIls();
    } else if (isCommandValue(inputBuffer, (char*) ILS_ACTIVATION3_COMMAND, 0, 10)) { 
        data.activationIls3 = commandValue;
        saveSettings();
        reloadIls();
    } else if (isCommandValue(inputBuffer, (char*) WAGON_FILL_DURATION_COMMAND, 1, 9999)) {
        data.fillingTime = commandValue;
        saveSettings();
    } else if (isCommandValue(inputBuffer, (char*) WAIT_AFTER_STOP_COMMAND, 1, 9999)) {
        data.waitAfterStop = commandValue;
        saveSettings();
    } else if (isCommandValue(inputBuffer, (char*) WAIT_AFTER_FILL_COMMAND, 1, 9999)) {
        data.waitAfterFill = commandValue;
        saveSettings();
    } else if (isCommandValue(inputBuffer, (char*) RELAY_PULSE_DURATION_COMMAND, 1, 999)) {
        data.pulseTime = commandValue;
        saveSettings();
    } else if (isCommandValue(inputBuffer, (char*) LOAD_DELAY_COMMAND, 1, 9999)) {
        data.loadDelay = commandValue;
        saveSettings();
    } else if (isCommandValue(inputBuffer, (char*) UNLOAD_DELAY_COMMAND, 1, 9999)) {
        data.unloadDelay = commandValue;
        saveSettings();
    } else if (isCommandValue(inputBuffer, (char*) RECLOSE_DELAY_COMMAND, 1, 9999)) {
        data.repeatCloseDelay = commandValue;
        saveSettings();
    #ifdef MP3_PIN
        } else if (isCommandValue(inputBuffer, (char*) FILL_SOUND_COMMAND, 0, 99)) {
            data.fillSound = commandValue;
            saveSettings();
        } else if (isCommandValue(inputBuffer, (char*) UNLOAD_SOUND_COMMAND, 0, 99)) {
            data.unloadSound = commandValue;
            saveSettings();
        } else if (isCommandValue(inputBuffer, (char*) SOUND_VOLUME_COMMAND, 0, 30)) {
            data.soundVolume = commandValue;
            saveSettings();
        } else if (isCommandValue(inputBuffer, (char*) SOUND_INCREMENT_COMMAND, 0, 999)) {
            data.soundIncrementDuration = commandValue;
            saveSettings();
        } else if (isCommandValue(inputBuffer, (char*) TEST_SOUND_COMMAND, 1, 99)) {
            playSound(commandValue);
    #endif
    } else {
        if (inputBuffer[0]) {
            printHelp();
        }
    }
    displayStatus();
}

// Display all variables (debug)
void displayVariables(void) {
    Serial.print(F("stateMachine=")); Serial.println(stateMachine);
    Serial.print(F("waitAfterStopTimer=")); Serial.println(millis() - waitAfterStopTimer);
    Serial.print(F("waitFilledTimer=")); Serial.println(millis() - waitFilledTimer);
    Serial.print(F("waitAfterFillingTimer=")); Serial.println(millis() - waitAfterFillingTimer);
    Serial.print(F("waitForIlsClosedTimer=")); Serial.println(millis() - waitForIlsClosedTimer);
    Serial.print(F("relayPulseActive=")); Serial.println(relayPulseActive);
    Serial.print(F("relayPulseTimer=")); Serial.println(millis() - relayPulseTimer);
    Serial.print(F("lastIlsDisplay=")); Serial.println(lastIlsDisplay);
    #ifdef VIBRATION_RELAY
        Serial.print(F("fillingVibrationActive=")); Serial.println(fillingVibrationActive);
        Serial.print(F("fillingVibrationTimer=")); Serial.println(millis() - fillingVibrationTimer);
        Serial.print(F("unloadingVibrationActive=")); Serial.println(unloadingVibrationActive);
        Serial.print(F("unloadingVibrationTimer=")); Serial.println(millis() - unloadingVibrationTimer);
        Serial.print(F("repeatCloseActive=")); Serial.println(repeatCloseActive);
        Serial.print(F("repeatCloseTimer=")); Serial.println(millis() - repeatCloseTimer);
    #endif
    Serial.print(F("doorOpened=")); Serial.println(doorOpened);
    Serial.println(F("ILS: 1234567890D"));
    Serial.print(F(  "     "));
    for (uint8_t i = 0; i<11; i++) {
        if (digitalRead(ilsPinMapping[i]) == ILS_CLOSED) {
            Serial.print(F("X"));
        } else {
            Serial.print(F("-"));
        }
    }
    Serial.println();
    #ifdef VERSION_FRANCAISE
        Serial.println(F("Relai : OFVA"));
    #else
        Serial.println(F("Relay : OCVS"));
    #endif
    Serial.print(F(  "        "));
    for (uint8_t i = 0; i<4; i++) {
        if (relayState[i] == RELAY_CLOSED) {
            Serial.print(F("X"));
        } else {
            Serial.print(F("-"));
        }
    }
    Serial.println();
}

// Change a relay state
void setRelay(uint8_t index, uint8_t state){
    digitalWrite(relayPinMapping[index], state);
    relayState[index] = state;
    // Set door opened depnding on states
    if (index == OPEN_RELAY && state == RELAY_CLOSED) {
        doorOpened = true;
        if (data.inDebug) {
            #ifdef VERSION_FRANCAISE
                Serial.println(F("Ouverture trémie"));
            #else
                Serial.println(F("Opening hopper"));
            #endif
        }
    }
    if (index == CLOSE_RELAY && state == RELAY_CLOSED) {
        #ifdef VIBRATION_RELAY
            fillingVibrationActive = true;
            fillingVibrationTimer = millis();
        #endif
        doorOpened = false;
        if (data.inDebug) {
            #ifdef VERSION_FRANCAISE
                Serial.println(F("Fermerture trémie"));
            #else
                Serial.println(F("Closing hopper"));
            #endif
        }
    }
    if (index == CLOSE_RELAY || index == OPEN_RELAY) {
        relayPulseActive = true;
        relayPulseTimer = millis();
    }
}

#ifdef MP3_PIN
    // Init sound chip
    void soundSetup(void) {
        mp3Player.set_storage(mp3Player.STORAGE_FLASH);			    // Use files in flash (should be downloaded before using USB)
        mp3Player.set_play_mode(mp3Player.PLAY_TRACK_REPEAT);       // Repeat track forever
    }

    // Play a sound
    void playSound(uint8_t index) {
        if (data.soundVolume && index) {                            // Only if volume and index set
            if (data.soundIncrementDuration && data.soundVolume > 1) {      // Sound increment and target volume > 1?
                soundVolume = 1;                                    // Start at volume 1
                soundIncrement = 1;                                 // Increase sound
                soundChangeDuration = data.soundIncrementDuration / data.soundVolume; // Wait time between increments
                lastSoundChangeTime = millis();                     // Set last volume change time

            } else {
                soundVolume = data.soundVolume;                     // Set directly target sound volume
                lastSoundChangeTime = 0;                            // Clear last volume set time
            }
            mp3Player.set_volume(soundVolume);                      // Set volume
            mp3Player.set_track_index(index);                       // Set track index to play
            mp3Player.play();                                       // Play track
        }
    }

    // Stop playing a sound
    void stopSound(void) {
        if (data.soundVolume) {                                     // Only if volume set
            if (data.soundIncrementDuration && data.soundVolume > 1) {      // Sound increment and target volume > 1?
                soundVolume -= 1;                                   // Start at volume - 1
                soundIncrement = -1;                                // Decrease sound
                soundChangeDuration = data.soundIncrementDuration / data.soundVolume; // Wait time between increments
                lastSoundChangeTime = millis();                     // Set last volume change time

            } else {
                soundVolume = 0;                                    // Set directly target sound volume
                lastSoundChangeTime = 0;                            // Clear last volume set time
            }
            mp3Player.set_volume(soundVolume);                      // Set volume
            if (!soundVolume) {                                     // Is volume set to zero?
                mp3Player.stop();                                   // Stop player
            }
        }

    }

    // Set sound volume giving increase/decrease
    void changeVolume() {
        soundVolume += soundIncrement;                              // Increment/decrement sound
        if (soundVolume < data.soundVolume && soundVolume > 0) {    // We're not yet at target sound
            lastSoundChangeTime = millis();                         // Set last volume change time
        } else {
            lastSoundChangeTime = 0;                                // Reset last volume change time
        }
        mp3Player.set_volume(soundVolume);                          // Set new volume
        if (!soundVolume) {                                         // Volume = 0?
            mp3Player.stop();                                       // Stop player
        }
    }
#endif

// Setup
void setup(void){
    initIO();
    Serial.begin(115200);
    Serial.println();
    #ifdef VERSION_FRANCAISE
        Serial.print(F("Chargeur de ballast à l'arrêt "));
    #else
        Serial.print(F("Ballast loader (train stopped) "));
    #endif
    Serial.print(CODE_VERSION);
    #ifdef VERSION_FRANCAISE
        Serial.println(F(" lancé ..."));
    #else
        Serial.println(F(" started..."));
    #endif
    resetInputBuffer();
    initSettings();
    loadSettings();
    #ifdef MP3_PIN
        soundSetup();
    #endif
    displayStatus();
    reloadIls();
}

// Main loop
void loop(void){
    unsigned long now = millis();

    // Scan all debouncers
    for (uint8_t i = 0; i < 4; i++) {
        // Check for dataIndex defined
        if (debouncer[i].dataIndex) {
            // Read current state
            bool currentState = (digitalRead(ilsPinMapping[debouncer[i].dataIndex-1]) == ILS_CLOSED);
            if (debouncer[i].lastWasClosed != currentState) {       // Does state change since last call?
                debouncer[i].lastWasClosed = currentState;          // Save state
                debouncer[i].lastChangeTime = now;                  // Save last change time
            }
            if (debouncer[i].isClosed != debouncer[i].lastWasClosed) {  //State changed?
                if ((now - debouncer[i].lastChangeTime) > 50 && data.isActive) { // Is pin stable for 50 ms and mode active
                    debouncer[i].isClosed = debouncer[i].lastWasClosed; // Load state with last stable one
                    if (data.inDebug) {
                        #ifdef VERSION_FRANCAISE
                            Serial.print(debouncer[i].isClosed?F("Fermeture"):F("Ouverture"));
                        #else
                            Serial.print(debouncer[i].isClosed?F("Closing"):F("Opening"));
                        #endif
                        Serial.print(F(" ILS "));
                        if (i == 3){                                // Is this unloading ILS
                            #ifdef VERSION_FRANCAISE
                                Serial.print(F("déchargement"));
                            #else
                                Serial.print(F("unloading"));
                            #endif
                        } else {                                    // This is filling ILS
                            Serial.print(debouncer[i].dataIndex);
                        }
                    }
                    if (data.inDebug) {
                        #ifdef VERSION_FRANCAISE
                            Serial.print(" à ");
                        #else
                            Serial.print(" at ");
                        #endif
                        Serial.print(millis());
                    }
                    if (debouncer[i].isClosed) {                    // Are we now closed?
                        if (i == 3){                            // Is this unloading ILS
                            if (data.inDebug) {
                                Serial.println("");
                            }
                            #ifdef VIBRATION_RELAY
                                startUnloading();               // Start unloading
                            #endif
                        } else {                                // This is filling ILS
                            if (stateMachine == waitForWagon) { // Are we waiting for wagon?
                                if (data.inDebug) {
                                    Serial.println("");
                                }
                                stopTrain();                    // Stop train
                                stateMachine = waitingAfterStop;// Set next step
                                waitAfterStopTimer = millis();  // Set timer
                            } else {
                                if (data.inDebug) {
                                    #ifdef VERSION_FRANCAISE
                                        Serial.print(F(" ignorée, état="));
                                    #else
                                        Serial.print(F(" ignored, state="));
                                    #endif
                                    Serial.println(stateMachine);
                                }
                            }
                        }
                    } else {
                        if (data.inDebug) {
                            Serial.println("");
                        }
                    }
                }
            }
        }
    }

    now = millis();                                                 // Refresh current time (to avoid side effects)
    // Are we at end of waiting after stop?
    if (stateMachine == waitingAfterStop && ((now-waitAfterStopTimer) > data.waitAfterStop)) {
        startFilling();
        stateMachine = waitFilled;
        waitFilledTimer = now;
    }

    // Are we at end of waiting filled?
    if (stateMachine == waitFilled && ((now-waitFilledTimer) > data.fillingTime)) {
        stopFilling();
        stateMachine = waitAfterFilling;
        waitAfterFillingTimer = now;
    }

    // Are we at end of waiting after filled?
    if (stateMachine == waitAfterFilling && ((now-waitAfterFillingTimer) > data.waitAfterFill)) {
        startTrain();
        stateMachine = waitForWagon;
    }

    now = millis();                                                 // Refresh current time (to avoid side effects)
    // Do we need to close any relay after pulse?
    if (relayPulseActive && ((now - relayPulseTimer) >= data.pulseTime)) {
        if (data.inDebug) {
            #ifdef VERSION_FRANCAISE
                Serial.print(F("Fin impulsion à "));
            #else
                Serial.print(F("Fin impulsion à "));
            #endif
            Serial.print(now);
            #ifdef VERSION_FRANCAISE
                Serial.print(F(", durée "));
            #else
                Serial.print(F(", duration "));
            #endif
            Serial.println(now - relayPulseTimer);
        }
        for (uint8_t i = 0; i < 2; i++) {
            setRelay(i, RELAY_OPENED);
        }
        relayPulseActive = false;      
    }

    #ifdef VIBRATION_RELAY
        // Any vibration active?
        if (fillingVibrationActive || unloadingVibrationActive) {
            // Stop vibrations if both expired
            if (((now - fillingVibrationTimer) >= data.loadDelay)
                    && ((now - unloadingVibrationTimer) >= data.unloadDelay)) {
                stopVibration();
            }
        }

        // Force close if needed
        if (repeatCloseActive && (now - repeatCloseTimer) > data.repeatCloseDelay) {
            repeatCloseTimer = now;
            setRelay(CLOSE_RELAY, RELAY_CLOSED);
        }
    #endif

    #ifdef MP3_PIN
        // Are we in volume change with delay expired?
        if (lastSoundChangeTime && ((millis() - lastSoundChangeTime) >= soundChangeDuration)) {
            changeVolume();                                         // Increase/decrease volume
        }
    #endif

    // Scan serial for input
    if (Serial.available()) {
        workWithSerial();
    }

    // Display ILS state if needed
    if (displayIls) {
        if ((millis() - lastIlsDisplay) > DISPLAY_ILS_TIME) {
            Serial.print (F("\rILS : "));
            for (uint8_t i = 0; i < 10; i++) {
                if (digitalRead(ilsPinMapping[i]) == ILS_CLOSED) {
                    Serial.print(i+1);
                } else {
                    Serial.print(F("-"));
                    if (i == 9) Serial.print(F(" "));
                }
                Serial.print(F(" "));
            }
            #ifdef VIBRATION_RELAY  
                // Display unload vibrations ILS
                if (digitalRead(ilsPinMapping[10]) == ILS_CLOSED) {
                    #ifdef VERSION_FRANCAISE
                        Serial.print(F("D"));
                    #else
                        Serial.print(F("U"));
                    #endif
                } else {
                    Serial.print(F("-"));
                }
            #endif
            Serial.print(F(" "));
            lastIlsDisplay = millis();
        }
    }
}
