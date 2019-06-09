# Arduino High Voltage Programmer / Eraser for ATtiny

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

Based on [Recover Bricked ATtiny Using Arduino as high voltage programmer](http://www.electronics-lab.com/recover-bricked-attiny-using-arduino-as-high-voltage-programmer/)
This program restores the default fuse settings of the ATtiny. 
The fuses can then easily be changed with the programmer you use for uploading your program.

### Modified for easy use with Nano board on a breadboard
- Added option to press button instead of sending character to start programming
- Improved serial output information
- After programming the internal LED blinks
- Added timeout for reading data

# Installation
Just copy the **[content](https://raw.githubusercontent.com/ArminJo/ATtiny-HighVoltageProgrammer_FuseEraser/master/src/HVProgrammer.cpp)** of the HVProgrammer.cpp file into a **new Arduino sketch** and run it.

In the **Arduino serial monitor** with baudrate **115200** you will see something like:
```
START C:\...\ArduinoSketches\sketch_jun09a\sketch_jun09a.ino
Version 3.0 from Jun  9 2019

Enter 'r' to only read fuses...
Enter 'e' to erase flash and lock bits...
Enter any other character Or press button at pin 6 to to write fuses to default...
```

and **after button press or sending a character** you see
```
Reading signature from connected ATtiny...
Reading complete..
Signature is: 6C6C
Reading fuse settings from ATtiny...
LFuse: 6C, HFuse: 7E, EFuse: 7F
Reading complete.
No valid ATtiny signature detected! Try again.
```
if **no ATtiny** is attached.

# Breadboard
This circuit uses an Arduino nano and an [ebay DC-DC Step-Up Modul](https://www.ebay.de/itm/2PCS-2A-Booster-Board-DC-DC-Step-Up-Modul-2-24V-5-9-12-28V-Replace-XL6009-BAF-/263413727169?hash=item3d54ae7fc1)

Breadboard top view
![Breadboard_Top](https://github.com/ArminJo/ATtiny-HighVoltageProgrammer_FuseEraser/blob/HEAD/media/HVProgrammer_Top.jpg)
Breadboard back view
![Breadboard_Back](https://github.com/ArminJo/ATtiny-HighVoltageProgrammer_FuseEraser/blob/HEAD/media/HVProgrammer_Back.jpg)

# Revision History
### Version 3.0
- Added erase and raed only commands
### Version 2.2
- First public version
