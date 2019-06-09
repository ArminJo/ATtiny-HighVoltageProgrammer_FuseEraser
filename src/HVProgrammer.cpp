/*
 *  HVProgrammer.cpp
 *
 */

#include <Arduino.h>

// AVR High-voltage Serial Fuse Reprogrammer
// Adapted from code and design by Paul Willoughby 03/20/2010
// http://www.rickety.us/2010/03/arduino-avr-high-voltage-serial-programmer/
// Fuse Calc:
// http://www.engbedded.com/fusecalc/

// It restores the default fuse settings of the ATtiny.
// The fuses can then easily be changed with the programmer you use for uploading your program.

// Modified for easy use with Nano board on a breadboard by Armin Joachimsmeyer - 3/2018
// - Added option to press button instead of sending character to start programming
// - Improved serial output information
// - After programming the internal LED blinks
// - Added timeout for reading data

//Modified to add chip erase ablity
// - added option selection via character entry
// - 'f' -> write fuses to defaults
// - 'e' -> chip erase
// - any other character -> just read fuses

#define VERSION "2.3"

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

unsigned int readSignature();
void writeFuse(unsigned int fuse, byte val);
void readFuses();

void setup() {
    Serial.begin(SERIAL_BAUDRATE);
    while (!Serial); //delay for Leonardo
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

    Serial.println("Enter 'f' to write fuses to defaults...");
    Serial.println("Enter 'e' to erase flash and lock bits...");
    Serial.println("Enter any other character to read fuses...");
    Serial.println("Or...");
    Serial.println("Press button at pin 6 to start process or enter any character to start process...");
    pinMode(START_BUTTON_PIN, INPUT_PULLUP);
    
}

void loop() {
    char inst;
    if (!digitalRead(START_BUTTON_PIN) || Serial.available() > 0) {
        delay(100); // debouncing wait for serial buffer to be filled (eg. with CR/LF)
        while (Serial.available() > 0) {
            inst = Serial.read();
        }
        digitalWrite(LED_BUILTIN, HIGH);
        pinMode(SDO, OUTPUT); // Set SDO to output
        digitalWrite(SDI, LOW);
        digitalWrite(SII, LOW);
        digitalWrite(SDO, LOW);
        digitalWrite(RST, HIGH); // 12v Off
        digitalWrite(VCC, HIGH); // Vcc On
        delayMicroseconds(20);
        digitalWrite(RST, LOW); // 12v On
        delayMicroseconds(10);
        pinMode(SDO, INPUT); // Set SDO to input
        delayMicroseconds(300);
        Serial.println("Reading signature from connected ATtiny...");
        unsigned int sig = readSignature();
        Serial.println("Reading complete..");
        Serial.print("Signature is: ");
        Serial.println(sig, HEX);
        readFuses();
        if (sig == ATTINY13) {

            Serial.println("The ATtiny is detected as ATtiny13/ATtiny13A.");
            if (inst == 'f')    {
                Serial.println("Write LFUSE: 0x6A");
                writeFuse(LFUSE, 0x6A);
                Serial.println("Write HFUSE: 0xFF");
                writeFuse(HFUSE, 0xFF);
                Serial.println("");
            }
            else if (inst =='e')  {
                chip_erase();           //erase flash and lock bits
            }
        } else if (sig == ATTINY24 || sig == ATTINY44 || sig == ATTINY84 || sig == ATTINY25 || sig == ATTINY45 || sig == ATTINY85) {

            Serial.print("The ATtiny is detected as ");
            if (sig == ATTINY24)
                Serial.println("ATTINY24.");
            else if (sig == ATTINY44)
                Serial.println("ATTINY44.");
            else if (sig == ATTINY84)
                Serial.println("ATTINY84.");
            else if (sig == ATTINY25)
                Serial.println("ATTINY25.");
            else if (sig == ATTINY45)
                Serial.println("ATTINY45.");
            else if (sig == ATTINY85)
                Serial.println("ATTINY85.");
            if (inst == 'f')    {
                Serial.println("Write LFUSE: 0x62");
                writeFuse(LFUSE, 0x62);
                Serial.println("Write HFUSE: 0xDF");
                writeFuse(HFUSE, 0xDF);
                Serial.println("Write EFUSE: 0xFF");
                writeFuse(EFUSE, 0xFF);
            }
            else if (inst == 'e')   {           //erase flash and lock bits
                chip_erase();   
            }
        } else {
            //Wait for button to release
            while (!digitalRead(START_BUTTON_PIN))
                ;
            delay(100); // debouncing
            Serial.println("No valid ATtiny signature detected! Try again.");
            // try again
            return;
        }

        Serial.println("Fuses will be read again to check if it's changed successfully...");
        readFuses();
        digitalWrite(SCI, LOW);
        digitalWrite(VCC, LOW); // Vcc Off
        digitalWrite(RST, HIGH); // 12v Off

        Serial.println("");
        delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
        /*
         * Blink forever after end of programming
         */
        while (true) {
            delay(150);
            digitalWrite(LED_BUILTIN, HIGH);
            delay(50);
            digitalWrite(LED_BUILTIN, LOW);
        }
    }
}

byte shiftOut(byte val1, byte val2) {
    int inBits = 0;
    uint32_t tMillis = millis();
    //Wait with timeout until SDO goes high
    while (!digitalRead(SDO)) {
        if (millis() > (tMillis + READING_TIMEOUT_MILLIS)) {
            break;
        }
    }
    unsigned int dout = (unsigned int) val1 << 2;
    unsigned int iout = (unsigned int) val2 << 2;
    for (int ii = 10; ii >= 0; ii--) {
        digitalWrite(SDI, !!(dout & (1 << ii)));
        digitalWrite(SII, !!(iout & (1 << ii)));
        inBits <<= 1;
        inBits |= digitalRead(SDO);
        digitalWrite(SCI, HIGH);
        digitalWrite(SCI, LOW);
    }
    return inBits >> 2;
}

void chip_erase () {

    Serial.println("Erasing Flash and Lock Bits...");

    shiftOut(0x80, 0x4C); 
    shiftOut(0x00, 0x64);
    shiftOut(0x00, 0x6C);

    while (!digitalRead(SDO));   //wait until chip erase completes (wait until SDO goes high)
 }

void writeFuse(unsigned int fuse, byte val) {

    Serial.print("Writing fuse value ");
    Serial.print(val, HEX);
    Serial.println(" to ATtiny...");

    shiftOut(0x40, 0x4C);
    shiftOut(val, 0x2C);
    shiftOut(0x00, (byte) (fuse >> 8));
    shiftOut(0x00, (byte) fuse);

    Serial.println("Writing complete.");
}

void readFuses() {

    Serial.println("Reading fuse settings from ATtiny...");

    byte val;
    shiftOut(0x04, 0x4C); // LFuse
    shiftOut(0x00, 0x68);
    val = shiftOut(0x00, 0x6C);
    Serial.print("LFuse: ");
    Serial.print(val, HEX);
    shiftOut(0x04, 0x4C); // HFuse
    shiftOut(0x00, 0x7A);
    val = shiftOut(0x00, 0x7E);
    Serial.print(", HFuse: ");
    Serial.print(val, HEX);
    shiftOut(0x04, 0x4C); // EFuse
    shiftOut(0x00, 0x6A);
    val = shiftOut(0x00, 0x6E);
    Serial.print(", EFuse: ");
    Serial.println(val, HEX);
    Serial.println("Reading complete.");
}

unsigned int readSignature() {
    unsigned int sig = 0;
    byte val;
    for (int ii = 1; ii < 3; ii++) {
        shiftOut(0x08, 0x4C);
        shiftOut(ii, 0x0C);
        shiftOut(0x00, 0x68);
        val = shiftOut(0x00, 0x6C);
        sig = (sig << 8) + val;
    }
    return sig;
}
