/*
 *  HVProgrammer.cpp
 */

#include <Arduino.h>

// AVR High-voltage Serial Fuse Reprogrammer
// Adapted from code and design by Paul Willoughby 03/20/2010
// http://www.rickety.us/2010/03/arduino-avr-high-voltage-serial-programmer/
// Fuse Calc:
// http://www.engbedded.com/fusecalc/

// Restores the default fuse settings of the ATtiny and erases flash memory to restore
// lock bits to their default unlocked state
// Fuses can then easily be changed with the programmer you use for uploading your program.

// Modified for easy use with Nano board on a breadboard by Armin Joachimsmeyer - 3/2018
// - Added option to press button instead of sending character to start programming
// - Improved serial output information
// - After programming the internal LED blinks
// - Added timeout for reading data

// Modified to add Lock bits processing - bWildered1 6/2019
// - read and report lock bits status
// - device memory erase to restore lock bits to their default unlocked state

#define VERSION "3.2"

//#define SERIAL_BAUDRATE 19200
#define SERIAL_BAUDRATE 115200

#define START_BUTTON_PIN 6 // connect a button to ground

#define READING_TIMEOUT_MILLIS 300 // for each shiftOut -> effective timeout is 4 times ore more this single timeout

#define RST A4 // Output to level shifter for !RESET from transistor
#define SCI A5 // Target Clock Input
#define SDO 5 // Target Data Output
#define SII 4 // Target Instruction Input
#define SDI 3 // Target Data Input
#define VCC 2 // Target VCC

/*
 * Alternate pin layout
 */
/*
 #define RST 13 // Output to level shifter for !RESET from transistor
 #define SCI 12 // Target Clock Input
 #define SDO 11 // Target Data Output
 #define SII 10 // Target Instruction Input
 #define SDI 9 // Target Data Input
 #define VCC 8 // Target VCC
 */

// Address of the fuses
#define HFUSE 0x747C
#define LFUSE 0x646C
#define EFUSE 0x666E

// Define ATTiny series signatures
#define ATTINY13 0x9007 // L: 0x6A, H: 0xFF 8 pin
#define ATTINY24 0x910B // L: 0x62, H: 0xDF, E: 0xFF 14 pin
#define ATTINY25 0x9108 // L: 0x62, H: 0xDF, E: 0xFF 8 pin
#define ATTINY44 0x9207 // L: 0x62, H: 0xDF, E: 0xFFF 14 pin
#define ATTINY45 0x9206 // L: 0x62, H: 0xDF, E: 0xFF 8 pin
#define ATTINY84 0x930C // L: 0x62, H: 0xDF, E: 0xFFF 14 pin
#define ATTINY85 0x930B // L: 0x62, H: 0xDF, E: 0xFF 8 pin

#define DEVICE_UNKNOWN 0
#define DEVICE_ATTINY13 1
#define DEVICE_ATTINY24_TO_85 2

uint16_t readSignature();
uint8_t checkAndPrintSignature(uint16_t aSignature);
void writeFuse(uint16_t aFuseAddress, uint8_t aFuseValue);
void readFuses();
void eraseFlashAndLockBits();
bool readLockBits();

void setup() {
    Serial.begin(SERIAL_BAUDRATE);
    while (!Serial)
        ; //delay for Leonardo
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ "\r\nVersion " VERSION " from " __DATE__));

    pinMode(LED_BUILTIN, OUTPUT);

    pinMode(VCC, OUTPUT);
    pinMode(RST, OUTPUT);
    pinMode(SDI, OUTPUT);
    pinMode(SII, OUTPUT);
    pinMode(SCI, OUTPUT);
    pinMode(SDO, OUTPUT); // Configured as input when in programming mode
    digitalWrite(RST, HIGH); // Level shifter is inverting, this shuts off 12V
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

    pinMode(START_BUTTON_PIN, INPUT_PULLUP);
}

void loop() {

    Serial.println();
    Serial.println();
    Serial.println("Enter 'r' to only read fuses and lock bits...");
    Serial.println("Enter 'e' to erase flash and lock bits...");
    Serial.println("Enter 'w' or any other character or press button at pin 6 to to write fuses to default...");
    Serial.println();

    /*
     * Wait until button pressed or serial available
     */
    char tReceivedChar = 0; // Default value taken, if button pressed
    while (digitalRead(START_BUTTON_PIN) && Serial.available() == 0) {
        delay(1); // just wait here
    }
    // read command
    if (Serial.available() > 0) {
        tReceivedChar = Serial.read();
    }

    // wait for serial buffer to receive CR/LF and consume it
    delay(100);
    while (Serial.available() > 0) {
        Serial.read();
    }

    // signal start of programming
    digitalWrite(LED_BUILTIN, HIGH);
    pinMode(SDO, OUTPUT); // Set SDO to output
    digitalWrite(SDI, LOW);
    digitalWrite(SII, LOW);
    digitalWrite(SDO, LOW);
    /*
     * Switch on VCC and 12 Volt
     */
    digitalWrite(RST, HIGH); // 12v Off
    digitalWrite(VCC, HIGH); // Vcc On
    delayMicroseconds(20);
    digitalWrite(RST, LOW); // 12v On
    delayMicroseconds(10);

    pinMode(SDO, INPUT); // Set SDO to input
    delayMicroseconds(300);

    uint16_t tSignature = readSignature();
    uint8_t tDeviceType = checkAndPrintSignature(tSignature);

    if (tDeviceType != DEVICE_UNKNOWN) {
        readFuses();
        bool tFusesAreLocked = readLockBits();
        if (tFusesAreLocked && tReceivedChar != 'r' && tReceivedChar != 'R') {
            // should write fuses, but they are locked
            Serial.println("Suppose to write fuses, but they are locked. -> Unlock them by performing an additional chip erase.");
        }

        if (tFusesAreLocked || tReceivedChar == 'e' || tReceivedChar == 'E') {
            /*
             * ERASE
             */
            eraseFlashAndLockBits();
        } else if (tReceivedChar != 'r' && tReceivedChar != 'R') {
            /*
             * WRITE FUSES
             */
            if (tDeviceType == DEVICE_ATTINY13) {
                Serial.println("Write LFUSE: 0x6A");
                writeFuse(LFUSE, 0x6A);
                Serial.println("Write HFUSE: 0xFF");
                writeFuse(HFUSE, 0xFF);
                Serial.println("");

            } else if (tDeviceType == DEVICE_ATTINY24_TO_85) {
                Serial.println("Write LFUSE: 0x62");
                writeFuse(LFUSE, 0x62);
                Serial.println("Write HFUSE: 0xDF");
                writeFuse(HFUSE, 0xDF);
                Serial.println("Write EFUSE: 0xFF");
                writeFuse(EFUSE, 0xFF);
            }
        }

        if (tReceivedChar == 'e' || tReceivedChar == 'E') {
            Serial.println("Lock bits will be read again to check values...");
            readLockBits();
        }
        if (tReceivedChar != 'r' && tReceivedChar != 'R') {
            Serial.println("Fuses will be read again to check values...");
            readFuses();
        }
    }

    /*
     * End of programming / reading
     * Wait for button to release if pressed and switch power off
     */
    if (tReceivedChar == 0) {
        /*
         * Wait for button to release, if pressed before
         */
        while (!digitalRead(START_BUTTON_PIN)) {
            delay(1);
        }
        delay(100); // button debouncing
    }

    /*
     * Switch off VCC and 12 Volt
     */
    digitalWrite(SCI, LOW);
    digitalWrite(VCC, LOW); // Vcc Off
    digitalWrite(RST, HIGH); // 12v Off

    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);

    if (tDeviceType == DEVICE_UNKNOWN) {
        Serial.println("Try again.");
    } else {
        /*
         * If controlled by serial input, enable a new run, otherwise blink forever
         */
        if (tReceivedChar == 0) {
            /*
             * Blink forever after end of programming triggered by button
             */
            while (true) {
                delay(150);
                digitalWrite(LED_BUILTIN, HIGH);
                delay(50);
                digitalWrite(LED_BUILTIN, LOW);
            }
        } else {
            Serial.println("Reading / programming finished, allow a new run.");
        }
    }
}

/*
 * Prints message and returns device type
 */
uint8_t checkAndPrintSignature(uint16_t aSignature) {

    uint8_t tReturnValue = DEVICE_ATTINY24_TO_85;
    const char * tTypeString = "";
    switch (aSignature) {
    case ATTINY13:
        tTypeString = "13/ATtiny13A.";
        tReturnValue = DEVICE_ATTINY13;
        break;
    case ATTINY24:
        tTypeString = "24.";
        break;
    case ATTINY44:
        tTypeString = "44.";
        break;
    case ATTINY84:
        tTypeString = "84.";
        break;
    case ATTINY25:
        tTypeString = "25.";
        break;
    case ATTINY45:
        tTypeString = "45.";
        break;
    case ATTINY85:
        tTypeString = "85.";
        break;
    default:
        tReturnValue = DEVICE_UNKNOWN;
        break;
    }
    if (tReturnValue == DEVICE_UNKNOWN) {
        Serial.println("No valid ATtiny signature detected!");
    } else {
        Serial.print("The ATtiny is detected as ATtiny");
        Serial.println(tTypeString);
        Serial.println();
    }
    return tReturnValue;
}

uint8_t shiftOut(uint8_t aValue, uint8_t aAddress) {
    uint16_t tInBits = 0;

//Wait with timeout until SDO goes high
    uint32_t tMillis = millis();
    while (!digitalRead(SDO)) {
        if (millis() > (tMillis + READING_TIMEOUT_MILLIS)) {
            break;
        }
    }
    uint16_t tSDIOut = (uint16_t) aValue << 2;
    uint16_t tSIIOut = (uint16_t) aAddress << 2;
    for (int8_t i = 10; i >= 0; i--) {
        digitalWrite(SDI, !!(tSDIOut & (1 << i)));
        digitalWrite(SII, !!(tSIIOut & (1 << i)));
        tInBits <<= 1;
        tInBits |= digitalRead(SDO);
        digitalWrite(SCI, HIGH);
        digitalWrite(SCI, LOW);
    }
    return tInBits >> 2;
    Serial.print(" tInBits=");
    Serial.println(tInBits);

}

void eraseFlashAndLockBits() {

    Serial.println("Erasing flash and lock bits...");
    shiftOut(0x80, 0x4C);
    shiftOut(0x00, 0x64);
    shiftOut(0x00, 0x6C);

//Wait with timeout until SDO goes high
    uint32_t tMillis = millis();
    while (!digitalRead(SDO)) {
        if (millis() > (tMillis + READING_TIMEOUT_MILLIS)) {
            break;
        }
    }
    Serial.println("Erasing complete.");
    Serial.println();

}

void writeFuse(uint16_t aFuseAddress, uint8_t aFuseValue) {

    Serial.print("Writing fuse value ");
    Serial.print(aFuseValue, HEX);
    Serial.println(" to ATtiny...");

    shiftOut(0x40, 0x4C);
    shiftOut(aFuseValue, 0x2C);
    shiftOut(0x00, (uint8_t) (aFuseAddress >> 8));
    shiftOut(0x00, (uint8_t) aFuseAddress);

    Serial.println("Writing complete.");
    Serial.println();

}

void readFuses() {

    Serial.println("Reading fuse settings from ATtiny...");

    uint8_t tValue;
    shiftOut(0x04, 0x4C); // LFuse
    shiftOut(0x00, 0x68);
    tValue = shiftOut(0x00, 0x6C);
    Serial.print("  LFuse: ");
    Serial.print(tValue, HEX);

    shiftOut(0x04, 0x4C); // HFuse
    shiftOut(0x00, 0x7A);
    tValue = shiftOut(0x00, 0x7E);
    Serial.print(", HFuse: ");
    Serial.print(tValue, HEX);

    shiftOut(0x04, 0x4C); // EFuse
    shiftOut(0x00, 0x6A);
    tValue = shiftOut(0x00, 0x6E);
    Serial.print(", EFuse: ");
    Serial.println(tValue, HEX);
    Serial.println("Reading fuse values complete.");
    Serial.println();

}

uint16_t readSignature() {
    Serial.println("Reading signature from connected ATtiny...");
    uint16_t tSignature = 0;
    uint8_t tValue;
    for (uint8_t tIndex = 1; tIndex < 3; tIndex++) {
        shiftOut(0x08, 0x4C);
        shiftOut(tIndex, 0x0C);
        shiftOut(0x00, 0x68);
        tValue = shiftOut(0x00, 0x6C);
        tSignature = (tSignature << 8) + tValue;
    }
    Serial.print("Signature is: ");
    Serial.println(tSignature, HEX);
    Serial.println("Reading signature complete..");
    Serial.println();

    return tSignature;
}

/*
 * Returns true if fuses are locked
 */
bool readLockBits() {

    bool tReturnValue = false;
    Serial.println("Reading lock bits...");
    uint8_t tValue;
    shiftOut(0x04, 0x4C); // Lock
    shiftOut(0x00, 0x78);
    tValue = shiftOut(0x00, 0x7C);
    Serial.print("  Lock: ");
    Serial.println(tValue, HEX);
    Serial.print("    ");

    // Mask lock bits
    tValue &= 0x03;

//check and report LB1 and LB2 state
//0 is programmed
//tValue: x x x x x x LB2 LB1
    if (tValue & 0x01) {
        Serial.println("LB1 Not Programmed");
    } else {
        Serial.println("LB1 Programmed");
    }
    if (tValue & 0x02) {
        Serial.println("    LB2 Not Programmed");
    } else {
        Serial.println("    LB2 Programmed");
    }

    if (tValue == 0x03) {
        Serial.println("No memory lock features enabled.");
    } else {
        if (!(tValue & 0x01)) {
            Serial.println("Further programming of the Flash and EEPROM is disabled in High-voltage and Serial Programming mode.");
            Serial.println("The Fuse bits are locked in both Serial and High-voltage Programming mode. debugWire is disabled.");
            tReturnValue = true;
        }
        if (!(tValue & 0x02)) {
            Serial.println("Additionally verification is also disabled in High-voltage and Serial Programming mode.");
        }
    }
//Wait with timeout until SDO goes high
    uint32_t tMillis = millis();
    while (!digitalRead(SDO)) {
        if (millis() > (tMillis + READING_TIMEOUT_MILLIS)) {
            break;
        }
    }

    Serial.println("Reading Lock Bits complete.");
    Serial.println();

    return tReturnValue;
}
