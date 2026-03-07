#define CODE_VERSION "V26.3.7-4"

/*

Contrôle d'un chargeur de ballast pour train miniature (version chargement à l'arrêt)

	Ce code contrôle un chargeur de ballast dans des wagons adéquats.

	Il est basé sur une trémie commandée par 2 relais à impulsion.

	Le premier relais ouvre la trémie alors que le second la ferme.

	Un rail a été équipé de 10 ILS régulièrement espacés, afin de détecter l'aimant placé sous chaque wagon.

	Lorsqu'un wagon passe au dessus de (l'un des jusqu'à trois) ILS d'ouverture (paramétrable), un relais coupe
        l'alimentation de la voie, stoppant le train. Après un temps (réglable), une impulsion (de durée réglable)
        est envoyée au relais d'ouverture. La trémie est fermée après un temps paramétrable, puis, après une attente
        réglable, l'aliemntation est remise et le train avance.
        
    Ce cycle peut être répété jusqu'à 3 fois par wagon, pour chaque wagon équipé d'un aimant.

    Noter que le sens de circulation des trains n'a pas d'impact, si l'aimant est placé au milieu des wagons.

	Il arrive que le granulat bloque la sortie de la trémie. Pour contrer ce problème, on peut ajouter
        un vibreur qui sera alimenté lorsque la trémie s'ouvre jusqu'à expiration d'un temps donné
        après la fermeture de la trémie.
        De plus, il est possible d'ajouter un ILS supplémentaire sur la zone de déchargement, qui
        activera également le vibreur pendant un temps donné, afin de faciliter la descente du ballast
        dans la trémie.
        Sur certains modèles de trémie, il arrive que ces vibrations (ré)ouvrent la trémie. Pour contrer
        cette tendance, il est possible de définir un délai de refermeture de la trémie, qui renverra une
        impulsion de fermeture à intervalle régulier tant que les vibrations seront actives.
    
    Les réglages sont envoyés à l'Arduino au travers de sa liaison série. Ce même moyen est utilisé pour
		envoyer les messages à l'utilisateur.

	Les réglages sont mémorisés dans l'EEPROM de l'Arduino afin d'être disponibles après son redémarrage.
	
Hardware Arduino Nano:
	- 11 entrées ILS (10 pour l'avancement du wagon lors du chargement, 1 pour la détection du déchargement)
	- 4 sorties relais :
        - pour l'ouverture de la trémie,
        - pour sa fermeture,
        - pour le vibreur,
        - pour la coupure de l'alimentation de la voie.

Paramétrage :
    - on cherche la durée d'impulsion nécessaire pour ouvrir ou fermer la trémie à coup sur, sans faire chauffer
        les bobines, en augmentant/réduisant la valeur du paramètre DIR et utilisant les commandes O et F
        pour ouvrir/fermer la trémie
	- on repère le numéro de l'ILS du remplissage 1 en avançant le wagon à la main
    - on répète l'opération jusqu'à 3 fois si besoin
    - si on souhaite utiliser les vibrations :
        - on active le vibreur à l'ouverture de la trémie, on l'arrête ferme après un délai à la fermeture
        - on active le vibreur au passage sur l'ILS de vidage, on l'arrête après un autre délai à la fermeture

Commandes :
    - I11-10 : ILS 1 (numéro)
    - I20-10 : ILS 2 (numéro)
    - I30-10 : ILS 3 (numéro)
    - DI1-999 : Durée Impulsion relais (ms)
    - DR1-9999 : Durée Remplissage wagon (ms)
    - VR0-9999 : Vibrations Remplissage (ms)
    - VV0-9999 : Vibrations Vidage (ms)
    - RF0-9999 : Répétition fermeture (ms)
    - AA0-9999 : Attente après Arrêt (ms)
    - AR0-9999 : Attente après remplissage (ms)
    - M : Marche (détection passage wagon activée)
    - A : Arrêt (stoppe la détection des wagons)
    - E : Etat ILS
    - U : arrêt d'urgence (ferme la trémie, arrêt du processus, des vibrations et du train)
    - O : Ouverture trémie
    - F : Fermeture trémie
    - AT : Arrêt Train
    - DT : Démarrage Train
    - D : Bascule déverminage
    - AV : Afficher variables
    - INIT : Initialisation globale

Appui sur <ESC> (parfois marquée Escape ou Echap.)
    - Arrêt d'urgence :
        - Arrêt du processus
        - Ferme la trémie
        - Arrête les vibrations
        - Arrête le train

Example:
    I15     Start loading on ILS 5
    DI100   Relay pulse duration = 100 ms
    DR2000  Filling duration = 2 s
    AA3000  Wait after train stop before starting loading wagon = 3 s
    AR3000  Wait after wagon loading before restarting train = 3 s
    VR5000  Continue vibration after filling process for 5 s
    VV5000  Continue vibration during unloading process for 5 s
    RF1000  Repeat door close every second during vibrations

Principles:
    State machine:
        State -> Action -> Timer armed
        waitForWagon -> trainStoped -> waitAfterStopTimer
        waitingAfterStop -> startFilling -> waitFilledTimer
        waitFilled -> stopFilling -> waitAfterFillingTimer
        waitAfterFilling -> trainStarting -> waitForWagon

    Relay pulse:
        Event -> Action
        Closing door relay -> start pulse timer
        Pulse timer expired -> reset relays

    Vibration relay:
        Opening door -> arm filling vibration timer
        Wagon unloading detected -> arm unloading vibration timer
        FillingVibrationExpired && loadingVibrationExpired -> stop vibration & deactivate repeat close timer

    Repeat close during vibration:
        Start vibration -> arm repeat close timer
        Repeat close timer expired -> force close door & rearm timer

Auteur : Flying Domotic, Février 2025, pour le FabLab
Licence: GNU GENERAL PUBLIC LICENSE - Version 3, 29 June 2007

*/

// Includes
#include "Arduino.h"
#include "EEPROM.h"

// Command and text
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
    if (data.inDebug) Serial.print(F(", déverminage"));
    Serial.println(data.isActive ? F(", en marche") : F(", à l'arrêt"));
}

// Load settings from EEPROM
void loadSettings(void) {
    initSettings();                                                 // Init data structure
    if (EEPROM.read(0) != MAGIC_NUMBER) {                           // Is first byte equal to magic number?
        Serial.print(F("Magic est "));
        Serial.print(EEPROM.read(0));
        Serial.print(F(", pas "));
        Serial.print(MAGIC_NUMBER);
        Serial.println(F("!"));
        return;
    }

    uint8_t version = EEPROM.read(1);                               // Get version
    if (version == 1) {
        EEPROM.get(0, data);                                        // Load EEPROM V2 structure
    } else {
        Serial.print(F("Version est "));
        Serial.print(version);
        Serial.print(F(", pas "));
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
                    Serial.println(F("Buffer plein - Reset!"));
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
        Serial.print(F(" hors limites "));
        Serial.print(minValue);
        Serial.print(F("-"));
        Serial.print(maxValue);
        Serial.print(F(" pour la commande "));
        Serial.println(commandToCheck);
        return false;
    }
    return true;
}

//  Stop train
void stopTrain(void) {
    if (data.inDebug) {
        Serial.print(F("Arrêt du train à "));
        Serial.println(millis());
    }
    // Close isolation relay
    setRelay(POWER_ISOLATION_RELAY, RELAY_CLOSED);
}

// Start filling
void startFilling(void) {
    if (data.inDebug) {
        Serial.print(F("Début chargement à "));
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
        Serial.print(F("Fin chargement à "));
        Serial.print(millis());
        Serial.print(F(", durée "));
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
        Serial.print(F("Départ du train à "));
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
                Serial.print(F("Début déchargement à "));
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
            Serial.print(F("Début vibrations à "));
            Serial.println(millis());
        }
        setRelay(VIBRATION_RELAY, RELAY_CLOSED);
    }

    // Stop vibration relay
    void stopVibration(void) {
        if (data.inDebug) {
            Serial.print(F("Fin vibrations à "));
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
    Serial.print(F(ILS_ACTIVATION1_COMMAND)); Serial.print(F("1-10 : ILS 1 (numéro) => ")); Serial.println(data.activationIls1);
    Serial.print(F(ILS_ACTIVATION2_COMMAND)); Serial.print(F("0-10 : ILS 2 (numéro) => ")); Serial.println(data.activationIls2);
    Serial.print(F(ILS_ACTIVATION3_COMMAND)); Serial.print(F("0-10 : ILS 3 (numéro) => ")); Serial.println(data.activationIls3);
    Serial.print(F(RELAY_PULSE_DURATION_COMMAND)); Serial.print(F("1-999 : Durée Impulsion relai (ms) => ")); Serial.println(data.pulseTime);
    #ifdef VIBRATION_RELAY
        Serial.print(F(WAGON_FILL_DURATION_COMMAND)); Serial.print(F("1-9999 : Durée Remplissage wagon (ms) => ")); Serial.println();
        Serial.print(F(LOAD_DELAY_COMMAND)); Serial.print(F("0-9999 : Vibrations Remplissage (ms) => ")); Serial.println(data.loadDelay);
        Serial.print(F(UNLOAD_DELAY_COMMAND)); Serial.print(F("0-9999 : Retard Vidage vibration (ms) => ")); Serial.println(data.unloadDelay);
        Serial.print(F(RECLOSE_DELAY_COMMAND)); Serial.print(F("0-9999 : Répétition fermeture (ms) => ")); Serial.println(data.repeatCloseDelay);
    #endif
    Serial.print(F(WAIT_AFTER_STOP_COMMAND)); Serial.print(F("0-9999 : Attente après Arrêt (ms) => ")); Serial.println(data.waitAfterStop);
    Serial.print(F(WAIT_AFTER_FILL_COMMAND)); Serial.print(F("0-9999 : Attente après remplissage (ms) => ")); Serial.println(data.waitAfterFill);
    Serial.print(F(START_COMMAND)); Serial.print(F(" : Marche")); Serial.println();
    Serial.print(F(STOP_COMMAND)); Serial.print(F(" : Arrêt")); Serial.println();
    Serial.print(F(EMERGENCY_COMMAND)); Serial.print(F(" : arrêt d'Urgence")); Serial.println();
    Serial.print(F(ILS_STATE_COMMAND)); Serial.print(F(" : Etat ILS")); Serial.println();
    Serial.print(F(OPEN_RELAY_COMMAND)); Serial.print(F(" : Ouverture trémie")); Serial.println();
    Serial.print(F(CLOSE_RELAY_COMMAND)); Serial.print(F(" : Fermeture trémie")); Serial.println();
    Serial.print(F(STOP_TRAIN_COMMAND)); Serial.print(F(" : Arrêt du train")); Serial.println();
    Serial.print(F(START_TRAIN_COMMAND)); Serial.print(F(" : Démarrage du train")); Serial.println();
    Serial.print(F(DEBUG_TOGGLE_COMMAND)); Serial.print(F(" : Bascule déverminage")); Serial.println();
    Serial.print(F(INIT_COMMAND)); Serial.print(F(" : Initialisation globale")); Serial.println();
    Serial.print(F(DISPLAY_VARIABLES_COMMAND)); Serial.print(F(" : Afficher variables")); Serial.println();
    Serial.print(F(INIT_COMMAND)); Serial.println(F(" : Initialisation globale"));
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
        Serial.print(F("Reçu: "));
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
    Serial.println(F("Relai : OFVA"));
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
            Serial.println(F("Ouverture trémie"));
        }
    }
    if (index == CLOSE_RELAY && state == RELAY_CLOSED) {
        #ifdef VIBRATION_RELAY
            fillingVibrationActive = true;
            fillingVibrationTimer = millis();
        #endif
        doorOpened = false;
        if (data.inDebug) {
            Serial.println(F("Fermerture trémie"));
        }
    }
    if (index == CLOSE_RELAY || index == OPEN_RELAY) {
        relayPulseActive = true;
        relayPulseTimer = millis();
    }
}

// Setup
void setup(void){
    initIO();
    Serial.begin(115200);
    Serial.println();
    Serial.print(F("Chargeur de ballast à l'arrêt "));
    Serial.print(CODE_VERSION);
    Serial.println(F(" lancé..."));
    resetInputBuffer();
    initSettings();
    loadSettings();
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
                if ((now - debouncer[i].lastChangeTime) > 500) {     // Is pin stable for 50 ms?
                    debouncer[i].isClosed = debouncer[i].lastWasClosed; // Load state with last stable one
                    if (data.inDebug) {
                        Serial.print(debouncer[i].isClosed?F("Fermeture"):F("Ouverture"));
                        Serial.print(F(" ILS "));
                        if (i == 3){                                // Is this unloading ILS
                            Serial.print(F("déchargement"));
                        } else {                                    // This is filling ILS
                            Serial.print(debouncer[i].dataIndex);
                        }
                    }
                    if (data.inDebug) {
                        Serial.print(" à ");
                        Serial.print(millis());
                    }
                    if (debouncer[i].isClosed) {                    // Are we now closed?
                        if (data.isActive) {                        // ... with process acctive?
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
                                        Serial.print(F(" ignorée, état="));
                                        Serial.println(stateMachine);
                                    }
                                }
                            }
                        } else {                                    // On n'est pas en mode actif
                            if (data.inDebug) {
                                Serial.println(F(" ignorée, process à l'arrêt"));
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
    if (stateMachine == waitFilled && ((now-waitFilledTimer) > data.loadDelay)) {
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
            Serial.print(F("Fin impulsion à "));
            Serial.print(now);
            Serial.print(F(", durée "));
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
                    Serial.print(F("D"));
                } else {
                    Serial.print(F("-"));
                }
            #endif
            Serial.print(F(" "));
            lastIlsDisplay = millis();
        }
    }
}
